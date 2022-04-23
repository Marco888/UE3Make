/*=============================================================================
UnProp.cpp: FProperty implementation
Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

//!!fix hardcoded lengths
/*-----------------------------------------------------------------------------
Helpers.
-----------------------------------------------------------------------------*/

//
// Parse a hex digit.
//
static INT HexDigit(TCHAR c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return c + 10 - 'a';
	else if (c >= 'A' && c <= 'F')
		return c + 10 - 'A';
	else
		return 0;
}

//
// Parse a token.
//
inline BYTE isNameChar(TCHAR c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '!' || c == '#' || c == '$' || c == '-' || c == '+' || c == '~' || c == '_';
}
static const TCHAR* ReadToken(const TCHAR* Context, const TCHAR* Buffer, FString& String, UBOOL DottedNames = 0)
{
	if (*Buffer == '\"' || *Buffer == '\'')
	{
		TCHAR QuoteChar = *Buffer;

		// Get quoted string.
		Buffer++;
		while (*Buffer && *Buffer != QuoteChar && *Buffer != 13 && *Buffer != 10)
		{
			if (*Buffer != '\\')
			{
				String = FString::Printf(TEXT("%ls%c"), *String, *Buffer++);
			}
			else if (*++Buffer == '\\')
			{
				String += TEXT("\\");
				Buffer++;
			}
			else
			{
				String = FString::Printf(TEXT("%ls%c"), *String, HexDigit(Buffer[0]) * 16 + HexDigit(Buffer[1]));
				Buffer += 2;
			}
		}
		if (*Buffer++ != QuoteChar)
		{
			GWarn->Logf(NAME_ExecWarning, TEXT("%ls [ReadToken]: Bad quoted string"), Context);
			return NULL;
		}
	}
	else if (isNameChar(*Buffer))
	{
		const TCHAR* Start = Buffer;

		// Get identifier.
		while (isNameChar(*Buffer) || (DottedNames && *Buffer == '.'))
			++Buffer;

		String += FString(Start, Buffer);
	}
	else
	{
		// Get just one.
		String = FString::Printf(TEXT("%ls%c"), *String, *Buffer);
	}
	return Buffer;
}
static const TCHAR* ReadNameToken(const TCHAR* Context, const TCHAR* Buffer, FString& String, UBOOL bTripSpaces)
{
	if (*Buffer == '\"' || *Buffer == '\'')
	{
		TCHAR QuoteChar = *Buffer;

		// Get quoted string.
		Buffer++;
		while (*Buffer && *Buffer != QuoteChar && *Buffer != 13 && *Buffer != 10)
		{
			if (*Buffer != '\\')
			{
				String = FString::Printf(TEXT("%ls%c"), *String, *Buffer++);
			}
			else if (*++Buffer == '\\')
			{
				String += TEXT("\\");
				Buffer++;
			}
			else
			{
				String = FString::Printf(TEXT("%ls%c"), *String, HexDigit(Buffer[0]) * 16 + HexDigit(Buffer[1]));
				Buffer += 2;
			}
		}
		if (*Buffer++ != QuoteChar)
		{
			GWarn->Logf(NAME_ExecWarning, TEXT("%ls [ReadNameToken]: Bad quoted string"), Context);
			return NULL;
		}
	}
	else if (isNameChar(*Buffer))
	{
		const TCHAR* Start = Buffer;
		const TCHAR* Fin = Buffer;

		// Get identifier.
		while (isNameChar(*Buffer) || *Buffer == '.' || (!bTripSpaces && *Buffer == ' '))
		{
			if (*Buffer != ' ')
				Fin = Buffer;
			++Buffer;
		}

		Buffer = Fin+1;
		String = FString(Start, Buffer);
	}
	else
	{
		String = TEXT("None");
	}
	return Buffer;
}

/*-----------------------------------------------------------------------------
UProperty implementation.
-----------------------------------------------------------------------------*/

UClass* UProperty::_ImportObject = NULL;
FObjectImportHandle* UProperty::_ImportHandle = NULL;
UObject* UProperty::ImportTextParent = NULL;

//
// Constructors.
//
UProperty::UProperty()
	: ArrayDim(1)
	, TempPropertyFlags(0)
	, ArraySizeEnum(NULL)
{}
UProperty::UProperty(ECppProperty, INT InOffset, const TCHAR* InCategory, DWORD InFlags)
	: ArrayDim(1)
	, PropertyFlags(InFlags)
    , Offset(InOffset)
	, Category(InCategory)
	, TempPropertyFlags(0)
	, ArraySizeEnum(NULL)
{
	GetOuterUField()->AddCppProperty(this);
}

void UProperty::StaticConstructor()
{
	guard(UProperty::StaticConstructor);
	unguard;
}

//
// Serializer.
//
void UProperty::Serialize(FArchive& Ar)
{
	guard(UProperty::Serialize);
	Super::Serialize(Ar);

	// Archive the basic info.
	Ar << ArrayDim << PropertyFlags;
	Ar << Category << ArraySizeEnum;

	if (PropertyFlags & CPF_Net)
	{
		Ar << RepOffset;
	}

	if (Ar.IsLoading())
	{
		Offset = 0;
		ConstructorLinkNext = NULL;
	}
	unguardobj;
}

//
// Export this class property to an output
// device as a C++ header file.
//
void UProperty::ExportCpp(FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm) const
{
	guard(UProperty::ExportCpp)
		TCHAR ArrayStr[256] = TEXT("");
	if
		(IsParm
		&&	IsA(UStrProperty::StaticClass())
		&& !(PropertyFlags & CPF_OutParm))
		Out.Log(TEXT("const "));
	ExportCppItem(Out);
	if (ArrayDim != 1)
		appSprintf(ArrayStr, TEXT("[%i]"), ArrayDim);
	if (IsA(UBoolProperty::StaticClass()))
	{
		if (ArrayDim == 1 && !IsLocal && !IsParm)
			Out.Logf(TEXT(" %ls%ls:1"), GetName(), ArrayStr);
		else if (IsParm && (PropertyFlags & CPF_OutParm))
			Out.Logf(TEXT("& %ls%ls"), GetName(), ArrayStr);
		else
			Out.Logf(TEXT(" %ls%ls"), GetName(), ArrayStr);
	}
	else if (IsA(UStrProperty::StaticClass()))
	{
		if (IsParm && ArrayDim>1)
			Out.Logf(TEXT("* %ls"), GetName());
		else if (IsParm)
			Out.Logf(TEXT("& %ls"), GetName());
		else if (IsLocal)
			Out.Logf(TEXT(" %ls"), GetName());
		else
			Out.Logf(TEXT("NoInit %ls%ls"), GetName(), ArrayStr);
	}
	else
	{
		if (IsParm && ArrayDim>1)
			Out.Logf(TEXT("* %ls"), GetName());
		else if (IsParm && (PropertyFlags & CPF_OutParm))
			Out.Logf(TEXT("& %ls%ls"), GetName(), ArrayStr);
		else
			Out.Logf(TEXT(" %ls%ls"), GetName(), ArrayStr);
	}
	unguardobj;
}

//
// Export the contents of a property.
//
UBOOL UProperty::ExportText
(
INT		Index,
FString& ValueStr,
BYTE*	Data,
BYTE*	Delta,
INT		PortFlags
) const
{
	ValueStr.Empty();
	guard(UProperty::ExportText);
	if (Data == Delta || !Matches(Data, Delta, Index))
	{
		ExportTextItem
			(
			ValueStr,
			Data + Offset + Index * ElementSize,
			Delta ? (Delta + Offset + Index * ElementSize) : NULL,
			PortFlags
			);
		return 1;
	}
	else return 0;
	unguardobj;
}

//
// Copy a unique instance of a value.
//
void UProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UProperty::CopySingleValue);
	appMemcpy(Dest, Src, ElementSize);
	unguardobjSlow;
}

//
// Destroy a value.
//
void UProperty::DestroyValue(void* Dest) const
{}

//
// Net serialization.
//
UBOOL UProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UProperty::NetSerializeItem);
#if UNICODE
	//	 GIsOnStrHack = 1;
#endif
	SerializeItem(Ar, Data);
#if UNICODE
	//	 GIsOnStrHack = 0;
#endif
	return 1;
	unguardobjSlow;
}

//
// Return whether the property should be exported.
//
UBOOL UProperty::Port() const
{
	return
		(GetSize()
		&& (Category != NAME_None || !(PropertyFlags & (CPF_Transient | CPF_Native)))
		&& GetFName() != NAME_Class);
}

//
// Return type id for encoding properties in .u files.
//
FName UProperty::GetID() const
{
	return GetClass()->GetFName();
}

//
// Copy a complete value.
//
void UProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UProperty::CopyCompleteValue);
	for (INT i = 0; i<ArrayDim; i++)
		CopySingleValue((BYTE*)Dest + i*ElementSize, (BYTE*)Src + i*ElementSize);
	unguardobjSlow;
}

//
// Link property loaded from file.
//
void UProperty::Link(FArchive& Ar, UProperty* Prev)
{}

// Hashing for Map property!
DWORD UProperty::GetMapHash(BYTE* Data) const
{
	return 0;
}

UBOOL UProperty::IsLocalized() const
{
	return (PropertyFlags & CPF_Localized) != 0;
}
UBOOL UArrayProperty::IsLocalized() const
{
	if (Inner->IsLocalized())
		return TRUE;
	return Super::IsLocalized();
}
UBOOL UStructProperty::IsLocalized() const
{
	// prevent recursion in the case of structs containing dynamic arrays of themselves
	static TArray<const UStructProperty*> EncounteredStructProps;
	if (EncounteredStructProps.FindItemIndex(this)>=0)
	{
		return Super::IsLocalized();
	}
	else
	{
		EncounteredStructProps.AddItem(this);
		for (TFieldIterator<UProperty> It(Struct); It; ++It)
		{
			if (It->IsLocalized())
			{
				EncounteredStructProps.RemoveItem(this);
				return TRUE;
			}
		}
		EncounteredStructProps.RemoveItem(this);
		return Super::IsLocalized();
	}
}
UBOOL UObjectProperty::ContainsInstancedObjectProperty() const
{
	return (PropertyFlags & CPF_NeedCtorLink) != 0;
}
UBOOL UArrayProperty::ContainsInstancedObjectProperty() const
{
	check(Inner);
	return Inner->ContainsInstancedObjectProperty();
}
UBOOL UStructProperty::ContainsInstancedObjectProperty() const
{
	check(Struct);
	for(TFieldIterator<UProperty> It(Struct); It; ++It)
		if (It->ContainsInstancedObjectProperty())
			return TRUE;
	return FALSE;
}
IMPLEMENT_CLASS(UProperty);

/*-----------------------------------------------------------------------------
UByteProperty.
-----------------------------------------------------------------------------*/

void UByteProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UByteProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize   = sizeof(BYTE);
	LinkAlignment = alignof(BYTE);
	Offset        = Align( GetOuterUField()->GetPropertiesSize(), LinkAlignment );
	unguardobj;
}
void UByteProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UByteProperty::CopySingleValue);
	*(BYTE*)Dest = *(BYTE*)Src;
	unguardSlow;
}
void UByteProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UByteProperty::CopyCompleteValue);
	if (ArrayDim == 1)
		*(BYTE*)Dest = *(BYTE*)Src;
	else
		appMemcpy(Dest, Src, ArrayDim);
	unguardSlow;
}
UBOOL UByteProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UByteProperty::Identical);
	return *(BYTE*)A == (B ? *(BYTE*)B : 0);
	unguardobjSlow;
}
void UByteProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UByteProperty::SerializeItem);
	// Serialize enum values by name unless we're not saving or loading OR for backwards compatibility
	const UBOOL bUseBinarySerialization = (Enum == NULL) || (!Ar.IsLoading() && !Ar.IsSaving());
	if (bUseBinarySerialization)
	{
		Ar << *(BYTE*)Value;
	}
	// Loading
	else if (Ar.IsLoading())
	{
		FName EnumValueName;
		Ar << EnumValueName;
		// Make sure enum is properly populated
		if (Enum->HasAnyFlags(RF_NeedLoad))
			Ar.Preload(Enum);

		// There's no guarantee EnumValueName is still present in Enum, in which case Value will be set to the enum's max value.
		// On save, it will then be serialized as NAME_None.
		*(BYTE*)Value = Enum->Names.FindItemIndex(EnumValueName);
		if (Enum->Names.Num() <= *(BYTE*)Value)
		{
			*(BYTE*)Value = Enum->Names.Num() - 1;
		}
	}
	// Saving
	else
	{
		FName EnumValueName;
		BYTE ByteValue = *(BYTE*)Value;

		// subtract 1 because the last entry in the enum's Names array
		// is the _MAX entry
		if (ByteValue < Enum->Names.Num())
		{
			EnumValueName = Enum->Names(ByteValue);
		}
		else
		{
			EnumValueName = NAME_None;
		}
		Ar << EnumValueName;
	}
	unguardobjSlow;
}
UBOOL UByteProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	Ar.SerializeBits(Data, Enum ? appCeilLogTwo(Enum->Names.Num()) : 8);
	return 1;
	unguardobjSlow;
}
void UByteProperty::Serialize(FArchive& Ar)
{
	guard(UByteProperty::Serialize);
	Super::Serialize(Ar);
	Ar << Enum;
	unguardobj;
}
void UByteProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UByteProperty::ExportCppItem);
	Out.Log(TEXT("BYTE"));
	unguardobj;
}
void UByteProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UByteProperty::ExportUScriptName);
	if (Enum)
		Out.Log(Enum->GetName());
	else Out.Log(TEXT("byte"));
	unguardobj;
}
void UByteProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UByteProperty::ExportTextItem);
	if (Enum)
		ValueStr += (Enum->Names.IsValidIndex(*PropertyValue) ? *Enum->Names(*PropertyValue) : TEXT("<Invalid>"));
	else
		ValueStr += FString::Printf(TEXT("%i"), *PropertyValue);
	unguardobj;
}
const TCHAR* UByteProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UByteProperty::ImportText);
	if (Enum)
	{
		if (appIsDigit(*Buffer))
		{
			INT Res = appAtoi(Buffer);
			if (Res >= 0 && Res<Enum->Names.Num())
			{
				*(BYTE*)Data = Res;
				while (appIsDigit(*Buffer))
					Buffer++;
				return Buffer;
			}
		}
		FString Temp;
		Buffer = ReadToken(GetFullName(), Buffer, Temp);
		if (!Buffer)
			return NULL;
		FName EnumName = FName(*Temp, FNAME_Find);
		if (EnumName != NAME_None)
		{
			INT EnumIndex = 0;
			if (Enum->Names.FindItem(EnumName, EnumIndex))
			{
				*(BYTE*)Data = EnumIndex;
				return Buffer;
			}
		}
		return NULL;
	}
	if ((PortFlags & PPF_ExecImport) && _ImportObject)
	{
		INT ResultValue;
		if (!_ImportObject->GetDefaultObject()->ParsePropertyValue(&ResultValue, &Buffer, 1))
			return NULL;
		*(BYTE*)Data = (BYTE)ResultValue;
		return Buffer;
	}

	//Buffer = *Temp;
	if (appIsDigit(*Buffer))
	{
		*(BYTE*)Data = appAtoi(Buffer);
		while (appIsDigit(*Buffer))
			Buffer++;
	}
	else
	{
		//debugf( "Import: Missing byte" );
		return NULL;
	}
	return Buffer;
	unguardobj;
}

// Hashing for Map property!
DWORD UByteProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(*Data);
}

IMPLEMENT_CLASS(UByteProperty);

/**
 * Find the longest common prefix of all items in the enumeration.
 *
 * @return	the longest common prefix between all items in the enum.  If a common prefix
 *			cannot be found, returns the full name of the enum.
 */
FString UEnum::GenerateEnumPrefix() const
{
	FString Prefix;
	if (Names.Num() > 0)
	{
		Prefix = *Names(0);

		// For each item in the enumeration, trim the prefix as much as necessary to keep it a prefix.
		// This ensures that once all items have been processed, a common prefix will have been constructed.
		// This will be the longest common prefix since as little as possible is trimmed at each step.
		for (INT NameIdx = 1; NameIdx < Names.Num(); NameIdx++)
		{
			FString EnumItemName = *Names(NameIdx);

			// Find the length of the longest common prefix of Prefix and EnumItemName.
			INT PrefixIdx = 0;
			while (PrefixIdx < Prefix.Len() && PrefixIdx < EnumItemName.Len() && Prefix[PrefixIdx] == EnumItemName[PrefixIdx])
			{
				PrefixIdx++;
			}

			// Trim the prefix to the length of the common prefix.
			Prefix = Prefix.Left(PrefixIdx);
		}

		// Find the index of the rightmost underscore in the prefix.
		INT UnderscoreIdx = Prefix.InStr(TEXT("_"), TRUE);

		// If an underscore was found, trim the prefix so only the part before the rightmost underscore is included.
		if (UnderscoreIdx > 0)
		{
			Prefix = Prefix.Left(UnderscoreIdx);
		}
		else
		{
			// no underscores in the common prefix - this probably indicates that the names
			// for this enum are not using Epic's notation, so just empty the prefix so that
			// the max item will use the full name of the enum
			Prefix.Empty();
		}
	}

	// If no common prefix was found, or if the enum does not contain any entries,
	// use the name of the enumeration instead.
	if (Prefix.Len() == 0)
	{
		Prefix = GetName();
	}
	return Prefix;
}

/**
 * Sets the array of enums.
 *
 * @return	TRUE unless the MAX enum already exists and isn't the last enum.
 */
UBOOL UEnum::SetEnums(TArray<FName>& InNames)
{
	Names.Empty();
	Names = InNames;
	return GenerateMaxEnum();
}

/**
 * Adds a virtual _MAX entry to the enum's list of names, unless the
 * enum already contains one.
 *
 * @return	TRUE unless the MAX enum already exists and isn't the last enum.
 */
UBOOL UEnum::GenerateMaxEnum()
{
	FString EnumPrefix = GenerateEnumPrefix();
	checkSlow(EnumPrefix.Len());
	EnumPrefix += TEXT("_MAX");

	const FName MaxEnumItem(*EnumPrefix);
	const INT MaxEnumItemIndex = Names.FindItemIndex(MaxEnumItem);
	if (MaxEnumItemIndex == INDEX_NONE)
	{
		Names.AddItem(MaxEnumItem);
	}

	if (MaxEnumItemIndex != INDEX_NONE && MaxEnumItemIndex != Names.Num() - 1)
	{
		// The MAX enum already exists, but isn't the last enum.
		return FALSE;
	}

	return TRUE;
}

/*-----------------------------------------------------------------------------
UIntProperty.
-----------------------------------------------------------------------------*/

void UIntProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UIntProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize   = sizeof(INT);

	// stijn: (hopefully) temporary hack to deal with Engine.Bitmap.InternalTime.
	// This property is kind of unique in UEngine because it is mirrored as an
	// INT array with 2 elements in UScript. In C++ it is an FTime.
	// INT arrays and FTime structs do not always have the same alignment requirements
	// On ARM, for example, FTime will _ALWAYS_ be aligned on an 8-byte boundary
	// even when embedded in structs with 4-byte packing.
#if __LINUX_ARM__ || BUILD_64
	if (!appStrcmp(GetPathName(), TEXT("Engine.Bitmap.InternalTime")))
		LinkAlignment = alignof(FTime);
	else
#endif
	LinkAlignment = alignof(INT);
	Offset        = Align( GetOuterUField()->GetPropertiesSize(), LinkAlignment );
	unguardobj;
}
void UIntProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UIntProperty::CopySingleValue);
	*(INT*)Dest = *(INT*)Src;
	unguardSlow;
	//if (GetName() == TEXT("AnimFrame") || GetName() == TEXT("AnimRate"))
	//debugf(TEXT("Name CopySingleValue: %ls"), GetOuter()->GetName());
	//debugf(TEXT("Name CopySingleValue: %ls"), GetName());
}
void UIntProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UIntProperty::CopyCompleteValue);
	if (ArrayDim == 1)
		*(INT*)Dest = *(INT*)Src;
	else appMemcpy(Dest, Src, sizeof(INT)*ArrayDim);
	unguardSlow;
}
UBOOL UIntProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UIntProperty::Identical);
	return *(INT*)A == (B ? *(INT*)B : 0);
	unguardobjSlow;
}
void UIntProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UIntProperty::SerializeItem);
	Ar << *(INT*)Value;
	unguardobjSlow;
}
UBOOL UIntProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UIntProperty::NetSerializeItem);
	if (Ar.Ver() == 0) // oldver
		Ar << *(INT*)Data;
	else SerializeCompressedInt(*(INT*)Data, Ar);
	return 1;
	unguardSlow;
}
void UIntProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UIntProperty::ExportCppItem);
	Out.Log(TEXT("INT"));
	unguardobj;
}
void UIntProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UIntProperty::ExportUScriptName);
	Out.Log(TEXT("int"));
	unguardobj;
}
void UIntProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UIntProperty::ExportTextItem);
	if (PortFlags & PPF_RotDegree)
	{
		if (PortFlags & PPF_Delimited)
			ValueStr += FString::Printf(TEXT("%.2f\xB0"), (*(INT*)PropertyValue * (360.0 / 65536.0)));
		else ValueStr += FString::Printf(TEXT("%.2f"), (*(INT*)PropertyValue * (360.0 / 65536.0)));
	}
	else ValueStr += FString::Printf(TEXT("%i"), *(INT *)PropertyValue);
	unguardobj;
}
//
// Fixes:
//  * '-' behind number led to addtional characters ignored by appAtoi to be eaten up.
//
// Additions:
//  * Leading '+' is now supported.
//
// Issues:
//  * Should leading tabs and spaces be accepted like appAtoi does?
//
const TCHAR* UIntProperty::ImportText( const TCHAR* Buffer, BYTE* Data, INT PortFlags ) const
{
	guard(UIntProperty::ImportText);
	if ((PortFlags & PPF_ExecImport) && _ImportObject)
	{
		if (!_ImportObject->GetDefaultObject()->ParsePropertyValue(Data, &Buffer, 1))
			return NULL;
		return Buffer;
	}
	if (*Buffer == '+' || *Buffer == '-' || appIsDigit(*Buffer))
		*(INT*)Data = appAtoi(Buffer);
	else return NULL;

	if (*Buffer == '+' || *Buffer == '-')
		Buffer++;
	while (appIsDigit(*Buffer))
		Buffer++;
	return Buffer;
	unguardobj;
}

// Hashing for Map property!
DWORD UIntProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(*((INT*)Data));
}

IMPLEMENT_CLASS(UIntProperty);

/*-----------------------------------------------------------------------------
UBoolProperty.
-----------------------------------------------------------------------------*/

void UBoolProperty::Link(FArchive& Ar, UProperty* Prev)
{
	guard(UBoolProperty::Link);
	Super::Link(Ar, Prev);
	UBoolProperty* PrevBool = Cast<UBoolProperty>(Prev);
	if (GetOuterUField()->MergeBools() && PrevBool && NEXT_BITFIELD(PrevBool->BitMask))
	{
		Offset = Prev->Offset;
		BitMask = NEXT_BITFIELD(PrevBool->BitMask);
	}
	else
	{
		Offset = Align(GetOuterUField()->GetPropertiesSize(), sizeof(BITFIELD));
		BitMask = FIRST_BITFIELD;
	}
	LinkAlignment = alignof(BITFIELD);
	ElementSize = sizeof(BITFIELD);
	unguardobj;
}
void UBoolProperty::Serialize(FArchive& Ar)
{
	guard(UBoolProperty::Serialize);
	Super::Serialize(Ar);
	if (!Ar.IsLoading() && !Ar.IsSaving())
		Ar << BitMask;
	unguardobj;
}
void UBoolProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UBoolProperty::ExportCppItem);
	Out.Log(TEXT("BITFIELD"));
	unguardobj;
}
void UBoolProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UBoolProperty::ExportUScriptName);
	Out.Log(TEXT("bool"));
	unguardobj;
}
void UBoolProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UBoolProperty::ExportTextItem);
	TCHAR* Temp
		= (TCHAR*)((PortFlags & PPF_Localized)
		? (((*(BITFIELD*)PropertyValue) & BitMask) ? GTrue : GFalse)
		: (((*(BITFIELD*)PropertyValue) & BitMask) ? TEXT("True") : TEXT("False")));
	ValueStr += FString::Printf(TEXT("%ls"), Temp);
	unguardobj;
}
const TCHAR* UBoolProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UBoolProperty::ImportText);
	FString Temp;
	Buffer = ReadToken(GetFullName(), Buffer, Temp);
	if (!Buffer)
		return NULL;
	if (Temp == TEXT("1") || Temp == TEXT("True") || Temp == GTrue)
	{
		*(BITFIELD*)Data |= BitMask;
	}
	else if (Temp == TEXT("0") || Temp == TEXT("False") || Temp == GFalse)
	{
		*(BITFIELD*)Data &= ~BitMask;
	}
	else
	{
		//debugf( "Import: Failed to get bool" );
		return NULL;
	}
	return Buffer;
	unguardobj;
}
UBOOL UBoolProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UBoolProperty::Identical);
	return ((*(BITFIELD*)A ^ (B ? *(BITFIELD*)B : 0)) & BitMask) == 0;
	unguardobjSlow;
}
void UBoolProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UBoolProperty::SerializeItem);
	BYTE B = (*(BITFIELD*)Value & BitMask) ? 1 : 0;
	Ar << B;
	if (B) *(BITFIELD*)Value |= BitMask;
	else    *(BITFIELD*)Value &= ~BitMask;
	unguardobjSlow;
}
UBOOL UBoolProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	BYTE Value = ((*(BITFIELD*)Data & BitMask) != 0);
	Ar.SerializeBits(&Value, 1);
	if (Value)
		*(BITFIELD*)Data |= BitMask;
	else
		*(BITFIELD*)Data &= ~BitMask;
	return 1;
	unguardobjSlow;
}
void UBoolProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UProperty::CopySingleValue);
	*(BITFIELD*)Dest = (*(BITFIELD*)Dest & ~BitMask) | (*(BITFIELD*)Src & BitMask);
	unguardobjSlow;
}

// Hashing for Map property!
DWORD UBoolProperty::GetMapHash(BYTE* Data) const
{
	return ((*(BITFIELD*)Data & BitMask) != 0) ? 1 : 0;
}

IMPLEMENT_CLASS(UBoolProperty);

/*-----------------------------------------------------------------------------
UFloatProperty.
-----------------------------------------------------------------------------*/

void UFloatProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UFloatProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize   = sizeof(FLOAT);
	LinkAlignment = alignof(FLOAT);
	Offset        = Align( GetOuterUField()->GetPropertiesSize(), LinkAlignment );
	unguardobj;
}
void UFloatProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UFloatProperty::CopySingleValue);
	*(FLOAT*)Dest = *(FLOAT*)Src;
	unguardSlow;
	//if (GetName() == TEXT("AnimFrame") || GetName() == TEXT("AnimRate"))
	//debugf(TEXT("Name CopySingleValue: %ls"), GetOuter()->GetName());
	//debugf(TEXT("Name CopySingleValue: %ls"), GetName());
}
void UFloatProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UFloatProperty::CopyCompleteValue);
	if (ArrayDim == 1)
		*(FLOAT*)Dest = *(FLOAT*)Src;
	else appMemcpy(Dest, Src, sizeof(FLOAT)*ArrayDim);
	unguardSlow;
}
UBOOL UFloatProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UFloatProperty::Identical);
	return *(FLOAT*)A == (B ? *(FLOAT*)B : 0);
	unguardobjSlow;
}
void UFloatProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UFloatProperty::SerializeItem);
	Ar << *(FLOAT*)Value;
	unguardobjSlow;
}
UBOOL UFloatProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UFloatProperty::NetSerializeItem);
	Ar << *(FLOAT*)Data;
	return 1;
	unguardSlow;
}
void UFloatProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UFloatProperty::ExportCppItem);
	Out.Log(TEXT("FLOAT"));
	unguardobj;
}
void UFloatProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UFloatProperty::ExportUScriptName);
	Out.Log(TEXT("float"));
	unguardobj;
}
void UFloatProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UFloatProperty::ExportTextItem);
	if (PortFlags & PPF_Delimited)
	{
		FString Result = FString::Printf(TEXT("%g"), *(FLOAT*)PropertyValue);
		if (!appStrstr(*Result, TEXT("."))) // Make sure we show that this is infact a float value.
			Result += TEXT(".0");
		ValueStr += Result;
	}
	else ValueStr += FString::Printf(TEXT("%f"), *(FLOAT*)PropertyValue);
	unguardobj;
}
const TCHAR* UFloatProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UFloatProperty::ImportText);
	if ((PortFlags & PPF_ExecImport) && _ImportObject)
	{
		if (!_ImportObject->GetDefaultObject()->ParsePropertyValue(Data, &Buffer, 0))
			return NULL;
		return Buffer;
	}
	if (*Buffer == '+' || *Buffer == '-' || *Buffer == '.' || appIsDigit(*Buffer))
		*(FLOAT*)Data = appAtof(Buffer);
	else return NULL;

	if (*Buffer == '+' || *Buffer == '-')
		++Buffer;
	while (appIsDigit(*Buffer))
		++Buffer;
	if (*Buffer == '.')
	{
		++Buffer;
		while (1)
		{
			if (appIsDigit(*Buffer))
				++Buffer;
			else if (*Buffer == 'e' && (Buffer[1] == '+' || Buffer[1] == '-'))
				Buffer += 2;
			else break;
		}
		if (*Buffer == 'f')
			++Buffer;
	}
	return Buffer;
	unguardobj;
}

// Hashing for Map property!
DWORD UFloatProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(appFloor(*((FLOAT*)Data)));
}

IMPLEMENT_CLASS(UFloatProperty);

/*-----------------------------------------------------------------------------
UObjectProperty.
-----------------------------------------------------------------------------*/

void UObjectProperty::Link(FArchive& Ar, UProperty* Prev)
{
	guard(UObjectProperty::Link);
	Super::Link(Ar, Prev);
	ElementSize = sizeof(UObject*);
	LinkAlignment = alignof(UObject*);
	Offset        = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	unguardobj;
}
void UObjectProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UObjectProperty::CopySingleValue);
	*(UObject**)Dest = *(UObject**)Src;
	unguardSlow;
}
void UObjectProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UObjectProperty::CopyCompleteValue);
	if ((PropertyFlags & CPF_NeedCtorLink) && Obj && !(GUglyHackFlags & HACK_DisableSubobjectInstancing))
	{
		for (INT i = 0; i<ArrayDim; i++)
		{
			UObject* Source = ((UObject**)Src)[i];
			((UObject**)Dest)[i] = (Source ? StaticConstructObject(Source->GetClass(), Obj->GetOuter(), NAME_None, 0, Source) : NULL);
		}
	}
	else
	{
		for (INT i = 0; i<ArrayDim; i++)
			((UObject**)Dest)[i] = ((UObject**)Src)[i];
	}
	unguardSlow;
}
void UObjectProperty::DestroyValue(void* Dest) const
{
	guardSlow(UObjectProperty::DestroyValue);
	unguardobjSlow;
}
UBOOL UObjectProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UObjectProperty::Identical);
	return *(UObject**)A == (B ? *(UObject**)B : NULL);
	unguardobjSlow;
}
void UObjectProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UObjectProperty::SerializeItem);
	Ar << *(UObject**)Value;
	unguardobjSlow;
}
UBOOL UObjectProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return Map->SerializeObject(Ar, PropertyClass, *(UObject**)Data);
	unguardobjSlow;
}
void UObjectProperty::Serialize(FArchive& Ar)
{
	guard(UObjectProperty::Serialize);
	Super::Serialize(Ar);
	Ar << PropertyClass;
	unguardobj;
}
void UObjectProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UObjectProperty::ExportCppItem);
	Out.Logf(TEXT("class %ls*"), PropertyClass->GetNameCPP());
	unguardobj;
}
void UObjectProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UObjectProperty::ExportUScriptName);
	Out.Log(PropertyClass->GetName());
	unguardobj;
}
void UObjectProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UObjectProperty::ExportTextItem);
	UObject* Temp = *(UObject **)PropertyValue;
	if (Temp != NULL && Temp->IsValid())
		ValueStr += FString::Printf(TEXT("%ls'%ls'"), Temp->GetClass()->GetName(), Temp->GetPathName());
	else
		ValueStr += TEXT("None");
	unguardobj;
}
const TCHAR* UObjectProperty::StaticImportObject(const TCHAR* Buffer, UObject** Result, INT PortFlags, UClass* MetaClass, const TCHAR* Method)
{
	guard(UObjectProperty::StaticImportObject);
	FString Temp;
	Buffer = ReadToken(Method, Buffer, Temp, 1);
	if (!Buffer || !Temp.Len())
	{
		return NULL;
	}
	if (Temp == TEXT("None"))
	{
		*Result = NULL;
	}
	else
	{
		while (*Buffer == ' ')
			Buffer++;
		if (*Buffer++ != '\'')
		{
			--Buffer;
			*Result = StaticFindObject(MetaClass, ANY_PACKAGE, *Temp);
			if (!*Result)
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: Couldn't find object %ls'%ls'"), Method, MetaClass->GetName(), *Temp);
		}
		else
		{
			UObject* OuterObject = ANY_PACKAGE;
			FString Other;
			Buffer = ReadToken(Method, Buffer, Other, 1);
			if (!Buffer)
				return NULL;
			if (*Buffer == ':')
			{
				OuterObject = StaticFindObject(UObject::StaticClass(), ANY_PACKAGE, *Other);
				if (!OuterObject && (PortFlags & PPF_ExecImport))
					OuterObject = StaticLoadObject(UObject::StaticClass(), NULL, *Other, NULL, LOAD_NoWarn);
				if (!OuterObject)
				{
					GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: Failed to find outer object '%ls'"), Method, *Other);
					return NULL;
				}
				++Buffer;
				Other.Empty();
				Buffer = ReadToken(Method, Buffer, Other, 1);
			}
			if (*Buffer++ != '\'')
			{
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: Missing end ' delimiter"), Method);
				return NULL;
			}
			UClass* ObjectClass = FindObject<UClass>(ANY_PACKAGE, *Temp);
			if (!ObjectClass)
			{
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: Couldn't find limitator class %ls"), Method, *Temp);
				return NULL;
			}
			UObject* Obj = StaticFindObject(ObjectClass, OuterObject, *Other);
			if (!Obj || (MetaClass && !Obj->IsA(MetaClass)))
			{
				if (PortFlags & PPF_ExecImport)
				{
					Obj = StaticLoadObject(ObjectClass, NULL, *Other, NULL, LOAD_NoWarn);
					if (Obj && MetaClass && !Obj->IsA(MetaClass))
						Obj = NULL;
				}
				else Obj = NULL;
			}
			*Result = Obj;
			if (!Obj)
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: Couldn't find object %ls'%ls'"), Method, ObjectClass->GetName(), *Other);
		}
	}
	return Buffer;
	unguard;
}
const TCHAR* UObjectProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UObjectProperty::ImportText);
	return StaticImportObject(Buffer, (UObject**)Data, PortFlags, PropertyClass, GetName());
	unguardobj;
}

// Hashing for Map property!
DWORD UObjectProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(*((UObject**)Data));
}

IMPLEMENT_CLASS(UObjectProperty);

IMPLEMENT_CLASS(UComponentProperty);

void UInterfaceProperty::Serialize(FArchive& Ar)
{
	guard(UInterfaceProperty::Serialize);
	Super::Serialize(Ar);
	Ar << InterfaceClass;
	unguard;
}
void UInterfaceProperty::Link(FArchive& Ar, UProperty* Prev)
{
	guard(UInterfaceProperty::Link);
	Super::Link(Ar, Prev);

	// we have a larger size than UObjectProperties
	ElementSize = sizeof(FScriptInterface);
	LinkAlignment = alignof(void*);
	Offset = Align(GetOuter()->IsA(UStruct::StaticClass()) ? ((UStruct*)GetOuter())->GetPropertiesSize() : 0, LinkAlignment);

	// for now, we won't support instancing of interface properties...it might be possible, but for the first pass we'll keep it simple
	PropertyFlags &= ~CPF_InterfaceClearMask;
	unguard;
}
const TCHAR* UInterfaceProperty::ImportText(const TCHAR* InBuffer, BYTE* Data, INT PortFlags) const
{
	guard(UInterfaceProperty::ImportText);
	FScriptInterface* InterfaceValue = (FScriptInterface*)Data;
	UObject* ResolvedObject = InterfaceValue->GetObject();
	void* InterfaceAddress = InterfaceValue->GetInterface();

	const TCHAR* Buffer = InBuffer;
	if (!UObjectProperty::StaticImportObject(Buffer, &ResolvedObject, PortFlags, UObject::StaticClass(), TEXT("InterfaceProperty::ImportText")))
	{
		// we only need to call SetObject here - if ObjectAddress was not modified, then InterfaceValue should not be modified either
		// if it was set to NULL, SetObject will take care of clearing the interface address too
		InterfaceValue->SetObject(ResolvedObject);
		return NULL;
	}

	// so we should now have a valid object
	if (ResolvedObject == NULL)
	{
		// if ParseObjectPropertyValue returned TRUE but ResolvedObject is NULL, the imported text was "None".  Make sure the interface pointer
		// is cleared, then stop
		InterfaceValue->SetObject(NULL);
		return Buffer;
	}

	void* NewInterfaceAddress = ResolvedObject->GetInterfaceAddress(InterfaceClass);
	if (NewInterfaceAddress == NULL)
	{
		// the object we imported doesn't implement our interface class
		GWarn->Logf(NAME_Error, TEXT("%s: specified object doesn't implement the required interface class '%s': %s"), GetFullName(), InterfaceClass->GetName(), InBuffer);
		return NULL;
	}

	InterfaceValue->SetObject(ResolvedObject);
	InterfaceValue->SetInterface(NewInterfaceAddress);
	return Buffer;
	unguardobj;
}
void UInterfaceProperty::CopySingleValue(void* Dest, void* Src) const
{
	guard(UInterfaceProperty::CopySingleValue);
	*(FScriptInterface*)Dest = *(FScriptInterface*)Src;
	unguardobj;
}
void UInterfaceProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guard(UInterfaceProperty::SerializeItem);
	FScriptInterface* InterfaceValue = (FScriptInterface*)Value;

	Ar << InterfaceValue->GetObjectRef();
	if (Ar.IsLoading())
	{
		if (InterfaceValue->GetObject() != NULL)
			InterfaceValue->SetInterface(InterfaceValue->GetObject()->GetInterfaceAddress(InterfaceClass));
		else InterfaceValue->SetInterface(NULL);
	}
	unguard;
}
IMPLEMENT_CLASS(UInterfaceProperty);

void UDelegateProperty::Link(FArchive& Ar, UProperty* Prev)
{
	Super::Link(Ar, Prev);
	ElementSize = sizeof(FScriptDelegate);
	LinkAlignment = alignof(void*);
	Offset = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	PropertyFlags |= CPF_NeedCtorLink;
}
void UDelegateProperty::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Function << SourceDelegate;
}
void UDelegateProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	Ar << *(FScriptDelegate*)Value;
}
void UDelegateProperty::CopySingleValue(void* Dest, void* Src) const
{
	*(FScriptDelegate*)Dest = *(FScriptDelegate*)Src;
}
void UDelegateProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	*(FScriptDelegate*)Dest = *(FScriptDelegate*)Src;
}
const TCHAR* UDelegateProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UDelegateProperty::ImportText);
	TCHAR ObjName[NAME_SIZE];
	TCHAR FuncName[NAME_SIZE];
	// Get object name
	INT i;
	for (i = 0; *Buffer && *Buffer != TCHAR('.') && *Buffer != TCHAR(')') && *Buffer != TCHAR(','); Buffer++)
	{
		ObjName[i++] = *Buffer;
	}
	ObjName[i] = TCHAR('\0');
	// Get function name
	if (*Buffer)
	{
		Buffer++;
		for (i = 0; *Buffer && *Buffer != TCHAR(')') && *Buffer != TCHAR(','); Buffer++)
		{
			FuncName[i++] = *Buffer;
		}
		FuncName[i] = '\0';
	}
	else
	{
		FuncName[0] = '\0';
	}
	UClass* Cls = FindObject<UClass>(ANY_PACKAGE, ObjName);
	UObject* Object = NULL;
	if (Cls != NULL)
	{
		// If we're importing defaults for a class and this delegate is being assigned to some function in the class
		// don't set the object, otherwise it will reference the default object for the owning class, execDelegateFunction
		// will "do the right thing"
		if (ImportTextParent == Cls->GetDefaultObject())
		{
			Object = NULL;
		}
		else
		{
			Object = Cls->GetDefaultObject();
		}
	}
	else
	{
		Object = StaticFindObject(UObject::StaticClass(), ANY_PACKAGE, ObjName);
		if (Object != NULL)
		{
			Cls = Object->GetClass();
		}
	}
	UFunction* Func = FindField<UFunction>(Cls, FuncName);
	// Check function params.
	if (Func != NULL)
	{
		// Find the delegate UFunction to check params
		UFunction* Delegate = Function;
		check(Delegate && "Invalid delegate property");

		// check return type and params
		if (Func->NumParms == Delegate->NumParms)
		{
			INT Count = 0;
			for (TFieldIterator<UProperty> It1(Func), It2(Delegate); Count < Delegate->NumParms; ++It1, ++It2, ++Count)
			{
				if (It1->GetClass() != It2->GetClass() || (It1->PropertyFlags & CPF_OutParm) != (It2->PropertyFlags & CPF_OutParm))
				{
					debugf(NAME_Warning, TEXT("Function %s does not match param types with delegate %s"), Func->GetName(), Delegate->GetName());
					Func = NULL;
					break;
				}
			}
		}
		else
		{
			debugf(NAME_Warning, TEXT("Function %s does not match number of params with delegate %s"), Func->GetName(), Delegate->GetName());
			Func = NULL;
		}
	}
	else
	{
		debugf(NAME_Warning, TEXT("Unable to find delegate function %s in object %s (found class: %s)"), FuncName, ObjName, Cls->GetName());
	}

	debugfSlow(TEXT("... importing delegate FunctionName:'%s'(%s)   Object:'%s'(%s)"), Func != NULL ? Func->GetName() : TEXT("NULL"), FuncName, Object != NULL ? Object->GetFullName() : TEXT("NULL"), ObjName);

	(*(FScriptDelegate*)Data).Object = Func ? Object : NULL;
	(*(FScriptDelegate*)Data).FunctionName = Func ? Func->GetFName() : NAME_None;

	return Func != NULL ? Buffer : NULL;
	unguardobj;
}
IMPLEMENT_CLASS(UDelegateProperty);

/*-----------------------------------------------------------------------------
UClassProperty.
-----------------------------------------------------------------------------*/

void UClassProperty::Serialize(FArchive& Ar)
{
	guard(UClassProperty::Serialize);
	Super::Serialize(Ar);
	Ar << MetaClass;
	if (!HasAnyFlags(RF_ClassDefaultObject) && !MetaClass->IsValid())
		appErrorf(TEXT("%ls has invalid metaclass!"), GetFullName());
	unguardobj;
}
const TCHAR* UClassProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UClassProperty::ImportText);
	//debugf(TEXT("CallSuper"));
	const TCHAR* Result = StaticImportObject(Buffer, (UObject**)Data, PortFlags, UClass::StaticClass(), GetName());
	//debugf(TEXT("Result %i"),Result);
	if (Result)
	{
		// Validate metaclass.
		UClass*& C = *(UClass**)Data;
		//debugf(TEXT("Validate %ls %ls"),(C ? C->GetName() : TEXT("Null")),MetaClass->GetName());
		if (C && MetaClass && !C->IsChildOf(MetaClass))
		{
			if (PortFlags & PPF_ExecImport)
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed: %ls wasn't metaclass of %ls"), GetName(), C->GetFullName(), MetaClass->GetFullName());
			C = NULL;
		}
		//debugf(TEXT("ValidateDone"));
	}
	//debugf(TEXT("ReturnRes"));
	return Result;
	unguardobj;
}
UBOOL UClassProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UClassProperty::NetSerializeItem);
	if (Ar.IsLoading())
	{
		UBOOL Result = Map->SerializeObject(Ar, UClass::StaticClass(), *(UObject**)Data);
		if (MetaClass && *(UObject**)Data && !(*(UClass**)Data)->IsChildOf(MetaClass))
		{
			*(UObject**)Data = NULL;
			Result = 0;
		}
		return Result;
	}
	return Map->SerializeObject(Ar, UClass::StaticClass(), *(UObject**)Data);
	unguardobjSlow;
}
void UClassProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UClassProperty::ExportUScriptName);
	if(!MetaClass || MetaClass==UObject::StaticClass())
		Out.Log(TEXT("Class"));
	else Out.Logf(TEXT("Class<%ls>"), MetaClass->GetName());
	unguardobj;
}
IMPLEMENT_CLASS(UClassProperty);

/*-----------------------------------------------------------------------------
UNameProperty.
-----------------------------------------------------------------------------*/

void UNameProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UNameProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize   = sizeof(FName);
	LinkAlignment = alignof(FName);
	Offset        = Align( GetOuterUField()->GetPropertiesSize(), LinkAlignment );
	unguardobj;
}
void UNameProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UNameProperty::CopySingleValue);
	*(FName*)Dest = *(FName*)Src;
	unguardSlow;
}
void UNameProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UNameProperty::CopyCompleteValue);
	if (ArrayDim == 1)
		*(FName*)Dest = *(FName*)Src;
	else appMemcpy(Dest, Src, sizeof(FName)*ArrayDim);
	unguardSlow;
}
UBOOL UNameProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UNameProperty::Identical);
	return *(FName*)A == (B ? *(FName*)B : FName(NAME_None));
	unguardobjSlow;
}
void UNameProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UNameProperty::SerializeItem);
	Ar << *(FName*)Value;
	unguardobjSlow;
}
void UNameProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UNameProperty::ExportCppItem);
	Out.Log(TEXT("FName"));
	unguardobj;
}
void UNameProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guard(UNameProperty::ExportUScriptName);
	Out.Log(TEXT("name"));
	unguardobj;
}
void UNameProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UNameProperty::ExportTextItem);
	FName Temp = *(FName*)PropertyValue;
	if (!(PortFlags & PPF_Delimited))
		ValueStr += *Temp;
	else
		ValueStr += FString::Printf(TEXT("\"%ls\""), *Temp);
	unguardobj;
}
const TCHAR* UNameProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UNameProperty::ImportText);
	FString Temp;
	Buffer = ReadNameToken(GetFullName(), Buffer, Temp, (PortFlags & (PPF_Exec | PPF_Delimited)) != 0);
	if (!Buffer)
		return NULL;
	*(FName*)Data = FName(*Temp);
	return Buffer;
	unguardobj;
}

// Hashing for Map property!
DWORD UNameProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(*((FName*)Data));
}

IMPLEMENT_CLASS(UNameProperty);

/*-----------------------------------------------------------------------------
UStrProperty.
-----------------------------------------------------------------------------*/

void UStrProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UStrProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize    = sizeof(FString);
	LinkAlignment  = alignof(FString);
	Offset         = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UStrProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UStrProperty::Identical);
	return appStricmp(**(const FString*)A, B ? **(const FString*)B : TEXT("")) == 0;
	unguardobjSlow;
}
void UStrProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UStrProperty::SerializeItem);
	Ar << *(FString*)Value;
	unguardobjSlow;
}
void UStrProperty::Serialize(FArchive& Ar)
{
	guard(UStrProperty::Serialize);
	Super::Serialize(Ar);
	unguardobj;
}
void UStrProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UStrProperty::ExportCppItem);
	Out.Log(TEXT("FString"));
	unguardobj;
}
void UStrProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guardSlow(UStrProperty::ExportUScriptName);
	Out.Log(TEXT("string"));
	unguardobjSlow;
}
inline TCHAR PortChar(DWORD In)
{
	if (In<10)
		return '0' + In;
	else return 'A' + (In - 10);
}
inline TCHAR* PortString(TCHAR c)
{
	static TCHAR Out[4] = { '\\', 0, 0, 0 };
	if (c == '\n' || c == '\"' || c == '\\')
	{
		Out[1] = (c == '\n' ? 'n' : (c == '\"' ? '\"' : '\\'));
		Out[2] = 0;
	}
	else
	{
		Out[1] = PortChar(c >> 4);
		Out[2] = PortChar(c & 15);
	}
	return Out;
}
inline UBOOL SafeIniChar(TCHAR c)
{
	return (c >= ' ' && c != '\"' && c != '\\');
}
void UStrProperty::SafeWriteString(FString& Dest, const TCHAR* Src)
{
	guard(UStrProperty::SafeWriteString);
	TCHAR* S = const_cast<TCHAR*>(Src);
	while (*S)
	{
		const TCHAR* Start = S;
		while (SafeIniChar(*S)) // Iterate until finds an unsafe char.
			++S;
		if (!*S)
		{
			if (Start != S)
				Dest += Start;
			break;
		}
		if (Start != S)
		{
			TCHAR Old = *S;
			*S = 0;
			Dest += Start;
			*S = Old;
		}
		while (*S && !SafeIniChar(*S))
		{
			Dest += PortString(*S);
			++S;
		}
	}
	unguard;
}
inline TCHAR DePortChar(TCHAR In)
{
	if (In >= '0' && In <= '9')
		return (In - '0');
	else if (In >= 'A' && In <= 'Z')
		return (In + 10 - 'A');
	return 1;
}
void UStrProperty::SafeReadString(FString& Dest, const TCHAR*& Src)
{
	guard(UStrProperty::SafeReadString);
	if (*Src == '\"')
		++Src;
	while (*Src && *Src != '\"')
	{
		const TCHAR* Start = Src;
		while (*Src && *Src != '\\' && *Src != '\"')
			++Src;
		if (Start != Src)
		{
			TCHAR* S = const_cast<TCHAR*>(Src);
			TCHAR Old = *S;
			*S = 0;
			Dest += Start;
			*S = Old;
		}
		if (!*Src || *Src == '\"')
			break;

		// Parse special char
		++Src;
		if (*Src == '"' || *Src == '\\' || *Src == 'n')
		{
			TCHAR L[2] = { (TCHAR)(*Src == 'n' ? '\n' : *Src), 0 };
			Dest += L;
			++Src;
		}
		else
		{
			TCHAR ValA = *Src;
			++Src;
			if (!*Src)
				break;
			TCHAR ValB = *Src;
			TCHAR L[2] = { (TCHAR)((DePortChar(ValA) << 4) | DePortChar(ValB)), 0 };
			Dest += L;
			++Src;
		}
	}
	if (*Src == '\"')
		++Src;
	unguard;
}
void ReadExecStr(FString& Dest, const TCHAR*& Src)
{
	guard(ReadExecStr);
	TCHAR DelimChar = ' ';
	if (*Src == '\"' || *Src == '\'') // Has delimitator, then expect one ending the string too!
		DelimChar = *Src++;

	const TCHAR* Start = Src;
	while (*Src && *Src != DelimChar)
		++Src;
	Dest = FString(Start, Src);
	if (*Src == DelimChar)
		++Src;
	unguard;
}

void UStrProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UStrProperty::ExportTextItem);
	if (!(PortFlags & PPF_Delimited))
		ValueStr += *(FString*)PropertyValue;
	else
	{
		ValueStr += TEXT("\"");
		SafeWriteString(ValueStr, *(*(FString*)PropertyValue));
		ValueStr += TEXT("\"");
	}
	unguardobj;
}
const TCHAR* UStrProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UStrProperty::ImportText);
	if (PortFlags & PPF_Exec)
	{
		((FString*)Data)->Empty();
		ReadExecStr(*(FString*)Data, Buffer);
	}
	else if (!(PortFlags & PPF_Delimited))
	{
		*(FString*)Data = Buffer;
	}
	else
	{
		((FString*)Data)->Empty();
		SafeReadString(*(FString*)Data, Buffer);
	}
	return Buffer;
	unguardobj;
}
void UStrProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UStrProperty::CopySingleValue);
	*(FString*)Dest = *(FString*)Src;
	unguardobjSlow;
}
void UStrProperty::DestroyValue(void* Dest) const
{
	guardSlow(UStrProperty::DestroyValue);
	for (INT i = 0; i<ArrayDim; i++)
	{
		// stijn: The original code called the FString destructor here, but then
		// continued to use the destroyed FString object.  This worked in 1999
		// because the FString destructor just reinitialized all FString member
		// variables to zero-initialized values, but this is not valid C++ and
		// causes crashes with modern glibcs and modern compilers.
		//
		// The reason why this causes crashes is because compilers like gcc now
		// throw out the reinitialization operations that happen within
		// destructors. This is a reasonable optimization since it is not legal
		// to keep using a destroyed C++ object anyway.		
		(*(FString*)((BYTE*)Dest + i*ElementSize)).Empty();
	}
	unguardobjSlow;
}

// Hashing for Map property!
DWORD UStrProperty::GetMapHash(BYTE* Data) const
{
	return GetTypeHash(*((FString*)Data));
}

IMPLEMENT_CLASS(UStrProperty);

/*-----------------------------------------------------------------------------
UFixedArrayProperty.
-----------------------------------------------------------------------------*/

void UFixedArrayProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UFixedArrayProperty::Link);
	checkSlow(Count>0);
	Super::Link( Ar, Prev );
	Ar.Preload( Inner );
	Inner->Link( Ar, NULL );
	ElementSize   = Inner->ElementSize * Count;
	LinkAlignment = Inner->LinkAlignment;
	Offset        = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= (Inner->PropertyFlags & CPF_NeedCtorLink);
	unguardobj;
}
UBOOL UFixedArrayProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UFixedArrayProperty::Identical);
	checkSlow(Inner);
	for (INT i = 0; i<Count; i++)
		if (!Inner->Identical((BYTE*)A + i*Inner->ElementSize, B ? ((BYTE*)B + i*Inner->ElementSize) : NULL))
			return 0;
	return 1;
	unguardobjSlow;
}
void UFixedArrayProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UFixedArrayProperty::SerializeItem);
	checkSlow(Inner);
	for (INT i = 0; i<Count; i++)
		Inner->SerializeItem(Ar, (BYTE*)Value + i*Inner->ElementSize);
	unguardobjSlow;
}
UBOOL UFixedArrayProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UFixedArrayProperty::Serialize(FArchive& Ar)
{
	guard(UFixedArrayProperty::Serialize);
	Super::Serialize(Ar);
	Ar << Inner << Count;
	checkSlow(Inner);
	unguardobj;
}
void UFixedArrayProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UFixedArrayProperty::ExportCppItem);
	checkSlow(Inner);
	Inner->ExportCppItem(Out);
	Out.Logf(TEXT("[%i]"), Count);
	unguardobj;
}
void UFixedArrayProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UFixedArrayProperty::ExportTextItem);
	checkSlow(Inner);
	ValueStr += TEXT("(");
	for (INT i = 0; i<Count; i++)
	{
		if (i>0)
			ValueStr += TEXT(",");
		Inner->ExportTextItem(ValueStr, PropertyValue + i*Inner->ElementSize, DefaultValue ? (DefaultValue + i*Inner->ElementSize) : NULL, PortFlags | PPF_Delimited);
	}
	ValueStr += TEXT(")");
	unguardobj;
}
const TCHAR* UFixedArrayProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UFixedArrayProperty::ImportText);
	checkSlow(Inner);
	if (*Buffer++ != '(')
		return NULL;
	appMemzero(Data, ElementSize);
	for (INT i = 0; i<Count; i++)
	{
		Buffer = Inner->ImportText(Buffer, Data + i*Inner->ElementSize, PortFlags | PPF_Delimited);
		if (!Buffer)
			return NULL;
		if (*Buffer != ',' && i != Count - 1)
			return NULL;
		Buffer++;
	}
	if (*Buffer++ != ')')
		return NULL;
	return Buffer;
	unguardobj;
}
void UFixedArrayProperty::AddCppProperty(UProperty* Property, INT InCount)
{
	guard(UFixedArrayProperty::AddCppProperty);
	check(!Inner);
	check(Property);
	check(InCount>0);

	Inner = Property;
	Count = InCount;

	unguardobj;
}
void UFixedArrayProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UFixedArrayProperty::CopySingleValue);
	for (INT i = 0; i<Count; i++)
		Inner->CopyCompleteValue((BYTE*)Dest + i*Inner->ElementSize, Src ? ((BYTE*)Src + i*Inner->ElementSize) : NULL);
	unguardobjSlow;
}
void UFixedArrayProperty::DestroyValue(void* Dest) const
{
	guardSlow(UFixedArrayProperty::DestroyValue);
	for (INT i = 0; i<Count; i++)
		Inner->DestroyValue((BYTE*)Dest + i*Inner->ElementSize);
	unguardobjSlow;
}
IMPLEMENT_CLASS(UFixedArrayProperty);

/*-----------------------------------------------------------------------------
UArrayProperty.
-----------------------------------------------------------------------------*/

void UArrayProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UArrayProperty::Link);
	Super::Link( Ar, Prev );
	Ar.Preload( Inner );
	Inner->Link( Ar, NULL );
	ElementSize    = sizeof(FArray);
	LinkAlignment  = alignof(FArray);
	Offset         = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	if( !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UArrayProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UArrayProperty::Identical);
	checkSlow(Inner);
	INT n = ((FArray*)A)->Num();
	if (n != (B ? ((FArray*)B)->Num() : 0))
		return 0;
	INT   c = Inner->ElementSize;
	BYTE* p = (BYTE*)((FArray*)A)->GetData();
	if (B)
	{
		BYTE* q = (BYTE*)((FArray*)B)->GetData();
		for (INT i = 0; i<n; i++)
			if (!Inner->Identical(p + i*c, q + i*c))
				return 0;
	}
	else
	{
		for (INT i = 0; i<n; i++)
			if (!Inner->Identical(p + i*c, 0))
				return 0;
	}
	return 1;
	unguardobjSlow;
}
void UArrayProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UArrayProperty::SerializeItem);
	checkSlow(Inner);
	INT   c = Inner->ElementSize;
	FArray* FArr = (FArray*)Value;
	INT   n = FArr->Num();
	Ar << n;
	if (Ar.IsLoading())
	{
		if (FArr->Num()) // If being re-serialized, then cleanup first.
		{
			BYTE* p = (BYTE*)FArr->GetData();
			for (INT i = 0; i < FArr->Num(); i++)
				Inner->DestroyValue(p + i * c);
		}
		FArr->Empty(c);
		FArr->AddZeroed(c, n);
	}
	BYTE* p = (BYTE*)FArr->GetData();
	FArr->CountBytes(Ar, c);
	for (INT i = 0; i<n; i++)
		Inner->SerializeItem(Ar, p + i*c);
	unguardobjSlow;
}
UBOOL UArrayProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	FArray* Array = (FArray*)Data;
	INT Len = Min<INT>(Array->Num(), 511);
	INT c = Inner->ElementSize;
	Ar.SerializeBits(&Len, 9);
	if (Ar.IsLoading())
	{
		if (Len>Array->Num())
			Array->AddZeroed(c, Len - Array->Num());
		else if (Len<Array->Num())
		{
			if (Inner->PropertyFlags & CPF_NeedCtorLink)
			{
				BYTE* DestData = (BYTE*)Array->GetData();
				for (INT i = (Array->Num() - 1); i >= Len; --i)
					Inner->DestroyValue(DestData + (i*c));
			}
			Array->Remove(Len, Array->Num() - Len, c);
		}
	}
	BYTE* DestData = (BYTE*)Array->GetData();
	for (INT i = 0; i<Len; ++i)
		Inner->NetSerializeItem(Ar, Map, DestData + (i*c));
	return 1;
}
void UArrayProperty::Serialize(FArchive& Ar)
{
	guard(UArrayProperty::Serialize);
	Super::Serialize(Ar);
	Ar << Inner;
	checkSlow(Inner);
	unguardobj;
}
void UArrayProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UArrayProperty::ExportCppItem);
	checkSlow(Inner);
	Out.Log(TEXT("TArray<"));
	Inner->ExportCppItem(Out);
	Out.Log(TEXT(">"));
	unguardobj;
}
void UArrayProperty::ExportCpp(FOutputDevice& Out, UBOOL IsLocal, UBOOL IsParm) const
{
	guard(UArrayProperty::ExportCpp)
	if (IsParm || IsLocal)
		Out.Log(TEXT("TArray<"));
	else Out.Log(TEXT("TArrayNoInit<"));
	Inner->ExportCppItem(Out);
	Out.Log(TEXT(">"));

	if (IsParm && (PropertyFlags & CPF_OutParm))
		Out.Logf(TEXT("& %ls"), GetName());
	else Out.Logf(TEXT(" %ls"), GetName());
	unguardobj;
}
void UArrayProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guardSlow(UArrayProperty::ExportUScriptName);
	Out.Log(TEXT("array<"));
	Inner->ExportUScriptName(Out);
	Out.Log(TEXT(">"));
	unguardobjSlow;
}
void UArrayProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UArrayProperty::ExportTextItem); // Note with DefaultValue: Do not export partial arrays because we only import full arrays.
	checkSlow(Inner);
	ValueStr += TEXT("(");
	FArray* Array = (FArray*)PropertyValue;
	INT     ElementSize = Inner->ElementSize;
	for (INT i = 0; i<Array->Num(); i++)
	{
		if (i>0)
			ValueStr += TEXT(",");
		Inner->ExportTextItem(ValueStr, (BYTE*)Array->GetData() + i * ElementSize, NULL, PortFlags | PPF_Delimited);
	}
	ValueStr += TEXT(")");
	unguardobj;
}
const TCHAR* UArrayProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UArrayProperty::ImportText);
	checkSlow(Inner);
	if (*Buffer++ != '(')
	{
		GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Missing '('"), GetPathName());
		return NULL;
	}
	FArray* Array = (FArray*)Data;
	INT     ElementSize = Inner->ElementSize;
	Array->Empty(ElementSize);
	while (*Buffer != ')')
	{
		while (*Buffer == ' ')
			++Buffer;
		INT Index = Array->Add(1, ElementSize);
		appMemzero((BYTE*)Array->GetData() + Index*ElementSize, ElementSize);
		const TCHAR* Result = Inner->ImportText(Buffer, (BYTE*)Array->GetData() + Index*ElementSize, PortFlags | PPF_Delimited);
		if (!Result)
		{
			FStringOutputDevice SOut;
			Inner->ExportUScriptName(SOut);
			GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Failed to import array value at (%ls): %ls"), GetPathName(), *SOut, Buffer);
			return NULL;
		}
		Buffer = Result;
		while (*Buffer == ' ')
			++Buffer;
		if (*Buffer != ',')
			break;
		Buffer++;
	}
	if (*Buffer++ != ')')
	{
		GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Missing ')'"), GetPathName());
		return NULL;
	}
	return Buffer;
	unguardobj;
}
void UArrayProperty::AddCppProperty(UProperty* Property)
{
	guard(UArrayProperty::AddCppProperty);
	check(!Inner);
	check(Property);

	Inner = Property;

	unguardobj;
}
void UArrayProperty::CopySingleValue(void* Dest, void* Src) const
{
	CopyCompleteValue(Dest, Src, NULL);
}
void UArrayProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guard(UArrayProperty::CopyCompleteValue);
	FArray* SrcArray = (FArray*)Src;
	FArray* DestArray = (FArray*)Dest;
	INT     Size = Inner->ElementSize;
	if (Inner->PropertyFlags & CPF_NeedCtorLink)
	{
		BYTE* DestData = (BYTE*)DestArray->GetData();
		INT i = 0;
		for (i = 0; i<DestArray->Num(); i++)
			Inner->DestroyValue(DestData + i*Size);
		DestArray->Empty(Size, SrcArray->Num());

		// Copy all the elements.
		DestArray->AddZeroed(Size, SrcArray->Num());
		BYTE* SrcData = (BYTE*)SrcArray->GetData();
		DestData = (BYTE*)DestArray->GetData();
		if (Obj)
		{
			for (i = 0; i < DestArray->Num(); i++)
				Inner->CopyCompleteValue(DestData + i * Size, SrcData + i * Size, Obj);
		}
		else
		{
			for (i = 0; i < DestArray->Num(); i++)
				Inner->CopySingleValue(DestData + i * Size, SrcData + i * Size);
		}
	}
	else
	{
		DestArray->Empty(Size, SrcArray->Num());

		// Copy all the elements.
		DestArray->Add(SrcArray->Num(), Size);
		appMemcpy(DestArray->GetData(), SrcArray->GetData(), SrcArray->Num()*Size);
	}
	unguardobj;
}
void UArrayProperty::DestroyValue(void* Dest) const
{
	guard(UArrayProperty::DestroyValue);
	FArray* DestArray = (FArray*)Dest;
	if (Inner->PropertyFlags & CPF_NeedCtorLink)
	{
		BYTE* DestData = (BYTE*)DestArray->GetData();
		INT   Size = Inner->ElementSize;
		for (INT i = 0; i<DestArray->Num(); i++)
			Inner->DestroyValue(DestData + i*Size);
	}	
	// stijn: was a ~FArray() call originally. See note in
	// UStrProperty::DestroyValue to see why this was changed.
	DestArray->EmptyFArray(GetName());
	unguardobj;
}
IMPLEMENT_CLASS(UArrayProperty);

/*-----------------------------------------------------------------------------
UMapProperty.
-----------------------------------------------------------------------------*/

void UMapProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UMapProperty::Link);
	Super::Link( Ar, Prev );
	ElementSize = sizeof(TMap<BYTE, BYTE>);
	LinkAlignment = alignof(TMap<BYTE, BYTE>);
	Offset = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);
	if (!(PropertyFlags & CPF_Native))
	{
		PropertyFlags |= CPF_NeedCtorLink;
	}
	unguardobj;
}
UBOOL UMapProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UMapProperty::Identical);
	return 1;
	unguardobjSlow;
}
void UMapProperty::SerializeItem(FArchive& Ar, void* Dest) const
{
	guardSlow(UMapProperty::SerializeItem);
	unguardobjSlow;
}
UBOOL UMapProperty::NetSerializeItem(FArchive& Ar, UPackageMap* PackageMap, void* Data) const
{
	guardSlow(UByteProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UMapProperty::Serialize(FArchive& Ar)
{
	guard(UMapProperty::Serialize);
	Super::Serialize(Ar);
	Ar << Key << Value;
	unguardobj;
}
void UMapProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UMapProperty::ExportCppItem);
	Out.Log(TEXT("TMap<void*,void*>"));
	unguardobj;
}
void UMapProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guardSlow(UMapProperty::ExportUScriptName);
	checkSlow(Key);
	checkSlow(Value);
	Out.Log(TEXT("map<"));
	Key->ExportUScriptName(Out);
	Out.Log(TEXT(","));
	Value->ExportUScriptName(Out);
	Out.Log(TEXT(">"));
	unguardobjSlow;
}
void UMapProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UMapProperty::ExportTextItem);
	ValueStr += TEXT("()");
	unguardobj;
}
const TCHAR* UMapProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UMapProperty::ImportText);
	return Buffer;
	unguardobj;
}
void UMapProperty::CopySingleValue(void* Dest, void* Src) const
{
}
void UMapProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UMapProperty::CopyCompleteValue);
	unguardSlow;
}
void UMapProperty::DestroyValue(void* Dest) const
{
	guardSlow(UMapProperty::DestroyValue);
	unguardobjSlow;
}

IMPLEMENT_CLASS(UMapProperty);

/*-----------------------------------------------------------------------------
UStructProperty.
-----------------------------------------------------------------------------*/

static INT GetPropAlignment(UProperty* Prop, UProperty* ExcludeStruct)
{
	INT Result = 1;

	if (!Prop)
		return Result;	

	if (Prop->IsA(UStructProperty::StaticClass()) && Prop != ExcludeStruct)
	{
		Result = Max(Result, ((UStructProperty*)Prop)->GetLinkAlignment());
	}
	else if (Prop->IsA(UIntProperty::StaticClass()) || Prop->IsA(UFloatProperty::StaticClass()) || Prop->IsA(UBoolProperty::StaticClass()))
	{
		Result = Max(Result, 4);
	}
	else if (Prop->IsA(UObjectProperty::StaticClass()))
	{
		Result = Max(Result, POINTER_ALIGNMENT);
	}
	else if (Prop->IsA(UNameProperty::StaticClass()))
	{
		Result = Max<WORD>(Result, alignof(FName));
	}
	else if (Prop->IsA(UStrProperty::StaticClass()))
	{
		Result = Max<WORD>(Result, alignof(FString));
	}
	else if (Prop->IsA(UMapProperty::StaticClass()))
	{
		Result = Max<WORD>(Result, alignof(FScriptMapBase));
	}
	else if (Prop->IsA(UArrayProperty::StaticClass()))
	{
		Result = Max<WORD>(Result, alignof(FArray));
	}
	else if (Prop->IsA(UFixedArrayProperty::StaticClass()))
	{
		// impossible to determine unless Inner is already linked
		Result = Max(Result, GetPropAlignment(((UFixedArrayProperty*)Prop)->Inner, nullptr));
	}

	return Result;
}

INT UStructProperty::GetLinkAlignment()
{
	if (GetFName() == NAME_Vector ||
		GetFName() == NAME_Plane ||
		GetFName() == NAME_Coords)
		return VECTOR_ALIGNMENT;

	if (GetFName() == NAME_Color)
		return 4;

	INT Result = Struct->PropertiesAlign;
	for (TFieldIterator<UProperty> It(Struct); It; ++It)
		Result = Max(Result, GetPropAlignment(*It, this));	

	return Result;
}

void UStructProperty::Link( FArchive& Ar, UProperty* Prev )
{
	guard(UStructProperty::Link);
	Super::Link( Ar, Prev );
	Ar.Preload( Struct );

	ElementSize   = Struct->PropertiesSize;
	LinkAlignment = GetLinkAlignment();
	//appDebugMessagef(TEXT("Calculated struct alignment %s => %d"), GetName(), LinkAlignment);

	Offset = Align(GetOuterUField()->GetPropertiesSize(), LinkAlignment);

	if( Struct->ConstructorLink && !(PropertyFlags & CPF_Native) )
		PropertyFlags |= CPF_NeedCtorLink;
	unguardobj;
}
UBOOL UStructProperty::Identical(const void* A, const void* B) const
{
	guardSlow(UStructProperty::Identical);
	for (TFieldIterator<UProperty> It(Struct); It; ++It)
		for (INT i = 0; i<It->ArrayDim; i++)
			if (!It->Matches(A, B, i))
				return 0;
	return 1;
	unguardobjSlow;
}
void UStructProperty::SerializeItem(FArchive& Ar, void* Value) const
{
	guardSlow(UStructProperty::SerializeItem);
	UBOOL bUseBinarySerialization = !(Ar.IsLoading() || Ar.IsSaving())
		|| ((Struct->StructFlags & STRUCT_Immutable) != 0
			// when the min package version is bumped, the remainder of this check can be removed
			&& (Struct->GetFName() != NAME_FontCharacter || Ar.Ver() >= VER_FIXED_FONTS_SERIALIZATION));

	Ar.Preload(Struct);
	
	if (bUseBinarySerialization)
		Struct->SerializeBin(Ar, (BYTE*)Value);
	else
	{
		UScriptStruct* SS = Cast<UScriptStruct>(Struct);
		Struct->SerializeTaggedProperties(Ar, (BYTE*)Value, Struct, (SS && SS->GetDefaultsCount()) ? SS->GetDefaults() : NULL, Struct->GetPropertiesSize());
	}

	unguardobjSlow;
}
UBOOL UStructProperty::NetSerializeItem(FArchive& Ar, UPackageMap* Map, void* Data) const
{
	guardSlow(UStructProperty::NetSerializeItem);
	return 1;
	unguardobjSlow;
}
void UStructProperty::Serialize(FArchive& Ar)
{
	guard(UStructProperty::Serialize);
	Super::Serialize(Ar);
	Ar << Struct;
	unguardobj;
}
void UStructProperty::ExportCppItem(FOutputDevice& Out) const
{
	guard(UStructProperty::ExportCppItem);
	Out.Logf(TEXT("%ls"), Struct->GetNameCPP());
	unguardobj;
}
void UStructProperty::ExportUScriptName(FOutputDevice& Out) const
{
	guardSlow(UStructProperty::ExportUScriptName);
	Out.Log(Struct->GetName());
	unguardobjSlow;
}
void UStructProperty::ExportTextItem(FString& ValueStr, BYTE* PropertyValue, BYTE* DefaultValue, INT PortFlags) const
{
	guard(UStructProperty::ExportTextItem);
	FString Value;
	INT Count = 0;
	if ((PortFlags & PPF_PropWindow) && !GSys->UseRegularAngles && Struct->GetFName() == NAME_Rotator)
		PortFlags |= PPF_RotDegree;
	for (TFieldIterator<UProperty> It(Struct); It; ++It)
	{
		if (It->Port())
		{
			for (INT Index = 0; Index < It->ArrayDim; Index++)
			{
				Value.Empty();
				if (It->ExportText(Index, Value, PropertyValue, DefaultValue, PortFlags | PPF_Delimited))
				{
					Count++;
					if (Count == 1)
						ValueStr += TEXT("(");
					else
						ValueStr += TEXT(",");
					if (It->ArrayDim == 1)
						ValueStr += FString::Printf(TEXT("%ls="), It->GetName());
					else
						ValueStr += FString::Printf(TEXT("%ls[%i]="), It->GetName(), Index);
					ValueStr += Value;
				}
			}
		}
	}
	if (Count > 0)
	{
		ValueStr += TEXT(")");
	}

	unguardf((TEXT("(%ls/%ls)"), GetFullName(), Struct->GetFullName()));
}
const TCHAR* UStructProperty::ImportText(const TCHAR* Buffer, BYTE* Data, INT PortFlags) const
{
	guard(UStructProperty::ImportText);
	if (*Buffer++ == '(')
	{
		// Parse all properties.
		while (*Buffer != ')')
		{
			// Get key name.
			while (*Buffer == ' ' || *Buffer == '\t')
				++Buffer;
			const TCHAR* Start = Buffer;
			while (*Buffer && *Buffer != '=' && *Buffer != '[')
				Buffer++;
			const TCHAR* EndB = Buffer;
			while (EndB > Start && (*(EndB - 1) == ' ' || *(EndB - 1) == '\t'))
				--EndB;
			FString Name(Start, EndB);

			// Get optional array element.
			INT Element = 0;
			UBOOL bHadElement = FALSE;
			if (*Buffer == '[')
			{
				Start = ++Buffer;
				while (*Buffer >= '0' && *Buffer <= '9')
					Buffer++;
				if (*Buffer++ != ']')
				{
					GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Illegal array element"), GetPathName());
					return NULL;
				}
				Element = appAtoi(Start);
				bHadElement = TRUE;
			}

			// Verify format.
			if (*Buffer++ != '=')
			{
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Illegal or missing key name"), GetPathName());
				return NULL;
			}

			while (*Buffer == ' ' || *Buffer == '\t')
				++Buffer;

			// See if the property exists in the struct.
			FName GotName(*Name, FNAME_Find);
			UBOOL Parsed = 0;
			if (GotName != NAME_None)
			{
				for (TFieldIterator<UProperty> It(Struct); It; ++It)
				{
					UProperty* Property = *It;
					if (Property->GetFName() != GotName)
						continue;

					BYTE* pData = Data + Property->Offset;
					if (bHadElement && Property->IsA(UArrayProperty::StaticClass()))
					{
						UProperty* InnerProp = ((UArrayProperty*)Property)->Inner;
						FArray* FAr = (FArray*)pData;
						if (Element >= FAr->Num())
						{
							INT num = FAr->Num();
							FAr->AddZeroed(InnerProp->ElementSize, Element + 1 - num);
							pData = (BYTE*)FAr->GetData();

							UStructProperty* StructProperty = Cast<UStructProperty>(InnerProp);
							if (StructProperty)
							{
								// Initialize struct defaults.
								BYTE* Def = StructProperty->Struct->GetDefaults();
								if (Def)
								{
									for (INT i = num; i <= Element; ++i)
										StructProperty->CopySingleValue(pData + (StructProperty->ElementSize * i), Def);
								}
							}
						}
						else pData = (BYTE*)FAr->GetData();

						pData += (Element * InnerProp->ElementSize);
						Element = 0;
						Property = InnerProp;
					}
					if (Element >= Property->ArrayDim)
					{
						GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Array index out of bounds in: %ls"), GetPathName(), Buffer);
						return NULL;
					}
					const TCHAR* Result = Property->ImportText(Buffer, pData + Property->Offset + Element*Property->ElementSize, PortFlags | PPF_Delimited);
					if (Result == NULL)
					{
						// Check if constant definition.
						Start = Buffer;
						EndB = Buffer;
						while (appIsAlnum(*EndB))
							EndB++;
						if (Start != EndB)
						{
							FString ConstName(Start, EndB);
							UConst* CC = FindObject<UConst>(ANY_PACKAGE, *ConstName, 1);
							if (CC)
							{
								Result = Property->ImportText(*CC->Value, pData + Property->Offset + Element * Property->ElementSize, PortFlags | PPF_Delimited);
								if (Result)
									Result = EndB;
							}
						}

						if (Result == NULL)
						{
							GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText failed in (%ls): %ls"), GetPathName(), Property->GetFullName(), Buffer);
							return NULL;
						}
					}
					Buffer = Result;
					Parsed = 1;
					break;
				}
			}

			// If not parsed, skip this property in the stream.
			if (!Parsed)
			{
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Property '%ls' not found in %ls"), GetPathName(), *Name, Struct->GetFullName());

				INT SubCount = 0;
				while
					(*Buffer
					&&	*Buffer != 10
					&& *Buffer != 13
					&& (SubCount>0 || *Buffer != ')')
					&& (SubCount>0 || *Buffer != ','))
				{
					if (*Buffer == 0x22)
					{
						while (*Buffer && *Buffer != 0x22 && *Buffer != 10 && *Buffer != 13)
							Buffer++;
						if (*Buffer != 0x22)
						{
							GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Bad quoted string"), GetPathName());
							return NULL;
						}
					}
					else if (*Buffer == '(')
					{
						SubCount++;
					}
					else if (*Buffer == ')')
					{
						SubCount--;
						if (SubCount < 0)
						{
							GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Bad parenthesised struct"), GetPathName());
							return NULL;
						}
					}
					Buffer++;
				}
				if (SubCount > 0)
				{
					GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Incomplete parenthesised struct"), GetPathName());
					return NULL;
				}
			}

			while (*Buffer == ' ' || *Buffer == '\t')
				++Buffer;

			// Skip comma.
			if (*Buffer == ',')
			{
				// Skip comma.
				Buffer++;
			}
			else if (*Buffer != ')')
			{
				GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Bad termination at: %ls"), GetPathName(), Buffer);
				return NULL;
			}
		}

		// Skip trailing ')'.
		Buffer++;
	}
	else
	{
		GWarn->Logf(NAME_ExecWarning, TEXT("%ls.ImportText: Struct missing '('"), GetPathName());
		return NULL;
	}
	return Buffer;
	unguardobj;
}
void UStructProperty::CopySingleValue(void* Dest, void* Src) const
{
	guardSlow(UStructProperty::CopySingleValue);
	if (PropertyFlags & CPF_NeedCtorLink)
	{
		for (TFieldIterator<UProperty> It(Struct); It; ++It)
			It->CopyCompleteValue((BYTE*)Dest + It->Offset, (BYTE*)Src + It->Offset, NULL);
	}
	else appMemcpy(Dest, Src, ElementSize);
	unguardobjSlow;
}
void UStructProperty::DestroyValue(void* Dest) const
{
	guardSlow(UStructProperty::DestroyValue);
	for (UProperty* P = Struct->ConstructorLink; P; P = P->ConstructorLinkNext)
		for (INT i = 0; i<ArrayDim; i++)
			P->DestroyValue((BYTE*)Dest + i*ElementSize + P->Offset);
	unguardobjSlow;
}
void UStructProperty::CopyCompleteValue(void* Dest, void* Src, UObject* Obj) const
{
	guardSlow(UStructProperty::CopyCompleteValue);
	if (PropertyFlags & CPF_NeedCtorLink)
	{
		for (INT i = 0; i<ArrayDim; i++)
		{
			for (TFieldIterator<UProperty> It(Struct); It; ++It)
				It->CopyCompleteValue((BYTE*)Dest + i*ElementSize + It->Offset, (BYTE*)Src + i*ElementSize + It->Offset, Obj);
		}
	}
	else appMemcpy(Dest, Src, ElementSize*ArrayDim);
	unguardobjSlow;
}
void UStructProperty::InitializeValue(BYTE* Dest) const
{
	guard(UStructProperty::InitializeValue);
	if (Struct && Struct->GetDefaultsCount())
	{
		BYTE* Src = Struct->GetDefaults();
		if (Src)
		{
			for (INT ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++)
				CopySingleValue(Dest + ArrayIndex * ElementSize, Src);
		}
	}
	unguardobj;
}

// Hashing for Map property!
DWORD UStructProperty::GetMapHash(BYTE* Data) const
{
	DWORD Result = 0;
	for (TFieldIterator<UProperty> It(Struct); It; ++It)
		Result ^= It->GetMapHash(Data + It->Offset);
	return Result;
}

IMPLEMENT_CLASS(UStructProperty);

/*-----------------------------------------------------------------------------
The End.
-----------------------------------------------------------------------------*/
