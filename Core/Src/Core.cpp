/*=============================================================================
	Core.cpp: Unreal core.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	Temporary startup objects.
-----------------------------------------------------------------------------*/

// Error malloc.
class FMallocError : public FMalloc
{
	void Called( const TCHAR* Str )
		{appErrorf( TEXT("Called %ls before memory init"), Str );}
	void* Malloc(size_t Count, const TCHAR* Tag)
		{Called(TEXT("appMalloc"));return NULL;}
	void* Realloc(void* Original, size_t Count, const TCHAR* Tag)
		{Called(TEXT("appRealloc"));return NULL;}
	void Free( void* Original )
		{Called(TEXT("appFree"));}
	void DumpAllocs()
		{Called(TEXT("appDumpAllocs"));}
	void HeapCheck()
		{Called(TEXT("appHeapCheck"));}
	void Init()
		{Called(TEXT("FMallocError::Init"));}
	void Exit()
		{Called(TEXT("FMallocError::Exit"));}
} MallocError;

// Error file manager.
class FFileManagerError : public FFileManager
{
public:
	FArchive* CreateFileReader( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
		{appErrorf(TEXT("Called FFileManagerError::CreateFileReader")); return 0;}
	FArchive* CreateFileWriter( const TCHAR* Filename, DWORD Flags, FOutputDevice* Error )
		{appErrorf(TEXT("Called FFileManagerError::CreateFileWriter")); return 0;}
	INT FileSize( const TCHAR* Filename )
		{return -1;}
	UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 )
		{return 0;}
	UBOOL Copy( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0, void (*Progress)(FLOAT Fraction)=NULL )
		{return 0;}
	UBOOL Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 )
		{return 0;}
	SQWORD GetGlobalTime( const TCHAR* Filename )
		{return 0;}
	UBOOL SetGlobalTime( const TCHAR* Filename )
		{return 0;}
	UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 )
		{return 0;}
	UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 )
		{return 0;}
	TArray<FString> FindFiles( const TCHAR* Filename, UBOOL Files, UBOOL Directories )
		{return TArray<FString>();}
	UBOOL SetDefaultDirectory( const TCHAR* Filename )
		{return 0;}
	FString GetDefaultDirectory()
		{return TEXT("");}
} FileError;

// Critical error output device.
class CORE_API FErrorOutError : public FOutputDeviceError
{
public:
	void Serialize( const TCHAR* V, EName Event )
	{}
	void HandleError()
	{}
} ErrorOutError;

// Log output device.
class CORE_API FLogOutError : public FOutputDevice
{
public:
	void Serialize( const TCHAR* V, EName Event )
	{}
} LogOutError;

// Exception thrower.
class CORE_API FThrowOut : public FOutputDevice
{
public:
	void Serialize( const TCHAR* V, EName Event )
	{
		throw( V );
	}
} ThrowOut;

// Null output device.
class CORE_API FNullOutError : public FOutputDevice
{
public:
	void Serialize( const TCHAR* V, enum EName Event )
	{}
} NullOutError;

// Dummy saver.
class CORE_API FArchiveDummySave : public FArchive
{
public:
	FArchiveDummySave() { ArIsSaving = 1; }
} GArchiveDummySave;

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

CORE_API FMemStack				GMem;							/* Global memory stack */
CORE_API FOutputDevice*			GLog=&LogOutError;				/* Regular logging */
CORE_API FOutputDeviceError*	GError=&ErrorOutError;			/* Critical errors */
CORE_API FOutputDevice*			GNull=&NullOutError;			/* Log to nowhere */
CORE_API FOutputDevice*			GThrow=&ThrowOut;				/* Exception thrower */
CORE_API FFeedbackContext*		GWarn=NULL;						/* User interaction and non critical warnings */
CORE_API FConfigCache*			GConfig=NULL;					/* Configuration database cache */
CORE_API FTransactionBase*		GUndo=NULL;						/* Transaction tracker, non-NULL when a transaction is in progress */
CORE_API FOutputDevice*			GLogHook=NULL;					/* Launch log output hook */
CORE_API FExec*					GExec=NULL;						/* Launch command-line exec hook */
CORE_API FMalloc*				GMalloc=&MallocError;			/* Memory allocator */
CORE_API FFileManager*			GFileManager=&FileError;		/* File manager */
CORE_API USystem*				GSys=NULL;						/* System control code */
CORE_API UProperty*				GProperty;						/* Property for UnrealScript interpretter */
CORE_API BYTE*					GPropAddr;						/* Property address for UnrealScript interpreter */
CORE_API USubsystem*			GWindowManager=NULL;			/* Window update routine called once per tick */
CORE_API TCHAR					GErrorHist[4096]=TEXT("");		/* For building call stack text dump in guard/unguard mechanism */
CORE_API TCHAR					GYes[64]=TEXT("Yes");			/* Localized "yes" text */
CORE_API TCHAR					GNo[64]=TEXT("No");				/* Localized "no" text */
CORE_API TCHAR					GTrue[64]=TEXT("True");			/* Localized "true" text */
CORE_API TCHAR					GFalse[64]=TEXT("False");		/* Localized "false" text */
CORE_API TCHAR					GNone[64]=TEXT("None");			/* Localized "none" text */
CORE_API TCHAR                  GCdPath[256]=TEXT("");			/* Cd path, if any */
CORE_API TCHAR                  GIni[256]=TEXT("");			    /* Cd path, if any */
CORE_API FLOAT					GSecondsPerCycle=1.0f;			/* Seconds per CPU cycle for this PC */
CORE_API FLOAT					GCorrection= 0.0;				// Correction
CORE_API FTime					GTempTime=0.0f;					/* Used during development for timing */
CORE_API void					(*GTempFunc)(void*)=NULL;		/* Used during development for debug hooks */
CORE_API SQWORD					GTicks=1;						/* Number of non-persistent ticks thus far in this level, for profiling */
CORE_API INT					GScriptCycles;					/* Times script execution CPU cycles per tick */
CORE_API DWORD					GPageSize=4096;					/* Operating system page size */
CORE_API INT					GProcessorCount=1;				/* Number of CPUs in this PC */
CORE_API QWORD					GPhysicalMemory=16384*1024;		/* Bytes of physical memory in this PC */
CORE_API DWORD                  GUglyHackFlags=0;               /* Flags for passing around globally hacked stuff */
CORE_API UBOOL					GIsScriptable=0;				/* Whether script execution is allowed */
CORE_API UBOOL					GIsEditor=0;					/* Whether engine was launched for editing */
CORE_API UBOOL					GIsClient=0;					/* Whether engine was launched as a client */
CORE_API UBOOL					GIsServer=0;					/* Whether engine was launched as a server, true if GIsClient */
CORE_API UBOOL					GIsCriticalError=0;				/* An appError() has occured */
CORE_API UBOOL					GIsStarted=0;					/* Whether execution is happening from within main()/WinMain() */
CORE_API UBOOL					GIsGuarded=0;					/* Whether execution is happening within main()/WinMain()'s try/catch handler */
CORE_API UBOOL					GIsRunning=0;					/* Whether execution is happening within MainLoop() */
CORE_API UBOOL					GIsSlowTask=0;					/* Whether there is a slow task in progress */
CORE_API UBOOL					GIsRequestingExit=0;			/* Indicates that MainLoop() should be exited at the end of the current iteration */
CORE_API UBOOL					GIsStrict=0;					/* Causes all UnrealScript execution warnings to be fatal errors */
CORE_API UBOOL					GIsCollectingGarbage=0;			/* Whether engine is collecting garbage at this moment */
CORE_API UBOOL					GScriptEntryTag=0;				/* Number of recursive UnrealScript calls currently on the stack */
CORE_API UBOOL					GSafeMode=0;					/* Run with minimal device drivers to prevent crashes */
CORE_API UBOOL					GLazyLoad=0;					/* Whether TLazyLoad arrays should be lazy-loaded or not */
CORE_API UBOOL					GIsUTracing=0;					/* Whether we are tracing script code to a log file. Search UDN for relevant info. */
CORE_API UBOOL					GUseTransientNames=0;		    /* Use server transient names. */
CORE_API UBOOL					GEnableRunAway=1;				/* Enable runaway loop checking */
CORE_API UBOOL					GIsDemoPlayback=0;				/* Whether we are playing a demo. */
//CORE_API UBOOL					GIsOnStrHack=0;					/* Hack to make Linux server charset network compatible with windows */
CORE_API FGlobalMath			GMath;							/* Math code */
CORE_API URenderDevice*			GRenderDevice=NULL;				/* S3TC Support - Global pointer to current rendering device */
CORE_API FArchive*				GDummySave=&GArchiveDummySave;	/* No-op save archive */
CORE_API UViewport*				GCurrentViewport = NULL;        /* Pointer to the currently active UViewport - the one with the white outline */
// Shambler
CORE_API UBOOL					GDuplicateNames=0;				/* Wether to allow matching names in the name list, hack fix for conforming */
CORE_API UBOOL					GAntiTCC=0;						/* Wether or not to allow temporary console commands */
CORE_API UBOOL					GTCCUsed=0;						/* Wether or not a temporary console command has been used during the game */
CORE_API UBOOL					GScriptHooksEnabled = 1;		/* Currently allows UnrealScript hooks */
CORE_API UINT					GFrameNumber = 0;
// Smirftsch
CORE_API GameCacheFolderIni*	GCacheFolderIni=NULL;			/* looking up of localy stored files in cache by guid */
#ifdef UTPG_MD5
CORE_API UZQ5Gnoyr*     TK5Ahisl=NULL; /* Package MD5 table */
CORE_API UBOOL          ForceMD5Calc=0; /* Used for packages.md5 generation */
CORE_API UBOOL          DisableMD5=0; /* disable MD5, used for the editor */
CORE_API UObject*       MD5NotifyObject=NULL; /* Object to notify when a package is loaded */
#endif

// Unicode.
#if UNICODE
CORE_API UBOOL GUnicode=1;
CORE_API UBOOL GUnicodeOS=0;
#else
CORE_API UBOOL GUnicode=0;
CORE_API UBOOL GUnicodeOS=0;
#endif

#if UPAK_Build
CORE_API UBOOL bUPakBuild = 1;
#else
CORE_API UBOOL bUPakBuild = 0;
#endif

// System identification.
CORE_API UBOOL GTimestamp	= 0;
CORE_API UBOOL GIsAltivec	= 0;
CORE_API UBOOL GIsMMX		= 0;
CORE_API UBOOL GIsSSE		= 0;
CORE_API UBOOL GIsSSE2		= 0;
CORE_API UBOOL GIsSSE3		= 0;
CORE_API UBOOL GIsSSSE3		= 0;
CORE_API UBOOL GIsSSE41		= 0;
CORE_API UBOOL GIsSSE42		= 0;
CORE_API UBOOL GIsSSE5		= 0;
CORE_API UBOOL GIsSSE4A		= 0;
CORE_API UBOOL GIsA3DNOW	= 0;
CORE_API UBOOL GIsMMXPLUS	= 0;
CORE_API UBOOL GIsA3DNOWEXT = 0;

// For development.
UBOOL GNoGC=0;
UBOOL GCheckConflicts=0;
UBOOL GExitPurge=0;

#include "FCodec.h"
BYTE* FCodecBWT::CompressBuffer;
INT FCodecBWT::CompressLength;

/*-----------------------------------------------------------------------------
	Package implementation.
-----------------------------------------------------------------------------*/

IMPLEMENT_PACKAGE(Core);

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
