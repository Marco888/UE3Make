/*=============================================================================
	UnCorObj.h: Standard core object definitions.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

/*-----------------------------------------------------------------------------
	UPackage.
-----------------------------------------------------------------------------*/

//
// A package.
//
class CORE_API UPackage : public UObject
{
	DECLARE_CLASS(UPackage,UObject,0,Core)

	// Variables.
	DWORD PackageFlags;
	UBOOL bIsAssetPackage;

	// Constructors.
	UPackage();

	// UObject interface.
	void Destroy();
	void Serialize( FArchive& Ar );

	class UMetaData* GetMetaData();

	class UMetaData* MetaData;
};

/*-----------------------------------------------------------------------------
	ULanguage.
-----------------------------------------------------------------------------*/

//
// A language (special case placeholder class).
//
class CORE_API ULanguage : public UObject
{
	DECLARE_ABSTRACT_CLASS(ULanguage,UObject,CLASS_Transient,Core)
	NO_DEFAULT_CONSTRUCTOR(ULanguage)
	ULanguage* SuperLanguage;
};

/*-----------------------------------------------------------------------------
	UTextBuffer.
-----------------------------------------------------------------------------*/

//
// An object that holds a bunch of text.  The text is contiguous and, if
// of nonzero length, is terminated by a NULL at the very last position.
//
class CORE_API UTextBuffer : public UObject, public FOutputDevice
{
	DECLARE_CLASS(UTextBuffer,UObject,0,Core)

	// Variables.
	INT Pos, Top;
	FString Text;

	// Constructors.
	UTextBuffer( const TCHAR* Str=TEXT("") );

	// UObject interface.
	void Serialize( FArchive& Ar );

	// FOutputDevice interface.
	void Serialize( const TCHAR* Data, EName Event );
};

/*----------------------------------------------------------------------------
	USystem.
----------------------------------------------------------------------------*/

class CORE_API USystem : public USubsystem
{
	DECLARE_CLASS(USystem,USubsystem,CLASS_Config,Core)

	// Variables.
#if 1 //LMode added by Legend on 4/12/2000
	//
	// a licensee configurable INI setting that can be used to augment
	// the behavior of LicenseeVer -- especially useful in supporting
	// the transition from "before LiceseeVer" to the new build; for
	// salvaging maps that have licensee-specific data.
	//
	INT LicenseeMode;
#endif
	INT PurgeCacheDays;
	INT UseCPU;						/* Force Unreal to use a specific core */
	FString SavePath;
	FString CachePath;
	FString CacheExt;
	FString LangPath;
	TArray<FString> Paths;
	TArray<FString> Extensions;
	TArray<FName> Suppress;

	UBOOL NoRunAway,NoCacheSearch,Force1msTimer,UseRegularAngles,CloseEmptyProperties;

	// Constructors.
	void StaticConstructor();
	USystem()
		: LicenseeMode(1)
	{}

	// FExec interface.
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=*GLog );

	// OS specific
	static const TCHAR* GetOS();
	static void appRelaunch(const TCHAR* Cmd);
	static void DumpMemStat(FOutputDevice& Ar);
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
