/*=============================================================================
	UnType.h: Unreal engine base type definitions.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	UProperty.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

// Property exporting flags.
enum EPropertyPortFlags
{
	PPF_Localized	= (1 << 0),
	PPF_Delimited	= (1 << 1),
	PPF_Exec		= (1 << 2),
	PPF_ExecImport	= (1 << 3),
	PPF_PropWindow	= (1 << 4),
	PPF_RotDegree	= (1 << 5),
	PPF_CheckReferences = (1 << 6),
	PPF_RestrictImportTypes = (1 << 7),
	PPF_ParsingDefaultProperties = (1 << 8),
};

// A custom handle for recognizing pointer indexes.
struct FObjectImportHandle
{
	virtual UObject* FindByIndex(INT InIndex) = 0;
};

//
// An UnrealScript variable.
//
class CORE_API UProperty : public UField
{
	DECLARE_ABSTRACT_CLASS(UProperty, UField, CLASS_NoScriptProps, Core);
	DECLARE_WITHIN(UField);

	// Persistent variables.
	INT			ArrayDim;
	INT			ElementSize;
	QWORD		PropertyFlags;
	INT			Offset;
	_WORD		RepOffset;
	_WORD		RepIndex;
	FString		Description;
	// stijn: we temporarily store the alignment of the property here
	_WORD   LinkAlignment;

	// In memory variables.
	FName		Category;
	UEnum*		ArraySizeEnum;
	UProperty*	PropertyLinkNext;
	UProperty*	ConfigLinkNext;
	UProperty*	ConstructorLinkNext;
	UProperty*	RepOwner;
	UProperty*	NextReference;
	DWORD		TempPropertyFlags; // Not serialized compile-time property flags.

	// Compile time information.
	static UClass* _ImportObject;
	static FObjectImportHandle* _ImportHandle;
	static UObject* ImportTextParent;

	// Constructors.
	UProperty();
	UProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void StaticConstructor();

	// UProperty interface.
	virtual void Link( FArchive& Ar, UProperty* Prev );
	virtual void ExportCpp( FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm ) const;
	virtual void ExportUScriptName(FOutputDevice& Out) const {}
	virtual UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	virtual UBOOL ExportText( INT ArrayElement, FString& ValueStr, BYTE* Data, BYTE* Delta, INT PortFlags ) const;
	virtual void CopySingleValue( void* Dest, void* Src ) const;
	virtual void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	virtual void DestroyValue( void* Dest ) const;
	virtual UBOOL Port() const;
	virtual FName GetID() const;
	virtual UBOOL CleanupDestroyed(BYTE* Data) const { return 0; }
	virtual UBOOL Identical(const void* A, const void* B) const { return 0; }
	virtual const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const { return NULL; }
	virtual void ExportCppItem(FOutputDevice& Out) const {}
	virtual void SerializeItem( FArchive& Ar, void* Value ) const {}
	virtual void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const {}
	virtual DWORD GetMapHash(BYTE* Data) const;
	virtual UBOOL IsLocalized() const;
	virtual UBOOL ContainsInstancedObjectProperty() const
	{
		return 0;
	}

	// Inlines.
	UBOOL Matches( const void* A, const void* B, INT ArrayIndex ) const
	{
		guard(UProperty::Matches);
		if (!A)
			return false;
		INT Ofs = Offset + ArrayIndex * ElementSize;
		return Identical( (BYTE*)A + Ofs, B ? (BYTE*)B + Ofs : NULL );
		unguardobj;
	}
	INT GetSize() const
	{
		guardSlow(UProperty::GetSize);
		return ArrayDim * ElementSize;
		unguardobjSlow;
	}
	UBOOL ShouldSerializeValue( FArchive& Ar ) const
	{
		guardSlow(UProperty::ShouldSerializeValue);
		UBOOL Skip
		=	((PropertyFlags & CPF_Native   )                      )
		||	((PropertyFlags & CPF_Transient) && Ar.IsPersistent() );
		return !Skip;
		unguardobjSlow;
	}
	inline UBOOL HasAnyPropertyFlags(QWORD Flags) const
	{
		return (PropertyFlags & Flags) != 0;
	}
};

/*-----------------------------------------------------------------------------
	UByteProperty.
-----------------------------------------------------------------------------*/

//
// Describes an unsigned byte value or 255-value enumeration variable.
//
class CORE_API UByteProperty : public UProperty
{
	DECLARE_CLASS(UByteProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	UEnum* Enum;

	// Constructors.
	UByteProperty()
	{}
	UByteProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UEnum* InEnum=NULL )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	Enum( InEnum )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	DWORD GetMapHash(BYTE* Data) const;
};

/*-----------------------------------------------------------------------------
	UIntProperty.
-----------------------------------------------------------------------------*/

//
// Describes a 32-bit signed integer variable.
//
class CORE_API UIntProperty : public UProperty
{
	DECLARE_CLASS(UIntProperty, UProperty, CLASS_NoScriptProps, Core);

	// Constructors.
	UIntProperty()
	{}
	UIntProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	DWORD GetMapHash(BYTE* Data) const;
};

/*-----------------------------------------------------------------------------
	UBoolProperty.
-----------------------------------------------------------------------------*/

//
// Describes a single bit flag variable residing in a 32-bit unsigned double word.
//
class CORE_API UBoolProperty : public UProperty
{
	DECLARE_CLASS(UBoolProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	BITFIELD BitMask;

	// Constructors.
	UBoolProperty()
	{}
	UBoolProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	BitMask( FIRST_BITFIELD )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	DWORD GetMapHash(BYTE* Data) const;
};

/*-----------------------------------------------------------------------------
	UFloatProperty.
-----------------------------------------------------------------------------*/

//
// Describes an IEEE 32-bit floating point variable.
//
class CORE_API UFloatProperty : public UProperty
{
	DECLARE_CLASS(UFloatProperty, UProperty, CLASS_NoScriptProps, Core);

	// Constructors.
	UFloatProperty()
	{}
	UFloatProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	DWORD GetMapHash(BYTE* Data) const;
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UObjectProperty : public UProperty
{
	DECLARE_CLASS(UObjectProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	class UClass* PropertyClass;

	// Constructors.
	UObjectProperty()
	{}
	UObjectProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UClass* InClass )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	PropertyClass( InClass )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	static const TCHAR* StaticImportObject(const TCHAR* Buffer, UObject** Result, INT PortFlags, UClass* MetaClass, const TCHAR* Method);
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	void DestroyValue( void* Dest ) const;
	DWORD GetMapHash(BYTE* Data) const;
	UBOOL ContainsInstancedObjectProperty() const;
};

/*-----------------------------------------------------------------------------
	UObjectProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class CORE_API UClassProperty : public UObjectProperty
{
	DECLARE_CLASS(UClassProperty, UObjectProperty, CLASS_NoScriptProps, Core);

	// Variables.
	class UClass* MetaClass;

	// Constructors.
	UClassProperty()
	{}
	UClassProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UClass* InMetaClass )
	:	UObjectProperty( EC_CppProperty, InOffset, InCategory, InFlags, UClass::StaticClass() )
	,	MetaClass( InMetaClass )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;

	// UProperty interface.
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	FName GetID() const
	{
		return NAME_ObjectProperty;
	}
	BYTE HasObjectRef() // Class references may never be Actor reference!
	{
		return 0;
	}
};

/*-----------------------------------------------------------------------------
	UInterfaceProperty.
-----------------------------------------------------------------------------*/

/**
 * This variable type provides safe access to a native interface pointer.  The data class for this variable is FScriptInterface, and is exported to auto-generated
 * script header files as a TScriptInterface.
 */
class UInterfaceProperty : public UProperty
{
	DECLARE_CLASS(UInterfaceProperty, UProperty, CLASS_NoScriptProps, Core);

	/** The native interface class that this interface property refers to */
	class	UClass* InterfaceClass;

	/** Manipulates the data referenced by this UProperty */
	void Serialize(FArchive& Ar);
	void SerializeItem(FArchive& Ar, void* Value) const;
	void Link(FArchive& Ar, UProperty* Prev);
	void CopySingleValue(void* Dest, void* Src) const;
	const TCHAR* ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const;
};

/*-----------------------------------------------------------------------------
	UNameProperty.
-----------------------------------------------------------------------------*/

//
// Describes a name variable pointing into the global name table.
//
class CORE_API UNameProperty : public UProperty
{
	DECLARE_CLASS(UNameProperty, UProperty, CLASS_NoScriptProps, Core);

	// Constructors.
	UNameProperty()
	{}
	UNameProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	DWORD GetMapHash(BYTE* Data) const;
};

/*-----------------------------------------------------------------------------
	UStrProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic string variable.
//
class CORE_API UStrProperty : public UProperty
{
	DECLARE_CLASS(UStrProperty, UProperty, CLASS_NoScriptProps, Core);

	// Constructors.
	UStrProperty()
	{}
	UStrProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;
	DWORD GetMapHash(BYTE* Data) const;

    // Ini safe string writing/reading
    static void SafeWriteString( FString& Dest, const TCHAR* Src );
    static void SafeReadString( FString& Dest, const TCHAR*& Src );
};

/*-----------------------------------------------------------------------------
	UFixedArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a fixed length array.
//
class CORE_API UFixedArrayProperty : public UProperty
{
	DECLARE_CLASS(UFixedArrayProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	UProperty* Inner;
	INT Count;

	// Constructors.
	UFixedArrayProperty()
	{}
	UFixedArrayProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void DestroyValue( void* Dest ) const;

	// UFixedArrayProperty interface.
	void AddCppProperty( UProperty* Property, INT Count );
};

/*-----------------------------------------------------------------------------
	UArrayProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic array.
//
class CORE_API UArrayProperty : public UProperty
{
	DECLARE_CLASS(UArrayProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	UProperty* Inner;

	// Constructors.
	UArrayProperty()
	{}
	UArrayProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportCpp( FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm  ) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	void DestroyValue( void* Dest ) const;
	UBOOL IsLocalized() const;
	UBOOL ContainsInstancedObjectProperty() const;

	// UArrayProperty interface.
	void AddCppProperty( UProperty* Property );
};

/*-----------------------------------------------------------------------------
	UMapProperty.
-----------------------------------------------------------------------------*/

//
// Describes a dynamic map.
//
class CORE_API UMapProperty : public UProperty
{
	DECLARE_CLASS(UMapProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	UProperty* Key;
	UProperty* Value;
	BYTE CleanupDestroyedMask;

	// Constructors.
	UMapProperty()
	{}
	UMapProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const;
	void DestroyValue( void* Dest ) const;
};

/*-----------------------------------------------------------------------------
	UStructProperty.
-----------------------------------------------------------------------------*/

//
// Describes a structure variable embedded in (as opposed to referenced by)
// an object.
//
class CORE_API UStructProperty : public UProperty
{
	DECLARE_CLASS(UStructProperty, UProperty, CLASS_NoScriptProps, Core);

	// Variables.
	class UScriptStruct* Struct;
	UStructProperty* NextStruct;

	// Constructors.
	UStructProperty()
	{}
	UStructProperty( ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags, UScriptStruct* InStruct )
	:	UProperty( EC_CppProperty, InOffset, InCategory, InFlags )
	,	Struct( InStruct )
	{}

	// UObject interface.
	void Serialize( FArchive& Ar );

	INT GetLinkAlignment();

	// UProperty interface.
	void Link( FArchive& Ar, UProperty* Prev );
	UBOOL Identical( const void* A, const void* B ) const;
	void SerializeItem( FArchive& Ar, void* Value ) const;
	UBOOL NetSerializeItem( FArchive& Ar, UPackageMap* Map, void* Data ) const;
	void ExportCppItem( FOutputDevice& Out ) const;
	void ExportUScriptName(FOutputDevice& Out) const;
	void ExportTextItem( FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags ) const;
	const TCHAR* ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const;
	void CopySingleValue( void* Dest, void* Src ) const;
	void CopyCompleteValue( void* Dest, void* Src, UObject* Obj=NULL ) const;
	void DestroyValue( void* Dest ) const;
	DWORD GetMapHash(BYTE* Data) const;
	UBOOL IsLocalized() const;
	UBOOL ContainsInstancedObjectProperty() const;
	void InitializeValue(BYTE* Dest) const;
};

/*-----------------------------------------------------------------------------
	UComponentProperty.
-----------------------------------------------------------------------------*/

//
// Describes a reference variable to another object which may be nil.
//
class UComponentProperty : public UObjectProperty
{
	DECLARE_CLASS(UComponentProperty, UObjectProperty, CLASS_NoScriptProps, Core);

	UComponentProperty() {}

	// UProperty interface.
	FName GetID() const
	{
		return NAME_ObjectProperty;
	}
};

/*-----------------------------------------------------------------------------
	UDelegateProperty.
-----------------------------------------------------------------------------*/

/**
 * Describes a pointer to a function bound to an Object.
 */
class UDelegateProperty : public UProperty
{
	DECLARE_CLASS(UDelegateProperty, UProperty, CLASS_NoScriptProps, Core);

	/** Function this delegate is mapped to */
	UFunction* Function;

	/**
	 * If this DelegateProperty corresponds to an actual delegate variable (as opposed to the hidden property the script compiler creates when you declare a delegate function)
	 * points to the source delegate function (the function declared with the delegate keyword) used in the declaration of this delegate property.
	 */
	UFunction* SourceDelegate;

	// Constructors.
	UDelegateProperty()
	{}
	UDelegateProperty(ECppProperty, INT InOffset, const TCHAR* InCategory, QWORD InFlags)
		: UProperty(EC_CppProperty, InOffset, InCategory, InFlags)
	{}

	// UObject interface.
	void Link(FArchive& Ar, UProperty* Prev);
	void SerializeItem(FArchive& Ar, void* Value) const;
	void Serialize(FArchive& Ar);
	void CopySingleValue(void* Dest, void* Src) const;
	void CopyCompleteValue(void* Dest, void* Src, UObject* Obj = NULL) const;
	const TCHAR* ImportText(const TCHAR* InBuffer, BYTE* Data, INT PortFlags) const;
};

/*-----------------------------------------------------------------------------
	Field templates.
-----------------------------------------------------------------------------*/

//
// Find a typed field in a struct.
//
template <class T> T* FindField( UStruct* Owner, const TCHAR* FieldName )
{
	guard(FindField);
	for( TFieldIterator<T>It( Owner ); It; ++It )
		if( appStricmp( *It->GetFieldName(), FieldName )==0 )
			return *It;
	return NULL;
	unguard;
}

template <class T> T* FindOuter(UObject* Obj,BYTE bOuterMost=0)
{
	guard(FindOuter);
	if (bOuterMost)
	{
		T* Best = NULL;
		while (Obj)
		{
			if (Obj->IsA(T::StaticClass()))
				Best = (T*)Obj;
			Obj = Obj->GetOuter();
		}
		return Best;
	}
	else
	{
		while (Obj)
		{
			if (Obj->IsA(T::StaticClass()))
				return (T*)Obj;
			Obj = Obj->GetOuter();
		}
	}
	return NULL;
	unguard;
}

/*-----------------------------------------------------------------------------
	UObject accessors that depend on UClass.
-----------------------------------------------------------------------------*/

//
// See if this object belongs to the specified class.
//
inline UBOOL UObject::IsA( class UClass* SomeBase ) const
{
	guardSlow(UObject::IsA);
	for (UClass* TempClass = Class; TempClass; TempClass = (UClass*)TempClass->SuperStruct)
		if (TempClass == SomeBase)
			return 1;
	return SomeBase==NULL;
	unguardobjSlow;
}

//
// See if this object is in a certain package.
//
inline UBOOL UObject::IsIn( class UObject* SomeOuter ) const
{
	guardSlow(UObject::IsIn);
	for( UObject* It=GetOuter(); It; It=It->GetOuter() )
		if( It==SomeOuter )
			return 1;
	return SomeOuter==NULL;
	unguardobjSlow;
}

/*-----------------------------------------------------------------------------
	UStruct inlines.
-----------------------------------------------------------------------------*/

//
// UStruct inline comparer.
//
inline UBOOL UStruct::StructCompare( const void* A, const void* B )
{
	guardSlow(UStruct::StructCompare);
	for( TFieldIterator<UProperty> It(this); It; ++It )
		for( INT i=0; i<It->ArrayDim; i++ )
			if( !It->Matches(A,B,i) )
				return 0;
	unguardobjSlow;
	return 1;
}

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*-----------------------------------------------------------------------------
	C++ property macros.
-----------------------------------------------------------------------------*/

// The below, using offset 0 (NULL), makes gcc think we're misusing
//  the offsetof macro...we get around this by using 1 instead.  :) --ryan.
/*
#define CPP_PROPERTY(name) \
	EC_CppProperty, (BYTE*)&((ThisClass*)NULL)->name - (BYTE*)NULL
*/

#define CPP_PROPERTY(name) \
	EC_CppProperty, (BYTE*)&((ThisClass*)1)->name - (BYTE*)1

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
