/*=============================================================================
	UnLinker.h: Unreal object linker.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
    * Added MD5 checksum by Michiel Hendriks
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

 #ifndef MD5CHUNK
 #define MD5CHUNK 1024
 #endif

 #ifndef SHABUFLEN
 #define SHABUFLEN 16384
 #endif

/*-----------------------------------------------------------------------------
	FObjectExport.
-----------------------------------------------------------------------------*/

//
// Information about an exported object.
//
struct CORE_API FObjectExport
{
	// Variables.
	INT         ClassIndex;		// Persistent.
	INT         SuperIndex;		// Persistent (for UStruct-derived objects only).
	INT			PackageIndex;	// Persistent.
	FName		ObjectName;		// Persistent.
	QWORD		ObjectFlags;	// Persistent.
	INT			ArchetypeIndex;
	DWORD		ExportFlags;
	TArray<INT>	GenerationNetObjectCount;
	FGuid		PackageGuid;
	DWORD		PackageFlags;
	INT         SerialSize;		// Persistent.
	INT         SerialOffset;	// Persistent (for checking only).
	UObject*	_Object;		// Internal.
	INT			_iHashNext;		// Internal.

	// Functions.
	FObjectExport();
	FObjectExport( UObject* InObject );

	friend FArchive& operator<<(FArchive& Ar, FObjectExport& E);
};

/*-----------------------------------------------------------------------------
	FObjectImport.
-----------------------------------------------------------------------------*/

//
// Information about an imported object.
//
struct CORE_API FObjectImport
{
	// Variables.
	FName			ClassPackage;	// Persistent.
	FName			ClassName;		// Persistent.
	INT				PackageIndex;	// Persistent.
	FName			ObjectName;		// Persistent.
	UObject*		XObject;		// Internal (only really needed for saving, can easily be gotten rid of for loading).
	ULinkerLoad*	SourceLinker;	// Internal.
	INT             SourceIndex;	// Internal.

	// Functions.
	FObjectImport();
	FObjectImport( UObject* InObject );

	friend FArchive& operator<<(FArchive& Ar, FObjectImport& I);
};

/*----------------------------------------------------------------------------
	Items stored in Unrealfiles.
----------------------------------------------------------------------------*/

struct FCompressedChunk
{
	/** Default constructor, zero initializing all members. */
	FCompressedChunk()
		: UncompressedOffset(0)
		, UncompressedSize(0)
		, CompressedOffset(0)
		, CompressedSize(0)
	{}

	/** Original offset in uncompressed file.	*/
	INT		UncompressedOffset;
	/** Uncompressed size in bytes.				*/
	INT		UncompressedSize;
	/** Offset in compressed file.				*/
	INT		CompressedOffset;
	/** Compressed size in bytes.				*/
	INT		CompressedSize;

	/** I/O function */
	friend FArchive& operator<<(FArchive& Ar, FCompressedChunk& Chunk)
	{
		Ar << Chunk.UncompressedOffset;
		Ar << Chunk.UncompressedSize;
		Ar << Chunk.CompressedOffset;
		Ar << Chunk.CompressedSize;
		return Ar;
	}
};

struct FTextureAllocations
{
	struct FTextureType
	{
		FTextureType()
		{}

		INT				SizeX;
		INT				SizeY;
		INT				NumMips;
		DWORD			Format;
		DWORD			TexCreateFlags;
		TArray<INT>		ExportIndices;

		friend FArchive& operator<<(FArchive& Ar, FTextureType& TextureType)
		{
			Ar << TextureType.SizeX;
			Ar << TextureType.SizeY;
			Ar << TextureType.NumMips;
			Ar << TextureType.Format;
			Ar << TextureType.TexCreateFlags;
			Ar << TextureType.ExportIndices;
			return Ar;
		}
	};

	FTextureAllocations()
	{}

	TArray<FTextureType>	TextureTypes;

	friend FArchive& operator<<(FArchive& Ar, FTextureAllocations& TextureAllocations)
	{
		Ar << TextureAllocations.TextureTypes;
		return Ar;
	}
};

//
// Unrealfile summary, stored at top of file.
//
struct FGenerationInfo
{
	INT ExportCount, NameCount, NetObjectCount;

	FGenerationInfo( INT InExportCount, INT InNameCount )
		: ExportCount(InExportCount), NameCount(InNameCount)
	{}
	friend FArchive& operator<<( FArchive& Ar, FGenerationInfo& Info )
	{
		guard(FGenerationInfo<<);
		return Ar << Info.ExportCount << Info.NameCount << Info.NetObjectCount;
		unguard;
	}
};

struct FPackageFileSummary
{
	// Variables.
	INT		Tag;
	INT		FileVersion;
	INT		TotalHeaderSize;
	FString FolderName;
	DWORD	PackageFlags;
	INT		NameCount,		NameOffset;
	INT		ExportCount,	ExportOffset;
	INT     ImportCount,	ImportOffset;
	INT		DependsOffset;
	FGuid	Guid;
	TArray<FGenerationInfo> Generations;

	INT		ImportExportGuidsOffset;
	INT		ImportGuidsCount, ExportGuidsCount;

	INT		ThumbnailTableOffset;

	INT		EngineVersion;

	INT     CookedContentVersion;
	DWORD	CompressionFlags;

	TArray<FCompressedChunk> CompressedChunks;
	DWORD	PackageSource;
	TArray<FString>	AdditionalPackagesToCook;
	FTextureAllocations	TextureAllocations;

	// Constructor.
	FPackageFileSummary()
		: Tag(0), FileVersion(0), PackageFlags(0), NameCount(0), NameOffset(0), ExportCount(0), ExportOffset(0), ImportCount(0), ImportOffset(0)
	{}

#if 1 //LVer added by Legend on 4/12/2000
	inline INT GetFileVersion() const { return (FileVersion & 0xffff); }
	inline INT GetFileVersionLicensee() const { return ((FileVersion >> 16) & 0xffff); }
	inline void SetFileVersions(INT Epic, INT Licensee) { FileVersion = ((Licensee << 16) | Epic); }
#endif

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, FPackageFileSummary& Sum);
};

struct FSavedGameSummary
{
	// Set by engine:
	FString OrgFile;

	// Variables.
	FString MapTitle, Timestamp, ExtraInfo;
	TArray<BYTE> Screenshot;

	// Constructor.
	FSavedGameSummary()
	{}

	// Serializer.
	friend FArchive& operator<<(FArchive& Ar, FSavedGameSummary& Sum)
	{
		guard(FSavedGameSummary << );

		Ar << Sum.OrgFile << Sum.MapTitle << Sum.Timestamp << Sum.ExtraInfo << Sum.Screenshot;

		return Ar;
		unguard;
	}
};

/*----------------------------------------------------------------------------
	ULinker.
----------------------------------------------------------------------------*/

//
// A file linker.
//
class CORE_API ULinker : public UObject
{
	DECLARE_CLASS(ULinker,UObject,CLASS_Transient,Core)
	NO_DEFAULT_CONSTRUCTOR(ULinker)

	// Variables.
	UObject*				LinkerRoot;			// The linker's root object.
	FPackageFileSummary		Summary;			// File summary.
	TArray<FName>			NameMap;			// Maps file name indices to name table indices.
	TArray<FObjectImport>	ImportMap;			// Maps file object indices >=0 to external object names.
	TArray<FObjectExport>	ExportMap;			// Maps file object indices >=0 to external object names.
	INT						Success;			// Whether the object was constructed successfully.
	FString					Filename;			// Filename.
	DWORD					_ContextFlags;		// Load flag mask.

	// Constructors.
	ULinker( UObject* InRoot, const TCHAR* InFilename );
	void Serialize( FArchive& Ar );
	FString GetImportFullName( INT i );
	FString GetExportFullName( INT i, const TCHAR* FakeRoot=NULL );
};

/*----------------------------------------------------------------------------
	ULinkerLoad.
----------------------------------------------------------------------------*/

//
// A file loader.
//
class CORE_API ULinkerLoad : public ULinker, public FArchive
{
	DECLARE_CLASS(ULinkerLoad,ULinker,CLASS_Transient,Core)
	NO_DEFAULT_CONSTRUCTOR(ULinkerLoad)

	// Friends.
	friend class UObject;
	friend class UPackageMap;

	// Variables.
	DWORD					LoadFlags;
	UBOOL					Verified;
	INT						ExportHash[256];
	TArray<FLazyLoader*>	LazyLoaders;
	FArchive*				Loader;

	#ifdef UTPG_MD5
	FString       ShortFilename;
	FString       PartialHash; // temp storage
	FString				FileLang;
	#endif

	ULinkerLoad( UObject* InParent, const TCHAR* InFilename, DWORD InLoadFlags );

	void Verify();
	FName GetExportClassPackage( INT i );
	FName GetExportClassName( INT i );
	void VerifyImport( INT i );
	void LoadAllObjects();
	INT FindExportIndex( FName ClassName, FName ClassPackage, FName ObjectName, INT PackageIndex );
	UObject* Create( UClass* ObjectClass, FName ObjectName, DWORD LoadFlags, UBOOL Checked );
	void Preload( UObject* Object );

	#ifdef UTPG_MD5
	FString GenFullSum();
	FString GenPartialSum();
	#endif

private:
	#ifdef UTPG_MD5
	FString DoRealFullMD5();
	#endif

	UObject* CreateExport( INT Index );
	UObject* CreateImport( INT Index );

	UObject* IndexToObject( INT Index );
	void DetachExport( INT i );
	ULinkerLoad* GetSourceLinker(INT iLinkerImport);

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// FArchive interface.
	void AttachLazyLoader( FLazyLoader* LazyLoader );
	void DetachLazyLoader( FLazyLoader* LazyLoader );
	void DetachAllLazyLoaders( UBOOL Load );
	void Seek( INT InPos );
	INT Tell();
	INT TotalSize();
	void Serialize( void* V, INT Length );
	FArchive& operator<<( UObject*& Object )
	{
		guard(ULinkerLoad<<UObject);
		INT Index = 0;
		*Loader << Index;
		UObject* Temporary = IndexToObject( Index );
		appMemcpy(&Object, &Temporary, sizeof(UObject*));

		return *this;
		unguardf(( TEXT("(%ls %i))"), GetFullName(), Tell() ));
	}

	inline FObjectImport* GetTopImport(INT Index)
	{
		FObjectImport* Top;
		for (Top = &ImportMap(Index); Top->PackageIndex < 0; Top = &ImportMap(-Top->PackageIndex - 1));
		return Top;
	}

	FArchive& operator<<(FName& Name);
};

/*----------------------------------------------------------------------------
	ULinkerSave.
----------------------------------------------------------------------------*/

//
// A file saver.
//
class ULinkerSave : public ULinker, public FArchive
{
	DECLARE_CLASS(ULinkerSave,ULinker,CLASS_Transient,Core);
	NO_DEFAULT_CONSTRUCTOR(ULinkerSave);

	// Variables.
	FArchive* Saver;
	TArray<INT> ObjectIndices;
	TArray<INT> NameIndices;
	TArray<TArray<INT> > DependsMap;

	// Constructor.
	ULinkerSave( UObject* InParent, const TCHAR* InFilename );
	void Destroy();

	// FArchive interface.
	INT MapName( FName* Name );
	INT MapObject( UObject* Object );
	FArchive& operator<<(FName& Name);
	FArchive& operator<<(UObject*& Obj);
	void Seek( INT InPos );
	INT Tell();
	void Serialize( void* V, INT Length );
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
