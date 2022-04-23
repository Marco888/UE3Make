/*=============================================================================
	Core.h: Unreal core public header file.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#pragma once

/*----------------------------------------------------------------------------
	Low level includes.
----------------------------------------------------------------------------*/

// API definition.
#ifndef CORE_API
#define CORE_API 
#endif

// Build options.
#include "UnBuild.h"
#include <float.h>


#if ((_MSC_VER))
#include <windows.h>
#pragma pack (push,OBJECT_ALIGNMENT)
#elif (HAVE_PRAGMA_PACK)
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

// Time.
#define FIXTIME 4294967296.f
class FTime
{
#if __GNUG__
#define TIMETYP long long
#else
#define TIMETYP __int64
#endif
public:

	        FTime      ()               {v=0;}
	        FTime      (float f)        {v=(TIMETYP)(f*FIXTIME);}
	        FTime      (double d)       {v=(TIMETYP)(d*FIXTIME);}
	float   GetFloat   ()               {return v/FIXTIME;}
	FTime   operator+  (float f) const  {return FTime(v+(TIMETYP)(f*FIXTIME));}
	FTime   operator-  (float f) const  {return FTime(v-(TIMETYP)(f*FIXTIME));}
	float   operator-  (FTime t) const  {return (v-t.v)/FIXTIME;}
	FTime   operator*  (float f) const  {return FTime(v*f);}
	FTime   operator/  (float f) const  {return FTime(v/f);}
	FTime&  operator+= (float f)        {v=v+(TIMETYP)(f*FIXTIME); return *this;}
	FTime&  operator-= (float f)        {v=v-(TIMETYP)(f*FIXTIME); return *this;}
	FTime&  operator*= (float f)        {v=(TIMETYP)(v*f); return *this;}
	FTime&  operator/= (float f)        {v=(TIMETYP)(v/f); return *this;}
	int     operator== (FTime t)        {return v==t.v;}
	int     operator!= (FTime t)        {return v!=t.v;}
	int     operator>  (FTime t)        {return v>t.v;}
	int     operator<  (FTime t)        {return v<t.v;}
	FTime&  operator=  (const FTime& t) {v=t.v; return *this;}
private:
	FTime (TIMETYP i) {v=i;}
	TIMETYP v;
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif


// Compiler specific include.
#include <string.h>
#if _MSC_VER
	#include "UnVcWin32.h"
#elif __GNUG__
	#include "UnGnuG.h"
#else
	#error Unknown Compiler
#endif

// If no asm, redefine __asm to cause compile-time error.
#if !ASM && !__GNUG__
	//#define __asm ERROR_ASM_NOT_ALLOWED
#endif

// OS specific include.
#if __GNUG__
	#include "UnUnix.h"
	#include <signal.h>
#endif

#ifdef SDL2BUILD
    #include "SDL.h"
#endif

#define FORCE_USCRIPT_PADDING_BYTE3(x) BYTE force_uscript_padding_byte##x
#define FORCE_USCRIPT_PADDING_BYTE2(x) FORCE_USCRIPT_PADDING_BYTE3(x)
#define FORCE_USCRIPT_PADDING_BYTE FORCE_USCRIPT_PADDING_BYTE2(__LINE__)

#if BUILD_64
#define FORCE_64BIT_PADDING_DWORD3(x) DWORD force_64bit_padding_dword##x
#define FORCE_64BIT_PADDING_DWORD2(x) FORCE_64BIT_PADDING_DWORD3(x)
#define FORCE_64BIT_PADDING_DWORD FORCE_64BIT_PADDING_DWORD2(__LINE__)
#define FORCE_64BIT_PADDING_BYTE3(x) BYTE force_64bit_padding_byte##x
#define FORCE_64BIT_PADDING_BYTE2(x) FORCE_64BIT_PADDING_BYTE3(x)
#define FORCE_64BIT_PADDING_BYTE FORCE_64BIT_PADDING_BYTE2(__LINE__)
#else
#define FORCE_64BIT_PADDING_DWORD
#define FORCE_64BIT_PADDING_BYTE
#endif


// Global constants.
enum {MAXBYTE		= 0xff       };
enum {MAXWORD		= 0xffffU    };
enum {MAXDWORD		= 0xffffffffU};
enum {MAXSBYTE		= 0x7f       };
enum {MAXSWORD		= 0x7fff     };
enum {MAXINT		= 0x7fffffff };
enum {INDEX_NONE	= -1         };
enum {UTF16_BOM    = 0xfeff     };
enum {UTF32_BOM    = 0x0000feff };
enum ENoInit {E_NoInit = 0};

enum // GUglyHackFlags
{
	HACKFLAGS_NoNearZ			= (1<<0), // Disable near Z mesh clipping
	HACKFLAGS_AlwaysTransform	= (1<<1), // Always transform BSP permanently.
	HACKFLAGS_AllowAutoDestruct	= (1<<2), // Allow autodestruct variables to kill off object refs.
	HACKFLAGS_UnsafeDrawPortal	= (1<<3), // Unsafe to draw portals in Scripted textures for moment.
	HACKFLAGS_NoCleanupIK		= (1<<4), // Unsafe to cleanup or spawn IK solvers right now.
};

// Unicode or single byte character set mappings.

// stijn: UEngine uses up to 3 different string representations, depending on the target platform and build configuration.
// In UNICODE builds, we use TCHAR strings for almost everything. The one notable exception is when we need to serialize
// FStrings before storing them on disk or sending them over the network. In this case, we use ANSICHAR or UNICHAR strings.
//
// ANSI/ANSICHAR strings:
// > 1-byte encoding
// > Windows-125x code pages
// > Used for serialization of pure ansi strings
//
// TCHAR strings:
// > Equivalent to ANSI strings if we build without unicode support. If we build WITH unicode support, then these are wide-character strings.
// The exact representation is platform-specific. Windows uses 2 bytes per char and UTF-16LE encoding.
// Linux usually does 4 bytes per char and UTF-32 encoding.
// > ISO 10646 aka UCS code pages
// > Used for most internal string operations and for storage in RAM
//
// UNICHAR strings:
// > Equivalent to ANSI strings if we build without unicode support. If we build WITH unicode support, these are UTF-16LE strings.
// > ISO 10646 aka UCS code pages
// > Used for serialization of strings that cannot be represented in pure ANSI
//
#ifndef _MSC_VER
	#include <iconv.h>
	CORE_API extern "C" iconv_t IconvAnsiToTchar;
	CORE_API extern "C" iconv_t IconvAnsiToUnichar;
	CORE_API extern "C" iconv_t IconvUnicharToAnsi;
	CORE_API extern "C" iconv_t IconvUnicharToTchar;
	CORE_API extern "C" iconv_t IconvTcharToAnsi;
	CORE_API extern "C" iconv_t IconvTcharToUnichar;
    CORE_API extern "C" iconv_t IconvTcharToUtf8;
    CORE_API extern "C" iconv_t IconvUtf8ToTchar;
#endif

#if UNICODE
	#ifndef _TCHAR_DEFINED
		typedef UNICHAR  TCHAR;
	#endif

	#ifndef _TEXT_DEFINED
	#undef TEXT
	#define TEXT(s) L##s
    #endif

	#undef US
	#define US FString(L"")
	inline TCHAR    FromAnsi   ( ANSICHAR In ) { return (BYTE)In;                                            }
	inline TCHAR    FromUnicode( UNICHAR In  ) { return In;                                                  }
	inline ANSICHAR ToAnsi     ( TCHAR In    ) { return (_WORD)In<0x100 ? (ANSICHAR)In : (ANSICHAR)MAXSBYTE; }
	inline UNICHAR  ToUnicode  ( TCHAR In    ) { return In;                                                  }
#else
	#ifndef _TCHAR_DEFINED
		typedef ANSICHAR  TCHAR;
		typedef ANSICHARU TCHARU;
	#endif
	#undef TEXT
	#define TEXT(s) s
	#undef US
	#define US FString("")
	inline TCHAR    FromAnsi   ( ANSICHAR In ) { return In;                              }
	inline TCHAR    FromUnicode( UNICHAR In  ) { return (_WORD)In<0x100 ? In : MAXSBYTE; }
	inline ANSICHAR ToAnsi     ( TCHAR In    ) { return (_WORD)In<0x100 ? In : MAXSBYTE; }
	inline UNICHAR  ToUnicode  ( TCHAR In    ) { return (BYTE)In;                        }
#endif

/*----------------------------------------------------------------------------
	Forward declarations.
----------------------------------------------------------------------------*/

// Objects.
class	UObject;
class	UExporter;
class	UFactory;
class	UField;
class	UConst;
class	UEnum;
class	UProperty;
class	UByteProperty;
class	UIntProperty;
class	UBoolProperty;
class	UFloatProperty;
class	UObjectProperty;
class	UClassProperty;
class	UNameProperty;
class	UStructProperty;
class   UStrProperty;
class   UArrayProperty;
class	UStruct;
class	UFunction;
class	UState;
class	UClass;
class	ULinker;
class	ULinkerLoad;
class	ULinkerSave;
class	UPackage;
class	USubsystem;
class	USystem;
class	UTextBuffer;
class   URenderDevice;
class	UPackageMap;
class   UViewport;

// Structs.
class FName;
class FArchive;
class FCompactIndex;
class FExec;
class FGuid;
class FMemCache;
class FMemStack;
class FPackageInfo;
class FTransactionBase;
class FUnknown;
class FRepLink;
class FArray;
class FLazyLoader;
class FString;
class FMalloc;

// Templates.
template<class T> class TArray;
template<class T> class TTransArray;
template<class T> class TLazyArray;
template<class TK, class TI> class TMap;
template<class TK, class TI> class TMultiMap;

#ifdef UTPG_MD5
class UZQ5Gnoyr;
class FZQ5Erpbeq;
class UMD5;
#endif

// Globals.
CORE_API extern class FOutputDevice* GNull;

// EName definition.
#include "UnNames.h"

/*-----------------------------------------------------------------------------
	Abstract interfaces.
-----------------------------------------------------------------------------*/

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (push,OBJECT_ALIGNMENT)
#endif

// An output device.
class CORE_API FOutputDevice
{
public:
	// FOutputDevice interface.
	virtual void Serialize( const TCHAR* V, EName Event ) _VF_BASE;

	// Simple text printing.
	void Log( const TCHAR* S );
	void Log( enum EName Type, const TCHAR* S );
	void Log( const FString& S );
	void Log( enum EName Type, const FString& S );
	void Logf( const TCHAR* Fmt, ... );
	void Logf( enum EName Type, const TCHAR* Fmt, ... );
	static void show_pure_virtual_call(const char *Message);
};

// Error device.
class CORE_API FOutputDeviceError : public FOutputDevice
{
public:
	virtual void HandleError() _VF_BASE;
};

// Memory allocator.
class CORE_API FMalloc
{
public:
	virtual void* Malloc(uintptr_t Count, const TCHAR* Tag) _VF_BASE_RET(NULL);
	virtual void* Realloc(void* Original, uintptr_t Count, const TCHAR* Tag) _VF_BASE_RET(NULL);
	virtual void Free( void* Original ) _VF_BASE;
	virtual void DumpAllocs() _VF_BASE;
	virtual void HeapCheck() _VF_BASE;
	virtual void Init() _VF_BASE;
	virtual void Exit() _VF_BASE;
};

// Configuration database cache.
class FConfigCache
{
public:
	virtual UBOOL GetBool( const TCHAR* Section, const TCHAR* Key, UBOOL& Value, const TCHAR* Filename=NULL ) _VF_BASE_RET(0);
	virtual UBOOL GetInt( const TCHAR* Section, const TCHAR* Key, INT& Value, const TCHAR* Filename=NULL ) _VF_BASE_RET(0);
	virtual UBOOL GetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT& Value, const TCHAR* Filename=NULL ) _VF_BASE_RET(0);
	virtual UBOOL GetString( const TCHAR* Section, const TCHAR* Key, FString& Value, const TCHAR* Filename=NULL ) _VF_BASE_RET(0);
	virtual UBOOL GetSection(const TCHAR* Section, TCHAR* Value, INT Size, const TCHAR* Filename = NULL) _VF_BASE_RET(0);
	virtual UBOOL GetFileSections( TArray<FString>& Sections, const TCHAR* Filename=NULL ) _VF_BASE_RET(0);
	virtual FString GetStr( const TCHAR* Section, const TCHAR* Key, const TCHAR* Filename=NULL ) =0;
	virtual TMultiMap<FString,FString>* GetSectionPrivate( const TCHAR* Section, UBOOL Force, UBOOL Const, const TCHAR* Filename=NULL ) _VF_BASE_RET(NULL);
	virtual void EmptySection( const TCHAR* Section, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void SetBool( const TCHAR* Section, const TCHAR* Key, UBOOL Value, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void SetInt( const TCHAR* Section, const TCHAR* Key, INT Value, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void SetFloat( const TCHAR* Section, const TCHAR* Key, FLOAT Value, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void SetString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void Flush( UBOOL Read, const TCHAR* Filename=NULL ) _VF_BASE;
	virtual void UnloadFile( const TCHAR* Filename ) _VF_BASE;
	virtual void Detach( const TCHAR* Filename ) _VF_BASE;
	virtual void Init( const TCHAR* InSystem, const TCHAR* InUser, UBOOL RequireConfig ) _VF_BASE;
	virtual void Exit() _VF_BASE;
	virtual void Dump( FOutputDevice& Ar ) _VF_BASE;

	void GetArray(const TCHAR* Section, const TCHAR* Key, class TArray<FString>* List, const TCHAR* Filename = NULL);

	virtual ~FConfigCache() noexcept(false){};
};

// Any object that is capable of taking commands.
class CORE_API FExec
{
public:
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar ) _VF_BASE_RET(0);
};

// Notification hook.
class CORE_API FNotifyHook
{
public:
	virtual void NotifyDestroy( void* Src ) {}
	virtual void NotifyPreChange( void* Src ) {}
	virtual void NotifyPostChange( void* Src ) {}
	virtual void NotifyExec( void* Src, const TCHAR* Cmd ) {}
};

// Interface for returning a context string.
class FContextSupplier
{
public:
	virtual FString GetContext() =0;
};

// A context for displaying modal warning messages.
class CORE_API FFeedbackContext : public FOutputDevice
{
public:
	virtual UBOOL YesNof( const TCHAR* Fmt, ... ) _VF_BASE_RET(0);
	virtual UBOOL VARARGS StatusUpdatef( INT Numerator, INT Denominator, const TCHAR* Fmt, ... ) _VF_BASE_RET(0);
	virtual void BeginSlowTask( const TCHAR* Task, UBOOL StatusWindow, UBOOL Cancelable ) _VF_BASE;
	virtual void EndSlowTask() _VF_BASE;
	virtual void SetContext( FContextSupplier* InSupplier ) _VF_BASE;
	virtual FContextSupplier* GetContext() = 0;
	virtual void OnCancelProgress() {}
	virtual INT GetErrors() { return 0; }
};

// Class for handling undo/redo transactions among objects.
typedef void( *STRUCT_AR )( FArchive& Ar, void* TPtr );
typedef void( *STRUCT_DTOR )( void* TPtr );
class CORE_API FTransactionBase
{
public:

    virtual ~FTransactionBase() {}
	virtual void SaveObject( UObject* Object ) _VF_BASE;
	virtual void SaveArray( UObject* Object, FArray* Array, INT Index, INT Count, INT Oper, INT ElementSize, STRUCT_AR Serializer, STRUCT_DTOR Destructor ) _VF_BASE;
	virtual void Apply() _VF_BASE;
};

// File manager.
enum EFileTimes
{
	FILETIME_Create      = 0,
	FILETIME_LastAccess  = 1,
	FILETIME_LastWrite   = 2,
};
enum EFileWrite
{
	FILEWRITE_NoFail            = 0x01,
	FILEWRITE_NoReplaceExisting = 0x02,
	FILEWRITE_EvenIfReadOnly    = 0x04,
	FILEWRITE_Unbuffered        = 0x08,
	FILEWRITE_Append			= 0x10,
	FILEWRITE_AllowRead         = 0x20,
};
enum EFileRead
{
	FILEREAD_NoFail             = 0x01,
};
class CORE_API FFileManager
{
public:
	virtual FArchive* CreateFileReader( const TCHAR* Filename, DWORD ReadFlags=0, FOutputDevice* Error=GNull ) _VF_BASE_RET(NULL);
	virtual FArchive* CreateFileWriter( const TCHAR* Filename, DWORD WriteFlags=0, FOutputDevice* Error=GNull ) _VF_BASE_RET(NULL);
	virtual INT FileSize( const TCHAR* Filename ) _VF_BASE_RET(0);
	virtual UBOOL Delete( const TCHAR* Filename, UBOOL RequireExists=0, UBOOL EvenReadOnly=0 ) _VF_BASE_RET(0);
	virtual UBOOL Copy( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0, void (*Progress)(FLOAT Fraction)=NULL ) _VF_BASE_RET(0);
	virtual UBOOL Move( const TCHAR* Dest, const TCHAR* Src, UBOOL Replace=1, UBOOL EvenIfReadOnly=0, UBOOL Attributes=0 ) _VF_BASE_RET(0);
	virtual SQWORD GetGlobalTime( const TCHAR* Filename ) _VF_BASE_RET(0);
	virtual UBOOL SetGlobalTime( const TCHAR* Filename ) _VF_BASE_RET(0);
	virtual UBOOL MakeDirectory( const TCHAR* Path, UBOOL Tree=0 ) _VF_BASE_RET(0);
	virtual UBOOL DeleteDirectory( const TCHAR* Path, UBOOL RequireExists=0, UBOOL Tree=0 ) _VF_BASE_RET(0);
	virtual UBOOL SetDefaultDirectory( const TCHAR* Filename ) _VF_BASE_RET(0);
	virtual TArray<FString> FindFiles( const TCHAR* Filename, UBOOL Files, UBOOL Directories ) =0;
	virtual FString GetDefaultDirectory() =0;
	virtual void Init(UBOOL Startup) {}
};

#if ((_MSC_VER) || (HAVE_PRAGMA_PACK))
#pragma pack (pop)
#endif

enum EUglyHackFlags
{
	HACK_ClassLoadingDisabled = (1 << 2),
	HACK_DisableSubobjectInstancing = (1 << 3),
	HACK_UpdateArchetypeFromInstance = (1 << 4),
	HACK_DisableComponentCreation = (1 << 5),
};

/*----------------------------------------------------------------------------
	Global variables.
----------------------------------------------------------------------------*/

// Core globals.
CORE_API extern FMemStack				GMem;
CORE_API extern FOutputDevice*			GLog;
CORE_API extern FOutputDevice*			GNull;
CORE_API extern FOutputDevice*		    GThrow;
CORE_API extern FOutputDeviceError*		GError;
CORE_API extern FFeedbackContext*		GWarn;
CORE_API extern FConfigCache*			GConfig;
CORE_API extern FTransactionBase*		GUndo;
CORE_API extern FOutputDevice*			GLogHook;
CORE_API extern FExec*					GExec;
CORE_API extern FMalloc*				GMalloc;
CORE_API extern FFileManager*			GFileManager;
CORE_API extern USystem*				GSys;
CORE_API extern UProperty*				GProperty;
CORE_API extern BYTE*					GPropAddr;
CORE_API extern USubsystem*				GWindowManager;
CORE_API extern TCHAR				    GErrorHist[4096];
CORE_API extern TCHAR                   GTrue[64], GFalse[64], GYes[64], GNo[64], GNone[64], GIni[256];
CORE_API extern TCHAR					GCdPath[];
CORE_API extern	FLOAT					GSecondsPerCycle;
CORE_API extern	FTime					GTempTime;
CORE_API extern void					(*GTempFunc)(void*);
CORE_API extern SQWORD					GTicks;
CORE_API extern INT                     GScriptCycles;
CORE_API extern DWORD					GPageSize;
CORE_API extern INT						GProcessorCount;
CORE_API extern QWORD					GPhysicalMemory;
CORE_API extern DWORD                   GUglyHackFlags;
CORE_API extern UBOOL					GIsScriptable;
CORE_API extern UBOOL					GIsEditor;
CORE_API extern UBOOL					GIsClient;
CORE_API extern UBOOL					GIsServer;
CORE_API extern UBOOL					GIsCriticalError;
CORE_API extern UBOOL					GIsStarted;
CORE_API extern UBOOL					GIsRunning;
CORE_API extern UBOOL					GIsSlowTask;
CORE_API extern UBOOL					GIsGuarded;
CORE_API extern UBOOL					GIsRequestingExit;
CORE_API extern UBOOL					GIsStrict;
CORE_API extern UBOOL					GIsCollectingGarbage;
CORE_API extern UBOOL                   GScriptEntryTag;
CORE_API extern UBOOL                   GLazyLoad;
CORE_API extern UBOOL                   GSafeMode;
CORE_API extern UBOOL					GUnicode;
CORE_API extern UBOOL					GUnicodeOS;
CORE_API extern UBOOL					GIsUTracing;
CORE_API extern UBOOL					GUseTransientNames;
CORE_API extern UBOOL					GIsDemoPlayback;
CORE_API extern	UBOOL					bUPakBuild;
CORE_API extern	UBOOL					GEnableRunAway;
//CORE_API extern UBOOL					GIsOnStrHack;
CORE_API extern class FGlobalMath		GMath;
CORE_API extern	URenderDevice*			GRenderDevice;
CORE_API extern class FArchive*         GDummySave;
CORE_API extern UViewport*				GCurrentViewport;
// Shambler
CORE_API extern UBOOL					GDuplicateNames;
CORE_API extern UBOOL					GAntiTCC; // Prevents temporary console commands...i.e. online use of set command on non-config variables
CORE_API extern UBOOL					GTCCUsed;
CORE_API extern UBOOL					GScriptHooksEnabled;
CORE_API extern class					GameCacheFolderIni*	GCacheFolderIni;
CORE_API extern	UINT					GFrameNumber; // Current frame number, increased every tick.

extern UBOOL GExitPurge;

#ifdef UTPG_MD5
CORE_API extern UZQ5Gnoyr*     TK5Ahisl; // MD5Table
CORE_API extern UBOOL          ForceMD5Calc;
CORE_API extern UBOOL          DisableMD5;
CORE_API extern UObject*       MD5NotifyObject;
#endif

// Per module globals.
#if __LINUX__ || MACOSX || MACOSXPPC
    DLL_EXPORT TCHAR GPackage[];
#else
    extern "C" DLL_EXPORT TCHAR GPackage[];
#endif

// Normal includes.
#include "UnFile.h"			// Low level utility code.
#include "Sha.h"			// SHA256.
#include "UnObjVer.h"		// Object version info.
#include "UnArc.h"			// Archive class.
#include "UnTemplate.h"     // Dynamic arrays.
#include "UnName.h"			// Global name subsystem.
#include "UnStack.h"		// Script stack definition.
#include "UnObjBas.h"		// Object base class.
#include "UnCoreNet.h"		// Core networking.
#include "CoreClasses.h"	// CoreClasses for locale management.
#include "UnCorObj.h"		// Core object class definitions.
#include "UnClass.h"		// Class definition.
#include "UnType.h"			// Base property type.
#include "UnScript.h"		// Script class.
#include "UFactory.h"		// Factory definition.
#include "UExporter.h"		// Exporter definition.
#include "UnCache.h"		// Cache based memory management.
#include "UnMem.h"			// Stack based memory management.
#include "UnCId.h"          // Cache ID's.
#include "UnBits.h"         // Bitstream archiver.
#include "UnMath.h"         // Vector math functions.
#include "UnFileCache.h"    // Unreal local file cache handler.

#ifdef UTPG_MD5
#include "UnMD5.h"      // MD5 table
#endif

#ifdef UTPG_HOOK_SCAN
#include "UnHook.h"
#endif

#if __STATIC_LINK
#include "UnCoreNative.h"
#endif

#include "EngineClasses.h"

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
