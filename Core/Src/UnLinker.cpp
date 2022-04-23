/*=============================================================================
	UnLinker.cpp: Unreal object linker.
	Copyright 2000 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
    * Added MD5 checksum by Michiel Hendriks
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	Hash function.
-----------------------------------------------------------------------------*/

inline INT HashNames( FName A, FName B, FName C )
{
	return A.GetIndex() + 7 * B.GetIndex() + 31*C.GetIndex();
}

static UObject* _LoadingObj = NULL;

/*-----------------------------------------------------------------------------
	FObjectExport.
-----------------------------------------------------------------------------*/

FObjectExport::FObjectExport()
:	ClassIndex		( 0															)
,	SuperIndex		( 0															)
,	PackageIndex	( 0															)
,	ObjectName		( FName(NAME_None)	                                        )
,	ObjectFlags		( 0			                                                )
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	_Object			( NULL													    )
,	_iHashNext		( INDEX_NONE												)
,	ExportFlags		( 0															)
{}

FObjectExport::FObjectExport( UObject* InObject )
:	ClassIndex		( 0															)
,	SuperIndex		( 0															)
,	PackageIndex	( 0															)
,	ObjectName		( InObject ? (InObject->GetFName()			) : FName(NAME_None)	)
,	ObjectFlags		( InObject ? (InObject->GetFlags() & RF_Load) : 0			)
,	SerialSize		( 0															)
,	SerialOffset	( 0															)
,	_Object			( InObject													)
,	ArchetypeIndex	( 0															)
,	_iHashNext		( INDEX_NONE												)
,	PackageGuid		( FGuid(0,0,0,0)											)
,	PackageFlags	( 0															)
,	ExportFlags		( 0															)
{}

FArchive& operator<<(FArchive& Ar, FObjectExport& E)
{
	guard(FObjectExport << );

	Ar << E.ClassIndex;
	Ar << E.SuperIndex;
	Ar << E.PackageIndex;
	Ar << E.ObjectName;
	Ar << E.ArchetypeIndex;
	Ar << E.ObjectFlags;

	Ar << E.SerialSize;
	Ar << E.SerialOffset;

	if (Ar.Ver() < VER_REMOVED_COMPONENT_MAP)
	{
		TMap<FName, INT>	LegacyComponentMap;
		Ar << LegacyComponentMap;
	}

	Ar << E.ExportFlags;

	Ar << E.GenerationNetObjectCount;
	Ar << E.PackageGuid;
	Ar << E.PackageFlags;

	return Ar;
	unguard;
}

/*-----------------------------------------------------------------------------
	FObjectImport.
-----------------------------------------------------------------------------*/

FObjectImport::FObjectImport()
:	SourceLinker	( NULL														)
,	XObject(NULL)
{}

FObjectImport::FObjectImport( UObject* InObject )
:	ClassPackage	( InObject->GetClass()->GetOuter()->GetFName())
,	ClassName		( InObject->GetClass()->GetFName()		 )
,	PackageIndex	( 0                                      )
,	ObjectName		( InObject->GetFName()					 )
,	XObject			( InObject								 )
,	SourceLinker	( NULL									 )
,	SourceIndex		( INDEX_NONE							 )
{
	if( XObject )
		UObject::GImportCount++;
}

FArchive& operator<<(FArchive& Ar, FObjectImport& I)
{
	guard(FObjectImport << );

	Ar << I.ClassPackage << I.ClassName;
	Ar << I.PackageIndex;
	Ar << I.ObjectName;
	if (Ar.IsLoading())
	{
		I.SourceLinker = NULL;
		I.SourceIndex = INDEX_NONE;
		I.XObject = NULL;
	}
	return Ar;

	unguard;
}

FArchive& operator<<(FArchive& Ar, FPackageFileSummary& Sum)
{
	guard(FUnrealfileSummary << );

	Ar << Sum.Tag;
	Ar << Sum.FileVersion;
	//debugf(TEXT("Ver %i (%i - %i)"), Sum.FileVersion, Sum.GetFileVersion(), Sum.GetFileVersionLicensee()); -> 73204583 (871 - 1117)
	Ar << Sum.TotalHeaderSize;
	Ar << Sum.FolderName;
	Ar << Sum.PackageFlags;
	Ar << Sum.NameCount << Sum.NameOffset;
	Ar << Sum.ExportCount << Sum.ExportOffset;
	Ar << Sum.ImportCount << Sum.ImportOffset;
	Ar << Sum.DependsOffset;

	if (Sum.GetFileVersion() >= VER_ADDED_CROSSLEVEL_REFERENCES)
	{
		Ar << Sum.ImportExportGuidsOffset << Sum.ImportGuidsCount << Sum.ExportGuidsCount;
	}
	else
	{
		Sum.ImportExportGuidsOffset = INDEX_NONE;
	}

	if (Sum.GetFileVersion() >= VER_ASSET_THUMBNAILS_IN_PACKAGES)
	{
		Ar << Sum.ThumbnailTableOffset;
	}

	INT GenerationCount = Sum.Generations.Num();
	Ar << Sum.Guid << GenerationCount;
	if (Ar.IsLoading() && GenerationCount > 0)
	{
		Sum.Generations.Empty(GenerationCount);
		Sum.Generations.Add(GenerationCount);
	}
	for (INT i = 0; i < GenerationCount; i++)
	{
		Ar << Sum.Generations(i);
	}
	
	Ar << Sum.EngineVersion;
	//debugf(TEXT("EngineVersion %i"), Sum.EngineVersion); -> 10897

	// grab the CookedContentVersion if we are saving while cooking or loading 
	if (Ar.IsLoading() == TRUE)
	{
		Ar << Sum.CookedContentVersion;
	}
	else
	{
		INT Temp = 0;
		Ar << Temp;  // just put a zero as it not a cooked package and we should not dirty the waters
	}

	Ar << Sum.CompressionFlags;
	Ar << Sum.CompressedChunks;
	Ar << Sum.PackageSource;

	// serialize the array of additional packages to cook if the version is high enough
	if (Sum.GetFileVersion() >= VER_ADDITIONAL_COOK_PACKAGE_SUMMARY)
	{
		Ar << Sum.AdditionalPackagesToCook;
	}

	if (Sum.GetFileVersion() >= VER_TEXTURE_PREALLOCATION)
	{
		Ar << Sum.TextureAllocations;
	}

	return Ar;
	unguard;
}

/*----------------------------------------------------------------------------
	ULinker.
----------------------------------------------------------------------------*/

ULinker::ULinker( UObject* InRoot, const TCHAR* InFilename )
:	LinkerRoot( InRoot )
,	Summary()
,	Success( 123456 )
,	Filename( InFilename )
,	_ContextFlags( 0 )
{
	check(LinkerRoot);
	check(InFilename);

	// Set context flags.
	if( GIsEditor ) _ContextFlags |= RF_LoadForEdit;
	if( GIsClient ) _ContextFlags |= RF_LoadForClient;
	if( GIsServer ) _ContextFlags |= RF_LoadForServer;
}

void
ULinker::Serialize( FArchive& Ar )
{
	guard(ULinker::Serialize);
	Super::Serialize( Ar );

	// Sizes.
	ImportMap	.CountBytes( Ar );
	ExportMap	.CountBytes( Ar );

	// Prevent garbage collecting of linker's names and package.
	Ar << NameMap << LinkerRoot;
	{for( INT i=0; i<ExportMap.Num(); i++ )
	{
		FObjectExport& E = ExportMap(i);
		Ar << E.ObjectName;
	}}
	{for( INT i=0; i<ImportMap.Num(); i++ )
	{
		FObjectImport& I = ImportMap(i);
		Ar << *(UObject**)&I.SourceLinker;
		Ar << I.ClassPackage << I.ClassName;
	}}
	unguard;
}

// ULinker interface.
FString
ULinker::GetImportFullName( INT i )
{
	guard(ULinkerLoad::GetImportFullName);
	FString S;
	for( INT j=-i-1; j!=0; j=ImportMap(-j-1).PackageIndex )
	{
		if( j != -i-1 )
			S = US + TEXT(".") + S;
		S = FString(*ImportMap(-j-1).ObjectName) + S;
	}
	return FString(*ImportMap(i).ClassName) + TEXT(" ") + S ;
	unguard;
}

FString
ULinker::GetExportFullName( INT i, const TCHAR* FakeRoot )
{
	guard(ULinkerLoad::GetExportFullName);
	FString S;
	for( INT j=i+1; j!=0; j=ExportMap(j-1).PackageIndex )
	{
		if( j != i+1 )
			S = US + TEXT(".") + S;
		S = FString(*ExportMap(j-1).ObjectName) + S;
	}
	INT ClassIndex = ExportMap(i).ClassIndex;
	FName ClassName = ClassIndex>0 ? ExportMap(ClassIndex-1).ObjectName : ClassIndex<0 ? ImportMap(-ClassIndex-1).ObjectName : NAME_Class;
	return FString(*ClassName) + TEXT(" ") + (FakeRoot ? FakeRoot : LinkerRoot->GetPathName()) + TEXT(".") + S;
	unguard;
}

/*----------------------------------------------------------------------------
	ULinkerLoad.
----------------------------------------------------------------------------*/

ULinkerLoad::ULinkerLoad( UObject* InParent, const TCHAR* InFilename, DWORD InLoadFlags )
:	ULinker( InParent, InFilename )
,	LoadFlags( InLoadFlags )
{
	guard(ULinkerLoad::ULinkerLoad);
	if(!(GUglyHackFlags & 2))
		debugf(NAME_DevLoad, TEXT("%1.1fms Loading: %ls (%ls - %i bytes)"), (appSeconds().GetFloat() * 1000.0), InParent->GetFullName(), InFilename, GFileManager->FileSize(InFilename));
	Loader = GFileManager->CreateFileReader( InFilename, 0, GWarn );
	if( !Loader )
		appThrowf( TEXT("Error opening file") );

	// Error if linker already loaded.
	{for( INT i=0; i<GObjLoaders.Num(); i++ )
		if( GetLoader(i)->LinkerRoot == LinkerRoot )
			appThrowf( TEXT("Linker for '%s' already exists"), LinkerRoot->GetName() );}

	// Begin.
	GWarn->StatusUpdatef( 0, 0, TEXT("Loading file %ls..."), *Filename );

	// Set status info.
	guard(InitAr);
	ArVer       = PACKAGE_FILE_VERSION;
	ArLicenseeVer = PACKAGE_FILE_VERSION_LICENSEE; //LVer added by Legend on 4/12/2000
	ArIsLoading = ArIsPersistent = 1;
	ArForEdit   = GIsEditor;
	ArForClient = 1;
	ArForServer = 1;
	unguard;

	// Read summary from file.
	guard(LoadSummary);
	*this << Summary;
	ArVer = Summary.GetFileVersion(); //LVer added by Legend on 4/12/2000
	ArLicenseeVer = Summary.GetFileVersionLicensee(); //LVer added by Legend on 4/12/2000
	unguard;

	// Check tag.
	guard(CheckTag);
	if( Summary.Tag != PACKAGE_FILE_TAG )
		appThrowf( TEXT("The file '%s' contains unrecognizable data"), *Filename );
	unguard;

	// DEBUG: Check depends maps.
	/*{
		Seek(Summary.DependsOffset);
		for (INT i = 0; i < Summary.ExportCount; i++)
		{
			TArray<INT> Depends;
			*this << Depends;

			debugf(TEXT("DependsonFor %i: %i"), i, Depends.Num());
			for(INT j=0; j< Depends.Num(); ++j)
				debugf(TEXT("-> %i"), i, Depends(j));
		}
	}*/

	// Slack everything according to summary.
	ImportMap   .Empty( Summary.ImportCount   );
	ExportMap   .Empty( Summary.ExportCount   );
	NameMap		.Empty( Summary.NameCount     );

	// Load and map names.
	guard(LoadNames);
	if( Summary.NameCount > 0 )
	{
		Seek( Summary.NameOffset );
		for( INT i=0; i<Summary.NameCount; i++ )
		{
			// Read the name entry from the file.
			FNameEntry NameEntry;

            // explicitly init fields to prevent Valgrind whining. --ryan.
            NameEntry.Index = 0;
            NameEntry.Flags = 0;
            NameEntry.HashNext = NULL;
            appMemzero(NameEntry.Name, sizeof(NameEntry.Name));

			*this << NameEntry;

			// Add it to the name table if it's needed in this context.
			NameMap.AddItem( FName( NameEntry.Name, FNAME_Add ) );
		}
	}
	unguard;

	// Load import map.
	guard(LoadImportMap);
	if( Summary.ImportCount > 0 )
	{
		Seek( Summary.ImportOffset );
		for (INT i = 0; i < Summary.ImportCount; i++)
			*this << *new(ImportMap)FObjectImport;
	}
	unguard;

	// Load export map.
	guard(LoadExportMap);
	if( Summary.ExportCount > 0 )
	{
		Seek( Summary.ExportOffset );
		for( INT i=0; i<Summary.ExportCount; i++ )
			*this << *new(ExportMap)FObjectExport;
	}
	unguard;

	// Create export hash.
	//warning: Relies on import & export tables, so must be done here.
	INT ExportHashCount = ARRAY_COUNT(ExportHash);
	{for( INT i=0; i<ExportHashCount; i++ )
	{
		ExportHash[i] = INDEX_NONE;
	}}
	{for( INT i=0; i<ExportMap.Num(); i++ )
	{
		INT iHash = HashNames( ExportMap(i).ObjectName, GetExportClassName(i), GetExportClassPackage(i) ) & (ARRAY_COUNT(ExportHash)-1);
		ExportMap(i)._iHashNext = ExportHash[iHash];
		ExportHash[iHash] = i;
	}}

	// Add this linker to the object manager's linker array.
	GObjLoaders.AddItem( this );
	if( !(LoadFlags & LOAD_NoVerify) )
		Verify();

	// Success.
	Success = 1;

	unguard;
}

void ULinkerLoad::Verify()
{
	guard(ULinkerLoad::Verify);
	if( !Verified )
	{
		INT iImport = 0;
		TCHAR* LastError = NULL;

		while (iImport < Summary.ImportCount)
		{
			try
			{
				// Validate all imports and map them to their remote linkers.
				guard(ValidateImports);
				while (iImport < Summary.ImportCount)
					VerifyImport(iImport++);
				unguard;
			}
			// !! PSX2 gcc doesn't like catch( TCHAR* Error )
#if UNICODE
			catch (TCHAR* Error)
#else
			catch (char* Error)
#endif
			{
				if (!LastError)
				{
					LastError = appStaticString1024();
					appStrncpy(LastError, Error, 1024);
				}
			}
		}
		if (LastError)
		{
			GObjLoaders.RemoveItem(this);
			appThrowf(LastError);
		}
	}
	Verified=1;
	unguard;
}

FName
ULinkerLoad::GetExportClassPackage( INT i )
{
	guardSlow(ULinkerLoad::GetExportClassPackage);
	FObjectExport& Export = ExportMap( i );
	if( Export.ClassIndex < 0 )
	{
		FObjectImport& Import = ImportMap( -Export.ClassIndex-1 );
		checkSlow(Import.PackageIndex<0);
		return ImportMap( -Import.PackageIndex-1 ).ObjectName;
	}
	else if( Export.ClassIndex > 0 )
	{
		return LinkerRoot->GetFName();
	}
	else
	{
		return NAME_Core;
	}
	unguardSlow;
}

FName
ULinkerLoad::GetExportClassName( INT i )
{
	guardSlow(GetExportClassName);
	FObjectExport& Export = ExportMap(i);
	if( Export.ClassIndex < 0 )
	{
		return ImportMap( -Export.ClassIndex-1 ).ObjectName;
	}
	else if( Export.ClassIndex > 0 )
	{
		return ExportMap( Export.ClassIndex-1 ).ObjectName;
	}
	else
	{
		return NAME_Class;
	}
	unguardSlow;
}

ULinkerLoad* ULinkerLoad::GetSourceLinker(INT iLinkerImport)
{
	guard(ULinkerLoad::GetSourceLinker);
	FObjectImport* Top = GetTopImport(iLinkerImport);
	if (!Top->SourceLinker)
	{
		if (Top->ObjectName == NAME_Invalid)
			return nullptr;
		try
		{
			//debugf(TEXT("CreateLinker %ls"), *Top->ObjectName);
			Top->SourceLinker = GetPackageLinker(Top->XObject, NULL, LOAD_Throw | (LoadFlags & LOAD_Propagate), LinkerRoot);
		}
		catch (const TCHAR* Error)//oldver
		{
			Top->ObjectName = NAME_Invalid;
			if (LoadFlags & LOAD_Forgiving)
			{
				debugf(TEXT("Broken import: %ls (%ls)"), *GetImportFullName(iLinkerImport), Error);
				return nullptr;
			}
			appThrowf(Error);
		}
	}
	return Top->SourceLinker;
	unguard;
}

// Safely verify an import.
void ULinkerLoad::VerifyImport( INT i )
{
	guard(ULinkerLoad::VerifyImport);
	FObjectImport& Import = ImportMap(i);
	if
	(	(Import.SourceLinker && Import.SourceIndex != INDEX_NONE)
	||	Import.ClassPackage	== NAME_None
	||	Import.ClassName	== NAME_None
	||	Import.ObjectName	== NAME_None )
	{
		// Already verified, or not relevent in this context.
		return;
	}

	// Find or load this import's linker.
	UPackage* Pkg = NULL;
	UObject* ParentObj = NULL;
	if( Import.PackageIndex == 0 )
	{
		if (Import.ObjectName == NAME_Invalid)
			return;

		check(Import.ClassName==NAME_Package);
		check(Import.ClassPackage==NAME_Core);

		Pkg = FindObject<UPackage>(NULL, *Import.ObjectName, 1);
		if (!Pkg)
		{
			Import.XObject = CreatePackage(NULL, *Import.ObjectName);
			GetSourceLinker(i);
		}
		//else debugf(TEXT("FoundPreLinker %ls"), *Import.ObjectName);
		Import.XObject = Pkg;
		Import.ClassPackage = NAME_None;
		return;
	}
	else
	{
		check(Import.PackageIndex<0);
		VerifyImport( -Import.PackageIndex-1 );
		ParentObj = ImportMap(-Import.PackageIndex - 1).XObject;
		FObjectImport* Top = GetTopImport(i);
		Pkg = Cast<UPackage>(Top->XObject);
		if (ParentObj && Pkg && Pkg->bIsAssetPackage)
		{
			UObject* ClassPck = StaticFindObject(UObject::StaticClass(), NULL, *Import.ClassPackage, 0);
			UClass* ObjClass = ClassPck ? FindObject<UClass>(ClassPck, *Import.ClassName, 1) : nullptr;

			if (ObjClass)
			{
				Import.XObject = StaticFindObject(ObjClass, ParentObj, *Import.ObjectName, 1);
				if (Import.XObject)
				{
					Import.ClassPackage = NAME_None;
					return;
				}
				Import.XObject = StaticConstructObject(ObjClass, ParentObj, Import.ObjectName, RF_Public);
				Import.ClassPackage = NAME_None;
				return;
			}
		}
		Import.SourceLinker = GetSourceLinker(i);
	}
	// Find this import within its existing linker.
	UBOOL SafeReplace = 0;
	//new:
	INT iHash = HashNames( Import.ObjectName, Import.ClassName, Import.ClassPackage) & (ARRAY_COUNT(ExportHash)-1);
	if( Import.SourceLinker )
	{
		for( INT j=Import.SourceLinker->ExportHash[iHash]; j!=INDEX_NONE; j=Import.SourceLinker->ExportMap(j)._iHashNext )
		{
			FObjectExport& Source = Import.SourceLinker->ExportMap( j );
			if
			(	(Source.ObjectName	                          ==Import.ObjectName               )
			&&	(Import.SourceLinker->GetExportClassName   (j)==Import.ClassName                )
			&&  (Import.SourceLinker->GetExportClassPackage(j)==Import.ClassPackage) )
			{
				if( Import.PackageIndex<0 )
				{
					FObjectImport& ParentImport = ImportMap(-Import.PackageIndex-1);
					if( ParentImport.SourceLinker )
					{
						if( ParentImport.SourceIndex==INDEX_NONE )
						{
							if( Source.PackageIndex!=0 )
							{
								continue;
							}
						}
						else if( ParentImport.SourceIndex+1 != Source.PackageIndex )
						{
							if( Source.PackageIndex!=0 )
							{
								continue;
							}
						}
					}
				}
				Import.SourceIndex = j;
				break;
			}
		}
	}

	// If not found in file, see if it's a public native transient class.
	if( Import.SourceIndex==INDEX_NONE && Pkg!=NULL )
	{
		UObject* ClassPackage = FindObject<UPackage>( NULL, *Import.ClassPackage );
		if( ClassPackage )
		{
			UClass* FindClass = FindObject<UClass>( ClassPackage, *Import.ClassName );
			if( FindClass )
			{
				Import.XObject = StaticFindObject(FindClass, ParentObj ? ParentObj : Pkg, *Import.ObjectName);
				GImportCount++;
			}
		}
		if( !Import.XObject && !SafeReplace )
		{
			debugf(TEXT("Failed import: %ls %ls (file %ls)"), *Import.ClassName, *GetImportFullName(i), *Import.SourceLinker->Filename);
			if( LoadFlags & LOAD_Forgiving )
				return;
			appThrowf( TEXT("Can't find %ls %ls in file '%ls'"), *Import.ClassName, *GetImportFullName(i), *Import.SourceLinker->Filename);
		}
	}
	unguard;
}

// Load all objects; all errors here are fatal.
void ULinkerLoad::LoadAllObjects()
{
	guard(ULinkerLoad::LoadAllObjects);
	for( INT i=0; i<Summary.ExportCount; i++ )
		CreateExport( i );
	unguardobj;
}

// Find the index of a specified object.
//!!without regard to specific package
INT ULinkerLoad::FindExportIndex( FName ClassName, FName ClassPackage, FName ObjectName, INT PackageIndex )
{
	guard(ULinkerLoad::FindExportIndex);
	INT iHash = HashNames( ObjectName, ClassName, ClassPackage ) & (ARRAY_COUNT(ExportHash)-1);
	for( INT i=ExportHash[iHash]; i!=INDEX_NONE; i=ExportMap(i)._iHashNext )
	{
		if
		(  (ExportMap(i).ObjectName  ==ObjectName                              )
		&& (ExportMap(i).PackageIndex==PackageIndex || PackageIndex==INDEX_NONE)
		&& (GetExportClassPackage(i) ==ClassPackage                            )
		&& (GetExportClassName   (i) ==ClassName                               ) )
		{
			return i;
		}
	}

	// If an object with the exact class wasn't found, look for objects with a subclass of the requested class.

	for(INT ExportIndex = 0;ExportIndex < ExportMap.Num();ExportIndex++)
	{
		FObjectExport&	Export = ExportMap(ExportIndex);

		if(Export.ObjectName == ObjectName && (PackageIndex == INDEX_NONE || Export.PackageIndex == PackageIndex))
		{
			UClass*	ExportClass = Cast<UClass>(IndexToObject(Export.ClassIndex));

			// See if this export's class inherits from the requested class.

			for(UClass* ParentClass = ExportClass;ParentClass;ParentClass = ParentClass->GetSuperClass())
			{
				if(ParentClass->GetFName() == ClassName)
					return ExportIndex;
			}
		}
	}

	return INDEX_NONE;
	unguard;
}

// Create a single object.
UObject*
ULinkerLoad::Create( UClass* ObjectClass, FName ObjectName, DWORD LoadFlags, UBOOL Checked )
{
	guard(ULinkerLoad::Create);
	//old:
	//for( INT i=0; i<ExportMap.Num(); i++ )
	//new:
	INT Index = FindExportIndex( ObjectClass->GetFName(), ObjectClass->GetOuter()->GetFName(), ObjectName, INDEX_NONE );
	if( Index!=INDEX_NONE )
		return (LoadFlags & LOAD_Verify) ? (UObject*)-1 : CreateExport(Index);
	if( Checked )
		appThrowf( TEXT("%s %s not found for creation"), ObjectClass->GetName(), *ObjectName );
	return NULL;
	unguard;
}

void
ULinkerLoad::Preload( UObject* Object )
{
	guard(ULinkerLoad::Preload);
	check(IsValid());
	if(Object && Object->GetLinker()==this)
	{
		// Preload the object if necessary.
		if( Object->GetFlags() & RF_NeedLoad )
		{
			// If this is a struct, preload its super.
			if(	Object->IsA(UStruct::StaticClass()) )
				if( ((UStruct*)Object)->SuperStruct)
					Preload( ((UStruct*)Object)->SuperStruct);

			// Load the local object now.
			guard(LoadObject);
			FObjectExport& Export = ExportMap( Object->_LinkerIndex );
			check(Export._Object==Object);
			INT SavedPos = Loader->Tell();
			Loader->Seek( Export.SerialOffset );
			Loader->Precache( Export.SerialSize );

			// Load the object.
			//debugf(TEXT("Serialize: %ls: %i"), Object->GetFullName(), Export.SerialSize);
			Object->ClearFlags ( RF_NeedLoad );
			UObject* OldLoader = _LoadingObj;
			_LoadingObj = Object;
			if (Object->HasAnyFlags(RF_ClassDefaultObject))
			{
				// In order to ensure that the CDO inherits config & localized property values from the parent class, we can't initialize the CDO until
				// the parent class's CDO has serialized its data from disk and called LoadConfig/LoadLocalized. Since ULinkerLoad::Preload guarantees that
				// Preload is always called on the archetype first, at this point we know that the CDO's archetype (the CDO of the parent class) has already
				// loaded its config/localized data, so it's now safe to initialize the CDO
				Object->InitClassDefaultObject(Object->GetClass());
				Object->GetClass()->SerializeDefaultObject(Object, *this);
			}
			else Object->Serialize  ( *this );
			ArIsError = 0;
			_LoadingObj = OldLoader;

			// Make sure we serialized the right amount of stuff.
			if( (Tell()-Export.SerialOffset) > Export.SerialSize )
				GWarn->Logf( TEXT("%s: Serial size mismatch: Got %i, Expected %i"), Object->GetFullName(), Tell()-Export.SerialOffset, Export.SerialSize );
			Loader->Seek( SavedPos );
			DEBUG_OBJECTS(Object->GetName());
			unguardf(( TEXT("(%ls %i==%i/%i %i %i)"), Object->GetFullName(), Loader->Tell(), Loader->Tell(), Loader->TotalSize(), ExportMap( Object->_LinkerIndex ).SerialOffset, ExportMap( Object->_LinkerIndex ).SerialSize ));
		}
	}
	else if( Object->GetLinker() )
	{
		// Send to the object's linker.
		Object->GetLinker()->Preload( Object );
	}
	unguard;
}

UObject*
ULinkerLoad::CreateExport( INT Index )
{
	guard(ULinkerLoad::CreateExport);

	// Map the object into our table.
	FObjectExport& Export = ExportMap( Index );
	if( !Export._Object )
	{
		check(Export.ObjectName!=NAME_None || !(Export.ObjectFlags&RF_Public));

		// Get the object's class.
		UClass* LoadClass = (UClass*)IndexToObject( Export.ClassIndex );
		if( !LoadClass )
			LoadClass = UClass::StaticClass();
		check(LoadClass);
		check(LoadClass->GetClass()==UClass::StaticClass());

		Preload( LoadClass );

		if (Export.ObjectName == NAME_None)
			return NULL;

		// Get the outer object. If that caused the object to load, return it.
		UObject* ThisParent = Export.PackageIndex ? IndexToObject(Export.PackageIndex) : LinkerRoot;
		if( Export._Object )
			return Export._Object;

		// Find the Archetype object for the one we are loading.
		UObject* Template = NULL;
		if (Export.ArchetypeIndex != 0)
		{
			Template = IndexToObject(Export.ArchetypeIndex);

			// we couldn't load the exports's ObjectArchetype; unless the object's class is deprecated, issue a warning that the archetype is missing
			if (Template == NULL && !LoadClass->HasAnyClassFlags(CLASS_Deprecated))
			{
				// otherwise, log a warning and just use the class default object
				FString ArchetypeName;
				if (Export.ArchetypeIndex<0)
					ArchetypeName = GetImportFullName(-Export.ArchetypeIndex - 1);
				else if (Export.ArchetypeIndex > 0)
					ArchetypeName = GetExportFullName(Export.ArchetypeIndex - 1);
				else ArchetypeName = TEXT("ClassDefaultObject");
				debugf(NAME_Warning, TEXT("Failed to load ObjectArchetype for resource '%s': %s in %s"), *Export.ObjectName, *ArchetypeName, ThisParent->GetPathName());
			}
		}

		// If we could not find the Template (eg. the package containing it was deleted), use the class defaults instead.
		if (Template == NULL)
		{
			// force the default object to be created because we can't create an object
			// of this class unless it has a template to initialize itself against

			//@script patcher (only need to call GetSuperClass()->GetDefaultObject() when running script patcher
			Template = ((Export.ObjectFlags & RF_ClassDefaultObject) == 0 || LoadClass->GetFName() == NAME_Object)
				? LoadClass->GetDefaultObject(TRUE)
				: LoadClass->GetSuperClass()->GetDefaultObject(TRUE);
		}

		check(Template);
		//@script patcher todo: might need to adjust the following assertion
		checkSlow((Export.ObjectFlags & RF_ClassDefaultObject) != 0 || Template->IsA(LoadClass));

		// make sure the object's archetype is fully loaded before creating the object
		Preload(Template);

		//if (LoadClass == UBoolProperty::StaticClass())
		//	GUglyHackFlags |= 1;

		// Create the export object.
		if (Export.ObjectFlags & RF_ClassDefaultObject)
		{
			Export._Object = LoadClass->GetDefaultObject(TRUE);
			if (Export._Object->GetFName() != Export.ObjectName)
				Export._Object->Rename(*Export.ObjectName);
		}
		else Export._Object = StaticConstructObject
			(
				LoadClass,
				ThisParent,
				Export.ObjectName,
				(Export.ObjectFlags & RF_Load) | RF_NeedLoad | RF_NeedPostLoad,
				Template
			);
		//GUglyHackFlags &= ~1;

		Export._Object->SetLinker(this, Index);
		GObjLoaded.AddItem(Export._Object);

		//debugf( NAME_DevLoad, TEXT("Created %ls"), Export._Object->GetFullName() );

		// If it's a struct or class, set its parent.
		if( Export._Object->IsA(UStruct::StaticClass()) && Export.SuperIndex!=0 )
			((UStruct*)Export._Object)->SuperStruct = (UStruct*)IndexToObject( Export.SuperIndex );

		// If it's a class, bind it to C++.
		if( Export._Object->IsA( UClass::StaticClass() ) )
			((UClass*)Export._Object)->Bind();
		DEBUG_OBJECTS(*Export.ObjectName);
	}
	return Export._Object;
	unguardf(( TEXT("(%ls %i %i)"), *ExportMap(Index).ObjectName, Tell(), Index));
}

// Return the loaded object corresponding to an import index; any errors are fatal.
UObject*
ULinkerLoad::CreateImport( INT Index )
{
	guard(ULinkerLoad::CreateImport);
	FObjectImport& Import = ImportMap( Index );
	if( !Import.XObject)
	{
		if (!Import.SourceLinker)
		{
			BeginLoad();
			VerifyImport(Index);
			EndLoad();
		}
		//debugf( "Imported new %ls %ls.%ls", *Import.ClassName, *Import.ObjectPackage, *Import.ObjectName );
//		check(Import.SourceLinker);
		if(Import.SourceIndex != INDEX_NONE)
		{
			Import.XObject = Import.SourceLinker->CreateExport( Import.SourceIndex );
			GImportCount++;
		}
	}
	return Import.XObject;
	unguard;
}

// Map an import/export index to an object; all errors here are fatal.
UObject*
ULinkerLoad::IndexToObject( INT Index )
{
	guard(IndexToObject);
	if( Index > 0 )
	{
		if (!ExportMap.IsValidIndex(Index - 1))
		{
			ArIsError = 1;
			GWarn->Logf(TEXT("Bad export index %i/%i (in %ls - %p)"), Index - 1, ExportMap.Num(), _LoadingObj->GetFullName(), _LoadingObj);
			return NULL;
		}
		return CreateExport( Index-1 );
	}
	else if( Index < 0 )
	{
		if (!ImportMap.IsValidIndex(-Index - 1))
		{
			ArIsError = 1;
			GWarn->Logf(TEXT("Bad import index %i/%i (in %ls - %p)"), -Index - 1, ImportMap.Num(), _LoadingObj->GetFullName(), _LoadingObj);
			return NULL;
		}
		return CreateImport( -Index-1 );
	}
	else return NULL;
	unguard;
}

FArchive& ULinkerLoad::operator<<(FName& Name)
{
	guard(ULinkerLoad << FName);

	NAME_INDEX NameIndex = 0;
	*Loader << NameIndex;

	if (!NameMap.IsValidIndex(NameIndex))
	{
		GWarn->Logf(TEXT("Bad name index %i/%i (in %ls - %p)"), NameIndex, NameMap.Num(), _LoadingObj->GetFullName(), _LoadingObj);
		Name = NAME_None;
		ArIsError = 1;
		return *this;
	}
	INT Number;
	*Loader << Number;

	FName Temporary = FName::GetNumberedName(NameMap(NameIndex), Number);
	appMemcpy(&Name, &Temporary, sizeof(FName));

	return *this;
	unguardf((TEXT("(%ls %i))"), GetFullName(), Tell()));
}

// Detach an export from this linker.
void
ULinkerLoad::DetachExport( INT i )
{
	guard(ULinkerLoad::DetachExport);
	FObjectExport& E = ExportMap( i );
	check(E._Object);
	if( !E._Object->IsValid() )
		appErrorf( TEXT("Linker object %ls %ls.%ls is invalid"), *GetExportClassName(i), LinkerRoot->GetName(), *E.ObjectName );
	E._Object->SetLinker(NULL, INDEX_NONE);
	unguard;
}

// UObject interface.
void
ULinkerLoad::Serialize( FArchive& Ar )
{
	guard(ULinkerLoad::Serialize);
	Super::Serialize( Ar );
	LazyLoaders.CountBytes( Ar );
	unguard;
}

void
ULinkerLoad::Destroy()
{
	guard(ULinkerLoad::Destroy);
	if (LinkerRoot)
	{
		//debugf(NAME_DevLoad, TEXT("%1.1fms Unloading: %ls"), (appSeconds().GetFloat() * 1000.0), LinkerRoot->GetFullName());

		// Detach all lazy loaders.
		DetachAllLazyLoaders(0);

		// Detach all objects linked with this linker.
		for (INT i = 0; i < ExportMap.Num(); i++)
			if (ExportMap(i)._Object)
				DetachExport(i);

		// Remove from object manager, if it has been added.
		GObjLoaders.RemoveItem(this);
		if (Loader)
			delete Loader;
		Loader = NULL;
	}
	Super::Destroy();
	unguardobj;
}

// FArchive interface.
void
ULinkerLoad::AttachLazyLoader( FLazyLoader* LazyLoader )
{
	guard(ULinkerLoad::AttachLazyLoader);
	checkSlow(LazyLoader->SavedAr==NULL);
	checkSlow(LazyLoaders.FindItemIndex(LazyLoader)==INDEX_NONE);

	LazyLoaders.AddItem( LazyLoader );
	LazyLoader->SavedAr  = this;
	LazyLoader->SavedPos = Tell();

	unguard;
}

void
ULinkerLoad::DetachLazyLoader( FLazyLoader* LazyLoader )
{
	guard(ULinkerLoad::DetachLazyLoader);
	checkSlow(LazyLoader->SavedAr==this);

	INT RemovedCount = LazyLoaders.RemoveItem(LazyLoader);
	if( RemovedCount!=1 )
		appErrorf( TEXT("Detachment inconsistency: %i (%ls)"), RemovedCount, *Filename );
	LazyLoader->SavedAr = NULL;
	LazyLoader->SavedPos = 0;

	unguard;
}

void
ULinkerLoad::DetachAllLazyLoaders( UBOOL Load )
{
	guard(ULinkerLoad::DetachAllLazyLoaders);
	for( INT i=0; i<LazyLoaders.Num(); i++ )
	{
		FLazyLoader* LazyLoader = LazyLoaders( i );
		if( Load )
			LazyLoader->Load();
		LazyLoader->SavedAr  = NULL;
		LazyLoader->SavedPos = 0;
	}
	LazyLoaders.Empty();
	unguard;
}

// FArchive interface.
void
ULinkerLoad::Seek( INT InPos )
{
	guard(ULinkerLoad::Seek);
	Loader->Seek( InPos );
	unguard;
}

INT
ULinkerLoad::Tell()
{
	guard(ULinkerLoad::Tell);
	return Loader->Tell();
	unguard;
}

INT
ULinkerLoad::TotalSize()
{
	guard(ULinkerLoad::TotalSize);
	return Loader->TotalSize();
	unguard;
}

void
ULinkerLoad::Serialize( void* V, INT Length )
{
	guard(ULinkerLoad::Serialize);
	if (ArIsError)
	{
		appMemzero(V, Length);
		return;
	}
	if ((Loader->Tell() + Length) > Loader->TotalSize())
	{
		GWarn->Logf(TEXT("ReadFile beyond EOF %i+%i/%i in %ls (%p)"), Loader->Tell(), Length, Loader->TotalSize(), _LoadingObj->GetFullName(), _LoadingObj);
		appMemzero(V, Length);
		ArIsError = 1;
		return;
	}
	Loader->Serialize( V, Length );
	unguard;
}

/*----------------------------------------------------------------------------
	ULinkerSave.
----------------------------------------------------------------------------*/

ULinkerSave::ULinkerSave( UObject* InParent, const TCHAR* InFilename )
:	ULinker( InParent, InFilename )
,	Saver( NULL )
{
	// Create file saver.
	Saver = GFileManager->CreateFileWriter( InFilename, 0, GThrow );
	if( !Saver )
		appThrowf( TEXT("Error opening file") );

	// Set main summary info.
	Summary.Tag           = PACKAGE_FILE_TAG;
	Summary.SetFileVersions(PACKAGE_FILE_VERSION, PACKAGE_FILE_VERSION_LICENSEE); // KF2 ver
	Summary.PackageFlags  = (Cast<UPackage>(LinkerRoot) ? Cast<UPackage>(LinkerRoot)->PackageFlags : 0) | PKG_ContainsScript | PKG_StrippedSource;
	Summary.EngineVersion = ENGINE_VERSION;
	Summary.CookedContentVersion = 129;
	Summary.CompressionFlags = 0;
	Summary.FolderName = TEXT("None");

	// Set status info.
	ArIsSaving     = 1;
	ArIsPersistent = 1;
	ArForEdit      = 1;
	ArForClient    = 1;
	ArForServer    = 1;

	// Allocate indices.
	ObjectIndices.Empty(UObject::GObjObjects.Num());
	ObjectIndices.AddZeroed( UObject::GObjObjects.Num() );
	NameIndices  .Empty(FName::GetMaxNames());
	NameIndices  .AddZeroed( FName::GetMaxNames() );

	// Success.
	Success=1;
}

void
ULinkerSave::Destroy()
{
	guard(ULinkerSave::Destroy);
	if( Saver )
		delete Saver;
	Saver = NULL;
	Super::Destroy();
	unguard;
}

// FArchive interface.
INT
ULinkerSave::MapName( FName* Name )
{
	guardSlow(ULinkerSave::MapName);
	INT Number;
	FName RN = FName::GrabDeNumberedName(*Name, Number);
	return NameIndices(RN.GetIndex());
	unguardobjSlow;
}

INT
ULinkerSave::MapObject( UObject* Object )
{
	guardSlow(ULinkerSave::MapObject);
	return Object ? ObjectIndices(Object->GetIndex()) : 0;
	unguardobjSlow;
}

FArchive& ULinkerSave::operator<<(FName& Name)
{
	guardSlow(ULinkerSave << FName);
	INT Number;
	FName RN = FName::GrabDeNumberedName(Name, Number);
	INT Save = NameIndices(RN.GetIndex());
	return *this << Save << Number;
	unguardobjSlow;
}
FArchive& ULinkerSave::operator<<(UObject*& Obj)
{
	guardSlow(ULinkerSave << UObject);
	INT Save = Obj ? ObjectIndices(Obj->GetIndex()) : 0;
	return *this << Save;
	unguardobjSlow;
}

void ULinkerSave::Seek( INT InPos )
{
	Saver->Seek( InPos );
}

INT ULinkerSave::Tell()
{
	return Saver->Tell();
}

void ULinkerSave::Serialize( void* V, INT Length )
{
	Saver->Serialize( V, Length );
}

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
