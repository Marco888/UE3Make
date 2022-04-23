/*=============================================================================
	UnClass.cpp: Object class implementation.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

#define VF_HASH_VARIABLES 0 /* Undecided!! */

#define DEBUG_EXPRS 0

#if DEBUG_EXPRS
static UBOOL bDumpExpressions = FALSE;
#endif

/*-----------------------------------------------------------------------------
	FPropertyTag.
-----------------------------------------------------------------------------*/

//
// A tag describing a class property, to aid in serialization.
//
struct FPropertyTag
{
	// Variables.
	FName	Type;		// Type of property
	BYTE	BoolVal;	// a boolean property's value (never need to serialize data for bool properties except here)
	FName	Name;		// Name of property.
	FName	StructName;	// Struct name if UStructProperty.
	FName	EnumName;	// Enum name if UByteProperty
	INT		Size;       // Property size.
	INT		ArrayIndex;	// Index if an array; else 0.
	INT		SizeOffset;	// location in stream of tag size member

	// Constructors.
	FPropertyTag()
	:	Type		( NAME_None )
	,	Name		( NAME_None )
	,	Size		( 0 )
	,	ArrayIndex	( 0 )
	{}
	FPropertyTag( FArchive& InSaveAr, UProperty* Property, INT InIndex, BYTE* Value )
	:	Type		(Property->GetID())
	,	Name		(Property->GetFName())
	,	StructName	(NAME_None)
	,	EnumName	(NAME_None)
	,	Size		(0)
	,	ArrayIndex	(InIndex)
	,	SizeOffset	(INDEX_NONE)
	{
		// Handle structs.
		UStructProperty* StructProperty = Cast<UStructProperty>(Property);
		if (StructProperty != NULL)
		{
			StructName = StructProperty->Struct->GetFName();
		}
		else
		{
			UByteProperty* ByteProp = Cast<UByteProperty>(Property);
			if (ByteProp != NULL && ByteProp->Enum != NULL)
			{
				EnumName = ByteProp->Enum->GetFName();
			}
		}

		UBoolProperty* Bool = Cast<UBoolProperty>(Property);
		BoolVal = (Bool && (*(BITFIELD*)Value & Bool->BitMask)) ? TRUE : FALSE;
	}

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FPropertyTag& Tag )
	{
		static TCHAR PrevTag[NAME_SIZE]=TEXT("");
		guard(FPropertyTag<<);
		BYTE SizeByte;
		_WORD SizeWord;
		INT SizeInt;

		// Name.
		guard(TagName);
		Ar << Tag.Name;
		if (Tag.Name == NAME_None || Ar.IsError())
			return Ar;
		appStrcpy( PrevTag, *Tag.Name );
		unguard;

		Ar << Tag.Type;
		//debugf(TEXT("Property %ls - %ls"), *Tag.Type, *Tag.Name);
		if (Ar.IsError())
			return Ar;
		if (Ar.IsSaving())
		{
			// remember the offset of the Size variable - UStruct::SerializeTaggedProperties will update it after the
			// property has been serialized.
			Tag.SizeOffset = Ar.Tell();
		}
		Ar << Tag.Size << Tag.ArrayIndex;

		// only need to serialize this for structs
		if (Tag.Type == NAME_StructProperty)
		{
			Ar << Tag.StructName;
		}
		// only need to serialize this for bools
		else if (Tag.Type == NAME_BoolProperty)
		{
			if (Ar.Ver() < VER_PROPERTYTAG_BOOL_OPTIMIZATION)
			{
				UBOOL Value = 0;
				Ar << Value;
				Tag.BoolVal = BYTE(Value);
			}
			else
			{
				Ar << Tag.BoolVal;
			}
		}
		// only need to serialize this for bytes
		else if (Tag.Type == NAME_ByteProperty && Ar.Ver() >= VER_BYTEPROP_SERIALIZE_ENUM)
		{
			Ar << Tag.EnumName;
		}
		return Ar;
		unguardf(( TEXT("(After %ls)"), PrevTag ));
	}

	// Property serializer.
	void SerializeTaggedProperty( FArchive& Ar, UProperty* Property, BYTE* Value )
	{
		guard(FPropertyTag::SerializeTaggedProperty);
		//debugf(TEXT("SerializeTaggedProperty %ls (offset %i)"), Property->GetFullName(), Property->Offset);
		//FString StrValue;
		//Property->ExportTextItem(StrValue, Value, NULL, 0);
		//debugf(TEXT(" - Value: %ls"), *StrValue);
		if (Property->GetClass() == UBoolProperty::StaticClass())
		{
			UBoolProperty* Bool = (UBoolProperty*)Property;
			if (Bool->BitMask == 0)
				appErrorf(TEXT("BitMask==0 (UnClass.cpp) in %ls %p"), Bool->GetFullName(), Bool);
			if (Ar.IsLoading())
			{
				if (BoolVal)
					*(BITFIELD*)Value |= Bool->BitMask;
				else *(BITFIELD*)Value &= ~Bool->BitMask;
			}
		}
		else Property->SerializeItem(Ar, Value);
		unguardf((TEXT("(%ls %p)"), Property->GetFullName(), Value));
	}
};

/*-----------------------------------------------------------------------------
	UField implementation.
-----------------------------------------------------------------------------*/

UField::UField()
: Next					( NULL )
, HashNext				( NULL )
, TempCompileFlags		( 0 )
, AltName				( NAME_None )
{}
UField::UField( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
: UObject				( EC_NativeConstructor, InClass, InName, InPackageName, InFlags )
, Next					( NULL )
, HashNext				( NULL )
, TempCompileFlags		( 0 )
, AltName				( NAME_None )
{}
UField::UField( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
: UObject				( EC_StaticConstructor, InName, InPackageName, InFlags )
, Next					( NULL )
, HashNext				( NULL )
, TempCompileFlags		( 0 )
, AltName				( NAME_None )
{}
UClass* UField::GetOwnerClass()
{
	guardSlow(UField::GetOwnerClass);
	UObject* Obj;
	for( Obj=this; Obj->GetClass()!=UClass::StaticClass(); Obj=Obj->GetOuter() );
	return (UClass*)Obj;
	unguardSlow;
}
void UField::Bind()
{
	guard(UField::Bind);
	unguard;
}
void UField::PostLoad()
{
	guard(UField::PostLoad);
	Super::PostLoad();
	Bind();
	unguard;
}
void UField::Serialize( FArchive& Ar )
{
	guard(UField::Serialize);
	Super::Serialize( Ar );

	if (Ar.IsError())
		return;

	//@compatibility:
	if (Ar.Ver() < VER_MOVED_SUPERFIELD_TO_USTRUCT)
	{
		UField* SuperField = NULL;
		Ar << SuperField;
		UStruct* Struct = Cast<UStruct>(this);
		if (Struct != NULL)
		{
			Struct->SuperStruct = Cast<UStruct>(SuperField);
		}
	}
	Ar << Next;

	unguardobj;
}
INT UField::GetPropertiesSize()
{
	return 0;
}
UBOOL UField::MergeBools()
{
	return 1;
}
void UField::AddCppProperty( UProperty* Property )
{
	guard(UField::AddCppProperty);
	appErrorf(TEXT("UField::AddCppProperty"));
	unguard;
}
void UField::Register()
{
	guard(UField::Register);
	Super::Register();
	unguard;
}
FName UField::GetFieldName() const
{
	guard(UField::GetFieldName);
	if (AltName != NAME_None)
		return AltName;
	return GetFName();
	unguard;
}

IMPLEMENT_CLASS(UField);

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

//
// Constructors.
//
UStruct::UStruct( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UStruct* InSuperStruct )
:	UField			( EC_NativeConstructor, UClass::StaticClass(), InName, InPackageName, InFlags )
,	ScriptText		( NULL )
,	Children		( NULL )
,	PropertiesSize	( InSize )
,	Script			()
,	TextPos			( 0 )
,	Line			( 0 )
,	PropertyLink	( NULL )
,	ConstructorLink	( NULL )
,	SuperStruct		( InSuperStruct )
{}
UStruct::UStruct( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	UField			( EC_StaticConstructor, InName, InPackageName, InFlags )
,	ScriptText		( NULL )
,	Children		( NULL )
,	PropertiesSize	( InSize )
,	Script			()
,	TextPos			( 0 )
,	Line			( 0 )
,	PropertyLink	( NULL )
,	ConstructorLink	( NULL )
,	SuperStruct		( NULL )
{}
UStruct::UStruct( UStruct* InSuperStruct )
:	PropertiesSize( InSuperStruct ? InSuperStruct->GetPropertiesSize() : 0 )
,	SuperStruct(InSuperStruct)
{}

//
// Add a property.
//
void UStruct::AddCppProperty( UProperty* Property )
{
	guard(UStruct::AddCppProperty);
	// Add at end of list.
	if( Children )
	{
		UField* Last = Children;
		while( Last->Next )
			Last = Last->Next;

		Last->Next = Property;
	}
	// First property.
	else
	{
		Children = Property;
	}
	Property->Next = NULL; // Should be zero initialized already.
	unguard;
}

//
// Register.
//
void UStruct::Register()
{
	guard(UStruct::Register);
	Super::Register();

	if (SuperStruct)
		SuperStruct->ConditionalRegister();

	unguard;
}

//
// Link offsets.
//
void UStruct::Link( FArchive& Ar, UBOOL Props )
{
	guard(UStruct::Link);

	// Link the properties.
	guard(LinkProperties);
	if( Props )
	{
		PropertiesSize  = 0;
		PropertiesAlign = 4; // minimum alignment for structs in UE1

		if (GetClass() == UClass::StaticClass())
			PropertiesSize = sizeof(UObject*); // Marco: Prefix with 'this' pointer.

//#define DEBUG_USCRIPT_CPP_MISMATCHES 1
#if DEBUG_USCRIPT_CPP_MISMATCHES
		appDebugMessagef(TEXT("# Linking Struct %ls"), GetFullName());
#endif

		if( GetInheritanceSuper() )
		{
			Ar.Preload( GetInheritanceSuper() );
			if (GetInheritanceSuper()->GetFName() == NAME_Vector)
				PropertiesSize = 12;
			else
				PropertiesSize = GetInheritanceSuper()->PropertiesSize;
			PropertiesAlign = GetInheritanceSuper()->PropertiesAlign;
		}
		UProperty* Prev = NULL;
		for (UField* Field = Children; Field; Field = Field->Next)
		{
			Ar.Preload(Field);
			if (Field->GetOuter() != this)
				break;

			UProperty* Property = Cast<UProperty>(Field);
			if (Property)
			{
				const INT SavedAlignment = Property->LinkAlignment;
				Property->Link(Ar, Prev);

				PropertiesSize = Property->Offset + Property->GetSize();
				Prev = Property;
				PropertiesAlign = Max(static_cast<INT>(Property->LinkAlignment), PropertiesAlign);
				Property->LinkAlignment = Max<WORD>(SavedAlignment, Property->LinkAlignment);
			}
		}
		PropertiesSize = Align(PropertiesSize,PropertiesAlign);
	}
	else
	{
		UProperty* Prev = NULL;
		for( UField* Field=Children; Field && Field->GetOuter()==this; Field=Field->Next )
		{
			UProperty* Property = Cast<UProperty>( Field );
			if( Property )
			{
                UBoolProperty* BoolProperty = Cast<UBoolProperty>(Property);
				INT SavedBitMask = BoolProperty ? BoolProperty->BitMask : 0;
				INT SavedOffset = Property->Offset;
				INT SavedAlignment = Property->LinkAlignment;

				Property->Link( Ar, Prev );

				Property->Offset = SavedOffset;
				Prev = Property;

				if ( BoolProperty )
					BoolProperty->BitMask = SavedBitMask;
                Property->LinkAlignment = SavedAlignment;
			}
		}
	}

	if (GetFName() == NAME_Vector)
	{
		PropertiesSize = Align(PropertiesSize, VECTOR_ALIGNMENT);
		PropertiesAlign = VECTOR_ALIGNMENT;
	}
	else if (GetFName() == NAME_Plane)
	{
		PropertiesAlign = VECTOR_ALIGNMENT;
	}
	else if (GetFName() == NAME_Coords)
	{
		PropertiesSize = 4 * Align(12, VECTOR_ALIGNMENT);
		PropertiesAlign = VECTOR_ALIGNMENT;
	}

	unguard;

	// Link the cleanup.
	guard(LinkCleanup);
	UProperty** PropertyLinkPtr		= &PropertyLink;
	UProperty** ConstructorLinkPtr	= &ConstructorLink;
	for( TFieldIterator<UProperty> ItC(this); ItC; ++ItC )
	{
		if( ItC->PropertyFlags & CPF_NeedCtorLink )
		{
			*ConstructorLinkPtr = *ItC;
			ConstructorLinkPtr  = &(*ConstructorLinkPtr)->ConstructorLinkNext;
		}
		*PropertyLinkPtr = *ItC;
		PropertyLinkPtr  = &(*PropertyLinkPtr)->PropertyLinkNext;
	}
	unguard;
	unguard;
}

//
// Serialize all of the class's data that belongs in a particular
// bin and resides in Data.
//
void UStruct::SerializeBin( FArchive& Ar, BYTE* Data )
{
	if (!Data)
		return;

	FName PropertyName(NAME_None);
	INT Index=0;
	guard(UStruct::SerializeBin);
	for( TFieldIterator<UProperty> It(this); It; ++It )
	{
		PropertyName = It->GetFName();
		if( It->ShouldSerializeValue(Ar) )
			for( Index=0; Index<It->ArrayDim; Index++ )
				It->SerializeItem( Ar, Data + It->Offset + Index*It->ElementSize );
	}
	unguardf(( TEXT("(%ls %ls[%i])"), GetFullName(), *PropertyName, Index ));
}
void UStruct::SerializeTaggedProperties(FArchive& Ar, BYTE* Data, UStruct* DefaultsStruct, BYTE* Defaults, INT DefaultsCount)
{
	if (!Data)
	{
		guard(UStruct::SerializeTaggedProperties);
		if (Ar.IsLoading())
		{
			// Load all stored properties.
			guard(LoadStream);
			//debugf(TEXT("LoadStream %ls"), GetFullName());
			while (1)
			{
				FPropertyTag Tag;
				Ar << Tag;
				//debugf(TEXT(" --- %ls sz %i (%i -> %i / %i)"), *Tag.Name, Tag.Size, Ar.Tell(), (Ar.Tell() + Tag.Size), Ar.TotalSize());
				if (Ar.IsError() || Tag.Name == NAME_None)
					return;
				Ar.Seek(Ar.Tell() + Tag.Size);
				if (Ar.IsError())
					return;
			}
			unguard;
		}
		else
		{
			// Save empty properties.
			guard(SaveStream);
			FName Temp(NAME_None);
			Ar << Temp;
			unguard;
		}
		unguardf((TEXT("(NoProperties)")));
		return;
	}
	FName PropertyName(NAME_None);
	INT Index=-1;
	INT Count = 0;
	guard(UStruct::SerializeTaggedProperties);
	check(Ar.IsLoading() || Ar.IsSaving());

	// Load/save.
#if VF_HASH_VARIABLES
	UClass* C = CastChecked<UClass>(this);
#endif
	if( Ar.IsLoading() )
	{
		// Load all stored properties.
		guard(LoadStream);
		//debugf(TEXT("LoadStream %ls"), GetFullName());
		while( 1 )
		{
			FPropertyTag Tag;
			Ar << Tag;
			if (Ar.IsError())
				return;
			if( Tag.Name == NAME_None )
				break;
			PropertyName = Tag.Name;
			UProperty* Property=NULL;
#if VF_HASH_VARIABLES
			for( UField* Node=C->VfHash[Tag.Name.GetIndex() & (UField::HASH_COUNT-1)]; Node; Node=Node->HashNext )
				if( Node->GetFName()==Tag.Name )
					{Property = Cast<UProperty>(Node); break;}
#else
			for( Property=PropertyLink; Property; Property=Property->PropertyLinkNext )
				if( Property->GetFName()==Tag.Name )
					break;
#endif
			if (!Property)
			{
				debugf(NAME_DevLoad, TEXT("Property %ls of %ls not found"), *Tag.Name, GetFullName());
			}
			else if (Tag.Type != Property->GetID())
			{
				debugf(NAME_DevLoad, TEXT("Type mismatch in %ls of %ls: file %i, class %i"), *Tag.Name, GetName(), Tag.Type, Property->GetID());
			}
			else if (Tag.ArrayIndex >= Property->ArrayDim)
			{
				debugf(NAME_DevLoad, TEXT("Array bounds in %ls of %ls: %i/%i"), *Tag.Name, GetName(), Tag.ArrayIndex, Property->ArrayDim);
			}
			else if (Tag.Type == NAME_StructProperty && Tag.StructName != CastChecked<UStructProperty>(Property)->Struct->GetFName())
			{
				// Marco: Added check for old Box struct type replaced with Core BoundingBox property (in Emitter).
				debugf( NAME_DevLoad, TEXT("Property %ls of %ls struct type mismatch %ls/%ls"), *Tag.Name, GetName(), *Tag.StructName, CastChecked<UStructProperty>(Property)->Struct->GetName() );
			}
			else if( !Property->ShouldSerializeValue(Ar) )
			{
				if( appStricmp(*Tag.Name,TEXT("visitedWeight"))!=0 )
					debugfSlow( NAME_DevLoad, TEXT("Property %ls of %ls is not serialiable"), *Tag.Name, GetName() );
			}
			else
			{
				// This property is ok.
				Tag.SerializeTaggedProperty( Ar, Property, Data + Property->Offset + Tag.ArrayIndex*Property->ElementSize );
				/*FString Val;
				Property->ExportTextItem(Val, Data + Property->Offset + Tag.ArrayIndex * Property->ElementSize, NULL, 0);
				debugf(TEXT("Value %ls[%i] = '%ls'"), Property->GetFullName(), Tag.ArrayIndex, *Val);*/
				continue;
			}

			// Skip unknown or bad property.
			debugfSlow( NAME_DevLoad, TEXT("Skipping %i bytes of type %i"), Tag.Size, Tag.Type );
			Ar.Seek(Ar.Tell() + Tag.Size);
			if (Ar.IsError())
				return;
		}
		unguardf(( TEXT("(Count %i)"), Count ));
	}
	else
	{
		// Save tagged properties.
		guard(SaveStream);

		//debugf(TEXT("SaveStream %ls===================================="), GetFullName());

		for( TFieldIterator<UProperty> It(this); It; ++It )
		{
			if( It->ShouldSerializeValue(Ar) )
			{
				PropertyName = It->GetFName();
				for( Index=0; Index<It->ArrayDim; Index++ )
				{
					INT Offset = It->Offset + Index*It->ElementSize;
					if( !It->Matches( Data, (Offset+It->ElementSize<=DefaultsCount) ? Defaults : NULL, Index) )
					{
						/*FString Val, DVal;
						It->ExportTextItem(Val, Data + It->Offset + Index * It->ElementSize, NULL, 0);
						if (Defaults && ((Offset + It->ElementSize) <= DefaultsCount))
							It->ExportTextItem(DVal, Defaults + It->Offset + Index * It->ElementSize, NULL, 0);
						debugf(TEXT("Prop %ls[%i] = '%ls' / '%ls'"), It->GetFullName(), Index, *Val, *DVal);*/

 						FPropertyTag Tag( Ar, *It, Index, Data + Offset );
						Ar << Tag;

						// need to know how much data this call to SerializeTaggedProperty consumes, so mark where we are
						INT DataOffset = Ar.Tell();

						Tag.SerializeTaggedProperty( Ar, *It, Data + Offset );

						// set the tag's size
						Tag.Size = Ar.Tell() - DataOffset;

						if (Tag.Size > 0)
						{
							// mark our current location
							DataOffset = Ar.Tell();

							// go back and re-serialize the size now that we know it
							Ar.Seek(Tag.SizeOffset);
							Ar << Tag.Size;

							// return to the current location
							Ar.Seek(DataOffset);
						}
					}
				}
			}
		}
		FName Temp(NAME_None);
		Ar << Temp;
		unguard;
	}
	unguardf((TEXT("(%ls[%i] %p)"), *PropertyName, Index, Data));
}
void UStruct::Destroy()
{
	guard(UStruct::Destroy);
	Script.Empty();
	Super::Destroy();
	unguard;
}
void UStruct::Serialize( FArchive& Ar )
{
	guard(UStruct::Serialize);
	Super::Serialize( Ar );

	if (Ar.IsError())
		return;

	// Serialize stuff.
	if (Ar.Ver() >= VER_MOVED_SUPERFIELD_TO_USTRUCT)
		Ar << SuperStruct;

	if (Ar.IsSaving())
		ScriptText = CppText = NULL;

	// Serialize stuff.
	Ar << ScriptText << Children;
	
	Ar << CppText;
	// Compiler info.
	Ar << Line << TextPos;

	// Ensure that last byte in script code is EX_EndOfScript to work around script debugger implementation.
	if (Ar.IsSaving() && Script.Num() && Script(Script.Num() - 1) != EX_EndOfScript)
		Script.AddItem(EX_EndOfScript);

	// Script code.
	INT ScriptSize = 0;
	INT ScriptStorageSize = 0;
	INT ScriptStorageSizeOffset = 0;

	if (Ar.IsLoading())
	{
		Ar << ScriptSize;
		if (Ar.Ver() >= VER_USTRUCT_SERIALIZE_ONDISK_SCRIPTSIZE)
		{
			Ar << ScriptStorageSize;
		}
		Script.Empty(ScriptSize);
		Script.Add(ScriptSize);
	}
	else if (Ar.IsSaving())
	{
		ScriptSize = Script.Num();
		Ar << ScriptSize;

		// drop a zero here.  will seek back later and re-write it when we know it
		ScriptStorageSizeOffset = Ar.Tell();
		Ar << ScriptStorageSize;
	}

	INT iCode = 0;
	INT const BytecodeStartOffset = Ar.Tell();
#if DEBUG_EXPRS
	bDumpExpressions = (Ar.IsSaving() && (!appStricmp(GetName(), TEXT("GetKFProjectileClass")) || !appStricmp(GetName(), TEXT("GetBotGrenade"))));
	if(bDumpExpressions)
		debugf(TEXT("%ls - SerializeExpr (%i)"), GetFullName(), ScriptSize);
#endif
	while (iCode < ScriptSize)
		SerializeExpr(iCode, Ar);

#if DEBUG_EXPRS
	bDumpExpressions = FALSE;
#endif

	if (iCode != ScriptSize)
		GWarn->Logf(TEXT("Script serialization mismatch: Got %i, expected %i (%i bytes diff) in %ls"), iCode, ScriptSize, (ScriptSize - iCode), GetFullName());

	if (Ar.IsSaving())
	{
		INT const BytecodeEndOffset = Ar.Tell();

		// go back and write on-disk size
		Ar.Seek(ScriptStorageSizeOffset);
		ScriptStorageSize = BytecodeEndOffset - BytecodeStartOffset;
		Ar << ScriptStorageSize;

		// back to where we were
		Ar.Seek(BytecodeEndOffset);

		//if(ScriptSize && !Ar.IsObjectReferenceCollector())
		//	debugf(TEXT("%ls size is %i"),GetFullName(), ScriptSize);
	}

	// Link the properties.
	if (Ar.IsLoading())
		Link(Ar, 1);

	unguardobj;
}

void UStruct::PropagateStructDefaults()
{
	UFunction* Function = NULL;
	guard(UStruct::PropagateStructDefaults);
	// flag any functions which contain struct properties that have defaults
	for (TFieldIterator<UFunction> Functions(this); Functions; ++Functions)
	{
		Function = *Functions;
		for (TFieldIterator<UStructProperty> Parameters(Function); Parameters; ++Parameters)
		{
			UStructProperty* Prop = *Parameters;
			if ((Prop->PropertyFlags & CPF_Parm) == 0 && Prop->Struct->GetDefaultsCount() > 0)
			{
				Function->FunctionFlags |= FUNC_HasDefaults;
				break;
			}
		}
	}
	unguardf((TEXT("(%ls - %ls)"),GetFullName(), Function->GetFullName()));
}

IMPLEMENT_CLASS(UStruct);

/*-----------------------------------------------------------------------------
	UState.
-----------------------------------------------------------------------------*/

UState::UState( UState* InSuperState )
: UStruct( InSuperState )
{}
UState::UState( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UState* InSuperState )
:	UStruct			( EC_NativeConstructor, InSize, InName, InPackageName, InFlags, InSuperState )
,	ProbeMask		( 0 )
,	IgnoreMask		( 0 )
,	StateFlags		( 0 )
,	LabelTableOffset( 0 )
{}
UState::UState( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags )
:	UStruct			( EC_StaticConstructor, InSize, InName, InPackageName, InFlags )
,	ProbeMask		( 0 )
,	IgnoreMask		( 0 )
,	StateFlags		( 0 )
,	LabelTableOffset( 0 )
{}
void UState::Destroy()
{
	guard(UState::Destroy);
	Super::Destroy();
	unguard;
}
void UState::Serialize( FArchive& Ar )
{
	guard(UState::Serialize);
	const UBOOL bRebuildFunctionMap = HasAnyFlags(RF_PendingFieldPatches);
	Super::Serialize( Ar );

	// @script patcher
	// if serialization set the label table offset, we want to ignore what we read below
	WORD const TmpLabelTableOffset = LabelTableOffset;

	// Class/State-specific union info.
	Ar << ProbeMask;
	Ar << LabelTableOffset << StateFlags;
	// serialize the function map
	//@todo ronp - why is this even serialized anyway?
	Ar << FuncMap;

	// @script patcher
	if (TmpLabelTableOffset > 0)
	{
		// ignore the serialized data by reverting to saved offset
		LabelTableOffset = TmpLabelTableOffset;
	}

	//@script patcher
	if (bRebuildFunctionMap)
	{
		for (TFieldIterator<UFunction> It(this); It; ++It)
		{
			// if this is a UFunction, we must also add it to the owning struct's lookup map
			FuncMap.Set(It->GetFName(), *It);
		}
	}

	unguard;
}
IMPLEMENT_CLASS(UState);
IMPLEMENT_CLASS(UInterface);

/*-----------------------------------------------------------------------------
	UScriptStruct.
-----------------------------------------------------------------------------*/
UScriptStruct::UScriptStruct(ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UScriptStruct* InSuperStruct)
	: UStruct(EC_NativeConstructor, InSize, InName, InPackageName, InFlags, InSuperStruct)
{}
UScriptStruct::UScriptStruct(EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags)
	: UStruct(EC_StaticConstructor, InSize, InName, InPackageName, InFlags)
{}
UScriptStruct::UScriptStruct(UScriptStruct* InSuperStruct)
	: UStruct(InSuperStruct)
{}

void UScriptStruct::AllocateStructDefaults()
{
	guard(UScriptStruct::AllocateStructDefaults);
	// We must use the struct's aligned size so that if Struct's aligned size is larger than its PropertiesSize, we don't overrun the defaults when
	// UStructProperty::CopyCompleteValue performs an appMemcpy using the struct property's ElementSize (which is always aligned)
	const INT BufferSize = GetPropertiesSize(); // Align(GetPropertiesSize(), GetMinAlignment());

	ScriptDefData.Empty(BufferSize);
	ScriptDefData.AddZeroed(BufferSize);
	unguard;
}

void UScriptStruct::Serialize(FArchive& Ar)
{
	guard(UScriptStruct::Serialize);
	Super::Serialize(Ar);

	// serialize the struct's flags
	Ar << StructFlags;

	// serialize the struct's defaults

	// look to see if our parent struct is a script struct, and has any defaults
	if (Ar.IsLoading() || Ar.IsSaving())
	{
		// Allocate struct defaults on load.
		if (Ar.IsLoading())
		{
			AllocateStructDefaults();
		}
		UScriptStruct* SStruct = Cast<UScriptStruct>(GetSuperStruct());
		SerializeTaggedProperties(Ar, &ScriptDefData(0), SStruct, SStruct ? SStruct->GetDefaults() : NULL, SStruct ? SStruct->GetPropertiesSize() : 0);
	}
	else
	{
		ScriptDefData.CountBytes(Ar);
		SerializeBin(Ar, &ScriptDefData(0));
	}
	unguardobj;
}

void UScriptStruct::PropagateStructDefaults()
{
	BYTE* DefaultData = GetDefaults();
	if (DefaultData != NULL)
	{
		for (TFieldIterator<UStructProperty> It(this); It; ++It)
		{
			UStructProperty* StructProperty = *It;

			// don't overwrite the values of properties which are marked native, since these properties
			// cannot be serialized by script.  For example, this would otherwise overwrite the 
			// VfTableObject property of UObject, causing all UObjects to have a NULL v-table.
			if (!(StructProperty->PropertyFlags & CPF_Native))
			{
				appMemzero(DefaultData + StructProperty->Offset, StructProperty->ElementSize);
				//StructProperty->InitializeValue(DefaultData + StructProperty->Offset);
			}
		}
	}

	Super::PropagateStructDefaults();
}

IMPLEMENT_CLASS(UScriptStruct);

/*-----------------------------------------------------------------------------
	UClass implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<(FArchive& Ar, FImplementedInterface& A)
{
	Ar << A.Class << A.PointerProperty;
	return Ar;
}

//
// Register the native class.
//
void UClass::Register()
{
	guard(UClass::Register);
	Super::Register();

	// Get stashed registration info.
	const TCHAR* InClassConfigName = *(TCHAR**)&ClassConfigName;
	ClassConfigName = InClassConfigName;

	// Init default object.
	GetDefaultObject(1);

	// Propagate inhereted flags.
	if(SuperStruct)
		ClassFlags |= (GetSuperClass()->ClassFlags & CLASS_Inherit);

	// Link the cleanup.
	FArchive ArDummy;
	Link( ArDummy, 0 );

	unguardf(( TEXT("(%ls)"), GetName() ));
}

//
// Find the class's native constructor.
//
void UClass::Bind()
{
	guard(UClass::Bind);
	//UStruct::Bind();
	if( !ClassConstructor && GetSuperClass() )
	{
		// Chase down constructor in parent class.
		GetSuperClass()->Bind();
		ClassConstructor = GetSuperClass()->ClassConstructor;
	}
	unguardobj;
}

/*-----------------------------------------------------------------------------
	UClass UObject implementation.
-----------------------------------------------------------------------------*/

void UClass::Destroy()
{
	guard(UClass::Destroy);
	Super::Destroy();
	unguard;
}
void UClass::PostLoad()
{
	guard(UClass::PostLoad);
	check(ClassWithin);
	Super::PostLoad();

	// Postload super.
	if( GetSuperClass() )
		GetSuperClass()->ConditionalPostLoad();

	unguardobj;
}
void UClass::Link( FArchive& Ar, UBOOL Props )
{
	guard(UClass::Link);
	Super::Link( Ar, Props );
	bNeedsPropertiesLinked = (PropertyLink == NULL) && (GUglyHackFlags & HACK_ClassLoadingDisabled) == 0;
	UClass* SClass = GetSuperClass();
	if (SClass)
		cppSize = Max(cppSize, SClass->cppSize);
	unguard;
}
void UClass::Serialize( FArchive& Ar )
{
	guard(UClass::Serialize);
	Super::Serialize( Ar );

	if (Ar.IsError())
		return;

	// Variables.
	if (Ar.IsSaving())
	{
		DWORD SaveFlags = ClassFlags & ~CLASS_SerializeFlags;
		Ar << SaveFlags;
	}
	else if (Ar.IsLoading())
	{
		UClass* SClass = GetSuperClass();
		DWORD PreFlags = ((SClass ? SClass->ClassFlags : 0) | ClassFlags) & CLASS_SerializeFlags;
		Ar << ClassFlags;
		ClassFlags |= (PreFlags | CLASS_Parsed | CLASS_Compiled);
		if (SClass && SClass->bDisableInstancing && !bDisableInstancing)
			bDisableInstancing = TRUE;
		if(bDisableInstancing)
			ClassFlags |= CLASS_NoScriptProps;
	}
	Ar << ClassWithin << ClassConfigName;
	Ar << ComponentNameToDefaultObjectMap;
	Ar << Interfaces;

	// if reading data that's cooked for console/pcserver, skip this data
	if (1)
	{
		if (Ar.Ver() >= VER_DONTSORTCATEGORIES_ADDED)
			Ar << DontSortCategories;

		Ar << HideCategories << AutoExpandCategories << AutoCollapseCategories;

		if (Ar.Ver() >= VER_FORCE_SCRIPT_DEFINED_ORDER_PER_CLASS)
			Ar << bForceScriptOrder;
		else bForceScriptOrder = 0;

		if (Ar.Ver() >= VER_ADDED_CLASS_GROUPS)
			Ar << ClassGroupNames;

		Ar << ClassHeaderFilename;
	}

	if (Ar.Ver() >= VER_SCRIPT_BIND_DLL_FUNCTIONS)
	{
		FName Dummy = NAME_None;
		Ar << Dummy;
	}

	if (Ar.IsLoading())
	{
		check(!GetSuperClass() || !GetSuperClass()->HasAnyFlags(RF_NeedLoad));
		Ar << ClassDefaultObject;

		// In order to ensure that the CDO inherits config & localized property values from the parent class, we can't initialize the CDO until
		// the parent class's CDO has serialized its data from disk and called LoadConfig/LoadLocalized - this occurs in ULinkerLoad::Preload so the
		// call to InitClassDefaultObject [for non-intrinsic classes] is deferred until then.
		// When running make, we don't load data from .ini/.int files, so there's no need to wait
		if(ClassDefaultObject)
			ClassDefaultObject->InitClassDefaultObject(this);
		ClassUnique = 0;
	}
	else
	{
#if SUPPORTS_SCRIPTPATCH_CREATION
		//@script patcher
		check(GIsScriptPatcherActive || GetDefaultsCount() == GetPropertiesSize());
#else
		check(GetDefaultsCount() == GetPropertiesSize());
#endif

		// only serialize the class default object if the archive allows serialization of ObjectArchetype
		// otherwise, serialize the properties that the ClassDefaultObject references
		// The logic behind this is the assumption that the reason for not serializing the ObjectArchetype
		// is because we are performing some actions on objects of this class and we don't want to perform
		// that action on the ClassDefaultObject.  However, we do want to perform that action on objects that
		// the ClassDefaultObject is referencing, so we'll serialize it's properties instead of serializing
		// the object itself
		Ar << ClassDefaultObject;
	}
	unguardobj;
}

void UClass::SerializeDefaultObject(UObject* Object, FArchive& Ar)
{
	guard(UClass::SerializeDefaultObject);
	Object->SerializeNetIndex(Ar);

	if (bDisableInstancing)
	{
		if (Ar.IsSaving() && !Ar.IsObjectReferenceCollector())
			appErrorf(TEXT("Can't save default object %ls!"), Object->GetFullName());
	}
	else if (Ar.IsLoading() || Ar.IsSaving())
	{
		UObject* ArchType = Object->GetArchetype();
		if(ArchType)
			SerializeTaggedProperties(Ar, Object->GetScriptData(), ArchType->GetClass(), ArchType->GetScriptData(), ArchType->GetScriptDataSize());
		else SerializeTaggedProperties(Ar, Object->GetScriptData(), NULL, NULL, 0);
	}
	else
	{
		SerializeBin(Ar, Object->GetScriptData());
	}
	unguardf((TEXT("(%ls - %ls)"), GetFullName(), Object->GetFullName()));
}

UObject* UClass::GetDefaultObject(UBOOL bForce /* = FALSE */)
{
	guard(UClass::GetDefaultObject);
	if (ClassDefaultObject == NULL)
	{
		UBOOL bCreateObject = bForce;
		if (!bCreateObject)
		{
			// when running make, only create default objects for intrinsic classes
			bCreateObject = HasAnyClassFlags(CLASS_Intrinsic) || (GUglyHackFlags & HACK_ClassLoadingDisabled);
		}

		if (bCreateObject)
		{
			UClass* ParentClass = GetSuperClass();
			UObject* ParentDefaultObject = NULL;
			if (ParentClass != NULL)
			{
				ParentDefaultObject = ParentClass->GetDefaultObject(bForce);
			}

			if (ParentDefaultObject != NULL || this == UObject::StaticClass())
			{
				ClassDefaultObject = UObject::StaticConstructObject(this, GetOuter(), NAME_None, (RF_Public | RF_ClassDefaultObject | RF_NeedLoad), ParentDefaultObject);

				// Perform static construction.
				if (HasAnyFlags(RF_Native) && ClassDefaultObject != NULL)
				{
					// only allowed to not have a static constructor during make
					if (ClassStaticConstructor != NULL && (!GetSuperClass() || GetSuperClass()->ClassStaticConstructor != ClassStaticConstructor))
						(ClassDefaultObject->*ClassStaticConstructor)();

					ConditionalLink();
				}
			}
		}
	}
	DEBUG_OBJECTS(GetName());
	return ClassDefaultObject;
	unguardobj;
}

void UClass::ConditionalLink()
{
	guard(UClass::ConditionalLink);
	// We need to know whether we're running the make commandlet before allowing any classes to be linked, since classes must
	// be allowed to link prior to serialization during make (since .u files may not exist).  However, GIsUCCMake can't be
	// set until after we've checked for outdated script files - otherwise, GIsUCCMake wouldn't be set if the user was attempting
	// to load a map and the .u files were outdated.  The check for outdated script files required GConfig, which isn't initialized until
	// appInit() returns;  appInit() calls UObject::StaticInit(), which registers all classes contained in Core.  Therefore, we need to delay
	// any linking until we can be sure that we're running make or not - by checking for GSys (which is set immediately after appInit() returns)
	// we can tell whether the value of GUglyHackFlags is reliable or not.
	if (GSys != NULL && bNeedsPropertiesLinked)
	{
		// script classes are normally linked at the end of UClass::Serialize, so only intrinsic classes should be
		// linked manually.  During make, .u files may not exist, so allow all classes to be linked from here.
		if (HasAnyClassFlags(CLASS_Intrinsic) || (GUglyHackFlags & HACK_ClassLoadingDisabled))
		{
			UBOOL bReadyToLink = TRUE;

			UClass* ParentClass = GetSuperClass();
			if (ParentClass != NULL)
			{
				ParentClass->ConditionalLink();

				// we're not ready to link until our parent has successfully linked, or we aren't able to load classes from disk
				bReadyToLink = ParentClass->PropertyLink != NULL || (GUglyHackFlags & HACK_ClassLoadingDisabled);
			}

			if (bReadyToLink)
			{
				// we must have a class default object by this point.
				checkSlow(ClassDefaultObject != NULL);
				if (GetSuperClass() != NULL && HasAnyClassFlags(CLASS_Intrinsic))
				{
					// re-propagate the class flags from the parent class, so that any class flags that were loaded from disk will be
					// correctly inherited
					UClass* SuperClass = GetSuperClass();
					ClassFlags |= (SuperClass->ClassFlags & CLASS_Inherit);
				}

				// now link the class, which sets up PropertyLink, ConfigLink, ConstructorLink, etc.
				FArchive DummyAr;
				Link(DummyAr, FALSE);

				// we may have inherited some properties which require construction (arrays, strings, etc.),
				// so now we should reinitialize the class default object against its archetype.
				ClassDefaultObject->InitClassDefaultObject(this);
			}
		}
	}
	DEBUG_OBJECTS(GetName());
	unguard;
}

void UClass::PropagateStructDefaults()
{
	UStructProperty* StructProperty = NULL;
	guard(UClass::PropagateStructDefaults);
	BYTE* DefaultData = GetDefaults();
	if (DefaultData != NULL)
	{
		for (TFieldIterator<UStructProperty> It(this); It; ++It)
		{
			StructProperty = *It;

			// don't overwrite the values of properties which are marked native, since these properties
			// cannot be serialized by script.  For example, this would otherwise overwrite the 
			// VfTableObject property of UObject, causing all UObjects to have a NULL v-table.
			if ((StructProperty->PropertyFlags & CPF_Native) == 0)
			{
				StructProperty->InitializeValue(DefaultData + StructProperty->Offset);
			}
		}
	}
	unguardf((TEXT("(%ls - %ls)"),GetFullName(), StructProperty->GetFullName()));
	Super::PropagateStructDefaults();
}

/*-----------------------------------------------------------------------------
	UClass constructors.
-----------------------------------------------------------------------------*/

//
// Internal constructor.
//
UClass::UClass()
:	ClassWithin( UObject::StaticClass() )
{}

//
// Create a new UClass given its superclass.
//
UClass::UClass( UClass* InBaseClass )
:	UState( InBaseClass )
,	ClassWithin( UObject::StaticClass() )
,	bNeedsPropertiesLinked(TRUE)
,	bDisableInstancing(FALSE)
{
	guard(UClass::UClass);
	UClass* ParentClass = GetSuperClass();
	if (ParentClass)
	{
		cppSize = ParentClass->cppSize;
		ClassWithin = ParentClass->ClassWithin;
		Bind();

		// if this is a native class, we may have defined a StaticConfigName() which overrides
		// the one from the parent class, so get our config name from there
		if (HasAnyFlags(RF_Native))
			ClassConfigName = StaticConfigName();
		else ClassConfigName = ParentClass->ClassConfigName;
	}
	unguardobj;
}

//
// UClass autoregistry constructor.
//warning: Called at DLL init time.
//
UClass::UClass
(
	ENativeConstructor,
	DWORD			InSize,
	DWORD			InClassFlags,
	UClass*			InSuperClass,
	UClass*			InWithinClass,
	FGuid			InGuid,
	const TCHAR*	InNameStr,
	const TCHAR*    InPackageName,
	const TCHAR*    InConfigName,
	EObjectFlags	InFlags,
	void			(*InClassConstructor)(void*),
	void			(UObject::*InClassStaticConstructor)(),
	UBOOL			bNoInst
)
:	UState					( EC_NativeConstructor, 0, InNameStr, InPackageName, InFlags, InSuperClass!=this ? InSuperClass : NULL )
,	ClassFlags				( InClassFlags | CLASS_Parsed | CLASS_Compiled )
,	ClassUnique				( 0 )
,	ClassGuid				( InGuid )
,	ClassWithin				( InWithinClass )
,	ClassConfigName			()
,	NetFields				()
,	Dependencies			()
,	PackageImports			()
,	ClassDefaultObject(NULL)
,	ClassConstructor		( InClassConstructor )
,	ClassStaticConstructor	( InClassStaticConstructor )
,	bNeedsPropertiesLinked(TRUE)
,	bDisableInstancing		(bNoInst)
,	cppSize					(InSize)
{
	*(const TCHAR**)&ClassConfigName = InConfigName;
}

// Called when statically linked.
UClass::UClass
(
	EStaticConstructor,
	DWORD			InSize,
	DWORD			InClassFlags,
	FGuid			InGuid,
	const TCHAR*	InNameStr,
	const TCHAR*    InPackageName,
	const TCHAR*    InConfigName,
	EObjectFlags	InFlags,
	void			(*InClassConstructor)(void*),
	void			(UObject::*InClassStaticConstructor)()
)
:	UState					( EC_StaticConstructor, 0, InNameStr, InPackageName, InFlags )
,	ClassFlags				( InClassFlags | CLASS_Parsed | CLASS_Compiled )
,	ClassUnique				( 0 )
,	ClassGuid				( InGuid )
,	ClassConfigName			()
,	NetFields				()
,	Dependencies			()
,	PackageImports			()
,	ClassDefaultObject(NULL)
,	ClassConstructor		( InClassConstructor )
,	ClassStaticConstructor	( InClassStaticConstructor )
,	bNeedsPropertiesLinked(TRUE)
,	bDisableInstancing(FALSE)
,	cppSize					(InSize)
{
	*(const TCHAR**)&ClassConfigName = InConfigName;
}

IMPLEMENT_CLASS(UClass);

/*-----------------------------------------------------------------------------
	FDependency.
-----------------------------------------------------------------------------*/

//
// FDepdendency inlines.
//
FDependency::FDependency()
{}
FDependency::FDependency( UClass* InClass, UBOOL InDeep )
:	Class( InClass )
,	Deep( InDeep )
,	ScriptTextCRC( Class ? Class->GetScriptTextCRC() : 0 )
{}
UBOOL FDependency::IsUpToDate()
{
	guard(FDependency::IsUpToDate);
	check(Class!=NULL);
	return Class->GetScriptTextCRC()==ScriptTextCRC;
	unguard;
}
CORE_API FArchive& operator<<( FArchive& Ar, FDependency& Dep )
{
	return Ar << Dep.Class << Dep.Deep << Dep.ScriptTextCRC;
}

/*-----------------------------------------------------------------------------
	FLabelEntry.
-----------------------------------------------------------------------------*/

FLabelEntry::FLabelEntry( FName InName, INT iInCode )
:	Name	(InName)
,	iCode	(iInCode)
{}
CORE_API FArchive& operator<<( FArchive& Ar, FLabelEntry &Label )
{
	Ar << Label.Name;
	Ar << Label.iCode;
	return Ar;
}

/*-----------------------------------------------------------------------------
	UStruct implementation.
-----------------------------------------------------------------------------*/

/**
 * Support functions for overlaying an object/name pointer onto an index (like in script code
 */
FORCEINLINE void* appSPtrToPointer(ScriptPointerType Value)
{
	return (void*)Value;
}

EExprToken UStruct::SerializeExpr(INT& iCode, FArchive& Ar)
{
	return SerializeSingleExpr(iCode, Script, Ar, this);
}

//
// Serialize an expression to an archive.
// Returns expression token.
//
EExprToken UStruct::SerializeSingleExpr(INT& iCode, TArray<BYTE>& Code, FArchive& Ar, UObject* RefObj)
{
	EExprToken Expr=(EExprToken)0;
	guard(SerializeSingleExpr);

#define SER_INNER SerializeSingleExpr(iCode, Code, Ar, RefObj)
#if DEBUG_EXPRS
	static TCHAR CallDepth[64] = TEXT("");
	static INT iStacking = 0;
	CallDepth[iStacking] = '-';
	CallDepth[++iStacking] = 0;
	#define GRABNAMEVALUE \
	if(bDumpExpressions) \
	{ \
		FName N; \
		if(Ar.IsSaving()) \
			N = *(FName*)&Code(iCode); \
		else \
		{ \
			INT o = Ar.Tell(); \
			Ar << N; Ar.Seek(o); \
		} \
		debugf(TEXT("%ls XFERName %ls"), CallDepth, *N); \
	}
	#define GRABOBJECTVALUE \
	if(bDumpExpressions) \
	{ \
		UObject* Obj; \
		if (Ar.IsSaving()) \
		{ \
			ScriptPointerType TempCode; \
			appMemcpy(&TempCode, &Code(iCode), sizeof(ScriptPointerType)); \
			Obj = (UObject*)appSPtrToPointer(TempCode); \
		} \
		else \
		{ \
			INT o = Ar.Tell(); \
			Ar << Obj; \
			Ar.Seek(o); \
		} \
		debugf(TEXT("%ls XFERObj %ls"), CallDepth, Obj->GetFullName()); \
	}
	#define GRAB_INT_VALUE \
	if(bDumpExpressions) \
	{ \
		INT iv; \
		if (Ar.IsSaving()) \
			iv = *(INT*)&Code(iCode); \
		else \
		{ \
			INT o = Ar.Tell(); \
			Ar << iv; \
			Ar.Seek(o); \
		} \
		debugf(TEXT("%ls XFERINT %i"), CallDepth, iv); \
	}
	#define GRAB_FLOAT_VALUE \
	if(bDumpExpressions) \
	{ \
		FLOAT iv; \
		if (Ar.IsSaving()) \
			iv = *(FLOAT*)&Code(iCode); \
		else \
		{ \
			INT o = Ar.Tell(); \
			Ar << iv; \
			Ar.Seek(o); \
		} \
		debugf(TEXT("%ls XFERFLOAT %f"), CallDepth, iv); \
	}
	#define GRAB_ANSI_VALUE \
	if(bDumpExpressions) \
	{ \
		FString Str; \
		if (Ar.IsSaving()) \
		{ \
			for (INT i = iCode; Code(i); ++i) \
				Str += TCHAR(Code(i)); \
		} \
		else \
		{ \
			INT o = Ar.Tell(); \
			BYTE ch; \
			while (1) \
			{ \
				Ar << ch; \
				if (!ch) \
					break; \
				Str += TCHAR(ch); \
			} \
			Ar.Seek(o); \
		} \
		debugf(TEXT("%ls XFERSTRING %ls"), CallDepth, *Str); \
	}
#else
	#define GRABNAMEVALUE
	#define GRABOBJECTVALUE
	#define GRAB_INT_VALUE
	#define GRAB_FLOAT_VALUE
	#define GRAB_ANSI_VALUE
#endif

	#define XFER(T) {Ar << *(T*)&Code(iCode); iCode += sizeof(T); }
	#define XFERNAME() XFER(FName)
	#define XFERPTR(T) \
	{ \
   	    T AlignedPtr = NULL; \
		ScriptPointerType TempCode; \
        if (!Ar.IsLoading()) \
		{ \
			appMemcpy( &TempCode, &Code(iCode), sizeof(ScriptPointerType) ); \
			AlignedPtr = (T)appSPtrToPointer(TempCode); \
		} \
		Ar << AlignedPtr; \
		if (!Ar.IsSaving()) \
		{ \
			TempCode = appPointerToSPtr(AlignedPtr); \
			appMemcpy( &Code(iCode), &TempCode, sizeof(ScriptPointerType) ); \
		} \
		iCode += sizeof(ScriptPointerType); \
	}
	#define XFER_FUNC_POINTER	XFERPTR(UStruct*)
	#define XFER_FUNC_NAME		XFERNAME()
	#define XFER_PROP_POINTER	XFERPTR(UProperty*)
	#define XFER_OBJECT_POINTER(Type)	XFERPTR(Type)
	#define XFER_LABELTABLE											\
	UState* const State = Cast<UState>(RefObj);						\
	if (State)														\
	{																\
		State->LabelTableOffset = iCode;							\
	}																\
	for( ; ; )														\
	{																\
		FLabelEntry* E = (FLabelEntry*)&Code(iCode);				\
		XFER(FLabelEntry);											\
		if ( E->Name == NAME_None )									\
		{															\
			break;													\
		}															\
	}
	#define HANDLE_OPTIONAL_DEBUG_INFO (void(0))

	// Get expr token.
	XFER(BYTE);
	Expr = (EExprToken)Code(iCode-1);
	if (Expr >= EX_FirstNative)
	{
#if DEBUG_EXPRS
		if (bDumpExpressions)
		{
			debugf(TEXT("%ls Expr EX_FirstNative - %i"), CallDepth, Expr);
			for (TObjectIterator<UFunction> It; It; ++It)
				if (It->iNative == Expr)
				{
					debugf(TEXT("%ls CallNative: %ls"), CallDepth, It->GetFullName());
					break;
				}
		}
#endif
		// Native final function with id 1-127.
		while (SER_INNER != EX_EndFunctionParms);
		HANDLE_OPTIONAL_DEBUG_INFO; //DEBUGGER
	}
	else if (Expr >= EX_ExtendedNative)
	{
		// Native final function with id 256-16383.
		XFER(BYTE);

#if DEBUG_EXPRS
		if (bDumpExpressions)
		{
			INT iNativeNum = ((Expr - EX_ExtendedNative) * 256) + Code(iCode-1);
			debugf(TEXT("%ls Expr EX_ExtendedNative - %i"), CallDepth, iNativeNum);
			for (TObjectIterator<UFunction> It; It; ++It)
				if (It->iNative == iNativeNum)
				{
					debugf(TEXT("%ls CallNative: %ls"), CallDepth, It->GetFullName());
					break;
				}
		}
#endif
		while (SER_INNER != EX_EndFunctionParms);
		HANDLE_OPTIONAL_DEBUG_INFO; //DEBUGGER
	}
	else
	{
#if DEBUG_EXPRS
		if (bDumpExpressions)
			debugf(TEXT("%ls Expr %ls %i"), CallDepth, GetExprName(Expr), iCode-1);
#endif
		switch (Expr)
		{
			case EX_PrimitiveCast:
			{
				// A type conversion.
				XFER(BYTE); //which kind of conversion
				SER_INNER;
				break;
			}
			case EX_InterfaceCast:
			{
				// A conversion from an object varible to a native interface variable.  We use a different bytecode to avoid the branching each time we process a cast token
				GRABOBJECTVALUE;
				XFERPTR(UClass*);	// the interface class to convert to
				SER_INNER;
				break;
			}
			case EX_Let:
			case EX_LetBool:
			case EX_LetDelegate:
			{
				SER_INNER; // Variable expr.
				SER_INNER; // Assignment expr.
				break;
			}
			case EX_Jump:
			{
				XFER(CodeSkipSizeType); // Code offset.
				break;
			}
			case EX_LocalVariable:
			case EX_InstanceVariable:
			case EX_DefaultVariable:
			case EX_LocalOutVariable:
			case EX_StateVariable:
			{
				GRABOBJECTVALUE;
				XFER_PROP_POINTER;
				break;
			}
			case EX_DebugInfo:
			{
				XFER(INT);	// Version
				XFER(INT);	// Line number
				XFER(INT);	// Character pos
				XFER(BYTE); // OpCode
				break;
			}
			case EX_BoolVariable:
			case EX_InterfaceContext:
			{
				SER_INNER;
				break;
			}
			case EX_Nothing:
			case EX_EndOfScript:
			case EX_EndFunctionParms:
			case EX_EmptyParmValue:
			case EX_IntZero:
			case EX_IntOne:
			case EX_True:
			case EX_False:
			case EX_NoObject:
			case EX_EmptyDelegate:
			case EX_Self:
			case EX_IteratorPop:
			case EX_Stop:
			case EX_IteratorNext:
			case EX_EndParmValue:
			{
				break;
			}
			case EX_ReturnNothing:
			{
				XFER_PROP_POINTER; // the return value property
				break;
			}
			case EX_EatReturnValue:
			{
				XFER_PROP_POINTER; // the return value property
				break;
			}
			case EX_Return:
			{
				SER_INNER; // Return expression.
				break;
			}
			case EX_FinalFunction:
			{
				GRABOBJECTVALUE;
				XFER_FUNC_POINTER;											// Stack node.
				while (SER_INNER != EX_EndFunctionParms); // Parms.
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_VirtualFunction:
			case EX_GlobalFunction:
			{
				GRABNAMEVALUE;
				XFER_FUNC_NAME;												// Virtual function name.
				while (SER_INNER != EX_EndFunctionParms);	// Parms.
				HANDLE_OPTIONAL_DEBUG_INFO;									//DEBUGGER
				break;
			}
			case EX_DelegateFunction:
			{
				XFER(BYTE);													// local prop
				XFER_PROP_POINTER;											// Delegate property
				XFERNAME();													// Delegate function name (in case the delegate is NULL)
				break;
			}
			case EX_NativeParm:
			{
				XFERPTR(UProperty*);
				break;
			}
			case EX_ClassContext:
			case EX_Context:
			{
				SER_INNER; // Object expression.
				XFER(CodeSkipSizeType);		// Code offset for NULL expressions.
				XFERPTR(UField*);			// Property corresponding to the r-value data, in case the l-value needs to be mem-zero'd
				XFER(BYTE);					// Property type, in case the r-value is a non-property such as dynamic array length
				SER_INNER; // Context expression.
				break;
			}
			case EX_DynArrayIterator:
			{
				SER_INNER; // Array expression
				SER_INNER; // Array item expression
				XFER(BYTE);					// Index parm present
				SER_INNER; // Index parm
				XFER(CodeSkipSizeType);		// Code offset
				break;
			}
			case EX_ArrayElement:
			case EX_DynArrayElement:
			{
				SER_INNER; // Index expression.
				SER_INNER; // Base expression.
				break;
			}
			case EX_DynArrayLength:
			{
				SER_INNER; // Base expression.
				break;
			}
			case EX_DynArrayAdd:
			{
				SER_INNER; // Base expression
				SER_INNER;	// Count
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArrayInsert:
			case EX_DynArrayRemove:
			{
				SER_INNER; // Base expression
				SER_INNER; // Index
				SER_INNER; // Count
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArrayAddItem:
			case EX_DynArrayRemoveItem:
			{
				SER_INNER; // Array property expression
				XFER(CodeSkipSizeType);		// Number of bytes to skip if NULL context encountered
				SER_INNER; // Item
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArrayInsertItem:
			{
				SER_INNER;	// Base expression
				XFER(CodeSkipSizeType);		// Number of bytes to skip if NULL context encountered
				SER_INNER; // Index
				SER_INNER;	// Item
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArrayFind:
			{
				SER_INNER; // Array property expression
				XFER(CodeSkipSizeType);		// Number of bytes to skip if NULL context encountered
				SER_INNER; // Search item
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArrayFindStruct:
			{
				SER_INNER; // Array property expression
				XFER(CodeSkipSizeType);		// Number of bytes to skip if NULL context encountered
				SER_INNER;	// Property name
				SER_INNER; // Search item
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_DynArraySort:
			{
				SER_INNER;	// Array property
				XFER(CodeSkipSizeType);		// Number of bytes to skip if NULL context encountered
				SER_INNER;	// Sort compare delegate
				SER_INNER; // EX_EndFunctionParms
				HANDLE_OPTIONAL_DEBUG_INFO;									// DEBUGGER
				break;
			}
			case EX_Conditional:
			{
				SER_INNER; // Bool Expr
				XFER(CodeSkipSizeType);		// Skip
				SER_INNER; // Result Expr 1
				XFER(CodeSkipSizeType);		// Skip2
				SER_INNER; // Result Expr 2
				break;
			}
			case EX_New:
			{
				SER_INNER; // Parent expression.
				SER_INNER; // Name expression.
				SER_INNER; // Flags expression.
				SER_INNER; // Class expression.
				break;
			}
			case EX_IntConst:
			{
				GRAB_INT_VALUE;
				XFER(INT);
				break;
			}
			case EX_FloatConst:
			{
				GRAB_FLOAT_VALUE;
				XFER(FLOAT);
				break;
			}
			case EX_StringConst:
			{
				GRAB_ANSI_VALUE;
				do XFER(BYTE) while (Code(iCode - 1));
				break;
			}
			case EX_UnicodeStringConst:
			{
				do XFER(WORD) while (Code(iCode - 1) || Code(iCode - 2));
				break;
			}
			case EX_ObjectConst:
			{
				GRABOBJECTVALUE;
				XFER_OBJECT_POINTER(UObject*);
				break;
			}
			case EX_NameConst:
			{
				GRABNAMEVALUE;
				XFERNAME();
				break;
			}
			case EX_RotationConst:
			{
				XFER(INT); XFER(INT); XFER(INT);
				break;
			}
			case EX_VectorConst:
			{
				XFER(FLOAT); XFER(FLOAT); XFER(FLOAT);
				break;
			}
			case EX_ByteConst:
			case EX_IntConstByte:
			{
				XFER(BYTE);
				break;
			}
			case EX_MetaCast:
			{
				GRABOBJECTVALUE;
				XFER_OBJECT_POINTER(UClass*);
				SER_INNER;
				break;
			}
			case EX_DynamicCast:
			{
				GRABOBJECTVALUE;
				XFER_OBJECT_POINTER(UClass*);
				SER_INNER;
				break;
			}
			case EX_JumpIfNot:
			{
				XFER(CodeSkipSizeType);		// Code offset.
				SER_INNER; // Boolean expr.
				break;
			}
			case EX_Iterator:
			{
				SER_INNER; // Iterator expr.
				XFER(CodeSkipSizeType);		// Code offset.
				break;
			}
			case EX_Switch:
			{
				XFER_PROP_POINTER;			// Size of property value, in case null context is encountered.
				XFER(BYTE);					// Property type, in case the r-value is a non-property such as dynamic array length
				SER_INNER; // Switch expr.
				break;
			}
			case EX_Assert:
			{
				XFER(WORD); // Line number.
				XFER(BYTE); // debug mode or not
				SER_INNER; // Assert expr.
				break;
			}
			case EX_Case:
			{
				CodeSkipSizeType W;
				XFER(CodeSkipSizeType);			// Code offset.
				appMemcpy(&W, &Code(iCode - sizeof(CodeSkipSizeType)), sizeof(CodeSkipSizeType));
				if (W != MAXWORD)
				{
					SER_INNER; // Boolean expr.
				}
				break;
			}
			case EX_LabelTable:
			{
				check((iCode & 3) == 0);

				XFER_LABELTABLE
				break;
			}
			case EX_GotoLabel:
			{
				SER_INNER; // Label name expr.
				break;
			}
			case EX_Skip:
			{
				XFER(CodeSkipSizeType);		// Skip size.
				SER_INNER; // Expression to possibly skip.
				break;
			}
			case EX_DefaultParmValue:
			{
				XFER(CodeSkipSizeType);		// Size of the expression for this default parameter - used by the VM to skip over the expression
											// if a value was specified in the function call

				HANDLE_OPTIONAL_DEBUG_INFO;	// DI_NewStack
				SER_INNER; // Expression for this default parameter value
				HANDLE_OPTIONAL_DEBUG_INFO;	// DI_PrevStack
				XFER(BYTE);					// EX_EndParmValue
				break;
			}
			case EX_StructCmpEq:
			case EX_StructCmpNe:
			{
				GRABOBJECTVALUE;
				XFERPTR(UStruct*);			// Struct.
				SER_INNER; // Left expr.
				SER_INNER; // Right expr.
				break;
			}
			case EX_StructMember:
			{
				XFER_PROP_POINTER;			// the struct property we're accessing
				GRABOBJECTVALUE;
				XFERPTR(UStruct*);			// the struct which contains the property
				XFER(BYTE);					// byte indicating whether a local copy of the struct must be created in order to access the member property
				XFER(BYTE);					// byte indicating whether the struct member will be modified by the expression it's being used in
				SER_INNER; // expression corresponding to the struct member property.
				break;
			}
			case EX_DelegateProperty:
			{
				XFER_FUNC_NAME;				// Name of function we're assigning to the delegate.
				XFER_PROP_POINTER;			// delegate property corresponding to the function we're assigning (if any)
				break;
			}
			case EX_InstanceDelegate:
			{
				XFER_FUNC_NAME;				// the name of the function assigned to the delegate.
				break;
			}
			case EX_EqualEqual_DelDel:
			case EX_NotEqual_DelDel:
			case EX_EqualEqual_DelFunc:
			case EX_NotEqual_DelFunc:
			{
				while (SER_INNER != EX_EndFunctionParms);
				HANDLE_OPTIONAL_DEBUG_INFO; //DEBUGGER
				break;
			}
			default:
			{
				// This should never occur.
				appErrorf(TEXT("Bad expr token %02x in %ls"), (BYTE)Expr, RefObj->GetFullName());
				break;
			}
		}
	}
#if DEBUG_EXPRS
	CallDepth[--iStacking] = 0;
#endif

	return Expr;

#undef XFER
#undef SER_INNER

	unguardf(( TEXT("%02X (%ls)"), Expr, GetExprName(Expr) ));
}

/*-----------------------------------------------------------------------------
	UFunction.
-----------------------------------------------------------------------------*/

UFunction::UFunction( UFunction* InSuperFunction )
: UStruct( InSuperFunction )
, FriendlyName(GetFName())
, CustomScope(NULL)
{}
void UFunction::Serialize( FArchive& Ar )
{
	guard(UFunction::Serialize);
	Super::Serialize( Ar );

	// Function info.
	Ar << iNative;
	Ar << OperPrecedence;
	Ar << FunctionFlags;

	// Replication info.
	if( FunctionFlags & FUNC_Net )
		Ar << RepOffset;

	// Precomputation.
	if( Ar.IsLoading() )
	{
		NumParms          = 0;
		ParmsSize         = 0;
		ReturnValueOffset = MAXWORD;
		for( UProperty* Property=Cast<UProperty>(Children); Property && (Property->PropertyFlags & CPF_Parm); Property=Cast<UProperty>(Property->Next) )
		{
			NumParms++;
			ParmsSize = Property->Offset + Property->GetSize();
			if( Property->PropertyFlags & CPF_ReturnParm )
				ReturnValueOffset = Property->Offset;
		}
	}

	Ar << FriendlyName;
	unguard;
}
void UFunction::PostLoad()
{
	guard(UFunction::PostLoad);
	Super::PostLoad();

	if (!SuperStruct)
	{
		UClass* ParentClass = FindOuter<UClass>(this)->GetSuperClass();
		if (ParentClass)
			SuperStruct = FindField<UFunction>(ParentClass,GetName());
	}
	unguard;
}
UProperty* UFunction::GetReturnProperty()
{
	guard(UFunction::GetReturnProperty);
	for( TFieldIterator<UProperty> It(this); It && (It->PropertyFlags & CPF_Parm); ++It )
		if( It->PropertyFlags & CPF_ReturnParm )
			return *It;
	return NULL;
	unguard;
}
void UFunction::Bind()
{
	guard(UFunction::Bind);
	unguard;
}
void UFunction::Link( FArchive& Ar, UBOOL Props )
{
	guard(UFunction::Link);
	Super::Link( Ar, Props );
	unguard;
}
void UFunction::Destroy()
{
	guard(UFunction::Destroy);
	Super::Destroy();
	unguard
}

IMPLEMENT_CLASS(UFunction);

/*-----------------------------------------------------------------------------
	UConst.
-----------------------------------------------------------------------------*/

UConst::UConst( const TCHAR* InValue )
:	Value( InValue )
{}
void UConst::Serialize( FArchive& Ar )
{
	guard(UConst::Serialize);
	Super::Serialize( Ar );
	Ar << Value;
	unguard;
}
IMPLEMENT_CLASS(UConst);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
