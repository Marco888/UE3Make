/*=============================================================================
	UnName.cpp: Unreal global name code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FName statics.
-----------------------------------------------------------------------------*/

// Static variables.
UBOOL				FName::Initialized = 0;
FNameEntry*			FName::NameHash[4096];
TArray<FNameEntry*>	FName::Names;
TArray<INT>         FName::Available;

// Register core names
#define NAMES_ONLY
#define AUTOGENERATE_NAME(name) FName CORE_##name;
#define AUTOGENERATE_FUNCTION(cls,idx,name)
#include "CoreClasses.h"
#undef AUTOGENERATE_FUNCTION
#undef AUTOGENERATE_NAME
#undef NAMES_ONLY

/*-----------------------------------------------------------------------------
	FName implementation.
-----------------------------------------------------------------------------*/

//
// Hardcode a name.
//
void FName::Hardcode( FNameEntry* AutoName )
{
	guard(FName::Hardcode);

	// Add name to name hash.
	INT iHash          = appStrihash(AutoName->Name) & (ARRAY_COUNT(NameHash)-1);
	AutoName->HashNext = NameHash[iHash];
	NameHash[iHash]    = AutoName;

	// Expand the table if needed.
	for( INT i=Names.Num(); i<=AutoName->Index; i++ )
		Names.AddItem( NULL );

	// Add name to table.
	if( Names(AutoName->Index) )
		appErrorf( TEXT("Hardcoded name %i was duplicated"), AutoName->Index );
	Names(AutoName->Index) = AutoName;
	unguard;
}

//
// FName constructor.
//
FName::FName( const TCHAR* Name, EFindName FindType )
{
	guard(FName::FName);
	check(Name);
	if( !Initialized )
		appErrorf( TEXT("FName constructed before FName::StaticInit") );

	// If empty or invalid name was specified, return NAME_None.
	if( !Name[0] )
	{
		Index = NAME_None;
		return;
	}

	// Try to find the name in the hash.
	INT iHash = appStrihash(Name) & (ARRAY_COUNT(NameHash) - 1);
	// Shambler (if statement is what I added)
	if (!GDuplicateNames)
	{
	// Try to find the name in the hash.
	for( FNameEntry* Hash=NameHash[iHash]; Hash; Hash=Hash->HashNext )
	{
		if( appStricmp( Name, Hash->Name )==0 )
		{
			// Found it in the hash.
			Index = Hash->Index;

			// If it already existed in the hash, but we are adding an auto-generated
			// native name entry, set the flag to prevent it being GC'd.
			if( FindType==FNAME_Intrinsic )
				Names(Index)->Flags |= RF_Native;
			return;
		}
	}
	}
	// Didn't find name.
	if( FindType==FNAME_Find )
	{
		// Not found.
		Index = NAME_None;
		return;
	}

	// Find an available entry in the name table.
	if( Available.Num() )
	{
		Index = Available( Available.Num()-1 );
		Available.Remove( Available.Num()-1 );
	}
	else
	{
		Index = Names.Add();
	}

	// Allocate and set the name.
	Names(Index) = NameHash[iHash] = AllocateNameEntry( Name, Index, 0, NameHash[iHash] );
	if( FindType==FNAME_Intrinsic )
		Names(Index)->Flags |= RF_Native;
	unguard;
}

FName FName::GetNumberedName(const FName& InName, INT InNumber)
{
	guard(FName::GetNumberedName);
	if (InNumber)
		return FName(*FString::Printf(TEXT("%ls_%i"), *InName, (InNumber - 1)));
	return InName;
	unguard;
}
FName FName::GrabDeNumberedName(const FName& InName, INT& OutNumber)
{
	guard(FName::GrabDeNumberedName);
	const TCHAR* Str = *InName;
	const TCHAR* S = Str;
	while (*S && *S != '_')
		++S;
	if (*S == '_' && S[1])
	{
		const TCHAR* End = S;
		++S;
		while (*S && appIsDigit(*S))
			++S;
		if (!*S)
		{
			FName RealName = FName(*FString(Str, End));
			++End;
			OutNumber = appAtoi(End) + 1;
			return RealName;
		}
	}
	OutNumber = 0;
	return InName;
	unguard;
}
void FName::SerializedNumberedName(FArchive& Ar) const
{
	guard(FName::SerializedNumberedName);
	const TCHAR* Str = **this;
	const TCHAR* S = Str;
	while (*S && *S != '_')
		++S;
	if (*S == '_' && S[1])
	{
		const TCHAR* End = S;
		++S;
		while (*S && appIsDigit(*S))
			++S;
		if (!*S)
		{
			FName RealName = FName(*FString(Str, End));
			++End;
			NAME_INDEX NameIndex = RealName.GetIndex();
			INT Num = appAtoi(End+1) + 1;
			Ar << NameIndex << Num;
			return;
		}
	}
	NAME_INDEX NameIndex = GetIndex();
	INT Num = 0;
	Ar << NameIndex << Num;
	return;
	unguard;
}

/*-----------------------------------------------------------------------------
	FName subsystem.
-----------------------------------------------------------------------------*/

CORE_API FNameEntry* AllocateNameEntry( const TCHAR* Name, DWORD Index, DWORD Flags, FNameEntry* HashNext )
{
	guard(AllocateNameEntry);
	FNameEntry* NameEntry = (FNameEntry*)appMalloc( sizeof(FNameEntry), TEXT("NameEntry") );
	NameEntry->Index      = Index;
	NameEntry->Flags      = Flags;
	NameEntry->HashNext   = HashNext;
	appStrncpy( NameEntry->Name, Name, ARRAY_COUNT(NameEntry->Name));
	return NameEntry;

	unguard;
}

//
// Initialize the name subsystem.
//
void FName::StaticInit()
{
	guard(FName::StaticInit);
	check(Initialized==0);
	check((ARRAY_COUNT(NameHash)&(ARRAY_COUNT(NameHash)-1)) == 0);
	Initialized = 1;

	// Init the name hash.
	INT NameHashCount = ARRAY_COUNT(FName::NameHash);
	{for( INT i=0; i < NameHashCount; i++ )
		NameHash[i] = NULL;}

	// Register all hardcoded names.
	#define REGISTER_NAME(num,namestr) \
		Hardcode(AllocateNameEntry(TEXT(#namestr),num,RF_Native,NULL));
	#define REG_NAME_HIGH(num,namestr) \
		Hardcode(AllocateNameEntry(TEXT(#namestr),num,RF_Native,NULL));
	#include "UnNames.h"

	// Register core names
	#define NAMES_ONLY
	#define AUTOGENERATE_NAME(name) CORE_##name = FName(TEXT(#name),FNAME_Intrinsic);
	#define AUTOGENERATE_FUNCTION(cls,idx,name)
	#include "CoreClasses.h"
	#undef AUTOGENERATE_FUNCTION
	#undef AUTOGENERATE_NAME
	#undef NAMES_ONLY

	INT NewNameHashCount ARRAY_COUNT(NameHash);
	// Verify no duplicate names.
	{for( INT i=0; i<NewNameHashCount; i++ )
		for( FNameEntry* Hash=NameHash[i]; Hash; Hash=Hash->HashNext )
			for( FNameEntry* Other=Hash->HashNext; Other; Other=Other->HashNext )
				if( appStricmp(Hash->Name,Other->Name)==0 )
					appErrorf( TEXT("Name '%ls' was duplicated"), Hash->Name );}

	debugf( NAME_Init, TEXT("Name subsystem initialized") );
	unguard;
}

//
// Shut down the name subsystem.
//
void FName::StaticExit()
{
	guard(FName::StaticExit);
	check(Initialized);

	// Kill all names.
	for( INT i=0; i<Names.Num(); i++ )
		if( Names(i) )
			delete Names(i);

	// Empty tables.
	Names.Empty();
	Available.Empty();
	Initialized = 0;

	debugf( NAME_Exit, TEXT("Name subsystem shut down") );
	unguard;
}

//
// Display the contents of the global name hash.
//
void FName::DisplayHash( FOutputDevice& Ar )
{
	guard(FName::DisplayHash);

	INT UsedBins=0, NameCount=0;
	INT NameHashCount= ARRAY_COUNT(NameHash);
	for( INT i=0; i < NameHashCount; i++ )
	{
		if( NameHash[i] != NULL ) UsedBins++;
		for( FNameEntry *Hash = NameHash[i]; Hash; Hash=Hash->HashNext )
			NameCount++;
	}
	Ar.Logf( TEXT("Hash: %i names, %i/%i hash bins"), NameCount, UsedBins, ARRAY_COUNT(NameHash) );

	unguard;
}

//
// Delete an name permanently; called by garbage collector.
//
void FName::DeleteEntry( INT i )
{
	guard(FName::DeleteEntry);

	// Unhash it.
	FNameEntry* NameEntry = Names(i);
	check(NameEntry);
	check(!(NameEntry->Flags & RF_Native));
	INT iHash = appStrihash(NameEntry->Name) & (ARRAY_COUNT(NameHash)-1);
	FNameEntry** HashLink;
	for( HashLink=&NameHash[iHash]; *HashLink && *HashLink!=NameEntry; HashLink=&(*HashLink)->HashNext );
	if( !*HashLink )
		appErrorf( TEXT("Unhashed name '%ls'"), NameEntry->Name );
	*HashLink = (*HashLink)->HashNext;

	// Delete it.
	delete NameEntry;
	Names(i) = NULL;
	Available.AddItem( i );

	unguard;
}

/*-----------------------------------------------------------------------------
	FNameEntry implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, FNameEntry& E )
{
	guard(FNameEntry<<);
	if (Ar.IsLoading())
	{
		FString Str;
		Ar << Str;
		appStrcpy(E.Name, *Str.Left(NAME_SIZE - 1));
	}
	else
	{
		FString Str(E.Name);
		Ar << Str;
	}
	if (Ar.IsSaving())
		E.Flags = (RF_LoadForClient | RF_LoadForServer | RF_LoadForEdit | RF_TagExp);
	return Ar << E.Flags;
	unguard;
}


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
