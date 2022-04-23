/*=============================================================================
	UnClass.h: UClass definition.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney.
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

enum EStructFlags
{
	// State flags.
	STRUCT_Native = 0x00000001,
	STRUCT_Export = 0x00000002,
	STRUCT_HasComponents = 0x00000004,
	STRUCT_Transient = 0x00000008,

	/** Indicates that this struct should always be serialized as a single unit */
	STRUCT_Atomic = 0x00000010,

	/** Indicates that this struct uses binary serialization; it is unsafe to add/remove members from this struct without incrementing the package version */
	STRUCT_Immutable = 0x00000020,

	/** Indicates that only properties marked config/globalconfig within this struct can be read from .ini */
	STRUCT_StrictConfig = 0x00000040,

	/** Indicates that this struct will be considered immutable on platforms using cooked data. */
	STRUCT_ImmutableWhenCooked = 0x00000080,

	/** Indicates that this struct should always be serialized as a single unit on platforms using cooked data. */
	STRUCT_AtomicWhenCooked = 0x00000100,

	/** Struct flags that are automatically inherited */
	STRUCT_Inherit = STRUCT_HasComponents | STRUCT_Atomic | STRUCT_AtomicWhenCooked | STRUCT_StrictConfig,
};

enum ECompileTimeFlags : DWORD
{
	TFFLAG_Stripped = 0x00000001,
};

/*-----------------------------------------------------------------------------
	FDependency.
-----------------------------------------------------------------------------*/

//
// One dependency record, for incremental compilation.
//
class CORE_API FDependency
{
public:
	// Variables.
	UClass*		Class;
	UBOOL		Deep;
	DWORD		ScriptTextCRC;

	// Functions.
	FDependency();
	FDependency( UClass* InClass, UBOOL InDeep );
	UBOOL IsUpToDate();
	CORE_API friend FArchive& operator<<( FArchive& Ar, FDependency& Dep );
};

/*-----------------------------------------------------------------------------
	FRepLink.
-----------------------------------------------------------------------------*/

//
// A tagged linked list of replicatable variables.
//
class FRepLink
{
public:
	UProperty*	Property;		// Replicated property.
	FRepLink*	Next;			// Next replicated link per class.
	FRepLink( UProperty* InProperty, FRepLink* InNext )
	:	Property	(InProperty)
	,	Next		(InNext)
	{}
};

//
// Information about a property to replicate.
//
struct FRepRecord
{
	UProperty* Property;
	INT Index;
	FRepRecord(UProperty* InProperty, INT InIndex)
		: Property(InProperty), Index(InIndex)
	{}
};

/*-----------------------------------------------------------------------------
	FLabelEntry.
-----------------------------------------------------------------------------*/

//
// Entry in a state's label table.
//
struct CORE_API FLabelEntry
{
	// Variables.
	FName	Name;
	INT		iCode;

	// Functions.
	FLabelEntry( FName InName, INT iInCode );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FLabelEntry &Label );
};

/*-----------------------------------------------------------------------------
	UField.
-----------------------------------------------------------------------------*/

//
// Base class of UnrealScript language objects.
//
class CORE_API UField : public UObject
{
	DECLARE_ABSTRACT_CLASS(UField, UObject, CLASS_NoScriptProps, Core);

	// Variables.
	UField*			Next;
	UField*			HashNext;
	DWORD			TempCompileFlags;
	FName			AltName;

	// Constructors.
	UField();
	UField( ENativeConstructor, UClass* InClass, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UField( EStaticConstructor, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Register();

	// UField interface.
	virtual void AddCppProperty( UProperty* Property );
	virtual UBOOL MergeBools();
	virtual void Bind();
	virtual UClass* GetOwnerClass();
	virtual INT GetPropertiesSize();
	virtual FName GetFieldName() const;
};

/*-----------------------------------------------------------------------------
	UStruct.
-----------------------------------------------------------------------------*/

//
// An UnrealScript structure definition.
//
class CORE_API UStruct : public UField
{
	DECLARE_CLASS(UStruct, UField, CLASS_NoScriptProps, Core);
	NO_DEFAULT_CONSTRUCTOR(UStruct);

	// Variables.
	UTextBuffer*		ScriptText;
	UTextBuffer*		CppText;
	UField*				Children;
	INT					PropertiesSize;
	INT					PropertiesAlign;
	TArray<BYTE>		Script;
	UStruct*			SuperStruct;

	// Compiler info.
	INT					TextPos;
	INT					Line;
	FString				CppExport;

	// In memory only.
	UProperty*			PropertyLink;
	UProperty*			ConstructorLink;

	// Constructors.
	UStruct( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UStruct* InSuperStruct );
	UStruct( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UStruct( UStruct* InSuperStruct );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();
	void Register();

	// UField interface.
	void AddCppProperty( UProperty* Property );

	// UStruct interface.
	virtual UStruct* GetInheritanceSuper() {return GetSuperStruct();}
	virtual void Link( FArchive& Ar, UBOOL Props );
	virtual void SerializeBin( FArchive& Ar, BYTE* Data );
	virtual void SerializeTaggedProperties(FArchive& Ar, BYTE* Data, UStruct* DefaultsStruct, BYTE* Defaults, INT DefaultsCount);
	virtual EExprToken SerializeExpr(INT& iCode, FArchive& Ar);

	static EExprToken SerializeSingleExpr(INT& iCode, TArray<BYTE>& Code, FArchive& Ar, UObject* RefObj);

	INT GetPropertiesSize()
	{
		return PropertiesSize;
	}
	DWORD GetScriptTextCRC()
	{
		return ScriptText ? appStrCrc(*ScriptText->Text) : 0;
	}
	void SetPropertiesSize( INT NewSize )
	{
		PropertiesSize = NewSize;
	}
	UBOOL IsChildOf( const UStruct* SomeBase ) const
	{
		guardSlow(UStruct::IsChildOf);
		for( const UStruct* Struct=this; Struct; Struct=Struct->GetSuperStruct() )
			if( Struct==SomeBase )
				return 1;
		return 0;
		unguardobjSlow;
	}
	virtual TCHAR* GetNameCPP()
	{
		TCHAR* Result = appStaticString1024();
		appSprintf( Result, TEXT("F%ls"), GetName() );
		return Result;
	}
	UStruct* GetSuperStruct() const
	{
		guardSlow(UStruct::GetSuperStruct);
		//checkSlow(!SuperField);
		return SuperStruct;
		unguardSlow;
	}
	UBOOL StructCompare( const void* A, const void* B );
	virtual void PropagateStructDefaults();
	virtual UBOOL NoInstanceVariables() { return FALSE; }
};

/**
 * An UnrealScript structure definition.
 */
class UScriptStruct : public UStruct
{
	DECLARE_CLASS(UScriptStruct, UStruct, CLASS_NoScriptProps, Core);
	NO_DEFAULT_CONSTRUCTOR(UScriptStruct);

	UScriptStruct(ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UScriptStruct* InSuperStruct);
	UScriptStruct(EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags);
	explicit UScriptStruct(UScriptStruct* InSuperStruct);

	FString				DefaultStructPropText;
	DWORD				StructFlags;
	TArray<BYTE>		ScriptDefData;

	// UObject Interface
	void Serialize(FArchive& Ar);

	// UStruct Interface
	void PropagateStructDefaults();

	// UScriptStruct Interface
	BYTE* GetDefaults() { return &ScriptDefData(0); }
	INT GetDefaultsCount() { return ScriptDefData.Num(); }
	TArray<BYTE>& GetDefaultArray() { return ScriptDefData; }
	BYTE* GetScriptData()
	{
		return &ScriptDefData(0);
	}

	void AllocateStructDefaults();
};

/*-----------------------------------------------------------------------------
	TFieldIterator.
-----------------------------------------------------------------------------*/

//
// For iterating through a linked list of fields.
//
template <class T> class TFieldIterator
{
public:
	TFieldIterator( const UStruct* InStruct, UBOOL bParentFields=TRUE )
	: Struct( const_cast<UStruct*>(InStruct) )
	, Field( InStruct ? InStruct->Children : NULL )
	, bCheckParent(bParentFields)
	{
		IterateToNext();
	}
	operator UBOOL()
	{
		return Field != NULL;
	}
	void operator++()
	{
		checkSlow(Field);
		Field = Field->Next;
		IterateToNext();
	}
	T* operator*()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	T* operator->()
	{
		checkSlow(Field);
		return (T*)Field;
	}
	UStruct* GetStruct()
	{
		return Struct;
	}
protected:
	void IterateToNext()
	{
		while( Struct )
		{
			while( Field )
			{
				if( Field->IsA(T::StaticClass()) )
					return;
				Field = Field->Next;
			}
			if (bCheckParent)
			{
				Struct = Struct->GetInheritanceSuper();
				if (Struct)
					Field = Struct->Children;
			}
			else Struct = NULL;
		}
	}
	UStruct* Struct;
	UField* Field;
	UBOOL bCheckParent;
};

/*-----------------------------------------------------------------------------
	UFunction.
-----------------------------------------------------------------------------*/

//
// An UnrealScript function.
//
class CORE_API UFunction : public UStruct
{
	DECLARE_CLASS(UFunction, UStruct, CLASS_NoScriptProps, Core);
	DECLARE_WITHIN(UState);
	NO_DEFAULT_CONSTRUCTOR(UFunction);

	// Persistent variables.
	DWORD FunctionFlags;
	_WORD iNative;
	_WORD RepOffset;
	BYTE  OperPrecedence;

	FName	FriendlyName;				// friendly version for this function, mainly for operators

	// Variables in memory only.
	BYTE  NumParms;
	_WORD ParmsSize;
	_WORD ReturnValueOffset;
#if DO_GUARD_SLOW
	SQWORD Calls,Cycles;
#endif
	UStruct* CustomScope;

	// Constructors.
	UFunction( UFunction* InSuperFunction );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Destroy();

	// UField interface.
	void Bind();

	// UStruct interface.
	UBOOL MergeBools() {return 0;}
	UStruct* GetInheritanceSuper() {return NULL;}
	void Link( FArchive& Ar, UBOOL Props );

	// UFunction interface.
	UFunction* GetSuperFunction() const
	{
		guardSlow(UFunction::GetSuperFunction);
		checkSlow(!SuperField||SuperField->IsA(UFunction::StaticClass()));
		return (UFunction*)SuperStruct;
		unguardSlow;
	}
	UProperty* GetReturnProperty();

	inline UBOOL HasAnyFunctionFlags(DWORD Flags) const
	{
		return (FunctionFlags & Flags) != 0;
	}
};

/*-----------------------------------------------------------------------------
	UState.
-----------------------------------------------------------------------------*/

//
// An UnrealScript state.
//
class CORE_API UState : public UStruct
{
	DECLARE_CLASS(UState, UStruct, CLASS_NoScriptProps, Core);
	NO_DEFAULT_CONSTRUCTOR(UState);

	// Variables.
	DWORD ProbeMask;
	QWORD IgnoreMask;
	DWORD StateFlags;
	_WORD LabelTableOffset;

	/** Map of all functions by name contained in this state */
	TMap<FName, UFunction*> FuncMap;

	// Constructors.
	UState( ENativeConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags, UState* InSuperState );
	UState( EStaticConstructor, INT InSize, const TCHAR* InName, const TCHAR* InPackageName, EObjectFlags InFlags );
	UState( UState* InSuperState );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// UStruct interface.
	UBOOL MergeBools() {return 1;}
	UStruct* GetInheritanceSuper() {return GetSuperState();}

	// UState interface.
	UState* GetSuperState() const
	{
		guardSlow(UState::GetSuperState);
		checkSlow(!SuperField||SuperField->IsA(UState::StaticClass()));
		return (UState*)SuperStruct;
		unguardSlow;
	}
};

/*-----------------------------------------------------------------------------
	UEnum.
-----------------------------------------------------------------------------*/

//
// An enumeration, a list of names usable by UnrealScript.
//
class CORE_API UEnum : public UField
{
	DECLARE_CLASS(UEnum, UField, CLASS_NoScriptProps, Core);
	DECLARE_WITHIN(UStruct);

public:
	UEnum() {}

	// Variables.
	TArray<FName> Names;

	// UObject interface.
	void Serialize( FArchive& Ar );

	inline INT FindEnumIndex(const FName& N) const
	{
		return Names.FindItemIndex(N);
	}

	UBOOL SetEnums(TArray<FName>& InNames);
	UBOOL GenerateMaxEnum();
	FString GenerateEnumPrefix() const;
};

/*-----------------------------------------------------------------------------
	UClass.
-----------------------------------------------------------------------------*/

struct FImplementedInterface
{
	/** the interface class */
	UClass* Class;
	/** the pointer property that is located at the offset of the interface's vtable */
	UProperty* PointerProperty;

	FImplementedInterface()
	{}
	FImplementedInterface(UClass* InClass, UProperty* InPointer)
		: Class(InClass), PointerProperty(InPointer)
	{}

	friend FArchive& operator<<(FArchive& Ar, FImplementedInterface& A);
};

//
// An object class.
//
class CORE_API UClass : public UState
{
	DECLARE_CLASS(UClass, UState, CLASS_NoScriptProps, Core);
	DECLARE_WITHIN(UPackage);

	// Variables.
	INT					cppSize;
	DWORD				ClassFlags;
	INT					ClassUnique;
	FGuid				ClassGuid;
	UClass*				ClassWithin;
	FName				ClassConfigName;
	TArray<FRepRecord>	ClassReps;
	TArray<UField*>		NetFields;
	TArray<FDependency> Dependencies;
	TArray<FName>		PackageImports;
	TArray<FName>       DependentOn;
	void(*ClassConstructor)(void*);
	void(UObject::*ClassStaticConstructor)();
	TArray<FImplementedInterface> Interfaces;

	TArray<FName>		DontSortCategories;
	TArray<FName>		HideCategories;
	TArray<FName>		AutoExpandCategories;
	TArray<FName>		AutoCollapseCategories;
	TArray<FName>		ClassGroupNames;
	UBOOL				bForceScriptOrder;
	FString				ClassHeaderFilename;

	/** A mapping of the component template names inside this class to the template itself */
	TMap<FName, class UComponent*>	ComponentNameToDefaultObjectMap;
	UObject* ClassDefaultObject;

	// In memory only.
	FString				DefaultPropText, CppExtension;

	/** Indicates whether this class has been property linked (e.g. PropertyLink chain is up-to-date) */
	UBOOL				bNeedsPropertiesLinked;
	UBOOL				bDisableInstancing; // Marco: This class shouldn't instance itself!

	// Constructors.
	UClass();
	UClass( UClass* InSuperClass );
	UClass( ENativeConstructor, DWORD InSize, DWORD InClassFlags, UClass* InBaseClass, UClass* InWithinClass, FGuid InGuid, const TCHAR* InNameStr, const TCHAR* InPackageName, const TCHAR* InClassConfigName, EObjectFlags InFlags, void(*InClassConstructor)(void*), void(UObject::*InClassStaticConstructor)(), UBOOL bNoInst=FALSE );
	UClass( EStaticConstructor, DWORD InSize, DWORD InClassFlags, FGuid InGuid, const TCHAR* InNameStr, const TCHAR* InPackageName, const TCHAR* InClassConfigName, EObjectFlags InFlags, void(*InClassConstructor)(void*), void(UObject::*InClassStaticConstructor)() );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void PostLoad();
	void Destroy();
	void Register();

	// UField interface.
	void Bind();

	// UStruct interface.
	UBOOL MergeBools() {return 1;}
	UStruct* GetInheritanceSuper() {return GetSuperClass();}
	TCHAR* GetNameCPP()
	{
		TCHAR* Result = appStaticString1024();
		UClass* C;
		for( C=this; C; C=C->GetSuperClass() )
			if( appStricmp(C->GetName(),TEXT("Actor"))==0 )
				break;
		appSprintf( Result, TEXT("%ls%ls"), C ? TEXT("A") : TEXT("U"), GetName() );
		return Result;
	}
	void Link( FArchive& Ar, UBOOL Props );
	UBOOL NoInstanceVariables() { return bDisableInstancing; }

	// UClass interface.
	void AddDependency( UClass* InClass, UBOOL InDeep )
	{
		guard(UClass::AddDependency);
		INT i;
		for( i=0; i<Dependencies.Num(); i++ )
			if( Dependencies(i).Class==InClass )
				Dependencies(i).Deep |= InDeep;
		if( i==Dependencies.Num() )
			new(Dependencies)FDependency( InClass, InDeep );
		unguard;
	}
	UClass* GetSuperClass() const
	{
		guardSlow(UClass::GetSuperClass);
		return (UClass *)SuperStruct;
		unguardSlow;
	}
	void SerializeDefaultObject(UObject* Object, FArchive& Ar);
	UObject* GetDefaultObject(UBOOL bForce = FALSE);

	class AActor* GetDefaultActor(UBOOL bForce = FALSE)
	{
		guardSlow(UClass::GetDefaultActor);
		return (AActor*)GetDefaultObject(bForce);
		unguardobjSlow;
	}
	void ConditionalLink();

	BYTE* GetDefaults()
	{
		UObject* Result = GetDefaultObject();
		return (Result ? Result->GetScriptData() : nullptr);
	}
	INT GetDefaultsCount()
	{
		return ClassDefaultObject != NULL ? GetPropertiesSize() : 0;
	}

	inline UBOOL HasAnyClassFlags(DWORD Flags) const
	{
		return (ClassFlags & Flags) != 0;
	}

	inline UBOOL ImplementsInterface(const class UClass* SomeInterface) const
	{
		if (SomeInterface != NULL && SomeInterface->HasAnyClassFlags(CLASS_Interface) && SomeInterface != UInterface::StaticClass())
		{
			for (const UClass* CurrentClass = this; CurrentClass; CurrentClass = CurrentClass->GetSuperClass())
			{
				// SomeInterface might be a base interface of our implemented interface
				for(INT i=0; i< CurrentClass->Interfaces.Num(); ++i)
				{
					const UClass* InterfaceClass = CurrentClass->Interfaces(i).Class;
					if (InterfaceClass->IsChildOf(SomeInterface))
					{
						return TRUE;
					}
				}
			}
		}

		return FALSE;
	}

	void PropagateStructDefaults();

private:
	// Hide IsA because calling IsA on a class almost always indicates
	// an error where the caller should use IsChildOf.
	UBOOL IsA( UClass* Parent ) const {return UObject::IsA(Parent);}
};

/*-----------------------------------------------------------------------------
	UConst.
-----------------------------------------------------------------------------*/

//
// An UnrealScript constant.
//
class CORE_API UConst : public UField
{
	DECLARE_CLASS(UConst, UField, CLASS_NoScriptProps, Core);
	DECLARE_WITHIN(UStruct);
	NO_DEFAULT_CONSTRUCTOR(UConst);

	// Variables.
	FString Value;

	// Constructors.
	UConst( const TCHAR* InValue );

	// UObject interface.
	void Serialize( FArchive& Ar );
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
