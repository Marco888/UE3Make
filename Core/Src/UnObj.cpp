/*=============================================================================
	UnObj.cpp: Unreal object manager.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Object manager internal variables.
UBOOL						UObject::GObjInitialized		= 0;
UBOOL						UObject::GObjNoRegister			= 0;
INT							UObject::GObjBeginLoadCount		= 0;
INT							UObject::GObjRegisterCount		= 0;
INT							UObject::GImportCount			= 0;
UObject*					UObject::GAutoRegister			= NULL;
UPackage*					UObject::GObjTransientPkg		= NULL;
FStringNoInit				UObject::TraceIndent			= FStringNoInit();
TCHAR						UObject::GObjCachedLanguage[32] = TEXT("");
TCHAR						UObject::GLanguage[256]          = TEXT("int");
UObject*					UObject::GObjHash[4096];
TArray<UObject*>			UObject::GObjLoaded;
TArray<UObject*>			UObject::GObjObjects;
TArray<INT>					UObject::GObjAvailable;
TArray<UObject*>			UObject::GObjLoaders;
TArray<UObject*>			UObject::GObjRoot;
TArray<UObject*>			UObject::GObjRegistrants;
TArray<FPreferencesInfo>	UObject::GObjPreferences;
TArray<FRegistryObjectInfo> UObject::GObjDrivers;
TMultiMap<FName,FName>*		UObject::GObjPackageRemap;

/*-----------------------------------------------------------------------------
	UObject constructors.
-----------------------------------------------------------------------------*/

UObject::UObject()
{}
UObject::UObject( const UObject& Src )
{
	guard(UObject::UObject);
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to copy-construct %ls from %ls"), GetFullName(), Src.GetFullName() );
	unguard;
}
UObject::UObject( EInPlaceConstructor, UClass* InClass, UObject* InOuter, FName InName, EObjectFlags InFlags )
{
	guard(UObject::UObject);
	StaticAllocateObject( InClass, InOuter, InName, InFlags, NULL, GError, this );
	unguard;
}
UObject::UObject( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	Index		( INDEX_NONE				)
,	HashNext	( NULL						)
,	StateFrame	( NULL						)
,	_Linker		( NULL						)
,	_LinkerIndex( INDEX_NONE				)
,	Outer       ( NULL						)
,	ObjectFlags	( InFlags | RF_Native	    )
,	Name        ( NAME_None					)
,	Class		( InClass					)
,	ObjectArchetype(NULL)
{
	guard(UObject::UObject);

	// Make sure registration is allowed now.
	check(!GObjNoRegister);

	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
//	check(sizeof(Name        )>=sizeof(InName       ));
//	check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));
	*(const TCHAR  **)&Outer        = InPackageName;

	*(const TCHAR  **)&Name         = InName;
	*(UObject      **)&_LinkerIndex = GAutoRegister;

	GAutoRegister                   = this;

	// Call native registration from terminal constructor.
	if( GetInitialized() && GetClass()==StaticClass() )
		Register();

	unguard;
}
UObject::UObject( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	Index		( INDEX_NONE				)
,	HashNext	( NULL						)
,	StateFrame	( NULL						)
,	_Linker		( NULL						)
,	_LinkerIndex( INDEX_NONE				)
,	Outer       ( NULL						)
,	ObjectFlags	( InFlags | RF_Native	    )
,	Name        ( NAME_None					)
,	ObjectArchetype(NULL)
{
	guard(UObject::UObject);

	// Setup registration info, for processing now (if inited) or later (if starting up).
	check(sizeof(Outer       )>=sizeof(InPackageName));
	//check(sizeof(Name        )>=sizeof(InName       ));
	//check(sizeof(_LinkerIndex)>=sizeof(GAutoRegister));
	*(const TCHAR  **)&Outer        = InPackageName;
	*(const TCHAR  **)&Name         = InName;

	// If we are not initialized yet, auto register.
	if (!GObjInitialized)
	{
		*(UObject      **)&_LinkerIndex = GAutoRegister;
		GAutoRegister = this;
	}

	unguard;
}
UObject& UObject::operator=( const UObject& Src )
{
	guard(UObject::operator=);
	check(&Src);
	if( Src.GetClass()!=GetClass() )
		appErrorf( TEXT("Attempt to assign %ls from %ls"), GetFullName(), Src.GetFullName() );
	return *this;
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UObject class initializer.
-----------------------------------------------------------------------------*/

void UObject::StaticConstructor()
{
	guard(UObject::StaticConstructor);
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject implementation.
-----------------------------------------------------------------------------*/

//
// Rename this object to a unique name.
//
void UObject::Rename( const TCHAR* InName, UObject* NewOuter )
{
	guard(UObject::Rename);

	FName NewName = InName ? FName(InName) : MakeUniqueObjectName( GetOuter(), GetClass() );
	UnhashObject( Outer ? Outer->GetIndex() : 0 );
	debugfSlow( TEXT("Renaming %ls to %ls"), *Name, *NewName );
	Name = NewName;
	if (NewOuter != ((UObject*)-1) && Outer != NewOuter) // 227j: allow change packages too!
	{
		Outer = NewOuter;
		onOuterChanged(); // Allow object change outer of sub-objects too!
	}
	HashObject();

	unguardobj;
}

//
// Shutdown after a critical error.
//
void UObject::ShutdownAfterError()
{
	guard(UObject::ShutdownAfterError);
	unguard;
}

//
// Make sure the object is valid.
//
UBOOL UObject::IsValid()
{
	guard(UObject::IsValid);
	try
	{
		if (!const_cast<volatile UObject*>(this))
		{
			debugf( NAME_Warning, TEXT("NULL object") );
			return 0;
		}
		else if( !GObjObjects.IsValidIndex(GetIndex()) )
		{
			debugf( NAME_Warning, TEXT("Invalid object index %i"), GetIndex() );
			debugf( NAME_Warning, TEXT("This is: %ls"), GetName() );
			return 0;
		}
		else if( GObjObjects(GetIndex())==NULL )
		{
			debugf( NAME_Warning, TEXT("Empty slot") );
			debugf( NAME_Warning, TEXT("This is: %ls"), GetName() );
			return 0;
		}
		else if( GObjObjects(GetIndex())!=this )
		{
			debugf( NAME_Warning, TEXT("Other object in slot") );
			debugf( NAME_Warning, TEXT("This is: %ls"), GetName() );
			debugf( NAME_Warning, TEXT("Other is: %ls"), GObjObjects(GetIndex())->GetName() );
			return 0;
		}
		else return 1;
	}
	catch(...)
	{
		debugf( NAME_Warning, TEXT("Object threw an exception") );
		return 0;
	}
	unguardobj;
}

//
// Do any object-specific cleanup required
// immediately after loading an object, and immediately
// after any undo/redo.
//
void UObject::PostLoad()
{
	guard(UObject::PostLoad);

	// Note that it has propagated.
	SetFlags( RF_DebugPostLoad );

	unguardobj;
}

//
// Edit change notification.
//
void UObject::PostEditChange()
{
	guard(UObject::PostEditChange);
	Modify();
	unguardobj;
}

//
// Do any object-specific cleanup required immediately
// before an object is killed.  Child classes may override
// this if they have to do anything here.
//
void UObject::Destroy()
{
	guard(UObject::Destroy);

	// Destroy properties.
	/*if (!(Class->GetFlags() & RF_Native) && UnrealScriptData.Num())
	{
		// Marco: Eh this is so short and unique code so we can just aswell inline it here.
		guard(ExitProperties);
		BYTE* Data = &UnrealScriptData(0);

		for (UProperty* P = Class->ConstructorLink; P && !(P->GetOuter()->GetFlags() & RF_Native); P = P->ConstructorLinkNext)
			if (!(P->GetFlags() & RF_NeedLoad))
				P->DestroyValue(Data + P->Offset);
		UnrealScriptData.Empty();
		unguard;
	}*/

	// Log message.
#if DO_GUARD_SLOW
	if( GObjInitialized && !GIsCriticalError )
		debugfSlow( NAME_DevKill, TEXT("Destroying %ls"), GetFullName() );
#endif

	// Unhook from linker.
	SetLinker( NULL, INDEX_NONE );

	// Remember outer name index.
	_LinkerIndex = Outer ? Outer->GetIndex() : 0;

	unguardobj;
}

//
// Set the object's linker.
//
void UObject::SetLinker( ULinkerLoad* InLinker, INT InLinkerIndex )
{
	guard(UObject::SetLinker);

	// Detach from existing linker.
	if( _Linker )
	{
		check(_Linker->ExportMap(_LinkerIndex)._Object!=NULL);
		check(_Linker->ExportMap(_LinkerIndex)._Object==this);
		_Linker->ExportMap(_LinkerIndex)._Object = NULL;
	}

	// Set new linker.
	_Linker      = InLinker;
	_LinkerIndex = InLinkerIndex;

	unguardobj;
}

static void GetPathNameInner(const UObject* Obj, const UObject* StopOuter, TCHAR* Str)
{
	UObject* Outer = Obj->GetOuter();

	if (Outer && Outer != StopOuter)
	{
		GetPathNameInner(Outer, StopOuter, Str);
		appStrcat(Str, TEXT("."));
	}
	appStrcat(Str, Obj->GetName());
}

//
// Return the object's path name.
//warning: Must be safe for NULL objects.
//warning: Must be safe for class-default meta-objects.
//
const TCHAR* UObject::GetPathName( UObject* StopOuter ) const
{
	guard(UObject::GetPathName);

	if (!const_cast<volatile UObject*>(this))
		return TEXT("None");

	// Return one of 8 circular results.
	TCHAR* Result = appStaticString1024();
	try
	{
		*Result = 0;
		GetPathNameInner(this, StopOuter, Result);
	}
	catch(...)
	{
		appSprintf(Result, TEXT("<Invalid GetPathName %p>"), (void*)this);
	}

	return Result;

	unguard; // Not unguardobj, because it causes crash recusion.
}

FString UObject::GetPathNameSafe(UObject* StopOuter) const
{
	guard(UObject::GetPathName);
	FString Result;
	if (this != StopOuter)
	{
		if (Outer != StopOuter)
			Result = Result + Outer->GetPathNameSafe(StopOuter) + TEXT(".");
		Result = Result + GetName();
	}
	else
		Result = TEXT("None");

	return Result;
	unguard; // Not unguardobj, because it causes crash recusion.
}

//
// Return the object's full name.
//warning: Must be safe for NULL objects.
//
const TCHAR* UObject::GetFullName() const
{
	guard(UObject::GetFullName);

	if (!const_cast<volatile UObject*>(this))
		return TEXT("None");

	// Return one of 8 circular results.
	TCHAR* Result = appStaticString1024();
	try
	{
		appSprintf(Result, TEXT("%ls "), GetClass()->GetName());
		GetPathNameInner(this, NULL, Result);
	}
	catch (...)
	{
		appSprintf(Result, TEXT("<Invalid GetFullName %p>"), (void*)this);
	}

	return Result;
	unguard; // Not unguardobj, because it causes crash recusion.
}

FString UObject::GetFullNameSafe() const
{
	guard(UObject::GetFullNameSafe);
	FString Result;
	if (this)
		Result = FString(GetClass()->GetName()) + TEXT(" ") + GetPathNameSafe();
	else
		Result = TEXT("None");
	return Result;
	unguard; // Not unguardobj, because it causes crash recusion.
}

//
// Destroy the object if necessary.
//
UBOOL UObject::ConditionalDestroy()
{
	guard(UObject::ConditionalDestroy);
	if( Index!=INDEX_NONE && !(GetFlags() & RF_BeginDestroyed) )
	{
		SetFlags(RF_BeginDestroyed);
		Destroy();
		SetFlags(RF_FinishDestroyed);
		return 1;
	}
	else return 0;
	unguardobj;
}

//
// Register if needed.
//
void UObject::ConditionalRegister()
{
	guard(UObject::ConditionalRegister);
	if( GetIndex()==INDEX_NONE )
	{
		// Verify this object is on the list to register.
		INT i;
		for( i=0; i<GObjRegistrants.Num(); i++ )
			if( GObjRegistrants(i)==this )
				break;
		check(i!=GObjRegistrants.Num());

		// Register it.
		Register();
	}
	unguardobj;
}

//
// PostLoad if needed.
//
void UObject::ConditionalPostLoad()
{
	guard(UObject::ConditionalPostLoad);
	if( GetFlags() & RF_NeedPostLoad )
	{
		ClearFlags( RF_NeedPostLoad | RF_DebugPostLoad );
		PostLoad();
		if( !(GetFlags() & RF_DebugPostLoad) )
 			appErrorf( TEXT("%ls failed to route PostLoad"), GetFullName() );
	}
	unguardobj;
}

//
// UObject destructor.
//warning: Called at shutdown.
//
UObject::~UObject() noexcept(false)
{
	guard(UObject::~UObject);

	if (GExitPurge)
		return;

	// If not initialized, skip out.
	if( Index!=INDEX_NONE && GObjInitialized && !GIsCriticalError )
	{
		// Validate it.
		check(IsValid());

		// Destroy the object if necessary.
		ConditionalDestroy();

		// Remove object from table.
		UnhashObject( _LinkerIndex );
		if (GObjObjects.Num())
		{
			GObjObjects(Index) = NULL;
			GObjAvailable.AddItem( Index );
		}
	}

	// Free execution stack.
	if( StateFrame )
	{
		delete[] StateFrame;
		StateFrame=NULL;
	}

	unguard;
}

//
// Archive for counting memory usage.
//
class FArchiveCountMem : public FArchive
{
public:
	FArchiveCountMem( UObject* Src )
	: Num(0), Max(0)
	{
		Src->Serialize( *this );
	}
	size_t GetNum()
	{
		return Num;
	}
	size_t GetMax()
	{
		return Max;
	}
	void CountBytes( size_t InNum, size_t InMax )
	{
		Num += InNum;
		Max += InMax;
	}
protected:
	size_t Num, Max;
};

//
// Archive for persistent CRC's.
//
class FArchiveCRC : public FArchive
{
public:
	FArchiveCRC( UObject* Src, UObject* InBase )
	: CRC( 0 )
	, Base( InBase )
	{
		ArIsSaving = ArIsPersistent = 1;
		Src->Serialize( *this );
	}
	size_t GetCRC()
	{
		return CRC;
	}
	void Serialize( void* Data, INT Num )
	{
		CRC = appMemCrc( Data, Num, CRC );
	}
	void SerializeCapsString( const TCHAR* Str )
	{
		TCHAR TCh;
		while( *Str )
		{
			TCh = appToUpper(*Str++);
			*this << TCh;
		}
		TCh = 0;
		*this << TCh;
	}
	virtual FArchive& operator<<( class FName& N )
	{
		SerializeCapsString( *N );
		return *this;
	}
	virtual FArchive& operator<<( class UObject*& Res )
	{
		if( Res )
		{
			SerializeCapsString( Res->GetClass()->GetName() );
			SerializeCapsString( Res->GetPathName(Res->IsIn(Base) ? Base : NULL) );
		}
		else SerializeCapsString( TEXT("None") );
		return *this;
	}
protected:
	DWORD CRC;
	UObject* Base;
};

//
// Note that the object has been modified.
//
void UObject::Modify()
{
	guard(UObject::Modify);

	// Perform transaction tracking.
	if( GUndo && (GetFlags() & RF_Transactional) )
		GUndo->SaveObject( this );

	unguardobj;
}

/** serializes NetIndex from the passed in archive; in a separate function to share with default object serialization */
void UObject::SerializeNetIndex(FArchive& Ar)
{
	// do not serialize NetIndex when duplicating objects via serialization
	INT InNetIndex = NetIndex;
	Ar << InNetIndex;
	/*if (Ar.IsLoading())
	{
#if SUPPORTS_SCRIPTPATCH_CREATION
		//@script patcher
		if (GIsScriptPatcherActive || _Linker == NULL || _Linker->LinkerRoot == NULL || (_Linker->LinkerRoot->PackageFlags & PKG_Cooked))
#else
		if (_Linker == NULL || _Linker->LinkerRoot == NULL || (_Linker->LinkerRoot->PackageFlags & PKG_Cooked))
#endif
		{
			// use serialized net index for cooked packages
			SetNetIndex(InNetIndex);
		}
		// set net index from linker
		else if (_Linker != NULL && _LinkerIndex != INDEX_NONE)
		{
			SetNetIndex(_LinkerIndex);
		}
	}*/
}

void UObject::SerializeEmpty(FArchive& Ar)
{
	guard(UObject::SerializeEmpty);
	// Make sure this object's class's data is loaded.
	if (Class != UClass::StaticClass())
	{
		Ar.Preload(Class);

		if (!(ObjectFlags & RF_ClassDefaultObject) && Class->GetDefaultsCount() > 0)
			Ar.Preload(Class->GetDefaultObject());
	}

	// Special info.
	if ((!Ar.IsLoading() && !Ar.IsSaving()) || Ar.IsTrans())
		Ar << Name << Outer << Class;
	if (!Ar.IsLoading() && !Ar.IsSaving())
		Ar << _Linker;
	unguardobj;
}

//
// UObject serializer.
//
void UObject::Serialize( FArchive& Ar )
{
	guard(UObject::Serialize);

	// Make sure this object's class's data is loaded.
	if (Class != UClass::StaticClass())
	{
		Ar.Preload(Class);

		// if this object's class is intrinsic, its properties may not have been linked by this point, if a non-intrinsic
		// class has a reference to an intrinsic class in its script or defaultproperties....so make osure the class is linked
		// before any data is loaded
		//if (Ar.IsLoading())
			//Class->ConditionalLink();

		// make sure this object's template data is loaded - the only objects
		// this should actually affect are those that don't have any defaults
		// to serialize.  for objects with defaults that actually require loading
		// the class default object should be serialized in ULinkerLoad::Preload, before
		// we've hit this code.
		if (!(ObjectFlags & RF_ClassDefaultObject) && Class->GetDefaultsCount() > 0)
		{
			Ar.Preload(Class->GetDefaultObject());
		}
	}

	// Special info.
	if( (!Ar.IsLoading() && !Ar.IsSaving()) || Ar.IsTrans() )
		Ar << Name << Outer << Class;
	if( !Ar.IsLoading() && !Ar.IsSaving() )
		Ar << _Linker;

	// Execution stack.
	//!!how does the stack work in conjunction with transaction tracking?
	guard(SerializeStack);
	if( !Ar.IsTrans() )
	{
		if( GetFlags() & RF_HasStack )
		{
			if( !StateFrame )
				StateFrame = NewTagged(TEXT("ObjectStateFrame")) FStateFrame( this );
			Ar << StateFrame->Node << StateFrame->StateNode;
			if (Ar.Ver() < VER_REDUCED_PROBEMASK_REMOVED_IGNOREMASK)
			{
				QWORD ProbeMask = 0;
				Ar << ProbeMask;
				// old mask was scrambled due to probe name changes, so just reset
				StateFrame->ProbeMask = (StateFrame->StateNode != NULL) ? (StateFrame->StateNode->ProbeMask | GetClass()->ProbeMask) : GetClass()->ProbeMask;
			}
			else
			{
				Ar << StateFrame->ProbeMask;
			}
			if (Ar.Ver() < VER_REDUCED_STATEFRAME_LATENTACTION_SIZE)
			{
				INT LatentAction = 0;
				Ar << LatentAction;
				StateFrame->LatentAction = WORD(LatentAction);
			}
			else Ar << StateFrame->LatentAction;

			Ar << StateFrame->StateStack;
			if (StateFrame->Node)
			{
				Ar.Preload(StateFrame->Node);
				INT Offset = StateFrame->Code ? StateFrame->Code - &StateFrame->Node->Script(0) : INDEX_NONE;
				Ar << Offset;
				StateFrame->Code = Offset != INDEX_NONE ? &StateFrame->Node->Script(Offset) : NULL;
			}
			else StateFrame->Code = NULL;
		}
		else if( StateFrame )
		{
			delete[] StateFrame;
			StateFrame = NULL;
		}
	}
	unguard;

	// if this is a subobject, then we need to copy the source defaults from the template subobject living in
	// inside the class in the .u file
	if (IsAComponent())
	{
		((UComponent*)this)->PreSerialize(Ar);
	}

	SerializeNetIndex(Ar);

	// Serialize object properties which are defined in the class.
	if (Class != UClass::StaticClass())
	{
		if (Class->bDisableInstancing)
		{
			if (Ar.IsSaving() && !Ar.IsObjectReferenceCollector())
				appErrorf(TEXT("Can't save object %ls!"), GetFullName());
		}
		else if (Ar.IsLoading() || Ar.IsSaving())
		{
			if(Class->ClassFlags | CLASS_NoScriptProps)
				GetClass()->SerializeTaggedProperties(Ar, NULL, Class, NULL, 0);
			else
			{
				UObject* DiffObject = GetArchetype();
				if (!DiffObject)
					DiffObject = Class->GetDefaultObject();
				GetClass()->SerializeTaggedProperties(Ar, GetScriptData(), HasAnyFlags(RF_ClassDefaultObject) ? Class->GetSuperClass() : Class, DiffObject->GetScriptData(), DiffObject->Class->GetPropertiesSize());
			}
		}
		else
			GetClass()->SerializeBin( Ar, GetScriptData() );
	}

	// Memory counting.
	size_t Size = GetClass()->GetPropertiesSize();
	Ar.CountBytes( Size, Size );
	unguardobj;
}

//
// Export text object properties.
//
// Helper function for UObject::ExportProperties
inline const TCHAR* GetSpaces( INT Num )
{
	if (Num < 0)
    {
		// stijn: lol guys seriously. did you even test this? :D
		// return 0;
		return TEXT("");
    }
	static TCHAR Res[256];
	for( INT i=0; i<Num; ++i )
		Res[i] = '\t';
	Res[Num] = 0;
	return Res;
}

void UObject::ExportProperties
(
	FOutputDevice&	Out,
	UClass*			ObjectClass,
	BYTE*			Object,
	INT				Indent,
	UClass*			DiffClass,
	BYTE*			Diff
)
{
	const TCHAR* ThisName = TEXT("(none)");
	guard(UObject::ExportProperties);
	check(ObjectClass!=NULL);
	for( TFieldIterator<UProperty> It(ObjectClass); It; ++It )
	{
		if( It->Port() )
		{
			if( It->GetFName()==NAME_Name )
				continue; // Don't export the name!

			ThisName = It->GetName();
			for( INT j=0; j<It->ArrayDim; j++ )
			{
				// Export dynamic array.
				if( It->IsA(UArrayProperty::StaticClass()) )
				{
					// Export dynamic array.
					UProperty* Inner = ((UArrayProperty*)*It)->Inner;
					bool bExpObj = (It->PropertyFlags & CPF_ExportObject) && Inner->IsA(UObjectProperty::StaticClass());

					FArray* Array = (FArray*)(Object + It->Offset + j*It->ElementSize);
					FArray*	Dif = (DiffClass && DiffClass->IsChildOf(It.GetStruct())) ?
						(FArray*)(Diff + It->Offset + j*It->ElementSize) : NULL;

					if( !Array->Num() && Dif && Dif->Num() ) // Empty array.
						Out.Logf( TEXT("%ls%ls.Empty\r\n"), GetSpaces(Indent), It->GetName() );
					else
					{
						if( Dif && Array->Num()<Dif->Num() )
						{
							// Empty array to remove any old entries
							Out.Logf( TEXT("%ls%ls.Empty()\r\n"), GetSpaces(Indent), It->GetName() );
							Dif = NULL;
						}
						for( INT z=0; z<Array->Num(); z++ )
						{
							FString	Value=TEXT("");
							if( Inner->ExportText( z, Value, (BYTE*)Array->GetData(), Dif && z < Dif->Num() ? (BYTE*)Dif->GetData() : NULL, PPF_Delimited ) )
							{
								if( bExpObj )
								{
									UObject* Obj = ((UObject**)Array->GetData())[z];
									if( Obj && !(Obj->GetFlags() & RF_TagImp) )
									{
										// Don't export more than once.
										Out.Logf( TEXT("\r\n") );
										UExporter::ExportToOutputDevice( Obj, NULL, Out, TEXT("T3D"), Indent );
										Obj->SetFlags( RF_TagImp );
									}
								}

								Out.Logf( TEXT("%ls%ls(%i)=%ls\r\n"), GetSpaces(Indent), It->GetName(), z, *Value );
							}
						}
					}
				}
				else
				{
					// Export single element.
					FString Value =TEXT("");
					if( It->ExportText( j, Value, Object, (DiffClass && DiffClass->IsChildOf(It.GetStruct())) ? Diff : NULL, PPF_Delimited ) )
					{
						if
						(	(It->IsA(UObjectProperty::StaticClass()))
						&&	(It->PropertyFlags & CPF_ExportObject) )
						{
							UObject* Obj = *(UObject **)(Object + It->Offset + j*It->ElementSize);
							if( Obj && !(Obj->GetFlags() & RF_TagImp) )
							{
								// Don't export more than once.
								Out.Logf( TEXT("\r\n") );
								UExporter::ExportToOutputDevice( Obj, NULL, Out, TEXT("T3D"), Indent );
								Obj->SetFlags( RF_TagImp );
							}
						}
						if( It->ArrayDim == 1 )
							Out.Logf( TEXT("%ls%ls=%ls\r\n"), GetSpaces(Indent), It->GetName(), *Value );
						else
							Out.Logf( TEXT("%ls%ls(%i)=%ls\r\n"), GetSpaces(Indent), It->GetName(), j, *Value );
					}
				}
			}
		}
	}
	unguardf(( TEXT("(%ls)"), ThisName ));
}
//
// Initialize script execution.
//
void UObject::InitExecution()
{
	guard(UObject::InitExecution);
	check(GetClass()!=NULL);

	if( StateFrame )
		delete[] StateFrame;
	StateFrame = NewTagged(TEXT("ObjectStateFrame"))FStateFrame( this );
	SetFlags( RF_HasStack );

	unguardobj;
}

void UObject::EndExecution()
{
	if( StateFrame )
		delete[] StateFrame;
	StateFrame = NULL;
	ClearFlags( RF_HasStack );
}

//
// Return whether an object wants to receive a named probe message.
//
UBOOL UObject::IsProbing( FName ProbeName )
{
	guardSlow(UObject::IsProbing);
	return	(ProbeName.GetIndex() <  NAME_PROBEMIN)
	||		(ProbeName.GetIndex() >= NAME_PROBEMAX)
	||		(!StateFrame)
	||		(StateFrame->ProbeMask & ((QWORD)1 << (ProbeName.GetIndex() - NAME_PROBEMIN)));
	unguardobjSlow;
}

//
// Find an UnrealScript field.
//warning: Must be safe with class default metaobjects.
//
UField* UObject::FindObjectField( FName InName, UBOOL Global )
{
	guardSlow(UObject::StaticFindField);
	// Search current state scope.
	if( StateFrame && StateFrame->StateNode && !Global )
		for( TFieldIterator<UField> It(StateFrame->StateNode); It; ++It )
			if( It->GetFName()==InName )
				return *It;

	// Search the global scope.
	for( TFieldIterator<UField> It(GetClass()); It; ++It )
		if( It->GetFName()==InName )
			return *It;

	// Not found.
	return NULL;
	unguardfSlow(( TEXT("%ls (node %ls)"), GetFullName(), *InName ));
}
UFunction* UObject::FindFunction( FName InName, UBOOL Global )
{
	guardSlow(UObject::FindFunction);
	return Cast<UFunction>(FindObjectField(InName, Global));
	unguardfSlow(( TEXT("%ls (function %ls)"), GetFullName(), *InName ));
}
UFunction* UObject::FindFunctionChecked( FName InName, UBOOL Global )
{
	guardSlow(UObject::FindFunctionChecked);
	if( !GIsScriptable )
		return NULL;
	UFunction* Result = Cast<UFunction>(FindObjectField(InName, Global));
	//debugf( TEXT("Found function %ls in %ls"), *InName, GetFullName() );
	if( !Result )
		appErrorf( TEXT("Failed to find function %ls in %ls"), *InName, GetFullName() );
	return Result;
	unguardfSlow(( TEXT("%ls (function %ls)"), GetFullName(), *InName ));
}
UState* UObject::FindState( FName InName )
{
	guardSlow(UObject::FindState);
	return Cast<UState>(FindObjectField(InName, 1));
	unguardfSlow(( TEXT("%ls (state %ls)"), GetFullName(), *InName ));
}
IMPLEMENT_CLASS(UObject);

/*-----------------------------------------------------------------------------
	Mo Functions.
-----------------------------------------------------------------------------*/

//
// Object accessor.
//
UObject* UObject::GetIndexedObject( INT Index )
{
	guardSlow(UObject::GetIndexedObject);
	if( Index>=0 && Index<GObjObjects.Num() )
		return GObjObjects(Index);
	else
		return NULL;
	unguardSlow;
}

//
// Find an optional object.
//
UObject* UObject::StaticFindObject( UClass* ObjectClass, UObject* InObjectPackage, const TCHAR* InName, UBOOL ExactClass )
{
	guard(UObject::StaticFindObject);

	// Resolve the object and package name.
	UObject* ObjectPackage = InObjectPackage!=ANY_PACKAGE ? InObjectPackage : NULL;
	FString InNameString = InName;
	if (!ResolveName(ObjectPackage, InNameString, 0, 0))
		return NULL;

    if (!InNameString.Len())
        return NULL;

	// Make sure it's an existing name.
	FName ObjectName(*InNameString,FNAME_Find);
	if( ObjectName==NAME_None )
		return NULL;

	// Find in the specified package.
	INT iHash = GetObjectHash(ObjectName, ObjectPackage ? ObjectPackage->GetIndex() : 0);
	for (UObject* Hash = GObjHash[iHash]; Hash != NULL; Hash = Hash->HashNext)
		if
			((Hash->GetFName() == ObjectName)
				&& (Hash->Outer == ObjectPackage)
				&& (ObjectClass == NULL || (ExactClass ? Hash->GetClass() == ObjectClass : Hash->IsA(ObjectClass))))
			return Hash;

	// Find in any package.
	if (InObjectPackage == ANY_PACKAGE)
	{
		for (FObjectIterator It(ObjectClass ? ObjectClass : UObject::StaticClass()); It; ++It)
			if((It->GetFName() == ObjectName)
				&& (ObjectClass == NULL || (ExactClass ? It->GetClass() == ObjectClass : It->IsA(ObjectClass))))
				return *It;
	}
#if 0
	else
	{
		for (FObjectIterator It(ObjectClass ? ObjectClass : UObject::StaticClass()); It; ++It)
			if ((It->GetFName() == ObjectName) && (It->Outer == ObjectPackage)
				&& (ObjectClass == NULL || (ExactClass ? It->GetClass() == ObjectClass : It->IsA(ObjectClass))))
				return *It;
	}
#endif

	// Not found.
	return NULL;
	unguard;
}

//
// Find an object; can't fail.
//
UObject* UObject::StaticFindObjectChecked( UClass* ObjectClass, UObject* ObjectParent, const TCHAR* InName, UBOOL ExactClass )
{
	guard(UObject::StaticFindObjectChecked);
	UObject* Result = StaticFindObject( ObjectClass, ObjectParent, InName, ExactClass );
	if( !Result )
		appErrorf( TEXT("Failed to find object '%ls %ls.%ls'"), ObjectClass->GetName(), ObjectParent==ANY_PACKAGE ? TEXT("Any") : ObjectParent ? ObjectParent->GetName() : TEXT("None"), InName );
	return Result;
	unguard;
}

//
// Binary initialize object properties to zero or defaults.
//
void UObject::InitProperties( BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* Defaults, INT DefaultsCount, UObject* RefObj)
{
	guard(UObject::InitProperties);
	if (!Data)
		return;
	INT Inited = Max<INT>(UObject::StaticClass()->GetPropertiesSize(), sizeof(UObject*));

	// Find class defaults if no template was specified.
	//warning: At startup, DefaultsClass->Defaults.Num() will be zero for some native classes.
	guardSlow(FindDefaults);
	if( !Defaults && DefaultsClass && DefaultsClass->GetDefaultsCount() )
	{
		Defaults      = DefaultsClass->GetDefaults();
		DefaultsCount = DefaultsClass->GetDefaultsCount();
	}
	unguardSlow;

	// Copy defaults.
	guardSlow(DefaultsFill);
	if( Defaults )
	{
		checkSlow(DefaultsCount>=Inited);
		checkSlow(DefaultsCount<=DataCount);
		appMemcpy( Data+Inited, Defaults+Inited, DefaultsCount-Inited );
		Inited = DefaultsCount;
	}
	unguardSlow;

	// Zero-fill any remaining portion.
	guardSlow(ZeroFill);
	if( Inited < DataCount )
		appMemzero( Data+Inited, DataCount-Inited );
	unguardSlow;

	// Construct anything required.
	guardSlow(ConstructProperties);
	if (DefaultsClass && Defaults)
	{
		for( UProperty* P=DefaultsClass->ConstructorLink; P; P=P->ConstructorLinkNext )
		{
			if( P->Offset < DefaultsCount )
			{
				appMemzero( Data + P->Offset, P->GetSize() );//bad for bools, but not a real problem because they aren't constructed!!
				P->CopyCompleteValue( Data + P->Offset, Defaults + P->Offset, RefObj );
			}
		}
	}
	unguardSlow;

	unguard;
}

//
// Destroy properties.
//
void UObject::ExitProperties( BYTE* Data, UClass* Class )
{
	if (!Data)
		return;
	UProperty* P=NULL;
	guard(UObject::ExitProperties);
	if (Class->bDisableInstancing)
		return;
	for (P = Class->ConstructorLink; P; P = P->ConstructorLinkNext)
		if (!(P->GetFlags() & RF_NeedLoad))
			P->DestroyValue(Data + P->Offset);
	unguardf(( TEXT("(%ls -> %ls)"), Class->GetFullName(), P->GetFullName()));
}

//
// Init class default object.
//
void UObject::InitClassDefaultObject( UClass* InClass )
{
	guard(UObject::InitClassDefaultObject);

	// Init UObject portion.
	//appMemset( this, 0, sizeof(UObject) );
	//*(void**)this = *(void**)InClass;
	//Class         = InClass;
	//Index         = INDEX_NONE;

	// Init post-UObject portion.
	BYTE* Data = GetScriptData();
	if (Data)
		SafeInitProperties(Data, InClass->GetPropertiesSize(), InClass->GetSuperClass(), NULL, 0, NULL);
	//InitProperties( (BYTE*)this, InClass->GetPropertiesSize(), InClass->GetSuperClass(), NULL, 0 );
	unguardobj;
}

void UObject::SafeInitProperties(BYTE* Data, INT DataCount, UClass* DefaultsClass, BYTE* DefaultData, INT DefaultsCount, UObject* DestObject/*=NULL*/, UObject* SubobjectRoot/*=NULL*/)
{
	guard(UObject::SafeInitProperties);
	if (HasAnyFlags(RF_InitializedProps))
	{
		// since this is not a new UObject, it's likely that it already has existing values
		// for all of its UProperties.  We must destroy these existing values in a safe way.
		// Failure to do this can result in memory leaks in FScriptArrays.
		ExitProperties(Data, GetClass());
	}

	SetFlags(RF_InitializedProps);
	InitProperties(Data, DataCount, DefaultsClass, DefaultData, DefaultsCount, DestObject);
	unguard;
}

/*-----------------------------------------------------------------------------
	Object registration.
-----------------------------------------------------------------------------*/

//
// Preregister an object.
//warning: Sometimes called at startup time.
//
void UObject::Register()
{
	guard(UObject::Register);
	check(GObjInitialized);

    // Get stashed registration info.
	const TCHAR* InOuter = *(const TCHAR**)&Outer;
	const TCHAR* InName  = *(const TCHAR**)&Name;

	// Set object properties.
	Outer        = CreatePackage(NULL,InOuter);
	Name         = InName;
	_LinkerIndex = INDEX_NONE;

	// Validate the object.
	if( Outer==NULL )
		appErrorf( TEXT("Autoregistered object %ls is unpackaged"), GetFullName() );
	if( GetFName()==NAME_None )
		appErrorf( TEXT("Autoregistered object %ls has invalid name"), GetFullName() );
	if( StaticFindObject( NULL, GetOuter(), GetName() ) )
		appErrorf( TEXT("Autoregistered object %ls already exists"), GetFullName() );

	// Add to the global object table.
	AddObject( INDEX_NONE );

	unguard;
}

/*-----------------------------------------------------------------------------
	StaticInit & StaticExit.
-----------------------------------------------------------------------------*/

//
// Init the object manager and allocate tables.
//
void UObject::StaticInit()
{
	guard(UObject::StaticInit);
	GObjNoRegister = 1;

	// Checks.
	check(sizeof(BYTE)==1);
	check(sizeof(SBYTE)==1);
	check(sizeof(_WORD)==2);
	check(sizeof(DWORD)==4);
	check(sizeof(QWORD)==8);
	check(sizeof(ANSICHAR)==1);
#if UNICODE && __GNUG__
	check(sizeof(UNICHAR)==4);
#else
    check(sizeof(UNICHAR)==2);
#endif
	check(sizeof(SWORD)==2);
	check(sizeof(INT)==4);
	check(sizeof(SQWORD)==8);
	check(sizeof(UBOOL)==4);
	check(sizeof(FLOAT)==4);

	// Development.
	GCheckConflicts = ParseParam(appCmdLine(),TEXT("CONFLICTS"));
	GNoGC           = 0;

	// Init hash.
	for( INT i=0; i<ARRAY_COUNT(GObjHash); i++ )
		GObjHash[i] = NULL;

	// If statically linked, initialize registrants.
	#if __STATIC_LINK
		AUTO_INITIALIZE_REGISTRANTS;
	#endif

	// Note initialized.
	GObjInitialized = 1;

	// Add all autoregistered classes.
	ProcessRegistrants();

	// Allocate special packages.
	GObjTransientPkg = new( NULL, TEXT("Transient") )UPackage;
	GObjTransientPkg->AddToRoot();

	debugf( NAME_Init, TEXT("Object subsystem initialized") );
	unguard;
}

//
// Process all objects that have registered.
//
void UObject::ProcessRegistrants()
{
	guard(UObject::ProcessRegistrants);
	if( ++GObjRegisterCount==1 )
	{
		// Make list of all objects to be registered.
		// stijn: uses UCompressedPointer now. See comment in UObject ENativeConstructor
		for( ; GAutoRegister; GAutoRegister=*(UObject **)&GAutoRegister->_LinkerIndex )
			GObjRegistrants.AddItem(GAutoRegister);
		for (INT i = 0; i < GObjRegistrants.Num(); i++)
		{
			GObjRegistrants(i)->ConditionalRegister();
			for (; GAutoRegister; GAutoRegister = *(UObject**)&GAutoRegister->_LinkerIndex)
				GObjRegistrants.AddItem(GAutoRegister);
		}
		GObjRegistrants.Empty();
		check(!GAutoRegister);
	}
	GObjRegisterCount--;
	unguard;
}

//
// Shut down the object manager.
//
void UObject::StaticExit()
{
	guard(UObject::StaticExit);
	check(GObjLoaded.Num()==0);
	check(GObjRegistrants.Num()==0);
	check(!GAutoRegister);

	// Cleanup root.
	GObjTransientPkg->RemoveFromRoot();

	// Tag all objects as unreachable.
	for( FObjectIterator It; It; ++It )
		It->SetFlags( RF_Unreachable | RF_TagGarbage );

	// Tag all names as unreachable.
	for( INT i=0; i<FName::GetMaxNames(); i++ )
		if( FName::GetEntry(i) )
			FName::GetEntry(i)->Flags |= RF_Unreachable;

	// Purge all objects.
    #if __GNUG__
	GObjObjects			.Empty();
	#endif
	GExitPurge = 1;
	PurgeGarbage();

	// Empty arrays to prevent falsely-reported memory leaks.
	GObjLoaded			.Empty();
	GObjObjects			.Empty();
	GObjAvailable		.Empty();
	GObjLoaders			.Empty();
	GObjRoot			.Empty();
	GObjRegistrants		.Empty();
	GObjPreferences		.Empty();
	GObjDrivers			.Empty();
	delete GObjPackageRemap;

	GObjInitialized = 0;
	debugf( NAME_Exit, TEXT("Object subsystem successfully closed.") );
	unguard;
}

/*-----------------------------------------------------------------------------
   Shutdown.
-----------------------------------------------------------------------------*/

//
// Make sure this object has been shut down.
//
void UObject::ConditionalShutdownAfterError()
{
	if( !(GetFlags() & RF_ErrorShutdown) )
	{
		SetFlags( RF_ErrorShutdown );
		try
		{
			ShutdownAfterError();
		}
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object ShutdownAfterError") );
		}
	}
}

//
// After a critical error, shutdown all objects which require
// mission-critical cleanup, such as restoring the video mode,
// releasing hardware resources.
//
void UObject::StaticShutdownAfterError()
{
	guard(UObject::StaticShutdownAfterError);
	if( GObjInitialized )
	{
		static UBOOL Shutdown=0;
		if( Shutdown )
			return;
		Shutdown = 1;
		debugf( NAME_Exit, TEXT("Executing UObject::StaticShutdownAfterError") );
		try
		{
			for( INT i=0; i<GObjObjects.Num(); i++ )
				if( GObjObjects(i) )
					GObjObjects(i)->ConditionalShutdownAfterError();
		}
		catch( ... )
		{
			debugf( NAME_Exit, TEXT("Double fault in object manager ShutdownAfterError") );
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
   File loading.
-----------------------------------------------------------------------------*/

//
// Safe load error-handling.
//
void UObject::SafeLoadError( DWORD LoadFlags, const TCHAR* Error, const TCHAR* Fmt, ... )
{
	// Variable arguments setup.
	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt );

	guard(UObject::SafeLoadError);
	if( (LoadFlags & LOAD_Quiet) == 0  ) debugf( NAME_Warning, TEXT("%ls"), TempStr );
	if( (LoadFlags & LOAD_Throw) != 0  ) appThrowf( TEXT("%ls"), Error   );
	if( (LoadFlags & LOAD_NoFail) != 0 ) appErrorf( TEXT("%ls"), TempStr );
	if( (LoadFlags & LOAD_NoWarn) == 0 ) GWarn->Logf( TEXT("%ls"), TempStr );
	unguard;
}

//
// Find or create the linker for a package.
//
ULinkerLoad* UObject::GetPackageLinker
(
	UObject*		InOuter,
	const TCHAR*	InFilename,
	DWORD			LoadFlags,
	UObject*		MainPackage
)
{
	guard(UObject::GetPackageLinker);
	check(GObjBeginLoadCount);

	// See if there is already a linker for this package.
	ULinkerLoad* Result = NULL;
	if( InOuter )
		for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
			if( GetLoader(i)->LinkerRoot == InOuter )
				Result = GetLoader( i );

#if DO_GUARD_SLOW
	if (InFilename)
		debugf( NAME_DevLoad, TEXT("Get PackageLinker for file: %ls"), InFilename );
#endif

	// Try to load the linker.
	try
	{
		// See if the linker is already loaded.
		TCHAR NewFilename[256]=TEXT("");
		if( Result )
		{
		    //debugf(TEXT("Linker found: %ls"),Result->GetFullName());
			// Linker already found.
			appStrcpy( NewFilename, TEXT("") );
		}
		else if( !InFilename )
		{
            // debugf(TEXT("No Filename yet..."));
			// Resolve filename from package name.
			if( !InOuter )
				appThrowf( TEXT("Can't resolve package name") );
			if( !appFindPackageFile( InOuter->GetName(), NewFilename ) )
			{
				appThrowf( TEXT("Can't find file for package '%ls'"), InOuter->GetName() );
			}
		}
		else
		{
		    //debugf(TEXT("Checking if file exists..."));
			// Verify that the file exists.
			if( !appFindPackageFile( InFilename, NewFilename ) )
				appThrowf( TEXT("Can't find file '%ls'"), InFilename );

			// Resolve package name from filename.
			TCHAR Tmp[256], *T=Tmp;
			//appStrcpy( Tmp, InFilename );
			appStrncpy( Tmp, InFilename, ARRAY_COUNT(Tmp) ); //elmuerte: security fix
			while( 1 )
			{
				if( appStrstr(T,PATH_SEPARATOR) )
					T = appStrstr(T,PATH_SEPARATOR)+appStrlen(PATH_SEPARATOR);
				else if( appStrstr(T,TEXT("/")) )
					T = appStrstr(T,TEXT("/"))+1;
				else if( appStrstr(T,TEXT(":")) )
					T = appStrstr(T,TEXT(":"))+1;
				else
					break;
			}
			if( appStrstr(T,TEXT(".")) )
				*appStrstr(T,TEXT(".")) = 0;
			UPackage* FilenamePkg = CreatePackage( NULL, T );

			// If no package specified, use package from file.
			if( InOuter==NULL )
			{
			    //debugf(TEXT("InOuter==NULL"));
				if( !FilenamePkg )
					appThrowf( TEXT("Can't convert filename '%ls' to package name"), InFilename );
				InOuter = FilenamePkg;
				for( INT i=0; i<GObjLoaders.Num() && !Result; i++ )
					if( GetLoader(i)->LinkerRoot == InOuter )
						Result = GetLoader(i);
			}
			else if( InOuter != FilenamePkg )//!!should be tested and validated in new UnrealEd
			{
			    //debugf(TEXT("InOuter != FilenamePkg"));
				// Loading a new file into an existing package, so reset the loader.
				debugf(NAME_DevLoad, TEXT("New File, Existing Package (%ls, %ls)"), InOuter->GetFullName(), FilenamePkg->GetFullName());
				ResetLoaders( InOuter, 0, 1 );
			}
		}

		// Create new linker.
		if( !Result )
		{
		    debugfSlow(NAME_DevLoad, TEXT("Create new linker for %ls"), NewFilename);
			Result = new ULinkerLoad( InOuter, NewFilename, LoadFlags );
		}
	}
	catch( const TCHAR* Error )
	{
		SafeLoadError( LoadFlags, Error, TEXT("Failed to load '%ls': %ls"), MainPackage ? MainPackage->GetName() : InFilename ? InFilename : InOuter ? InOuter->GetName() : TEXT("NULL"), Error );
	}

	// Success.
	return Result;
	unguard;
}

//
// Find or optionally create a package.
//
UPackage* UObject::CreatePackage( UObject* InOuter, const TCHAR* InName )
{
	guard(UObject::CreatePackage);

	FString InNameString = InName;
	ResolveName( InOuter, InNameString, 1, 0 );
	UPackage* Result = FindObject<UPackage>( InOuter, *InNameString );
	if( !Result )
		Result = new( InOuter, *InNameString, RF_Public )UPackage;
	return Result;

	unguard;
}

//
// Resolve a package and name.
//
UBOOL UObject::ResolveName( UObject*& InPackage, FString& InName, UBOOL Create, UBOOL Throw )
{
	guard(UObject::ResolveName);

	// Handle specified packages.
	const TCHAR* Start = *InName;
	const TCHAR* iFind = appFindChar(Start, '.');
	if (!iFind)
		return TRUE;
	TCHAR* PartialName = appStaticString1024();
	while (iFind)
	{
		appStrncpy(PartialName, Start, iFind - Start + 1);
		if (Create)
			InPackage = CreatePackage(InPackage, PartialName);
		else
		{
			UObject* NewPackage = FindObject<UPackage>(InPackage, PartialName);
			if (!NewPackage)
			{
				NewPackage = FindObject<UObject>(InPackage, PartialName);
				if (!NewPackage)
				{
					if (Throw)
						appThrowf(TEXT("Couldn't resolve name '%ls'"), *InName);
					return FALSE;
				}
			}
			InPackage = NewPackage;
		}
		Start = iFind + 1;
		iFind = appFindChar(Start, '.');
	}
	InName = FString(Start);
	return TRUE;
	unguard;
}

//
// Load an object.
//
UObject* UObject::StaticLoadObject( UClass* ObjectClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags )
{
	guard(UObject::StaticLoadObject);
	check(ObjectClass);
	check(InName);

    // Try to load.
    FString InNameString = InName;
	UObject* Result=NULL;
	BeginLoad();
	try
	{
		// Create a new linker object which goes off and tries load the file.
		ULinkerLoad* Linker = NULL;
		ResolveName( InOuter, InNameString, 1, 1 );
		UObject*	TopOuter = InOuter;
		while( TopOuter && TopOuter->GetOuter() )//!!can only load top-level packages from files
			TopOuter = TopOuter->GetOuter();
		if (!(LoadFlags & LOAD_DisallowFiles))
			Linker = GetPackageLinker(TopOuter, Filename, (LoadFlags | LOAD_Throw | LOAD_AllowDll));
		//!!this sucks because it supports wildcard sub-package matching of InNameString, which requires a long search.
		//!!also because linker classes require exact match
		if( Linker )
			Result = Linker->Create( ObjectClass, *InNameString, LoadFlags, 0 );
		if( !Result )
			Result = StaticFindObject( ObjectClass, InOuter, *InNameString );
		if( !Result )
			appThrowf( TEXT("Failed to find object '%ls %ls.%ls'"), ObjectClass->GetName(), InOuter ? InOuter->GetPathName() : TEXT("None"), *InNameString );
		EndLoad();
	}
	catch( const TCHAR* Error )
	{
		EndLoad();
		SafeLoadError( LoadFlags, Error, TEXT("Failed to load '%ls %ls.%ls': %ls"), ObjectClass->GetName(), InOuter ? InOuter->GetName() : TEXT("None"), *InNameString, Error );
	}
	return Result;
	unguardf(( TEXT("(%ls %ls.%ls %ls)"), ObjectClass->GetPathName(), InOuter ? InOuter->GetPathName() : TEXT("None"), InName, Filename ? Filename : TEXT("NULL") ));
}

//
// Load a class.
//
UClass* UObject::StaticLoadClass( UClass* BaseClass, UObject* InOuter, const TCHAR* InName, const TCHAR* Filename, DWORD LoadFlags )
{
	guard(UObject::StaticLoadClass);
	check(BaseClass);
	try
	{
		UClass* Class = LoadObject<UClass>(InOuter, InName, Filename, (LoadFlags | LOAD_Throw));
		if( Class && !Class->IsChildOf(BaseClass) )
			appThrowf( TEXT("%ls is not a child class of %ls."), Class->GetFullName(), BaseClass->GetFullName() );
		return Class;
	}
	catch( const TCHAR* Error )
	{
		// Failed.
		SafeLoadError( LoadFlags, Error, TEXT("%ls"), Error );
		return NULL;
	}
	unguard;
}

//
// Load all objects in a package.
//
UObject* UObject::LoadPackage( UObject* InOuter, const TCHAR* Filename, DWORD LoadFlags )
{
	guard(UObject::LoadPackage);
	UObject* Result;

    // gam ---
	if (Filename && appStrlen(Filename) == 0)
		return(NULL);
    // --- gam

	// Try to load.
	BeginLoad();
	try
	{
		// Create a new linker object which goes off and tries load the file.
		ULinkerLoad* Linker = GetPackageLinker( InOuter, Filename ? Filename : InOuter->GetName(), LoadFlags | LOAD_Throw );

		if( Linker )
		{
			if( !(LoadFlags & LOAD_Verify) )
				Linker->LoadAllObjects();
			Result = Linker->LinkerRoot;
		}
		else
			Result = NULL;

		EndLoad();
	}
	catch( const TCHAR* Error )
	{
		EndLoad();
		SafeLoadError( LoadFlags, Error, TEXT("Failed loading package: %ls (Package %ls)"), Error, Filename ? Filename : InOuter->GetName());
		Result = NULL;
	}
	return Result;
	unguard;
}

//
// Verify a linker.
//
void UObject::VerifyLinker( ULinkerLoad* Linker )
{
	guard(UObject::VerifyLinker);
	Linker->Verify();
	unguard;
}

//
// Begin loading packages.
//warning: Objects may not be destroyed between BeginLoad/EndLoad calls.
//
void UObject::BeginLoad()
{
	guard(UObject::BeginLoad);
	if( ++GObjBeginLoadCount == 1 )
	{
		// Validate clean load state.
		//check(GObjLoaded.Num()==0);
		check(!GAutoRegister);
		for( INT i=0; i<GObjLoaders.Num(); i++ )
			check(GetLoader(i)->Success);
	}
	unguard;
}

//
// End loading packages.
//
void UObject::EndLoad()
{
	guard(UObject::EndLoad);
	check(GObjBeginLoadCount>0);
	if( --GObjBeginLoadCount == 0 )
	{
		try
		{
			TArray<UObject*>	ObjLoaded;

			while(GObjLoaded.Num())
			{
				appMemcpy(&ObjLoaded(ObjLoaded.Add(GObjLoaded.Num())),&GObjLoaded(0),GObjLoaded.Num() * sizeof(UObject*));
				GObjLoaded.Empty();

				// Finish loading everything.
				//warning: Array may expand during iteration.
				guard(PreLoadObjects);
				debugfSlow( NAME_DevLoad, TEXT("Loading objects...") );
				for( INT i=0; i<ObjLoaded.Num(); i++ )
				{
					// Preload.
					UObject* Obj = ObjLoaded(i);
					{
						if( Obj->GetFlags() & RF_NeedLoad )
						{
							check(Obj->GetLinker());
							Obj->GetLinker()->Preload( Obj );
						}
					}
				}
				unguard;

				if(GObjLoaded.Num())
					continue;

				// Postload objects.
				guard(PostLoadObjects);
				for( INT i=0; i<ObjLoaded.Num(); i++ )
					ObjLoaded(i)->ConditionalPostLoad();
				unguard;
			}

			// Dissociate all linker import object references, since they
			// may be destroyed, causing their pointers to become invalid.
			guard(DissociateImports);
			/*if( GImportCount )
			{
				for( INT i=0; i<GObjLoaders.Num(); i++ )
				{
					for( INT j=0; j<GetLoader(i)->ImportMap.Num(); j++ )
					{
						FObjectImport& Import = GetLoader(i)->ImportMap(j);
						if( Import.XObject && !(Import.XObject->GetFlags() & RF_Native) )
							Import.XObject = NULL;
					}
				}
			}*/
			GImportCount=0;
			unguard;
		}
		catch( const TCHAR* Error )
		{
			// Any errors here are fatal.
			appErrorf( Error ); //elmuerte: vulnerable?
		}
	}
	unguard;
}

//
// Empty the loaders.
//
void UObject::ResetLoaders( UObject* Pkg, UBOOL DynamicOnly, UBOOL ForceLazyLoad )
{
	guard(UObject::ResetLoaders);
	INT i = 0;
	for( i=GObjLoaders.Num()-1; i>=0; i-- )
	{
		ULinkerLoad* Linker = CastChecked<ULinkerLoad>( GetLoader(i) );
		if( Pkg==NULL || Linker->LinkerRoot==Pkg )
		{
			// Fully reset the loader.
			if( ForceLazyLoad )
				Linker->DetachAllLazyLoaders( 1 );
		}
		else
		{
			for(INT j = 0;j < Linker->ImportMap.Num();j++)
			{
				if(Linker->ImportMap(j).SourceLinker && Linker->ImportMap(j).SourceLinker->LinkerRoot == Pkg)
				{
					Linker->ImportMap(j).SourceLinker = NULL;
					Linker->ImportMap(j).SourceIndex = INDEX_NONE;
				}
			}
		}
	}
	for( i=GObjLoaders.Num()-1; i>=0; i-- )
	{
		ULinkerLoad* Linker = CastChecked<ULinkerLoad>( GetLoader(i) );
		if( Pkg==NULL || Linker->LinkerRoot==Pkg )
		{
			if( !DynamicOnly )
			{
				delete Linker;
			}
		}
	}
	unguard;
}

/**
 * Returns a pointer to this object safely converted to a pointer to the specified interface class.
 *
 * @param	InterfaceClass	the interface class to use for the returned type
 *
 * @return	a pointer that can be assigned to a variable of the interface type specified, or NULL if this object's
 *			class doesn't implement the interface indicated.  Will be the same value as 'this' if the interface class
 *			isn't native.
 */
void* UObject::GetInterfaceAddress(UClass* InterfaceClass)
{
	void* Result = NULL;

	if (InterfaceClass != NULL && InterfaceClass->HasAnyClassFlags(CLASS_Interface) && InterfaceClass != UInterface::StaticClass())
	{
		if (!InterfaceClass->HasAnyClassFlags(CLASS_Native))
		{
			if (GetClass()->ImplementsInterface(InterfaceClass))
			{
				// if it isn't a native interface, the address won't be different
				Result = this;
			}
		}
		else
		{
			for (UClass* CurrentClass = GetClass(); Result == NULL && CurrentClass != NULL; CurrentClass = CurrentClass->GetSuperClass())
			{
				for (TArray<FImplementedInterface>::TIterator It(CurrentClass->Interfaces); It; ++It)
				{
					UClass* ImplementedInterfaceClass = It->Class;
					if (ImplementedInterfaceClass == InterfaceClass || ImplementedInterfaceClass->IsChildOf(InterfaceClass))
					{
						UProperty* VfTableProperty = It->PointerProperty;
						if (VfTableProperty != NULL)
						{
							checkSlow(VfTableProperty->ArrayDim == 1);
							Result = GetScriptData() + VfTableProperty->Offset;
							break;
						}
						else
						{
							// if it isn't a native interface, the address won't be different
							Result = this;
							break;
						}
					}
				}
			}
		}
	}

	return Result;
}

/*-----------------------------------------------------------------------------
	File saving.
-----------------------------------------------------------------------------*/

//
// Archive for tagging objects and names that must be exported
// to the file.  It tags the objects passed to it, and recursively
// tags all of the objects this object references.
//
class FArchiveSaveTagExports : public FArchive
{
	TArray<UObject*> PendingObj;

public:
	FArchiveSaveTagExports( UObject* InOuter )
	: Parent(InOuter)
	{
		ArIsSaving = 1;
		ArIsPersistent = 1;
		ArIsObjectReferenceCollector = 1;
	}
	void SerializeRemaining()
	{
		UObject* Obj = NULL;
		INT i;
		guard(SerializeRemaining);
		for (i = 0; i < PendingObj.Num(); ++i)
		{
			Obj = PendingObj(i);
			if (Obj->IsValid())
			{
				if (Obj->HasAnyFlags(RF_ClassDefaultObject))
					Obj->GetClass()->SerializeDefaultObject(Obj, *this);
				else Obj->Serialize(*this);
			}
		}
		unguardf((TEXT("(%ls - %i/%i)"),Obj->GetFullName(),i, PendingObj.Num()));
	}
	FArchive& operator<<( UObject*& Obj )
	{
		guard(FArchiveSaveTagExports<<Obj);
#if 0 //Fix added by Legend on 4/12/2000
		//
		// warn and abort export for this object if "pending kill"
		// for levels that contain garbage -- avoids save crashes
		//
		if( Obj && Obj->IsPendingKill() )
		{
			debugf( NAME_Warning, TEXT("trying to archive deleted object: %ls"), Obj->GetName() );
		}

		if( Obj && !Obj->IsPendingKill() && Obj->IsIn(Parent) && !(Obj->GetFlags() & (RF_Transient|RF_TagExp)) )
#else
		if( Obj && Obj->IsValid() && Obj->IsIn(Parent) && !(Obj->GetFlags() & (RF_Transient|RF_TagExp)) )//&& !Obj->IsPendingKill() )
#endif
		{
			// Set flags.
			Obj->SetFlags(RF_TagExp);
			if( !(Obj->GetFlags() & RF_NotForEdit  ) ) Obj->SetFlags(RF_LoadForEdit);
			if( !(Obj->GetFlags() & RF_NotForClient) ) Obj->SetFlags(RF_LoadForClient);
			if( !(Obj->GetFlags() & RF_NotForServer) ) Obj->SetFlags(RF_LoadForServer);

			// Recurse with this object's class and package.
			UClass*  Class = Obj->GetClass();
			UObject* Parent = Obj->GetOuter();
			UObject* Arch = Obj->GetArchetype();
			*this << Class << Parent << Arch;

			// Recurse with this object's children.
			PendingObj.AddUniqueItem(Obj);
		}
		return *this;
		unguard;
	}
	UObject* Parent;
};

//
// QSort comparators.
//
#define IS_IMPORT_INDEX(index) (index < 0)

static ULinkerSave* GTempSave;
INT CDECL LinkerNameSort(const FName* A, const FName* B )
{
	return appStricmp(**A, **B);
	//return GTempSave->MapName(B) - GTempSave->MapName(A);
}
INT CDECL LinkerImportSort( const FObjectImport* A, const FObjectImport* B )
{
	INT Result = 0;
	if (A->XObject == NULL)
		Result = 1;
	else if (B->XObject == NULL)
		Result = -1;
	else Result = appStricmp(A->XObject->GetName(), B->XObject->GetName());

	return Result;
	//return GTempSave->MapObject(((FObjectImport*)B)->XObject) - GTempSave->MapObject(((FObjectImport*)A)->XObject);
}
INT CDECL LinkerExportSort( const FObjectExport* A, const FObjectExport* B )
{
	INT Result = 0;
	if (A->_Object == NULL)
		Result = 1;
	else if (B->_Object == NULL)
		Result = -1;
	else Result = appStricmp(A->_Object->GetName(), B->_Object->GetName());
	return Result;
	//return GTempSave->MapObject(((FObjectExport*)B)->_Object) - GTempSave->MapObject(((FObjectExport*)A)->_Object);
}

class FExportReferenceSorter : public FArchive
{
	/**
	 * Verifies that all objects which will be force-loaded when the export at RelativeIndex is created and/or loaded appear in the sorted list of exports
	 * earlier than the export at RelativeIndex.
	 *
	 * Used for tracking down the culprit behind dependency sorting bugs.
	 *
	 * @param	RelativeIndex	the index into the sorted export list to check dependencies for
	 * @param	CheckObject		the object that will be force-loaded by the export at RelativeIndex
	 * @param	ReferenceType	the relationship between the object at RelativeIndex and CheckObject (archetype, class, etc.)
	 * @param	out_ErrorString	if incorrect sorting is detected, receives data containing more information about the incorrectly sorted object.
	 *
	 * @param	TRUE if the export at RelativeIndex appears later than the exports associated with any objects that it will force-load; FALSE otherwise.
	 */
	UBOOL VerifyDependency(const INT RelativeIndex, UObject* CheckObject, const FString& ReferenceType, FString& out_ErrorString)
	{
		guard(FExportReferenceSorter::VerifyDependency);
		UBOOL bResult = FALSE;

		checkf(ReferencedObjects.IsValidIndex(RelativeIndex), TEXT("Invalid index specified: %i (of %i)"), RelativeIndex, ReferencedObjects.Num());

		UObject* SourceObject = ReferencedObjects(RelativeIndex);
		checkf(SourceObject, TEXT("NULL Object at location %i in ReferencedObjects list"), RelativeIndex);
		checkf(CheckObject, TEXT("CheckObject is NULL for %s (%s)"), *SourceObject->GetFullName(), *ReferenceType);

		if (SourceObject->GetOutermost() != CheckObject->GetOutermost())
		{
			// not in the same package; therefore we can assume that the dependent object will exist
			bResult = TRUE;
		}
		else
		{
			INT OtherIndex = ReferencedObjects.FindItemIndex(CheckObject);
			if (OtherIndex != INDEX_NONE)
			{
				if (OtherIndex < RelativeIndex)
				{
					bResult = TRUE;
				}
				else
				{
					out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s appears later in ReferencedObjects list)!  %i) %s   =>  %i) %s"), *ReferenceType, RelativeIndex,
						*SourceObject->GetFullName(), OtherIndex, *CheckObject->GetFullName());

					bResult = FALSE;
				}
			}
			else
			{
				// the object isn't in the list of ReferencedObjects, which means it wasn't processed as a result of processing the source object; this
				// might indicate a bug, but might also just mean that the CheckObject was first referenced by an earlier export
				INT pOtherIndex = ProcessedObjects.FindIndex(CheckObject);
				if (pOtherIndex != INDEX_NONE)
				{
					OtherIndex = pOtherIndex;
					INT pSourceIndex = ProcessedObjects.FindIndex(SourceObject);
					check(pSourceIndex != INDEX_NONE);

					if (OtherIndex < pSourceIndex)
					{
						bResult = TRUE;
					}
					else
					{
						out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s was processed but not added to ReferencedObjects list)!  %i/%i) %s   =>  %i) %s"),
							*ReferenceType, RelativeIndex, pSourceIndex, *SourceObject->GetFullName(), OtherIndex, *CheckObject->GetFullName());
						bResult = FALSE;
					}
				}
				else
				{
					INT pSourceIndex = ProcessedObjects.FindIndex(SourceObject);
					check(pSourceIndex);

					out_ErrorString = FString::Printf(TEXT("Sorting error detected (%s has not yet been processed)!  %i/%i) %s   =>  %s"),
						*ReferenceType, RelativeIndex, pSourceIndex, *SourceObject->GetFullName(), *CheckObject->GetFullName());

					bResult = FALSE;
				}
			}
		}

		return bResult;
		unguard;
	}

	/**
	 * Pre-initializes the list of processed objects with the boot-strap classes.
	 */
	void InitializeCoreClasses()
	{
		// initialize the tracking maps with the core classes
		UClass* CoreClassList[] =
		{
			UObject::StaticClass(),				UField::StaticClass(),				UStruct::StaticClass(),
			UScriptStruct::StaticClass(),		UFunction::StaticClass(),			UState::StaticClass(),
			UEnum::StaticClass(),				UClass::StaticClass(),				UConst::StaticClass(),
			UProperty::StaticClass(),			UByteProperty::StaticClass(),		UIntProperty::StaticClass(),
			UBoolProperty::StaticClass(),		UFloatProperty::StaticClass(),		UObjectProperty::StaticClass(),
			UComponentProperty::StaticClass(),	UClassProperty::StaticClass(),		UInterfaceProperty::StaticClass(),
			UNameProperty::StaticClass(),		UStrProperty::StaticClass(),		UArrayProperty::StaticClass(),
			UMapProperty::StaticClass(),		UStructProperty::StaticClass(),		UDelegateProperty::StaticClass(),
			UComponent::StaticClass(),			UInterface::StaticClass(),			USubsystem::StaticClass(),
		};

		for (INT CoreClassIndex = 0; CoreClassIndex < ARRAY_COUNT(CoreClassList); CoreClassIndex++)
		{
			UClass* CoreClass = CoreClassList[CoreClassIndex];
			CoreClasses.AddUniqueItem(CoreClass);

			ReferencedObjects.AddItem(CoreClass);
			ReferencedObjects.AddItem(CoreClass->GetDefaultObject());
		}

		for (INT CoreClassIndex = 0; CoreClassIndex < CoreClasses.Num(); CoreClassIndex++)
		{
			UClass* CoreClass = CoreClasses(CoreClassIndex);
			ProcessStruct(CoreClass);
		}

		CoreReferencesOffset = ReferencedObjects.Num();
	}

	/**
	 * Adds an object to the list of referenced objects, ensuring that the object is not added more than one.
	 *
	 * @param	Object			the object to add to the list
	 * @param	InsertIndex		the index to insert the object into the export list
	 */
	void AddReferencedObject(UObject* Object, INT InsertIndex)
	{
		guard(FExportReferenceSorter::AddReferencedObject);
		if (Object != NULL && ReferencedObjects.FindItemIndex(Object)==INDEX_NONE)
		{
			ReferencedObjects.Insert(InsertIndex);
			ReferencedObjects(InsertIndex) = Object;
		}
		unguard;
	}

	/**
	 * Handles serializing and calculating the correct insertion point for an object that will be force-loaded by another object (via an explicit call to Preload).
	 * If the RequiredObject is a UStruct or TRUE is specified for bProcessObject, the RequiredObject will be inserted into the list of exports just before the object
	 * that has a dependency on this RequiredObject.
	 *
	 * @param	RequiredObject		the object which must be created and loaded first
	 * @param	bProcessObject		normally, only the class and archetype for non-UStruct objects are inserted into the list;  specify TRUE to override this behavior
	 *								if RequiredObject is going to be force-loaded, rather than just created
	 */
	void HandleDependency(UObject* RequiredObject, UBOOL bProcessObject = FALSE)
	{
		guard(FExportReferenceSorter::HandleDependency);
#ifdef _DEBUG
		// while debugging, makes it much easier to see the object's name in the watch window if all object parameters have the same name
		UObject* Object = RequiredObject;
#endif
		if (RequiredObject != NULL)
		{
			check(CurrentInsertIndex != INDEX_NONE);

			const INT PreviousReferencedObjectCount = ReferencedObjects.Num();
			const INT PreviousInsertIndex = CurrentInsertIndex;

			if (RequiredObject->IsA(UStruct::StaticClass()))
			{
				// if this is a struct/class/function/state, it may have a super that needs to be processed first
				ProcessStruct((UStruct*)RequiredObject);
			}
			else
			{
				if (bProcessObject)
				{
					// this means that RequiredObject is being force-loaded by the referencing object, rather than simply referenced
					ProcessObject(RequiredObject);
				}
				else
				{
					// only the object's class and archetype are force-loaded, so only those objects need to be in the list before
					// whatever object was referencing RequiredObject
					if (ProcessedObjects.Find(RequiredObject->GetOuter()) == NULL)
					{
						HandleDependency(RequiredObject->GetOuter());
					}

					// class is needed before archetype, but we need to process these in reverse order because we are inserting into the list.
					ProcessObject(RequiredObject->GetArchetype());
					ProcessStruct(RequiredObject->GetClass());
				}
			}

			// InsertIndexOffset is the amount the CurrentInsertIndex was incremented during the serialization of SuperField; we need to
			// subtract out this number to get the correct location of the new insert index
			const INT InsertIndexOffset = CurrentInsertIndex - PreviousInsertIndex;
			const INT InsertIndexAdvanceCount = (ReferencedObjects.Num() - PreviousReferencedObjectCount) - InsertIndexOffset;
			if (InsertIndexAdvanceCount > 0)
			{
				// if serializing SuperField added objects to the list of ReferencedObjects, advance the insertion point so that
				// subsequence objects are placed into the list after the SuperField and its dependencies.
				CurrentInsertIndex += InsertIndexAdvanceCount;
			}
		}
		unguard;
	}

public:
	/**
	 * Constructor
	 */
	FExportReferenceSorter()
		: FArchive(), CurrentInsertIndex(INDEX_NONE)
	{
		guard(FExportReferenceSorter::FExportReferenceSorter);
		ArIsObjectReferenceCollector = TRUE;
		ArIsPersistent = TRUE;
		ArIsSaving = TRUE;

		InitializeCoreClasses();
		unguard;
	}

	/**
	 * Verifies that the sorting algorithm is working correctly by checking all objects in the ReferencedObjects array to make sure that their
	 * required objects appear in the list first
	 */
	void VerifySortingAlgorithm()
	{
		guard(FExportReferenceSorter::VerifySortingAlgorithm);
		FString ErrorString;
		for (INT VerifyIndex = CoreReferencesOffset; VerifyIndex < ReferencedObjects.Num(); VerifyIndex++)
		{
			UObject* Object = ReferencedObjects(VerifyIndex);

			// first, make sure that the object's class and archetype appear earlier in the list
			UClass* ObjectClass = Object->GetClass();
			if (!VerifyDependency(VerifyIndex, ObjectClass, TEXT("Class"), ErrorString))
			{
				debugf(*ErrorString);
			}

			UObject* ObjectArchetype = Object->GetArchetype();
			if (ObjectArchetype != NULL && !VerifyDependency(VerifyIndex, ObjectArchetype, TEXT("Archetype"), ErrorString))
			{
				debugf(*ErrorString);
			}

			// if this is a UComponent, it will force-load its TemplateOwnerClass and SourceDefaultObject when it is loaded
			UComponent* Component = Cast<UComponent>(Object);
			if (Component != NULL)
			{
				if (Component->TemplateOwnerClass != NULL)
				{
					if (!VerifyDependency(VerifyIndex, Component->TemplateOwnerClass, TEXT("TemplateOwnerClass"), ErrorString))
					{
						debugf(*ErrorString);
					}
				}
				else if (!Component->HasAnyFlags(RF_ClassDefaultObject) && Component->TemplateName == NAME_None && Component->IsTemplate())
				{
					UObject* SourceDefaultObject = Component->ResolveSourceDefaultObject();
					if (SourceDefaultObject != NULL)
					{
						if (!VerifyDependency(VerifyIndex, SourceDefaultObject, TEXT("SourceDefaultObject"), ErrorString))
						{
							debugf(*ErrorString);
						}
					}
				}
			}
			/*else
			{
				// UObjectRedirectors are always force-loaded as the loading code needs immediate access to the object pointed to by the Redirector
				UObjectRedirector* Redirector = Cast<UObjectRedirector>(Object);
				if (Redirector != NULL && Redirector->DestinationObject != NULL)
				{
					// the Redirector does not force-load the destination object, so we only need its class and archetype.
					UClass* RedirectorDestinationClass = Redirector->DestinationObject->GetClass();
					if (!VerifyDependency(VerifyIndex, RedirectorDestinationClass, TEXT("Redirector DestinationObject Class"), ErrorString))
					{
						debugf(*ErrorString);
					}

					UObject* RedirectorDestinationArchetype = Redirector->DestinationObject->GetArchetype();
					if (RedirectorDestinationArchetype != NULL
						&& !VerifyDependency(VerifyIndex, RedirectorDestinationArchetype, TEXT("Redirector DestinationObject Archetype"), ErrorString))
					{
						debugf(*ErrorString);
					}
				}
			}*/
		}
		unguard;
	}

	/**
	 * Clears the list of encountered objects; should be called if you want to re-use this archive.
	 */
	void Clear()
	{
		guard(FExportReferenceSorter::Clear);
		ReferencedObjects.Remove(CoreReferencesOffset, ReferencedObjects.Num() - CoreReferencesOffset);
		unguard;
	}

	/**
	 * Get the list of new objects which were encountered by this archive; excludes those objects which were passed into the constructor
	 */
	void GetExportList(TArray<UObject*>& out_Exports, UBOOL bIncludeCoreClasses = FALSE)
	{
		guard(FExportReferenceSorter::GetExportList);
		if (!bIncludeCoreClasses)
		{
			const INT NumReferencedObjects = ReferencedObjects.Num() - CoreReferencesOffset;
			if (NumReferencedObjects > 0)
			{
				INT OutputIndex = out_Exports.Num();

				out_Exports.Add(NumReferencedObjects);
				for (INT RefIndex = CoreReferencesOffset; RefIndex < ReferencedObjects.Num(); RefIndex++)
				{
					out_Exports(OutputIndex++) = ReferencedObjects(RefIndex);
				}
			}
		}
		else
		{
			out_Exports += ReferencedObjects;
		}
		unguard;
	}

	/**
	 * UObject serialization operator
	 *
	 * @param	Object	an object encountered during serialization of another object
	 *
	 * @return	reference to instance of this class
	 */
	FArchive& operator<<(UObject*& Object)
	{
		guard(FExportReferenceSorter::<<Object);
		// we manually handle class default objects, so ignore those here
		if (Object != NULL && !Object->HasAnyFlags(RF_ClassDefaultObject))
		{
			if (ProcessedObjects.Find(Object) == NULL)
			{
				// if this object is not a UField, it is an object instance that is referenced through script or defaults (when processing classes) or
				// through an normal object reference (when processing the non-class exports).  Since classes and class default objects
				// are force-loaded (and thus, any objects referenced by the class's script or defaults will be created when the class
				// is force-loaded), we'll need to be sure that the referenced object's class and archetype are inserted into the list
				// of exports before the class, so that when CreateExport is called for this object reference we don't have to seek.
				// Note that in the non-UField case, we don't actually need the object itself to appear before the referencing object/class because it won't
				// be force-loaded (thus we don't need to add the referenced object to the ReferencedObject list)
				if (Object->IsA(UField::StaticClass()))
				{
					// when field processing is enabled, ignore any referenced classes since a class's class and CDO are both intrinsic and
					// attempting to deal with them here will only cause problems
					if (!bIgnoreFieldReferences && !Object->IsA(UClass::StaticClass()))
					{
						if (CurrentClass == NULL || Object->GetOuter() != CurrentClass)
						{
							if (Object->IsA(UStruct::StaticClass()))
							{
								// if this is a struct/class/function/state, it may have a super that needs to be processed first (Preload force-loads UStruct::SuperField)
								ProcessStruct((UStruct*)Object);
							}
							else
							{
								// byte properties that are enum references need their enums loaded first so that config importing works
								UByteProperty* ByteProp = Cast<UByteProperty>(Object);
								if (ByteProp != NULL && ByteProp->Enum != NULL)
								{
									HandleDependency(ByteProp->Enum, TRUE);
								}
								// a normal field - property, enum, const; just insert it into the list and keep going
								ProcessedObjects.Set(Object);

								AddReferencedObject(Object, CurrentInsertIndex);
								if (!SerializedObjects.Find(Object))
								{
									SerializedObjects.Set(Object);
									Object->Serialize(*this);
								}
							}
						}
					}
				}
				else
				{
					// since normal references to objects aren't force-loaded, we do not need to pass TRUE for bProcessObject
					// (which would indicate that Object must be inserted into the sorted export list before the object that contains
					// this object reference - i.e. the object we're currently serializing)
					HandleDependency(Object);
				}
			}
		}

		return *this;
		unguard;
	}

	/**
	 * Adds a normal object to the list of sorted exports.  Ensures that any objects which will be force-loaded when this object is created or loaded are inserted into
	 * the list before this object.
	 *
	 * @param	Object	the object to process.
	 */
	void ProcessObject(UObject* Object)
	{
		guard(FExportReferenceSorter::ProcessObject);
		// we manually handle class default objects, so ignore those here
		if (Object != NULL)
		{
			if (!Object->HasAnyFlags(RF_ClassDefaultObject))
			{
				if (!ProcessedObjects.Find(Object))
				{
					ProcessedObjects.Set(Object);

					const UBOOL bRecursiveCall = CurrentInsertIndex != INDEX_NONE;
					if (!bRecursiveCall)
					{
						CurrentInsertIndex = ReferencedObjects.Num();
					}

					// when an object is created (CreateExport), its class and archetype will be force-loaded, so we'll need to make sure that those objects
					// are placed into the list before this object so that when CreateExport calls Preload on these objects, no seeks occur
					// The object's Outer isn't force-loaded, but it will be created before the current object, so we'll need to ensure that its archetype & class
					// are placed into the list before this object.
					HandleDependency(Object->GetClass(), TRUE);
					HandleDependency(Object->GetOuter());
					HandleDependency(Object->GetArchetype(), TRUE);

					// if this is a UComponent, it will force-load its TemplateOwnerClass and SourceDefaultObject when it is loaded
					UComponent* Component = Cast<UComponent>(Object);
					if (Component != NULL)
					{
						if (Component->TemplateOwnerClass != NULL)
						{
							HandleDependency(Component->TemplateOwnerClass);
						}
						else if (!Component->HasAnyFlags(RF_ClassDefaultObject) && Component->TemplateName == NAME_None && Component->IsTemplate())
						{
							UObject* SourceDefaultObject = Component->ResolveSourceDefaultObject();
							if (SourceDefaultObject != NULL)
							{
								HandleDependency(SourceDefaultObject, TRUE);
							}
						}
					}
					/*else
					{
						// UObjectRedirectors are always force-loaded as the loading code needs immediate access to the object pointed to by the Redirector
						UObjectRedirector* Redirector = Cast<UObjectRedirector>(Object);
						if (Redirector != NULL && Redirector->DestinationObject != NULL)
						{
							// the Redirector does not force-load the destination object, so we only need its class and archetype.
							HandleDependency(Redirector->DestinationObject);
						}
					}*/

					// now we add this object to the list
					AddReferencedObject(Object, CurrentInsertIndex);

					// then serialize the object - any required references encountered during serialization will be inserted into the list before this object, but after this object's
					// class and archetype
					if (SerializedObjects.Find(Object) == NULL)
					{
						SerializedObjects.Set(Object);
						Object->Serialize(*this);
					}

					if (!bRecursiveCall)
					{
						CurrentInsertIndex = INDEX_NONE;
					}
				}
			}
		}
		unguard;
	}

	/**
	 * Adds a UStruct object to the list of sorted exports.  Handles serialization and insertion for any objects that will be force-loaded by this struct (via an explicit call to Preload).
	 *
	 * @param	StructObject	the struct to process
	 */
	void ProcessStruct(UStruct* StructObject)
	{
		guard(FExportReferenceSorter::ProcessStruct);
		if (StructObject != NULL)
		{
			if (!ProcessedObjects.Find(StructObject))
			{
				ProcessedObjects.Set(StructObject);

				const UBOOL bRecursiveCall = CurrentInsertIndex != INDEX_NONE;
				if (!bRecursiveCall)
				{
					CurrentInsertIndex = ReferencedObjects.Num();
				}

				// this must be done after we've established a CurrentInsertIndex
				HandleDependency(StructObject->GetInheritanceSuper());

				// insert the class/function/state/struct into the list
				AddReferencedObject(StructObject, CurrentInsertIndex);
				if (!SerializedObjects.Find(StructObject))
				{
					const UBOOL bPreviousIgnoreFieldReferences = bIgnoreFieldReferences;

					// first thing to do is collect all actual objects referenced by this struct's script or defaults
					// so we turn off field serialization so that we don't have to worry about handling this struct's fields just yet
					bIgnoreFieldReferences = TRUE;

					SerializedObjects.Set(StructObject);
					StructObject->Serialize(*this);

					// at this point, any objects which were referenced through this struct's script or defaults will be in the list of exports, and 
					// the CurrentInsertIndex will have been advanced so that the object processed will be inserted just before this struct in the array
					// (i.e. just after class/archetypes for any objects which were referenced by this struct's script)

					// now re-enable field serialization and process the struct's properties, functions, enums, structs, etc.  They will be inserted into the list
					// just ahead of the struct itself, so that those objects are created first during seek-free loading.
					bIgnoreFieldReferences = FALSE;

					// invoke the serialize operator rather than calling Serialize directly so that the object is handled correctly (i.e. if it is a struct, then we should
					// call ProcessStruct, etc. and all this logic is already contained in the serialization operator)
					if (StructObject->GetClass() != UClass::StaticClass())
					{
						// before processing the Children reference, set the CurrentClass to the class which contains this StructObject so that we
						// don't inadvertently serialize other fields of the owning class too early.
						CurrentClass = StructObject->GetOwnerClass();
					}
					(*this) << (UObject*&)StructObject->Children;
					CurrentClass = NULL;

					(*this) << (UObject*&)StructObject->Next;

					bIgnoreFieldReferences = bPreviousIgnoreFieldReferences;
				}

				// Preload will force-load the class default object when called on a UClass object, so make sure that the CDO is always immediately after its class
				// in the export list; we can't resolve this circular reference, but hopefully we the CDO will fit into the same memory block as the class during 
				// seek-free loading.
				UClass* ClassObject = Cast<UClass>(StructObject);
				if (ClassObject != NULL)
				{
					UObject* CDO = ClassObject->GetDefaultObject();
					if (!ProcessedObjects.Find(CDO))
					{
						ProcessedObjects.Set(CDO);

						if (!SerializedObjects.Find(CDO))
						{
							SerializedObjects.Set(CDO);
							ClassObject->SerializeDefaultObject(CDO, *this);
							//CDO->Serialize(*this);
						}

						INT ClassIndex = ReferencedObjects.FindItemIndex(ClassObject);
						check(ClassIndex != INDEX_NONE);

						// we should be the only one adding CDO's to the list, so this assertion is to catch cases where someone else
						// has added the CDO to the list (as it will probably be in the wrong spot).
						check(ReferencedObjects.FindItemIndex(CDO) == INDEX_NONE || CoreClasses.FindItemIndex(ClassObject) != INDEX_NONE);
						AddReferencedObject(CDO, ClassIndex + 1);
					}
				}

				if (!bRecursiveCall)
				{
					CurrentInsertIndex = INDEX_NONE;
				}
			}
		}
		unguardf((TEXT("(%ls)"), StructObject->GetFullName()));
	}

private:

	/**
	 * The index into the ReferencedObjects array to insert new objects
	 */
	INT CurrentInsertIndex;

	/**
	 * The index into the ReferencedObjects array for the first object not referenced by one of the core classes
	 */
	INT CoreReferencesOffset;

	/**
	 * The classes which are pre-added to the array of ReferencedObjects.  Used for resolving a number of circular dependecy issues between
	 * the boot-strap classes.
	 */
	TArray<UClass*> CoreClasses;

	/**
	 * The list of objects that have been evaluated by this archive so far.
	 */
	TSingleMap<UObject*> ProcessedObjects;

	/**
	 * The list of objects that have been serialized; used to prevent calling Serialize on an object more than once.
	 */
	TSingleMap<UObject*> SerializedObjects;

	/**
	 * The list of new objects that were encountered by this archive
	 */
	TArray<UObject*> ReferencedObjects;

	/**
	 * Controls whether to process UField objects encountered during serialization of an object.
	 */
	UBOOL bIgnoreFieldReferences;

	/**
	 * The UClass currently being processed.  This is used to prevent serialization of a UStruct's Children member causing other fields of the same class to be processed too early due
	 * to being referenced (directly or indirectly) by that field.  For example, if a class has two functions which both have a struct parameter of a struct type which is declared in the same class,
	 * the struct would be inserted into the list immediately before the first function processed.  The second function would be inserted into the list just before the struct.  At runtime,
	 * the "second" function would be created first, which would end up force-loading the struct.  This would cause an unacceptible seek because the struct appears later in the export list, thus
	 * hasn't been created yet.
	 */
	UClass* CurrentClass;
};

#define EXPORT_SORTING_DETAILED_LOGGING 0

/**
 * Helper structure encapsulating functionality to sort a linker's export map to allow seek free
 * loading by creating the exports in the order they are in the export map.
 */
struct FObjectExportSeekFreeSorter
{
	/**
	 * Sorts exports in passed in linker in order to avoid seeking when creating them in order and also
	 * conform the order to an already existing linker if non- NULL.
	 *
	 * @param	Linker				LinkerSave to sort export map
	 * @param	LinkerToConformTo	LinkerLoad to conform LinkerSave to if non- NULL
	 */
	void SortExports(ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo)
	{
		INT					FirstSortIndex = LinkerToConformTo ? LinkerToConformTo->ExportMap.Num() : 0;
		TMap<UObject*, INT>	OriginalExportIndexes;

		// Populate object to current index map.
		for (INT ExportIndex = FirstSortIndex; ExportIndex < Linker->ExportMap.Num(); ExportIndex++)
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if (Export._Object)
			{
				// Set the index (key) in the map to the index of this object into the export map.
				OriginalExportIndexes.Set(Export._Object, ExportIndex);
			}
		}

		UBOOL bRetrieveInitialReferences = TRUE;

		// Now we need to sort the export list according to the order in which objects will be loaded.  For the sake of simplicity, 
		// process all classes first so they appear in the list first (along with any objects those classes will force-load) 
		for (INT ExportIndex = FirstSortIndex; ExportIndex < Linker->ExportMap.Num(); ExportIndex++)
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if (Export._Object && Export._Object->IsA(UClass::StaticClass()))
			{
				SortArchive.Clear();
				SortArchive.ProcessStruct((UClass*)Export._Object);
#if EXPORT_SORTING_DETAILED_LOGGING
				TArray<UObject*> ReferencedObjects;
				SortArchive.GetExportList(ReferencedObjects, bRetrieveInitialReferences);

				debugf(TEXT("Referenced objects for (%i) %s in %s"), ExportIndex, *Export._Object->GetFullName(), *Linker->LinkerRoot->GetName());
				for (INT RefIndex = 0; RefIndex < ReferencedObjects.Num(); RefIndex++)
				{
					debugf(TEXT("\t%i) %s"), RefIndex, *ReferencedObjects(RefIndex)->GetFullName());
				}
				if (ReferencedObjects.Num() > 1)
				{
					// insert a blank line to make the output more readable
					debugf(TEXT(""));
				}

				SortedExports += ReferencedObjects;
#else
				SortArchive.GetExportList(SortedExports, bRetrieveInitialReferences);
#endif
				bRetrieveInitialReferences = FALSE;
			}

		}

#if EXPORT_SORTING_DETAILED_LOGGING
		debugf(TEXT("*************   Processed %i classes out of %i possible exports for package %s.  Beginning second pass...   *************"), SortedExports.Num(), Linker->ExportMap.Num() - FirstSortIndex, *Linker->LinkerRoot->GetName());
#endif

		// All UClasses, CDOs, functions, properties, etc. are now in the list - process the remaining objects now
		for (INT ExportIndex = FirstSortIndex; ExportIndex < Linker->ExportMap.Num(); ExportIndex++)
		{
			const FObjectExport& Export = Linker->ExportMap(ExportIndex);
			if (Export._Object)
			{
				SortArchive.Clear();
				SortArchive.ProcessObject(Export._Object);
#if EXPORT_SORTING_DETAILED_LOGGING
				TArray<UObject*> ReferencedObjects;
				SortArchive.GetExportList(ReferencedObjects, bRetrieveInitialReferences);

				debugf(TEXT("Referenced objects for (%i) %s in %s"), ExportIndex, *Export._Object->GetFullName(), *Linker->LinkerRoot->GetName());
				for (INT RefIndex = 0; RefIndex < ReferencedObjects.Num(); RefIndex++)
				{
					debugf(TEXT("\t%i) %s"), RefIndex, *ReferencedObjects(RefIndex)->GetFullName());
				}
				if (ReferencedObjects.Num() > 1)
				{
					// insert a blank line to make the output more readable
					debugf(TEXT(""));
				}

				SortedExports += ReferencedObjects;
#else
				SortArchive.GetExportList(SortedExports, bRetrieveInitialReferences);
#endif
				bRetrieveInitialReferences = FALSE;
			}
		}
#if EXPORT_SORTING_DETAILED_LOGGING
		SortArchive.VerifySortingAlgorithm();
#endif
		// Back up existing export map and empty it so we can repopulate it in a sorted fashion.
		TArray<FObjectExport> OldExportMap = Linker->ExportMap;
		Linker->ExportMap.Empty(OldExportMap.Num());

		// Add exports that can't be re-jiggled as they are part of the exports of the to be
		// conformed to Linker.
		for (INT ExportIndex = 0; ExportIndex < FirstSortIndex; ExportIndex++)
		{
			Linker->ExportMap.AddItem(OldExportMap(ExportIndex));
		}

		// Create new export map from sorted exports.
		for (INT ObjectIndex = 0; ObjectIndex < SortedExports.Num(); ObjectIndex++)
		{
			// See whether this object was part of the to be sortable exports map...
			UObject* Object = SortedExports(ObjectIndex);
			INT* ExportIndexPtr = OriginalExportIndexes.Find(Object);
			if (ExportIndexPtr)
			{
				// And add it if it has been.
				Linker->ExportMap.AddItem(OldExportMap(*ExportIndexPtr));
			}
		}

		// Manually add any new NULL exports last as they won't be in the SortedExportsObjects list. 
		// A NULL Export._Object can occur if you are e.g. saving an object in the game that is 
		// RF_NotForClient.
		for (INT ExportIndex = FirstSortIndex; ExportIndex < OldExportMap.Num(); ExportIndex++)
		{
			const FObjectExport& Export = OldExportMap(ExportIndex);
			if (Export._Object == NULL)
			{
				Linker->ExportMap.AddItem(Export);
			}
		}
	}

private:
	/**
	 * Archive for sorting an objects references according to the order in which they'd be loaded.
	 */
	FExportReferenceSorter SortArchive;

	/** Array of regular objects encountered by CollectExportsInOrderOfUse					*/
	TArray<UObject*>	SortedExports;
};

/**
 * Helper structure to encapsulate sorting a linker's import table according to the of the import table of the package being conformed against.
 */
struct FObjectImportSortHelper
{
private:
	/**
	 * Allows Compare access to the object full name lookup map
	 */
	static FObjectImportSortHelper* Sorter;

	/**
	 * Map of UObject => full name; optimization for sorting.
	 */
	TMap<UObject*, FString>			ObjectToFullNameMap;

	/** Comparison function used by Sort */
	static INT CDECL QCompare(const FObjectImport& A, const FObjectImport& B)
	{
		checkSlow(Sorter);

		INT Result = 0;
		if (A.XObject == NULL)
			Result = 1;
		else if (B.XObject == NULL)
			Result = -1;
		else
		{
			FString* FullNameA = Sorter->ObjectToFullNameMap.Find(A.XObject);
			FString* FullNameB = Sorter->ObjectToFullNameMap.Find(B.XObject);
			checkSlow(FullNameA);
			checkSlow(FullNameB);

			Result = appStricmp(**FullNameA, **FullNameB);
		}

		return Result;
	}

public:

	/**
	 * Sorts imports according to the order in which they occur in the list of imports.  If a package is specified to be conformed against, ensures that the order
	 * of the imports match the order in which the corresponding imports occur in the old package.
	 *
	 * @param	Linker				linker containing the imports that need to be sorted
	 * @param	LinkerToConformTo	optional linker to conform against.
	 */
	void SortImports(ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo = NULL)
	{
		INT SortStartPosition = 0;
		TArray<FObjectImport>& Imports = Linker->ImportMap;
		if (LinkerToConformTo)
		{
			// intended to be a copy
			TArray<FObjectImport> Orig = Imports;
			Imports.Empty(Imports.Num());

			// this array tracks which imports from the new package exist in the old package
			TArray<BYTE> Used;
			Used.AddZeroed(Orig.Num());

			TMap<FString, INT> OriginalImportIndexes;
			for (INT i = 0; i < Orig.Num(); i++)
			{
				FObjectImport& Import = Orig(i);
				FString ImportFullName = Import.XObject->GetFullName();

				OriginalImportIndexes.Set(*ImportFullName, i);
				ObjectToFullNameMap.Set(Import.XObject, *ImportFullName);
			}

			for (INT i = 0; i < LinkerToConformTo->ImportMap.Num(); i++)
			{
				// determine whether the new version of the package contains this export from the old package
				INT* OriginalImportPosition = OriginalImportIndexes.Find(*LinkerToConformTo->GetImportFullName(i));
				if (OriginalImportPosition)
				{
					// this import exists in the new package as well,
					// create a copy of the FObjectImport located at the original index and place it
					// into the matching position in the new package's import map
					FObjectImport* NewImport = new(Imports) FObjectImport(Orig(*OriginalImportPosition));
					check(NewImport->XObject == Orig(*OriginalImportPosition).XObject);
					Used(*OriginalImportPosition) = 1;
				}
				else
				{
					// this import no longer exists in the new package
					new(Imports)FObjectImport(NULL);
				}
			}

			SortStartPosition = LinkerToConformTo->ImportMap.Num();
			for (INT i = 0; i < Used.Num(); i++)
			{
				if (!Used(i))
				{
					// the FObjectImport located at pos "i" in the original import table did not
					// exist in the old package - add it to the end of the import table
					new(Imports) FObjectImport(Orig(i));
				}
			}
		}
		else
		{
			for (INT ImportIndex = 0; ImportIndex < Linker->ImportMap.Num(); ImportIndex++)
			{
				const FObjectImport& Import = Linker->ImportMap(ImportIndex);
				if (Import.XObject)
				{
					ObjectToFullNameMap.Set(Import.XObject, Import.XObject->GetFullName());
				}
			}
		}

		if (SortStartPosition < Linker->ImportMap.Num())
		{
			Sorter = this;
			appQsort(&Linker->ImportMap(SortStartPosition), Linker->ImportMap.Num() - SortStartPosition, sizeof(FObjectImport), (QSORT_COMPARE)QCompare);
			Sorter = NULL;
		}
	}
};
FObjectImportSortHelper* FObjectImportSortHelper::Sorter = NULL;

struct FObjectExportSortHelper
{
private:
	/**
	 * Allows Compare access to the object full name lookup map
	 */
	static FObjectExportSortHelper* Sorter;

	/**
	 * Map of UObject => full name; optimization for sorting.
	 */
	TMap<UObject*, FString>			ObjectToFullNameMap;

	/** Comparison function used by Sort */
	static INT CDECL QCompare(const FObjectExport& A, const FObjectExport& B)
	{
		checkSlow(Sorter);

		INT Result = 0;
		if (A._Object == NULL)
		{
			Result = 1;
		}
		else if (B._Object == NULL)
		{
			Result = -1;
		}
		else
		{
			FString* FullNameA = Sorter->ObjectToFullNameMap.Find(A._Object);
			FString* FullNameB = Sorter->ObjectToFullNameMap.Find(B._Object);
			checkSlow(FullNameA);
			checkSlow(FullNameB);

			Result = appStricmp(**FullNameA, **FullNameB);
		}

		return Result;
	}

public:
	/**
	 * Sorts exports alphabetically.  If a package is specified to be conformed against, ensures that the order
	 * of the exports match the order in which the corresponding exports occur in the old package.
	 *
	 * @param	Linker				linker containing the exports that need to be sorted
	 * @param	LinkerToConformTo	optional linker to conform against.
	 */
	void SortExports(ULinkerSave* Linker, ULinkerLoad* LinkerToConformTo = NULL)
	{
		INT SortStartPosition = 0;
		if (LinkerToConformTo)
		{
			// build a map of object full names to the index into the new linker's export map prior to sorting.
			// we need to do a little trickery here to generate an object path name that will match what we'll get back
			// when we call GetExportFullName on the LinkerToConformTo's exports, due to localized packages and forced exports.
			const FString LinkerName = Linker->LinkerRoot->GetName();
			const FString PathNamePrefix = LinkerName + TEXT(".");

			// Populate object to current index map.
			TMap<FString, INT> OriginalExportIndexes;
			for (INT ExportIndex = 0; ExportIndex < Linker->ExportMap.Num(); ExportIndex++)
			{
				const FObjectExport& Export = Linker->ExportMap(ExportIndex);
				if (Export._Object)
				{
					// get the path name for this object; if the object is contained within the package we're saving,
					// we don't want the returned path name to contain the package name since we'll be adding that on
					// to ensure that forced exports have the same outermost name as the non-forced exports
					//@script patcher: when generating a script patch, we need to sort exports according to their "non-forced-export" name
					FString ObjectPathName = Export._Object != Linker->LinkerRoot
						? Export._Object->GetPathName(Linker->LinkerRoot)
						: LinkerName;

					FString ExportFullName = FString(Export._Object->GetClass()->GetName()) + TEXT(" ") + PathNamePrefix + ObjectPathName;

					// Set the index (key) in the map to the index of this object into the export map.
					OriginalExportIndexes.Set(*ExportFullName, ExportIndex);
					ObjectToFullNameMap.Set(Export._Object, *ExportFullName);
				}
			}

			// backup the existing export list so we can empty the linker's actual list
			TArray<FObjectExport> OldExportMap = Linker->ExportMap;
			Linker->ExportMap.Empty(Linker->ExportMap.Num());

			// this array tracks which exports from the new package exist in the old package
			TArray<BYTE> Used;
			Used.AddZeroed(OldExportMap.Num());

			//@script patcher
			TArray<INT> MissingFunctionIndexes;
			for (INT i = 0; i < LinkerToConformTo->ExportMap.Num(); i++)
			{
				// determine whether the new version of the package contains this export from the old package
				FString ExportFullName = LinkerToConformTo->GetExportFullName(i, *LinkerName);
				INT* OriginalExportPosition = OriginalExportIndexes.Find(*ExportFullName);
				if (OriginalExportPosition)
				{
					// this export exists in the new package as well,
					// create a copy of the FObjectExport located at the original index and place it
					// into the matching position in the new package's export map
					FObjectExport* NewExport = new(Linker->ExportMap) FObjectExport(OldExportMap(*OriginalExportPosition));
					check(NewExport->_Object == OldExportMap(*OriginalExportPosition)._Object);
					Used(*OriginalExportPosition) = 1;
				}
				else
				{
					//@{
					//@script patcher fixme: it *might* be OK to have missing functions in conformed packages IF we are only planning to use them on PC
					// then again...it might not - need to figure out what happens when a function in a child class from another package [which
					// hasn't been recompiled] attempts to call Super.XX
					FObjectExport& MissingExport = LinkerToConformTo->ExportMap(i);
					if (MissingExport.ClassIndex != 0)
					{
						FName& ClassName = IS_IMPORT_INDEX(MissingExport.ClassIndex)
							? LinkerToConformTo->ImportMap(-MissingExport.ClassIndex - 1).ObjectName
							: LinkerToConformTo->ExportMap(MissingExport.ClassIndex - 1).ObjectName;

						if (ClassName == NAME_Function)
						{
							MissingFunctionIndexes.AddItem(i);
						}
					}
					//@}

					// this export no longer exists in the new package; to ensure that the _LinkerIndex matches, add an empty entry to pad the list
					new(Linker->ExportMap)FObjectExport(NULL);
					debugf(TEXT("No matching export found in new package for original export %i: %s"), i, *ExportFullName);
				}
			}

			SortStartPosition = LinkerToConformTo->ExportMap.Num();
			for (INT i = 0; i < Used.Num(); i++)
			{
				if (!Used(i))
				{
					// the FObjectExport located at pos "i" in the original export table did not
					// exist in the old package - add it to the end of the export table
					new(Linker->ExportMap) FObjectExport(OldExportMap(i));
				}
			}
		}
		else
		{
			for (INT ExportIndex = 0; ExportIndex < Linker->ExportMap.Num(); ExportIndex++)
			{
				const FObjectExport& Export = Linker->ExportMap(ExportIndex);
				if (Export._Object)
				{
					ObjectToFullNameMap.Set(Export._Object, Export._Object->GetFullName());
				}
			}
		}

		if (SortStartPosition < Linker->ExportMap.Num())
		{
			Sorter = this;
			appQsort(&Linker->ExportMap(SortStartPosition), Linker->ExportMap.Num() - SortStartPosition, sizeof(FObjectExport), (QSORT_COMPARE)QCompare);
			Sorter = NULL;
		}
	}
};
FObjectExportSortHelper* FObjectExportSortHelper::Sorter = NULL;

//
// Archive for tagging objects and names that must be listed in the
// file's imports table.
//
class FArchiveSaveTagImports : public FArchive
{
public:
	DWORD ContextFlags;
	TArray<UObject*> Dependencies;

	FArchiveSaveTagImports( ULinkerSave* InLinker, DWORD InContextFlags )
	: ContextFlags( InContextFlags ), Linker( InLinker )
	{
		ArIsSaving = 1;
		ArIsPersistent = 1;
		ArIsObjectReferenceCollector = 1;
	}
	FArchive& operator<<( UObject*& Obj )
	{
		guard(FArchiveSaveTagImports<<Obj);
		if( Obj && Obj->IsValid() && !Obj->IsPendingKill() )
		{
			if( !(Obj->GetFlags() & RF_Transient) || (Obj->GetFlags() & RF_Public) )
			{
				Linker->ObjectIndices(Obj->GetIndex())++;
				if( !(Obj->GetFlags() & RF_TagExp ) )
				{
					// remember it as a dependency, unless it's a top level pacakge or native
					UBOOL bIsTopLevelPackage = Obj->GetOuter() == NULL && Obj->IsA(UPackage::StaticClass());
					if (!bIsTopLevelPackage)
					{
						UBOOL bIsNative = Obj->HasAnyFlags(RF_Native);
						if (!bIsNative)
						{
							UClass* COuter = FindOuter<UClass>(Obj->GetOuter());
							if (COuter && COuter->HasAnyFlags(RF_Native))
								bIsNative = TRUE;
						}

						// only add valid objects
						if (!bIsNative)
							Dependencies.AddUniqueItem(Obj);
					}

					Obj->SetFlags( RF_TagImp );
					if( !(Obj->GetFlags() & RF_NotForEdit  ) ) Obj->SetFlags(RF_LoadForEdit);
					if( !(Obj->GetFlags() & RF_NotForClient) ) Obj->SetFlags(RF_LoadForClient);
					if( !(Obj->GetFlags() & RF_NotForServer) ) Obj->SetFlags(RF_LoadForServer);
					UObject* Parent = NULL;
					Parent = Obj->GetOuter();
					if( Parent )
						*this << Parent;
				}
			}
		}
		return *this;
		unguard;
	}
	FArchive& operator<<( FName& Name )
	{
		guard(FArchiveSaveTagImports<<Name);
		INT Number;
		FName RN = FName::GrabDeNumberedName(Name, Number);
		RN.SetFlags( RF_TagExp | ContextFlags );
		Linker->NameIndices(RN.GetIndex())++;
		return *this;
		unguard;
	}
	ULinkerSave* Linker;
};

//
// Save one specific object into an Unrealfile.
//
UBOOL UObject::SavePackage( UObject* InOuter, UObject* Base, DWORD TopLevelFlags, const TCHAR* Filename, FOutputDevice* Error, ULinkerLoad* Conform, UBOOL bAutoSave, FSavedGameSummary* SaveSummary)
{
	guard(UObject::SavePackage);
	check(InOuter);
	check(Filename);

	// Make temp file.
	TCHAR TempFilename[256];
	//appStrcpy( TempFilename, Filename );
	appStrncpy( TempFilename, Filename, ARRAY_COUNT(TempFilename) ); //elmuerte: security fix
	INT c = appStrlen(TempFilename);
	while( c>0 && TempFilename[c-1]!=PATH_SEPARATOR[0] && TempFilename[c-1]!='/' && TempFilename[c-1]!=':' )
		c--;
	TempFilename[c]=0;
	appStrcat( TempFilename, TEXT("Save.tmp") );

	// Init.
	UBOOL Success = 0;
	
	// If we have a loader for the package, unload it to prevent conflicts.
	ResetLoaders( InOuter, 0, 1 );

	// Untag all objects and names.
	guard(Untag);
	for( FObjectIterator It; It; ++It )
		It->ClearFlags( RF_TagImp | RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
	for( INT i=0; i<FName::GetMaxNames(); i++ )
		if( FName::GetEntry(i) )
			FName::GetEntry(i)->Flags &= ~(RF_TagImp | RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
	unguard;

	// Export objects.
	guard(TagExports);
	FArchiveSaveTagExports Ar( InOuter );
	if( Base )
		Ar << Base;
	for( FObjectIterator It; It; ++It )
	{
		if (/*((It->GetFlags() & TopLevelFlags) || TopLevelFlags & RF_ExportAll) &&*/ It->IsIn(InOuter))
		{
			UObject* Obj = *It;
			Ar << Obj;
		}
	}
	Ar.SerializeRemaining();
	unguard;

	ULinkerSave* Linker = NULL;
	try
	{
		// Allocate the linker.
		Linker = new ULinkerSave( InOuter, TempFilename );

		// structure to track what ever export needs to import
		TMap<UObject*, TArray<UObject*> > ObjectDependencies;

		// Import objects and names.
		guard(TagImports);
		for( FObjectIterator It; It; ++It )
		{
			if( It->GetFlags() & RF_TagExp )
			{
				// Build list.
				FArchiveSaveTagImports Ar( Linker, It->GetFlags() & RF_LoadContextFlags );
				if (It->HasAnyFlags(RF_ClassDefaultObject))
					It->GetClass()->SerializeDefaultObject(*It, Ar);
				else It->Serialize(Ar);
				UClass* Class = It->GetClass();
				Ar << Class;
				UObject* Arch = It->GetArchetype();
				if (Arch)
					Ar << Arch;
				if( It->IsIn(GetTransientPackage()) )
					appErrorf( TEXT("Trying to import Transient object %ls"), It->GetFullName() );

				// add the list of dependencies to the dependency map
				ObjectDependencies.Set(*It, Ar.Dependencies);
			}
		}
		unguard;

		// Export all relevant object, class, and package names.
		guard(ExportNames);
		for( FObjectIterator It; It; ++It )
		{
			if( It->GetFlags() & (RF_TagExp|RF_TagImp) )
			{
				INT Number;
				FName RN = FName::GrabDeNumberedName(It->GetFName(), Number);
				RN.SetFlags( RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer );
				if (It->GetOuter())
				{
					RN = FName::GrabDeNumberedName(It->GetOuter()->GetFName(), Number);
					RN.SetFlags(RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
				}
				if (It->GetArchetype())
				{
					RN = FName::GrabDeNumberedName(It->GetArchetype()->GetFName(), Number);
					RN.SetFlags(RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
				}
				if( It->GetFlags() & RF_TagImp )
				{
					RN = FName::GrabDeNumberedName(It->Class->GetFName(), Number);
					RN.SetFlags(RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
					check(It->Class->GetOuter());
					RN = FName::GrabDeNumberedName(It->Class->GetOuter()->GetFName(), Number);
					RN.SetFlags(RF_TagExp | RF_LoadForEdit | RF_LoadForClient | RF_LoadForServer);
					if (!(It->GetFlags() & RF_Public))
						appThrowf( TEXT("Can't save %ls: Graph is linked to external private object %ls"), Filename, It->GetFullName() );
				}
				else debugfSlow( NAME_DevSave, TEXT("Saving %ls"), It->GetFullName() );
			}
		}
		unguard;

		// Write fixed-length file summary to overwrite later.
		guard(SaveSummary);
		if( Conform )
		{
			// Conform to previous generation of file.
			debugf( NAME_DevSave, TEXT("Conformal save, relative to: %ls, Generation %i"), *Conform->Filename, Conform->Summary.Generations.Num()+1 );
			Linker->Summary.Guid        = Conform->Summary.Guid;
			Linker->Summary.Generations = Conform->Summary.Generations;
		}
		else
		{
			// First generation file.
			Linker->Summary.Guid        = appCreateGuid();
			Linker->Summary.Generations = TArray<FGenerationInfo>();
		}

		new(Linker->Summary.Generations)FGenerationInfo( 0, 0 );
		*Linker << Linker->Summary;
		unguard;

		// Build NameMap.
		guard(BuildNameMap);
		Linker->Summary.NameOffset = Linker->Tell();
		for( INT i=0; i<FName::GetMaxNames(); i++ )
		{
			if( FName::GetEntry(i) )
			{
				FName Name( (EName)i );
				if( Name.GetFlags() & RF_TagExp )
					Linker->NameMap.AddItem( Name );
			}
		}
		unguard;

		// Sort names by usage count in order to maximize compression.
		guard(SortNames);
		GTempSave = Linker;
		INT FirstSort = 0;
		if( Conform )
		{
			INT i;
			for( i=0; i<Conform->NameMap.Num(); i++ )
			{
				INT Index = Linker->NameMap.FindItemIndex(Conform->NameMap(i));
				if( Conform->NameMap(i)==NAME_None || Index==INDEX_NONE )
				{
					Linker->NameMap.Add();
					Linker->NameMap.Last() = Linker->NameMap(i);
				}
				else Exchange( Linker->NameMap(i), Linker->NameMap(Index) );
				Linker->NameMap(i) = Conform->NameMap(i);
			}
			FirstSort = Conform->NameMap.Num();
		}
		appQsort( &Linker->NameMap(FirstSort), Linker->NameMap.Num()-FirstSort, sizeof(FName), (QSORT_COMPARE)LinkerNameSort );
		unguard;

		// Save names.
		guard(SaveNames);
		Linker->Summary.NameCount = Linker->NameMap.Num();
		for( INT i=0; i<Linker->NameMap.Num(); i++ )
		{
			*Linker << *FName::GetEntry( Linker->NameMap(i).GetIndex() );
			Linker->NameIndices(Linker->NameMap(i).GetIndex()) = i;
		}
		unguard;

		// Build ImportMap.
		guard(BuildImportMap);
		for( FObjectIterator It; It; ++It )
			if (It->GetFlags() & RF_TagImp)
				new(Linker->ImportMap)FObjectImport(*It);
		Linker->Summary.ImportCount = Linker->ImportMap.Num();
		unguard;

		guard(SortImports);
#if 0
		// sort and conform imports
		FObjectImportSortHelper ImportSortHelper;
		ImportSortHelper.SortImports(Linker, Conform);
		Linker->Summary.ImportCount = Linker->ImportMap.Num();
#else
		// Sort imports by usage count.
		GTempSave = Linker;
		appQsort( &Linker->ImportMap(0), Linker->ImportMap.Num(), sizeof(FObjectImport), (QSORT_COMPARE)LinkerImportSort );
#endif
		unguard;

		// Build ExportMap.
		guard(BuildExports);
		for( FObjectIterator It; It; ++It )
			if( It->GetFlags() & RF_TagExp )
				new( Linker->ExportMap )FObjectExport( *It );
		unguard;

		guard(SortExports);
#if 0
		// Sort exports alphabetically and conform the export table (if necessary)
		FObjectExportSortHelper ExportSortHelper;
		ExportSortHelper.SortExports(Linker, Conform);
		Linker->Summary.ExportCount = Linker->ExportMap.Num();
#else
		// Sort exports by usage count.
		INT FirstSort = 0;
		if( Conform )
		{
			TArray<FObjectExport> Orig = Linker->ExportMap;
			Linker->ExportMap.Empty( Linker->ExportMap.Num() );
			TArray<BYTE> Used; Used.AddZeroed( Orig.Num() );
			TMap<FString,INT> Map;
			{for( INT i=0; i<Orig.Num(); i++ )
				Map.Set( Orig(i)._Object->GetFullName(), i );}
			{for( INT i=0; i<Conform->ExportMap.Num(); i++ )
			{
				INT* Find = Map.Find( Conform->GetExportFullName(i,Linker->LinkerRoot->GetPathName()) );
				if( Find )
				{
					new(Linker->ExportMap)FObjectExport( Orig(*Find) );
					check(Linker->ExportMap.Last()._Object == Orig(*Find)._Object);
					Used( *Find ) = 1;
				}
				else new(Linker->ExportMap)FObjectExport( NULL );
			}}
			FirstSort = Conform->ExportMap.Num();
			{for( INT i=0; i<Used.Num(); i++ )
				if( !Used(i) )
					new(Linker->ExportMap)FObjectExport( Orig(i) );}
		}
		appQsort( &Linker->ExportMap(FirstSort), Linker->ExportMap.Num()-FirstSort, sizeof(Linker->ExportMap(0)), (QSORT_COMPARE)LinkerExportSort );
		Linker->Summary.ExportCount = Linker->ExportMap.Num();
#endif
		unguard;

		guard(SeekFreeSort);
		// Sort exports for seek-free loading.
		FObjectExportSeekFreeSorter SeekFreeSorter;
		SeekFreeSorter.SortExports(Linker, Conform);
		unguard;

		// Set linker reverse mappings.
		guard(SetLinkerMappings);
		{for( INT i=0; i<Linker->ExportMap.Num(); i++ )
			if( Linker->ExportMap(i)._Object )
				Linker->ObjectIndices(Linker->ExportMap(i)._Object->GetIndex()) = i+1;}
		{for (INT i = 0; i < Linker->ImportMap.Num(); i++)
			Linker->ObjectIndices(Linker->ImportMap(i).XObject->GetIndex()) = -i-1;}
		unguard;

		// Save exports.
		guard(SaveExports);
		Linker->DependsMap.AddZeroed(Linker->ExportMap.Num());
		for( INT i=0; i<Linker->ExportMap.Num(); i++ )
		{
			FObjectExport& Export = Linker->ExportMap(i);
			if( Export._Object )
			{
				TArray<INT>& DependIndices = Linker->DependsMap(i);
				// find all the objects needed by this export
				TArray<UObject*>* SrcDepends = ObjectDependencies.Find(Export._Object);
				if (SrcDepends)
				{
					// go through each object and...
					for (INT DependIndex = 0; DependIndex < SrcDepends->Num(); DependIndex++)
					{
						UObject* DependentObject = (*SrcDepends)(DependIndex);

						INT DependencyIndex = Linker->ObjectIndices(DependentObject->GetIndex());

						// if we didn't find it (FindRef returns 0 on failure, which is good in this case), then we are in trouble, something went wrong somewhere
						checkf(DependencyIndex != 0, TEXT("Failed to find dependency index for %s (%s)"), DependentObject->GetFullName(), Export._Object->GetFullName());

						// add the import as an import for this export
						DependIndices.AddItem(DependencyIndex);
					}
				}

				// Set class index.
				if (!Export._Object->IsA(UClass::StaticClass()))
				{
					Export.ClassIndex = Linker->ObjectIndices(Export._Object->GetClass()->GetIndex());
					check(Export.ClassIndex != 0);
				}
				else Export.ClassIndex = 0;

				if (Export._Object->IsA(UStruct::StaticClass()))
				{
					UStruct* Struct = (UStruct*)Export._Object;
					if (Struct->SuperStruct)
					{
						Export.SuperIndex = Linker->ObjectIndices(Struct->SuperStruct->GetIndex());
						check(Export.SuperIndex != 0);
					}
					else Export.SuperIndex = 0;
				}
				else Export.SuperIndex = 0;

				// Set package index.
				if (Export._Object->GetOuter() != InOuter)
				{
					check(Export._Object->GetOuter()->IsIn(InOuter));
					Export.PackageIndex = Linker->ObjectIndices(Export._Object->GetOuter()->GetIndex());
					check(Export.PackageIndex > 0);
				}
				else Export.PackageIndex = 0;

				UObject* Template = Export._Object->GetArchetype();
				if (Template != NULL && Template != Export._Object->GetClass()->GetDefaultObject())
					Export.ArchetypeIndex = Linker->ObjectIndices(Template->GetIndex());
				else Export.ArchetypeIndex = 0;

				// Save it.
				Export.SerialOffset = Linker->Tell();
				Export._Object->NetIndex = i;
				if (Export._Object->HasAnyFlags(RF_ClassDefaultObject))
					Export._Object->GetClass()->SerializeDefaultObject(Export._Object, *Linker);
				else Export._Object->Serialize(*Linker);
				Export.SerialSize = Linker->Tell() - Export.SerialOffset;
			}
		}
		unguard;

		// Save the import map.
		guard(SaveImportMap);
		Linker->Summary.ImportOffset = Linker->Tell();
		for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		{
			FObjectImport& Import = Linker->ImportMap( i );

			// Set the package index.
			if( Import.XObject->GetOuter() )
			{
				check(!Import.XObject->GetOuter()->IsIn(InOuter));
				Import.PackageIndex = Linker->ObjectIndices(Import.XObject->GetOuter()->GetIndex());
				check(Import.PackageIndex<0);
				if (Import.PackageIndex >= 0)
					debugf(TEXT("ErrSave %ls %i"), Import.XObject->GetFullName(), Import.PackageIndex);
			}

			// Save it.
			*Linker << Import;
		}
		unguard;

		// Save the export map.
		guard(SaveExportMap);
		Linker->Summary.ExportOffset = Linker->Tell();
		for( int i=0; i<Linker->ExportMap.Num(); i++ )
			*Linker << Linker->ExportMap( i );
		unguard;

		// save depends map (no need for later patching)
		check(Linker->DependsMap.Num() == Linker->ExportMap.Num());
		Linker->Summary.DependsOffset = Linker->Tell();
		for (INT i = 0; i < Linker->ExportMap.Num(); i++)
		{
			TArray<INT>& Depends = Linker->DependsMap(i);
			*Linker << Depends;
		}

		// Rewrite updated file summary.
		guard(RewriteSummary);
		GWarn->StatusUpdatef( 0, 0, TEXT("%ls"), TEXT("Closing") );
		Linker->Summary.Generations.Last().ExportCount = Linker->Summary.ExportCount;
		Linker->Summary.Generations.Last().NameCount   = Linker->Summary.NameCount;
		Linker->Summary.Generations.Last().NetObjectCount = Linker->Summary.ExportCount;
		Linker->Summary.PackageSource = appStrCrcCaps(InOuter->GetName());
		Linker->Seek(0);
		*Linker << Linker->Summary;
		unguard;

		Success = 1;
	}
	catch( const TCHAR* Msg )
	{
		// Delete the temporary file.
		GFileManager->Delete( TempFilename );
		Error->Logf( NAME_Warning, TEXT("%ls"), Msg );
	}
	if( Linker )
	{
		delete Linker;
	}

	if (Success)
	{
		// Move the temporary file.
		debugf(NAME_Log, TEXT("Moving '%ls' to '%ls'"), TempFilename, Filename);
		if (!GFileManager->Move(Filename, TempFilename))
		{
			GFileManager->Delete(TempFilename);
			GWarn->Logf(TEXT("Error saving '%ls'"), Filename);
			Success = 0;
		}
	}
	return Success;
	unguard;
}

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

//
// Return whether statics are initialized.
//
UBOOL UObject::GetInitialized()
{
	return UObject::GObjInitialized;
}

//
// Return the static transient package.
//
UPackage* UObject::GetTransientPackage()
{
	return UObject::GObjTransientPkg;
}

//
// Return the ith loader.
//
ULinkerLoad* UObject::GetLoader( INT i )
{
	return (ULinkerLoad*)GObjLoaders(i);
}

//
// Add an object to the root array. This prevents the object and all
// its descendents from being deleted during garbage collection.
//
void UObject::AddToRoot()
{
	guard(UObject::AddToRoot);
	GObjRoot.AddItem( this );
	unguard;
}

//
// Remove an object from the root array.
//
void UObject::RemoveFromRoot()
{
	guard(UObject::RemoveFromRoot);
	GObjRoot.RemoveItem( this );
	unguard;
}

BYTE* UObject::GetScriptData()
{
	guard(UObject::GetScriptData);
	if (Class->ClassFlags & CLASS_NoScriptProps)
		return NULL;
	return GetInternalScriptData(Class->PropertiesSize);
	unguardobj;
}
BYTE* UObject::GetInternalScriptData(INT DesiredSize)
{
	guardSlow(UObject::GetInternalScriptData);
	if (UnrealScriptData.Num() < Class->PropertiesSize)
	{
		UnrealScriptData.Empty(Class->PropertiesSize);
		UnrealScriptData.AddZeroed(Class->PropertiesSize);
		*(UObject**)UnrealScriptData.GetData() = this;
	}
	return &UnrealScriptData(0);
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Object name functions.
-----------------------------------------------------------------------------*/

//
// Create a unique name by combining a base name and an arbitrary number string.
// The object name returned is guaranteed not to exist.
//
FName UObject::MakeUniqueObjectName( UObject* Parent, UClass* Class )
{
	guard(UObject::MakeUniqueObjectName);
	check(Class);

	TCHAR NewBase[NAME_SIZE], Result[NAME_SIZE];
	TCHAR TempIntStr[NAME_SIZE];

	// Make base name sans appended numbers.
	appStrcpy( NewBase, Class->GetName() );

	TCHAR* End = NewBase + appStrlen(NewBase);
	while( End>NewBase && appIsDigit(End[-1]) )
		End--;
	*End = 0;

	// Append numbers to base name.
	do
	{
		appSprintf( TempIntStr, TEXT("_%i"), Class->ClassUnique++ );
		appStrncpy( Result, NewBase, NAME_SIZE-appStrlen(TempIntStr)-1 );
		appStrcat( Result, TempIntStr );
	} while( StaticFindObject( NULL, Parent, Result ) );

	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	Object hashing.
-----------------------------------------------------------------------------*/

//
// Add an object to the hash table.
//
void UObject::HashObject()
{
	guard(UObject::HashObject);

	if (this && this->IsValid())
	{
		INT iHash = GetObjectHash(Name, Outer ? Outer->GetIndex() : 0);
		HashNext = GObjHash[iHash];
		GObjHash[iHash] = this;
	}

	unguard;
}

//
// Remove an object from the hash table.
//
void UObject::UnhashObject( INT OuterIndex )
{
	guard(UObject::UnhashObject);
	INT       iHash   = GetObjectHash( Name, OuterIndex );
	UObject** Hash    = &GObjHash[iHash];
	INT       Removed = 0;

	while( *Hash != NULL )
	{
		if( *Hash != this )
		{
			Hash = &(*Hash)->HashNext;
 		}
		else
		{
			*Hash = (*Hash)->HashNext;
			Removed++;
		}
	}

	unguard;
}

/*-----------------------------------------------------------------------------
	Creating and allocating data for new objects.
-----------------------------------------------------------------------------*/

//
// Add an object to the table.
//
void UObject::AddObject( INT InIndex )
{
	guard(UObject::AddObject);

	// Find an available index.
	if( InIndex==INDEX_NONE )
	{
		if( GObjAvailable.Num() )
		{
			InIndex = GObjAvailable.Pop();
			check(GObjObjects(InIndex)==NULL);
		}
		else InIndex = GObjObjects.Add();
	}

	// Add to global table.
	GObjObjects(InIndex) = this;
	Index = InIndex;
	HashObject();

	unguard;
}

//
// Create a new object or replace an existing one.
// If Name is NAME_None, tries to create an object with an arbitrary unique name.
// No failure conditions.
//
UObject* UObject::StaticAllocateObject
(
	UClass*			InClass,
	UObject*		InOuter,
	FName			InName,
	EObjectFlags	InFlags,
	UObject*		InTemplate,
	FOutputDevice*	Error,
	UObject*		Ptr,
	UObject*		SubobjectRoot
)
{
	guard(UObject::StaticAllocateObject);
	check(Error);
	check(!InClass || InClass->ClassWithin);
	check(!InClass || InClass->ClassConstructor);

	// Validation checks.
	if( !InClass )
	{
		Error->Logf( TEXT("Empty class for object %ls"), *InName );
		return NULL;
	}
	if( InClass->GetIndex()==INDEX_NONE && GObjRegisterCount==0 )
	{
		Error->Logf( TEXT("Unregistered class for %ls"), *InName );
		return NULL;
	}
	if ((InClass->ClassFlags & CLASS_Abstract) && !(InFlags & RF_ClassDefaultObject))
		GWarn->Logf( TEXT("Can't create object %ls: class %ls is abstract"), *InName, InClass->GetName() );
	if( !InOuter && InClass!=UPackage::StaticClass() )
	{
		Error->Logf( TEXT("Object is not packaged: %ls %ls"), InClass->GetName(), *InName );
		return NULL;
	}
	/*if( InOuter && !InOuter->IsA(InClass->ClassWithin) )
	{
		Error->Logf( TEXT("Object %ls %ls created in %ls instead of %ls"), InClass->GetName(), *InName, InOuter->GetClass()->GetName(), InClass->ClassWithin->GetName() );
		return NULL;
	}*/

	// Compose name, if public and unnamed.
	UObject* Obj = NULL;
	if (InName == NAME_None)
	{
		if (InFlags & RF_ClassDefaultObject)
		{
			InName = *FString::Printf(TEXT("Default__%ls"), InClass->GetName());
			Obj = StaticFindObject(InClass, InOuter, *InName, 1);
		}
		else InName = MakeUniqueObjectName(InOuter, InClass);
	}
	else Obj = StaticFindObject(InClass, InOuter, *InName, 1);

	// See if object already exists.
	UClass*  Cls                        = Cast<UClass>( Obj );
	INT      Index                      = INDEX_NONE;
	UClass*  ClassWithin				= NULL;
	DWORD    ClassFlags                 = 0;
	void     (*ClassConstructor)(void*) = NULL;
	UObject* PrePtr = Obj;
	
	if( !Obj )
	{
		// Create a new object.
		Obj = Ptr ? Ptr : (UObject*)appMalloc( InClass->cppSize, *InName );
		appMemzero(Obj, InClass->cppSize);
	}
	else
	{
		// Replace an existing object without affecting the original's address or index.
		check(!Ptr || Ptr==Obj);
		debugfSlow( NAME_DevReplace, TEXT("Replacing %ls %p"), Obj->GetName(), Obj );

		// Can only replace if class is identical.
		if( Obj->GetClass() != InClass )
			appErrorf( TEXT("Can't replace %ls with %ls"), Obj->GetFullName(), InClass->GetName() );

		// if this is the class default object, it means that InClass is native and the CDO was created during static registration.
		// Propagate the load flags but don't replace it - the CDO will be initialized by InitClassDefaultObject
		if (Obj->HasAnyFlags(RF_ClassDefaultObject))
		{
			Obj->ObjectFlags |= InFlags;

			// never call PostLoad on class default objects
			Obj->ClearFlags(RF_NeedPostLoad | RF_NeedPostLoadSubobjects);
			return Obj;
		}

		// Remember flags, index, and native class info.
		InFlags |= (Obj->GetFlags() & RF_Keep);
		Index    = Obj->Index;
		if( Cls )
		{
			ClassWithin		 = Cls->ClassWithin;
			ClassFlags       = Cls->ClassFlags & CLASS_Abstract;
			ClassConstructor = Cls->ClassConstructor;
		}

		// Destroy the object.
		Obj->~UObject();
		check(GObjAvailable.Num() && GObjAvailable.Last()==Index);
		GObjAvailable.Pop();
	}

	if ((InFlags & RF_ClassDefaultObject) == 0)
	{
		// If class is transient, objects must be transient.
		if ((InClass->ClassFlags & CLASS_Transient) != 0)
			InFlags |= RF_Transient;
	}
	else
	{
		// never call PostLoad on class default objects
		InFlags &= ~(RF_NeedPostLoad | RF_NeedPostLoadSubobjects);
	}

	// If the class is marked PerObjectConfig, mark the object as needing per-instance localization.
	if ((InClass->ClassFlags & (CLASS_PerObjectConfig | CLASS_Localized)) == (CLASS_PerObjectConfig | CLASS_Localized) || ((InClass->ClassFlags & CLASS_PerObjectLocalized) != 0))
		InFlags |= RF_PerObjectLocalized;

	// Set the base properties.
	Obj->Index			 = INDEX_NONE;
	Obj->HashNext		 = NULL;
	Obj->StateFrame      = NULL;
	Obj->_Linker		 = NULL;
	Obj->_LinkerIndex	 = INDEX_NONE;
	Obj->Outer			 = InOuter;
	Obj->ObjectFlags	 = InFlags;
	Obj->Name			 = InName;
	Obj->Class			 = InClass;
	Obj->ObjectArchetype = InTemplate;

	if (!(InClass->ClassFlags | CLASS_NoScriptProps))
	{
		// Init the properties.
		UClass* BaseClass = Obj->HasAnyFlags(RF_ClassDefaultObject) ? InClass->GetSuperClass() : InClass;
		if (BaseClass == NULL)
			BaseClass = InClass;
		INT DefaultsCount = InTemplate
			? InTemplate->GetClass()->GetPropertiesSize()
			: BaseClass->GetPropertiesSize();
		Obj->SafeInitProperties(Obj->GetInternalScriptData(InClass->PropertiesSize), InClass->GetPropertiesSize(), InClass, (InTemplate ? InTemplate->GetScriptData() : nullptr), DefaultsCount, Obj->HasAnyFlags(RF_NeedLoad) ? NULL : Obj, SubobjectRoot);
	}

	// Add to global table.
	Obj->AddObject( Index );
	check(Obj->IsValid());

	// Restore class information if replacing native class.
	if( Cls )
	{
		Cls->ClassWithin	   = ClassWithin;
		Cls->ClassFlags       |= ClassFlags;
		Cls->ClassConstructor  = ClassConstructor;
	}

	// Success.
	return Obj;
	unguardf(( TEXT("(%ls %ls)"), InClass ? InClass->GetName() : TEXT("NULL"), *InName ));
}

//
// Construct an object.
//
UObject* UObject::StaticConstructObject
(
	UClass*			InClass,
	UObject*		InOuter,
	FName			InName,
	EObjectFlags	InFlags,
	UObject*		InTemplate,
	FOutputDevice*	Error,
	UObject*		SubobjectRoot
)
{
	guard(UObject::StaticConstructObject);
	check(Error);

	// Allocate the object.
	UObject* Result = StaticAllocateObject( InClass, InOuter, InName, InFlags, InTemplate, Error, NULL, SubobjectRoot);
	if( Result )
		(*InClass->ClassConstructor)( Result );
	DEBUG_OBJECTS(Result->GetName());
	return Result;

	unguard;
}

//
// Attempt to delete an object. Only succeeds if unreferenced.
//
UBOOL UObject::AttemptDelete( UObject*& Obj, DWORD KeepFlags, UBOOL IgnoreReference )
{
	guard(UObject::AttemptDelete);
	if( !(Obj->GetFlags() & RF_Native) && !IsReferenced( Obj, KeepFlags, IgnoreReference ) )
	{
		PurgeGarbage();
		return 1;
	}
	else return 0;
	unguard;
}

void UObject::SetArchetype(UObject* NewArchetype, UBOOL bReinitialize)
{
	check(NewArchetype);
	check(NewArchetype != this);

	ObjectArchetype = NewArchetype;
	if (bReinitialize)
	{
		InitializeProperties(NULL);
	}
}

void UObject::InitializeProperties(UObject* SourceObject)
{
	guard(UObject::InitializeProperties);
	if (SourceObject == NULL)
		SourceObject = GetArchetype();

	check(SourceObject || Class == UObject::StaticClass());
	checkSlow(Class == UObject::StaticClass() || IsA(SourceObject->Class));

	UClass* SourceClass = SourceObject ? SourceObject->GetClass() : NULL;

	// Recreate this object based on the new archetype - using StaticConstructObject rather than manually tearing down and re-initializing
	// the properties for this object ensures that any cleanup required when an object is reinitialized from defaults occurs properly
	// for example, when re-initializing UPrimitiveComponents, the component must notify the rendering thread that its data structures are
	// going to be re-initialized
	StaticConstructObject(GetClass(), GetOuter(), GetFName(), GetFlags(), SourceObject, GError, HasAnyFlags(RF_ClassDefaultObject) ? NULL : this);
	unguard;
}

/*-----------------------------------------------------------------------------
	Importing and exporting.
-----------------------------------------------------------------------------*/

//
// Import an object from a file.
//
void UObject::ParseParms( const TCHAR* Parms )
{
	guard(ParseObjectParms);
	if( !Parms )
		return;
	BYTE* ThisData = GetScriptData();
	if (!ThisData)
		return;
	for( TFieldIterator<UProperty> It(GetClass()); It; ++It )
	{
		if( It->GetOuter()!=UObject::StaticClass() )
		{
			FString Value;
			if (Parse(Parms, *(FString(It->GetName()) + TEXT("=")), Value))
				It->ImportText(*Value, ThisData + It->Offset, PPF_Localized | PPF_ExecImport);
			else if (It->ArrayDim > 1)
			{
				for (INT a = 0; a < It->ArrayDim; ++a)
				{
					if (Parse(Parms, *FString::Printf(TEXT("%ls[%i]="), It->GetName(), a), Value))
						It->ImportText(*Value, ThisData + It->Offset + (a * It->ElementSize), PPF_Localized | PPF_ExecImport);
				}
			}
		}
	}
	unguard;
}

void UObject::DEBUG_VerifyObjects(const TCHAR* Tag)
{
	static UBOOL bInVerify = FALSE;
	if (bInVerify || !(GUglyHackFlags & 32))
		return;
	UObject* Obj = NULL;
	INT i = 0;
	guard(UObject::DEBUG_VerifyObjects);
	bInVerify = TRUE;
	for (i = 0; i < GObjObjects.Num(); ++i)
	{
		Obj = GObjObjects(i);
		if (Obj && Obj->Index != INDEX_NONE)
			Obj->IsAComponent();
	}
	bInVerify = FALSE;
	unguardf((TEXT("(%ls %p %i/%i %ls)"), Obj->GetFullName(), Obj, i, GObjObjects.Num(), Tag));
}

/*-----------------------------------------------------------------------------
	UTextBuffer implementation.
-----------------------------------------------------------------------------*/

UTextBuffer::UTextBuffer( const TCHAR* InText )
: Text( InText )
{}
void UTextBuffer::Serialize( const TCHAR* Data, EName Event )
{
	guard(UTextBuffer::Serialize);
	Text += (TCHAR*)Data;
	unguardobj;
}
void UTextBuffer::Serialize( FArchive& Ar )
{
	guard(UTextBuffer::Serialize);
	Super::Serialize(Ar);
	Ar << Pos << Top << Text;
	unguardobj;
}
IMPLEMENT_CLASS(UTextBuffer);

/*-----------------------------------------------------------------------------
	UEnum implementation.
-----------------------------------------------------------------------------*/

void UEnum::Serialize( FArchive& Ar )
{
	guard(UEnum::Serialize);
	Super::Serialize(Ar);
	Ar << Names;
	unguardobj;
}
IMPLEMENT_CLASS(UEnum);

/*-----------------------------------------------------------------------------
	FCompactIndex implementation.
-----------------------------------------------------------------------------*/

//
// FCompactIndex serializer.
//
FArchive& operator<<( FArchive& Ar, FCompactIndex& I )
{
	guard(FCompactIndex<<);
	BYTE  B0       = 0;
    BYTE  B1       = 0;
    DWORD V        = 0;
    INT   Original = 0;
	if( !Ar.IsLoading() && !Ar.IsSaving() )
	{
		Ar << I.Value;
	}
	else
	{
		Original = I.Value;
		V = Abs(I.Value);
		B0 = ((I.Value>=0) ? 0 : 0x80) + ((V < 0x40) ? V : ((V & 0x3f)+0x40));
		I.Value        = 0;
		Ar << B0;
		if( B0 & 0x40 )
		{
			V >>= 6;
			B1 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
			Ar << B1;
			if( B1 & 0x80 )
			{
				V >>= 7;
				BYTE B2 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
				Ar << B2;
				if( B2 & 0x80 )
				{
					V >>= 7;
					BYTE B3 = (V < 0x80) ? V : ((V & 0x7f)+0x80);
					Ar << B3;
					if( B3 & 0x80 )
					{
						V >>= 7;
						BYTE B4 = V;
						Ar << B4;
						I.Value = B4;
					}
					I.Value = (I.Value << 7) + (B3 & 0x7f);
				}
				I.Value = (I.Value << 7) + (B2 & 0x7f);
			}
			I.Value = (I.Value << 7) + (B1 & 0x7f);
		}
		I.Value = (I.Value << 6) + (B0 & 0x3f);
		if( B0 & 0x80 )
			I.Value = -I.Value;
		if( Ar.IsSaving() && I.Value!=Original )
			appErrorf( TEXT("Mismatch: %08X %08X"), I.Value, Original );
	}
	return Ar;
	unguard;
}

/*-----------------------------------------------------------------------------
	Implementations.
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(ULinker);
IMPLEMENT_CLASS(ULinkerLoad);
IMPLEMENT_CLASS(ULinkerSave);
IMPLEMENT_CLASS(USubsystem);

/*-----------------------------------------------------------------------------
	UPackage.
-----------------------------------------------------------------------------*/

UPackage::UPackage()
	: PackageFlags(PKG_AllowDownload), bIsAssetPackage(FALSE)
{}
void UPackage::Serialize( FArchive& Ar )
{
	guard(UPackage::Serialize);
	Super::Serialize( Ar );
	unguard;
}
void UPackage::Destroy()
{
	guard(UPackage::Destroy);
	Super::Destroy();
	unguard;
}
UPackage* UObject::GetOutermost()
{
	return Cast<UPackage>(TopOuter());
}
UMetaData* UPackage::GetMetaData()
{
	guard(UPackage::GetMetaData);
	if (MetaData == NULL)
	{
		// if it wasn't found, then create it
		if (MetaData == NULL)
		{
			// make a metadata object, but only allow it to be loaded in the editor
			//MetaData = ConstructObject<UMetaData>(UMetaData::StaticClass(), this, UMetaData::StaticClass()->GetFName(), RF_NotForClient | RF_NotForServer | RF_Standalone);
			MetaData = new(UObject::GetTransientPackage(), NAME_None, RF_Transient) UMetaData();
		}
		check(MetaData);
	}

	return MetaData;
	unguardobj;
}
IMPLEMENT_CLASS(UPackage);

/*-----------------------------------------------------------------------------
	UTextBufferFactory.
-----------------------------------------------------------------------------*/

void UTextBufferFactory::StaticConstructor()
{
	guard(UTextBufferFactory::StaticConstructor);

	SupportedClass = UTextBuffer::StaticClass();
	bCreateNew     = 0;
	bText          = 1;
	INT ThisSize = sizeof(UTextBufferFactory);
	new(Formats)FString( TEXT("txt;Text files") );

	unguard;
}
UTextBufferFactory::UTextBufferFactory()
{}
UObject* UTextBufferFactory::FactoryCreateText
(
	UClass*				Class,
	UObject*			InOuter,
	FName				InName,
	DWORD				InFlags,
	UObject*			Context,
	const TCHAR*		Type,
	const TCHAR*&		Buffer,
	const TCHAR*		BufferEnd,
	FFeedbackContext*	Warn
)
{
	guard(UTextBufferFactory::FactoryCreateText);

	new BYTE[25];

	// Import.
	UTextBuffer* Result = new(InOuter,InName,InFlags)UTextBuffer;
	Result->Text = Buffer;
	return Result;

	unguard;
}
IMPLEMENT_CLASS(UTextBufferFactory);

/*----------------------------------------------------------------------------
	ULanguage.
----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(ULanguage);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
