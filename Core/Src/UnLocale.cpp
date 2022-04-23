/*=============================================================================
	UnLocale.cpp: Unreal localization helpers.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Smirftsch
=============================================================================*/

#include "CorePrivate.h"

IMPLEMENT_CLASS(ULocale);

/*-----------------------------------------------------------------------------
	UMetaData implementation.
-----------------------------------------------------------------------------*/

void UMetaData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << ObjectMetaDataMap;

#if 0
	debugf(TEXT("METADATA %s"), *GetPathName());
	for (TMap<UObject*, TMap<FName, FString> >::TIterator It(ObjectMetaDataMap); It; ++It)
	{
		TMap<FName, FString>& MetaDataValues = It.Value();
		for (TMap<FName, FString>::TIterator MetaDataIt(MetaDataValues); MetaDataIt; ++MetaDataIt)
		{
			debugf(TEXT("%s: %s=%s"), *It.Key()->GetPathName(), *MetaDataIt.Key().ToString(), *MetaDataIt.Value());
		}
	}
#endif
}

/**
 * Return the value for the given key in the given property
 * @param Object the object to lookup the metadata for
 * @param Key The key to lookup
 * @return The value if found, otherwise an empty string
 */
const FString& UMetaData::GetValue(const UObject* Object, FName Key)
{
	// if not found, return a static empty string
	static FString EmptyString;

	// every key needs to be valid
	if (Key == NAME_None)
	{
		return EmptyString;
	}

	// look up the existing map if we have it
	TMap<FName, FString>* ObjectValues = ObjectMetaDataMap.Find(const_cast<UObject*>(Object));

	// if not, return empty
	if (ObjectValues == NULL)
	{
		return EmptyString;
	}

	// look for the property
	FString* ValuePtr = ObjectValues->Find(Key);

	// if we didn't find it, return NULL
	if (!ValuePtr)
	{
		return EmptyString;
	}

	// if we found it, return the pointer to the character data
	return *ValuePtr;

}

/**
 * Return the value for the given key in the given property
 * @param Object the object to lookup the metadata for
 * @param Key The key to lookup
 * @return The value if found, otherwise an empty string
 */
const FString& UMetaData::GetValue(const UObject* Object, const TCHAR* Key)
{
	// only find names, don't bother creating a name if it's not already there
	// (GetValue will return an empty string if Key is NAME_None)
	return GetValue(Object, FName(Key, FNAME_Find));
}

/**
 * Return whether or not the Key is in the meta data
 * @param Object the object to lookup the metadata for
 * @param Key The key to query for existence
 * @return TRUE if found
 */
UBOOL UMetaData::HasValue(const UObject* Object, FName Key)
{
	// every key needs to be valid
	if (Key == NAME_None)
	{
		return FALSE;
	}

	// look up the existing map if we have it
	TMap<FName, FString>* ObjectValues = ObjectMetaDataMap.Find(const_cast<UObject*>(Object));

	// if not, return FALSE
	if (ObjectValues == NULL)
	{
		return FALSE;
	}

	// if we had the map, see if we had the key
	return ObjectValues->Find(Key) != NULL;
}

/**
 * Return whether or not the Key is in the meta data
 * @param Object the object to lookup the metadata for
 * @param Key The key to query for existence
 * @return TRUE if found
 */
UBOOL UMetaData::HasValue(const UObject* Object, const TCHAR* Key)
{
	// only find names, don't bother creating a name if it's not already there
	// (HasValue will return FALSE if Key is NAME_None)
	return HasValue(Object, FName(Key, FNAME_Find));
}

/**
 * Is there any metadata for this property?
 * @param Object the object to lookup the metadata for
 * @return TrUE if the property has any metadata at all
 */
UBOOL UMetaData::HasObjectValues(const UObject* Object)
{
	return ObjectMetaDataMap.Find(const_cast<UObject*>(Object)) != NULL;
}

/**
 * Set the key/value pair in the Property's metadata
 * @param Object the object to set the metadata for
 * @Values The metadata key/value pairs
 */
void UMetaData::SetObjectValues(const UObject* Object, const TMap<FName, FString>& ObjectValues)
{
	ObjectMetaDataMap.Set(const_cast<UObject*>(Object), ObjectValues);
}

/**
 * Set the key/value pair in the Property's metadata
 * @param Object the object to set the metadata for
 * @param Key A key to set the data for
 * @param Value The value to set for the key
 * @Values The metadata key/value pairs
 */
void UMetaData::SetValue(const UObject* Object, FName Key, const FString& Value)
{
	check(Key != NAME_None);

	// look up the existing map if we have it
	TMap<FName, FString>* ObjectValues = ObjectMetaDataMap.Find(const_cast<UObject*>(Object));

	// if not, create an empty map
	if (ObjectValues == NULL)
	{
		ObjectValues = &ObjectMetaDataMap.Set(const_cast<UObject*>(Object), TMap<FName, FString>());
	}

	// set the value for the key
	ObjectValues->Set(Key, *Value);
}

/**
 * Set the key/value pair in the Property's metadata
 * @param Object the object to set the metadata for
 * @param Key A key to set the data for
 * @param Value The value to set for the key
 * @Values The metadata key/value pairs
 */
void UMetaData::SetValue(const UObject* Object, const TCHAR* Key, const FString& Value)
{
	SetValue(Object, FName(Key), Value);
}

/**
 * Method to help support metadata for intrinsic classes. Attempts to find and set meta data for a given intrinsic class and subobject
 * within an ini file. The keys in the ini file are expected to be in the format of ClassToUseName.ObjectToUseName, where all of the characters
 * in the names are alphanumeric.
 *
 * @param	ClassToUse			Intrinsic class to attempt to find meta data for
 * @param	ObjectToUse			Sub-object, such as a property or enum, of the provided intrinsic class
 * @param	MetaDataToAddTo		Metadata object that any parsed metadata found in the ini will be added to
 *
 * @return	TRUE if any data was found, parsed, and added; FALSE otherwise
 */
UBOOL UMetaData::AttemptParseIntrinsicMetaData(const UClass& ClassToUse, const UObject& ObjectToUse, UMetaData& MetaDataToAddTo)
{
	// Assume no data will be parsed
	UBOOL bParsedData = FALSE;

	// Only try to parse meta data inside the editor
	if (GIsEditor)
	{
		static TArray<FString> ParsedIntrinsicMetaData;

		const FString& ObjectName = ObjectToUse.GetName();

		// Construct a string in the form of ClassToUseName_ObjectToUseName in order to uniquely identify this object
		const FString& ClassAndObjName = FString::Printf(TEXT("%s_%s"), *ClassToUse.GetName(), *ObjectName);

		// If class to use is intrinsic and the class/object combo hasn't had an attempted parse performed for it before, try to parse metadata from an ini file
		if (ClassToUse.HasAnyClassFlags(CLASS_Intrinsic) && ParsedIntrinsicMetaData.FindItemIndex(ClassAndObjName)==INDEX_NONE && GConfig)
		{
			// Add this ClassToUseName_ObjectToUseName to the TSet to mark it as having been parsed
			new (ParsedIntrinsicMetaData) FString(ClassAndObjName);

			// Construct a string in the form of ClassToUseName_ObjectToUseName where ObjectToUseName is modified to only contain alphanumeric characters
			FString AlphaNumClassAndObjName = FString(ClassToUse.GetName()) + TEXT("_");
			for (INT CharIndex = 0; CharIndex < ObjectName.Len(); ++CharIndex)
			{
				if (appIsAlnum(ObjectName[CharIndex]))
				{
					AlphaNumClassAndObjName += ObjectName[CharIndex];
				}
			}

			// Query the ini file for a key in the form ClassToUseName_ObjectToUseName
			FString IntrinsicMetaData;
			if (GConfig->GetString(TEXT("IntrinsicMetaData"), *AlphaNumClassAndObjName, IntrinsicMetaData))
			{
				// If the ini file had a string for the class/object combo, it should specify any number of metadata keys/values, each delimited
				// by the | character. Parse the string into an array split on the | character.
				TArray<FString> MetaDataSubStrings;
				IntrinsicMetaData.ParseIntoArray(&MetaDataSubStrings, TEXT("|"), TRUE);

				// Ensure that the parsed array has an even number of elements. If it doesn't, then each metadata key does not have a corresponding
				// value and the data is likely incorrect and should be ignored.
				if ((MetaDataSubStrings.Num() & 1) == 0)
				{
					// Loop through each key/value pair, adding them to the meta data for the provided object
					for (INT MetaDataIndex = 0; MetaDataIndex + 1 < MetaDataSubStrings.Num(); MetaDataIndex += 2)
					{
						// Trim any stray whitespace from the key/value pairs
						FString& CurKey = MetaDataSubStrings(MetaDataIndex);
						CurKey = CurKey.Trim();
						CurKey = CurKey.TrimTrailing();

						FString& CurValue = MetaDataSubStrings(MetaDataIndex + 1);
						CurValue = CurValue.Trim();
						CurValue = CurValue.TrimTrailing();

						// Ensure the current key doesn't already have metadata for it, and if it doesn't, add the new metadata
						if (!MetaDataToAddTo.HasValue(&ObjectToUse, *CurKey))
						{
							MetaDataToAddTo.SetValue(&ObjectToUse, *CurKey, CurValue);

							// Confirm that data has been parsed
							bParsedData = TRUE;
						}
					}
				}
			}
		}
	}

	return bParsedData;
}

/**
 * Removes any metadata entries that are to objects not inside the same package as this UMetaData object.
 */
void UMetaData::RemoveMetaDataOutsidePackage()
{
	TArray<UObject*> ObjectsToRemove;

	// Get the package that this MetaData is in
	UPackage* MetaDataPackage = GetOutermost();

	// Iterate over all entries..
	for (TMap<UObject*, TMap<FName, FString> >::TIterator It(ObjectMetaDataMap); It; ++It)
	{
		UObject* Obj = It.Key();
		// See if its package is not the same as the MetaData's
		if ((Obj != NULL) && (Obj->GetOutermost() != MetaDataPackage))
		{
			// Add to list of things to remove
			ObjectsToRemove.AddItem(Obj);
		}
	}

	// Go through and remove any objects that need it
	for (INT i = 0; i < ObjectsToRemove.Num(); i++)
	{
		UObject* Obj = ObjectsToRemove(i);

		debugf(TEXT("Removing '%s' ref from Metadata '%s'"), *Obj->GetPathName(), *GetPathName());
		ObjectMetaDataMap.Remove(Obj);
	}
}

IMPLEMENT_CLASS(UMetaData);
