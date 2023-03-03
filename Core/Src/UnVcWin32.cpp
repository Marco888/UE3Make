/*=============================================================================
	UnVcWin32.cpp: Visual C++ Windows 32-bit core.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/
// Core includes.
#include "CorePrivate.h"


#if _MSC_VER

// Windows and runtime includes.
#pragma warning( disable : 4201 )
#include <windows.h>
#include <commctrl.h> //oldver: Remnant of old editor!!
#include <stdio.h>
#include <float.h>
#include <time.h>
#include <io.h>
#include <direct.h>
#include <errno.h>
#include <sys/stat.h>
#include <iptypes.h>
#include <iphlpapi.h>
#include <stdlib.h>

#include "dep.h"

/*-----------------------------------------------------------------------------
	Unicode helpers.
-----------------------------------------------------------------------------*/

#if UNICODE
CORE_API ANSICHAR* winToANSI( ANSICHAR* ACh, const TCHAR* InUCh, INT Count )
{
	guardSlow(winToANSI);
	WideCharToMultiByte( CP_ACP, 0, InUCh, -1, ACh, Count, NULL, NULL );
	return ACh;
	unguardSlow;
}
CORE_API ANSICHAR* winToOEM( ANSICHAR* ACh, const TCHAR* InUCh, INT Count )
{
	guardSlow(winToOEM);
	WideCharToMultiByte( CP_OEMCP, 0, InUCh, -1, ACh, Count, NULL, NULL );
	return ACh;
	unguardSlow;
}
CORE_API INT winGetSizeANSI( const TCHAR* InUCh )
{
	guardSlow(winGetSizeANSI);
	return WideCharToMultiByte( CP_ACP, 0, InUCh, -1, NULL, 0, NULL, NULL );
	unguardSlow;
}
CORE_API TCHAR* winToUNICODE( TCHAR* UCh, const ANSICHAR* InACh, INT Count )
{
	MultiByteToWideChar( CP_ACP, 0, InACh, -1, UCh, Count );
	return UCh;
}
CORE_API INT winGetSizeUNICODE( const ANSICHAR* InACh )
{
	return MultiByteToWideChar( CP_ACP, 0, InACh, -1, NULL, 0 );
}
// String conversion helpers. These convert between ANSICHAR, TCHAR, and UNICHAR.
// These conversion function must terminate the output string


CORE_API size_t appToAnsiInPlace(ANSICHAR* Dst, const TCHAR* Src, size_t MaxCount)
{
	if (MaxCount == 0) return 0;
	const size_t SrcLen = appStrlen(Src);
	// We must leave room for a null terminator
	if (SrcLen >= MaxCount)
	{
		// we have to truncate
		WideCharToMultiByte(CP_ACP, 0, Src, static_cast<INT>(MaxCount) - 1, Dst, static_cast<INT>(MaxCount) - 1, NULL, NULL);
		// manual null-termination
		Dst[MaxCount - 1] = 0;
		return MaxCount;
	}
	else
	{
		// no truncation needed
		WideCharToMultiByte(CP_ACP, 0, Src, static_cast<INT>(SrcLen) + 1, Dst, static_cast<INT>(SrcLen) + 1, NULL, NULL);
		return SrcLen + 1;
	}
}

CORE_API size_t appToUnicodeInPlace(UNICHAR* Dst, const TCHAR* Src, size_t MaxCount)
{
	if (MaxCount == 0) return 0;
	const size_t SrcLen = appStrlen(Src);
	if (SrcLen >= MaxCount)
	{
		memcpy(Dst, Src, (MaxCount - 1) * sizeof(TCHAR));
		Dst[MaxCount - 1] = 0;
		return MaxCount;
	}
	else
	{
		memcpy(Dst, Src, (SrcLen + 1) * sizeof(TCHAR));
		return SrcLen + 1;
	}
}

CORE_API size_t appFromAnsiInPlace(TCHAR* Dst, const ANSICHAR* Src, size_t MaxCount)
{
	if (MaxCount == 0)
		return 0;
	const size_t SrcLen = strlen(Src);
	if (SrcLen >= MaxCount)
	{
		MultiByteToWideChar(CP_ACP, 0, Src, static_cast<INT>(MaxCount) - 1, Dst, static_cast<INT>(MaxCount) - 1);
		Dst[MaxCount - 1] = 0;
		return MaxCount;
	}
	else
	{
		MultiByteToWideChar(CP_ACP, 0, Src, static_cast<INT>(SrcLen) + 1, Dst, static_cast<INT>(SrcLen) + 1);
		return SrcLen + 1;
	}
}

CORE_API size_t appFromUnicodeInPlace(TCHAR* Dst, const UNICHAR* Src, size_t MaxCount)
{
	if (MaxCount == 0) return 0;
	const size_t SrcLen = appStrlen(reinterpret_cast<const TCHAR*>(Src));
	if (SrcLen >= MaxCount)
	{
		memcpy(Dst, Src, (MaxCount - 1) * sizeof(TCHAR));
		// manual null-termination
		Dst[MaxCount - 1] = 0;
		return MaxCount;
	}
	else
	{
		// no truncation needed
		memcpy(Dst, Src, (SrcLen + 1) * sizeof(TCHAR));
		return SrcLen + 1;
	}
}

CORE_API size_t appFromUtf8InPlace(TCHAR* Dst, const ANSICHAR* Src, size_t MaxCount)
{
	if (MaxCount == 0) return 0;
	const size_t SrcLen = MultiByteToWideChar( CP_UTF8, 0, Src, -1, NULL, 0 );
	if (SrcLen >= MaxCount)
	{
		INT Result = MultiByteToWideChar(CP_UTF8, 0, Src, static_cast<INT>(MaxCount) - 1, Dst, static_cast<INT>(MaxCount) - 1);
		Dst[MaxCount - 1] = 0;
		return Result;
	}
	else
	{
		return MultiByteToWideChar(CP_UTF8, 0, Src, static_cast<INT>(SrcLen) + 1, Dst, static_cast<INT>(SrcLen) + 1);
	}
}
#endif

/*-----------------------------------------------------------------------------
	FOutputDeviceWindowsError.
-----------------------------------------------------------------------------*/

//
// Immediate exit.
//
CORE_API void appRequestExit( UBOOL Force, FString Msg )
{
	guard(appForceExit);
	debugf( TEXT("appRequestExit(%i) %ls"), Force, *Msg );
	if( Force )
	{
		// Force immediate exit. Dangerous because config code isn't flushed, etc.
		ExitProcess( 1 );
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
	unguard;
}

//
// Get system error.
//
CORE_API const TCHAR* appGetSystemErrorMessage( INT Error )
{
	guard(appGetSystemErrorMessage);
	static TCHAR Msg[1024];
	*Msg = 0;
	if( Error==0 )
		Error = GetLastError();
#if UNICODE
	if( !GUnicodeOS )
	{
		ANSICHAR ACh[1024];
		FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ACh, 1024, NULL );
		appStrcpy( Msg, appFromAnsi(ACh) );
	}
	else
#endif
	{
		FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), Msg, 1024, NULL );
	}
	if( appStrchr(Msg,'\r') )
		*appStrchr(Msg,'\r')=0;
	if( appStrchr(Msg,'\n') )
		*appStrchr(Msg,'\n')=0;
	return Msg;
	unguard;
}

/*-----------------------------------------------------------------------------
	USystem.
-----------------------------------------------------------------------------*/

//
// System manager.
//

const TCHAR* USystem::GetOS()
{
	return TEXT("Windows");
}
void USystem::appRelaunch(const TCHAR* Cmd)
{
#if UNICODE
	if (!GUnicodeOS)
	{
		ANSICHAR ThisFile[256];
		GetModuleFileNameA(NULL, ThisFile, ARRAY_COUNT(ThisFile));
		ShellExecuteA(NULL, "open", ThisFile, TCHAR_TO_ANSI(Cmd), TCHAR_TO_ANSI(appBaseDir()), SW_SHOWNORMAL);
	}
	else
#endif
	{
		TCHAR ThisFile[256];
		GetModuleFileName(NULL, ThisFile, ARRAY_COUNT(ThisFile));
		ShellExecute(NULL, TEXT("open"), ThisFile, Cmd, appBaseDir(), SW_SHOWNORMAL);
	}
}
void USystem::DumpMemStat(FOutputDevice& Ar)
{
	MEMORYSTATUS B; B.dwLength = sizeof(B);
	GlobalMemoryStatus(&B);
	Ar.Logf(TEXT("Memory available: Phys=%iK Pagef=%iK Virt=%iK"), B.dwAvailPhys / 1024, B.dwAvailPageFile / 1024, B.dwAvailVirtual / 1024);
	Ar.Logf(TEXT("Memory load = %i%%"), B.dwMemoryLoad);
}

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

//
// Copy text to clipboard.
//
CORE_API void appClipboardCopy( const TCHAR* Str )
{
	guard(appClipboardCopy);
	if( OpenClipboard(GetActiveWindow()) )
	{
		verify(EmptyClipboard());
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			INT Count = WideCharToMultiByte(CP_ACP,0,Str,-1,NULL,0,NULL,NULL);
			ANSICHAR* Data = (ANSICHAR*)GlobalAlloc( GMEM_DDESHARE, Count+1 );
			WideCharToMultiByte(CP_ACP,0,Str,-1,Data,Count,NULL,NULL);
			Data[Count] = 0;
			verify(SetClipboardData( CF_TEXT, Data ));
		}
		else
#endif
		{
			TCHAR* Data = (TCHAR*)GlobalAlloc( GMEM_DDESHARE, sizeof(TCHAR)*(appStrlen(Str)+1) );
			appStrcpy( Data, Str );
			verify(SetClipboardData( GUnicode ? CF_UNICODETEXT : CF_TEXT, Data ));
		}
		verify(CloseClipboard());
	}
	unguard;
}

//
// Paste text from clipboard.
//
CORE_API FString appClipboardPaste()
{
	guard(appClipboardPaste);
	FString Result;
	if( OpenClipboard(GetActiveWindow()) )
	{
		void* V = NULL;
#if 0 // Marco: This seams to be broken if clipboard has ANSI text instead of UNICODE.
		if( GUnicode && GUnicodeOS )
		{
			V = GetClipboardData( CF_UNICODETEXT );
			debugf(TEXT("GetUnicode %ls"), ((TCHAR*)V));
			if( V )
				Result = (TCHAR*)V;
		}
#endif
		if( !V )
		{
			V = GetClipboardData( CF_TEXT );
			if( V )
			{
				ANSICHAR* ACh = (ANSICHAR*)V;
				INT i;
				for( i=0; ACh[i]; i++ );
				TArray<TCHAR> Ch(i+1);
				for( i=0; i<Ch.Num(); i++ )
					Ch(i)=FromAnsi(ACh[i]);
				Result = &Ch(0);
			}
		}
		if( !V )
		{
			Result = TEXT("");
		}
		verify(CloseClipboard());
	}
	else Result=TEXT("");
	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	DLLs.
-----------------------------------------------------------------------------*/

//
// Load a library.
//warning: Intended only to be called by UPackage::BindPackage. Don't call directly.
//
static void* GetDllHandleHelper( const TCHAR* Filename )
{
	return TCHAR_CALL_OS(LoadLibraryW(Filename),LoadLibraryA(TCHAR_TO_ANSI(Filename)));
}
CORE_API void* appGetDllHandle( const TCHAR* Filename )
{
	guard(appGetDllHandle);
	check(Filename);

	void* Result = GetDllHandleHelper( Filename );
	if( !Result )
		Result = GetDllHandleHelper( *(FString(Filename) + DLLEXT) );
	if( !Result )
		Result = GetDllHandleHelper( *(US + TEXT("_") + Filename + DLLEXT) );

	return Result;
	unguard;
}

//
// Free a DLL.
//
CORE_API void appFreeDllHandle( void* DllHandle )
{
	guard(appFreeDllHandle);
	check(DllHandle);

	FreeLibrary( (HMODULE)DllHandle );

	unguard;
}

//
// Lookup the address of a DLL function.
//
CORE_API void* appGetDllExport( void* DllHandle, const TCHAR* ProcName )
{
	guard(appGetDllExport);
	check(DllHandle);
	check(ProcName);

	return (void*)GetProcAddress( (HMODULE)DllHandle, TCHAR_TO_ANSI(ProcName) );

	unguard;
}

CORE_API const void appDebugMessagef( const TCHAR* Fmt, ... )
{
	TCHAR TempStr[65536];
	GET_VARARGS(TempStr, 65536, Fmt);

	guard(appDebugMessagef);
	//MessageBox(NULL,*TempStr,TEXT("appDebugMessagef"),MB_OK);
	OutputDebugStringW(TempStr);
	OutputDebugStringW(L"\n");
	unguard;
}

#if 1 //U2Ed
CORE_API const void appMsgf( const TCHAR* Fmt, ... )
{
	guard(appMsgf);
	TCHAR TempStr[4096]=TEXT("");
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt );
	MessageBox( NULL, TempStr, TEXT("Message"), MB_OK );
	unguard;
}

CORE_API const void appGetLastError( void )
{
	guard(appGetLastError);
	TCHAR TempStr[4096]=TEXT("");
	appSprintf( TempStr, TEXT("GetLastError : %d\n\n%ls"),
		GetLastError(), appGetSystemErrorMessage() );
	MessageBox( NULL, TempStr, TEXT("System Error"), MB_OK );
	unguard;
}

// This is a special version of ANSI_TO_TCHAR that does things a little differently.  The code editing window seems
// to need this version because the original ANSI_TO_TCHAR makes it crash on large scripts.
CORE_API TCHAR* LEGEND_ANSI_TO_TCHAR(char* str)
{
	int iLength = winGetSizeUNICODE(str);
	TCHAR* pBuffer = new TCHAR[iLength];
	appStrcpy(pBuffer,TEXT(""));
	TCHAR* ret = winToUNICODE(pBuffer,str,iLength);
	//delete [] pBuffer;
	return ret;
}
#endif

//
// Break the debugger.
//
#ifndef DEFINED_appDebugBreak
void appDebugBreak()
{
	guard(appDebugBreak);
	::DebugBreak();
	unguard;
}
#endif

#if HUNG_SAFE_BUILD
INT UnGuardBlock::GuardIndex=-1;
const TCHAR* UnGuardBlock::GuardTexts[MAX_RYANS_HACKY_GUARD_BLOCKS];

void UnGuardBlock::OutputToFile()
{
	HANDLE Handle    = TCHAR_CALL_OS( CreateFileW( TEXT("HungTrace.log"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ), CreateFileA("HungTrace.log", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) );
	if( Handle==INVALID_HANDLE_VALUE )
		return;
	INT Result=0;
	INT Num = Min<INT>(MAX_RYANS_HACKY_GUARD_BLOCKS-1,GuardIndex);
	for( INT i=0; i<=Num; ++i )
	{
		if( !WriteFile(Handle,GuardTexts[i],wcslen(GuardTexts[i])*sizeof(TCHAR),(DWORD*)&Result,NULL) ||
			!WriteFile( Handle, TEXT(" -> "), 4*sizeof(TCHAR),(DWORD*)&Result,NULL) )
			return;
	}
	CloseHandle(Handle);
}
#endif

/*-----------------------------------------------------------------------------
	Timing.
-----------------------------------------------------------------------------*/

//
// Get file time.
//
CORE_API DWORD appGetTime( const TCHAR* Filename )
{
	guard(appGetTime);

  struct _stat Buf;
	INT Result = 0;
#if UNICODE
	if( GUnicodeOS )
	{
		Result = _wstat(Filename,&Buf);
	}
	else
#endif
	{
		Result = _stat(TCHAR_TO_ANSI(Filename),&Buf);
	}
	if( Result==0 )
	{
		return (DWORD) Buf.st_mtime;
	}
	return 0;

	unguard;
}
void TimerBegin()
{
	// This will increase the precision of the kernel interrupt
	// timer. Although this will slightly increase resource usage
	// this will also increase the precision of sleep calls and
	// this will in turn increase the stability of the framerate
	timeBeginPeriod(1);
}
void TimerEnd()
{
	// Restore the kernel timer config
	timeEndPeriod(1);
}

//
// Get time in seconds. Origin is arbitrary.
//
CORE_API FTime appSeconds()
{
	#define DEFINED_appSeconds 1
	//elmuerte: backported from UT2004
	static DWORD  InitialTime = timeGetTime();
	static DOUBLE TimeCounter = 0.0;

	// Accumulate difference to prevent wraparound.
	DWORD NewTime = timeGetTime();
	DWORD DeltaTime;

	if (NewTime < InitialTime) // Ignore any timewarps backwards (including wraparound)
		DeltaTime = 0.0;
	else
		DeltaTime = NewTime - InitialTime;

	TimeCounter += DeltaTime * 0.001;	// Convert to Seconds.milliseconds
	InitialTime = NewTime;

	return TimeCounter;
}

//
// Return number of CPU cycles passed. Origin is arbitrary.
//
CORE_API QWORD appCycles()
{
	return __rdtsc();
	//return (DWORD) appSeconds();
}

//
// Sleep this thread for Seconds, 0.0 means release the current
// timeslice to let other threads get some attention.
//
CORE_API void appSleep( FLOAT Seconds )
{
	guard(appSleep);
	Sleep( (DWORD)(Seconds * 1000.0) );
	unguard;
}

//
// Return the system time.
//
CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec )
{
	guard(appSystemTime);

	SYSTEMTIME st;
	GetLocalTime( &st );

	Year		= st.wYear;
	Month		= st.wMonth;
	DayOfWeek	= st.wDayOfWeek;
	Day			= st.wDay;
	Hour		= st.wHour;
	Min			= st.wMinute;
	Sec			= st.wSecond;
	MSec		= st.wMilliseconds;

	unguard;
}

#if HUNG_SAFE_BUILD
class CORE_API FHungChecker
{
public:
	INT GameStateCounter;

private:
	// Variables.
	DWORD   ThreadId;
	INT		OldStateCounter;
	BYTE	bExitThread;
	QWORD	CrashTimer;
	HANDLE	appThread;

	static inline QWORD CurrentTime() // should be threading safe...
	{
		SYSTEMTIME st;
		GetLocalTime( &st );

		enum
		{
			ESPerMin=60,
			ESPerHour=(ESPerMin*60),
			ESPerDay=(ESPerHour*24),
			ESPerMonth=(ESPerDay*30),
			ESPerYear=(ESPerDay*365),
		};
		return st.wSecond + (st.wMinute*ESPerMin) + (st.wHour*ESPerHour) + (st.wDay*ESPerDay)
			 + (st.wMonth*ESPerMonth) + ((st.wYear-2010)*ESPerYear);
	}

	// Resolution thread entrypoint.
	static DWORD __stdcall HungThreadEntry( void* Arg )
	{
		FHungChecker& C = *((FHungChecker*)Arg);
		while( !C.bExitThread )
		{
			if( C.OldStateCounter!=C.GameStateCounter )
			{
				C.OldStateCounter = C.GameStateCounter;
				C.CrashTimer = CurrentTime();
				// debugf(TEXT("%i"),INT(C.CrashTimer));
			}
			else if( (CurrentTime()-C.CrashTimer)>15 )
			{
				C.ThreadId = 0;
				UnGuardBlock::OutputToFile();
				ExitProcess( 1 );
			}
		}
		C.ThreadId = 0;
		return 0;
	}

public:
	// Functions.
	FHungChecker()
		: bExitThread(0), CrashTimer(CurrentTime()), GameStateCounter(0), appThread(GetCurrentThread())
	{
		HANDLE hThread = CreateThread( NULL, 0, HungThreadEntry, this, 0, &ThreadId );
		check(hThread);
		CloseHandle( hThread );
	}
	~FHungChecker()
	{
		bExitThread = 1;
		while( ThreadId!=0 )
		{}
	}
	inline void Refresh()
	{
		++GameStateCounter;
	}
};
#endif

//
// Return seconds counter.
//
inline CORE_API FTime appSecondsSlow()
{
	return appSeconds();
}

/*-----------------------------------------------------------------------------
	File finding.
-----------------------------------------------------------------------------*/

//
// Clean out the file cache.
//
static INT GetFileAgeDays( const TCHAR* Filename )
{
	guard(GetFileAgeDays);
	struct _stat Buf;
	INT Result = 0;
#if UNICODE
	if( GUnicodeOS )
	{
		Result = _wstat(Filename,&Buf);
	}
	else
#endif
	{
		Result = _stat(TCHAR_TO_ANSI(Filename),&Buf);
	}
	if( Result==0 )
	{
		time_t CurrentTime, FileTime;
		FileTime = Buf.st_mtime;
		time( &CurrentTime );
		return appRound(difftime(CurrentTime,FileTime) / 60 / 60 / 24);
	}
	return 0;
	unguard;
}
CORE_API void appCleanFileCache()
{
	guard(appCleanFileCache);

	// Delete all temporary files.
	guard(DeleteTemps);
	FString Temp = FString::Printf( TEXT("%ls") PATH_SEPARATOR TEXT("*.tmp"), *GSys->CachePath );
	TArray<FString> Found = GFileManager->FindFiles( *Temp, 1, 0 );
	for( INT i=0; i<Found.Num(); i++ )
	{
		Temp = FString::Printf( TEXT("%ls") PATH_SEPARATOR TEXT("%ls"), *GSys->CachePath, *Found(i) );
		debugf( TEXT("Deleting temporary file: %ls"), *Temp );
		GFileManager->Delete( *Temp );
	}
	unguard;

	// Delete cache files that are no longer wanted.
	guard(DeleteExpired);
	TArray<FString> Found = GFileManager->FindFiles( *(GSys->CachePath * TEXT("*") + GSys->CacheExt), 1, 0 );
	if( GSys->PurgeCacheDays )
	{
		for( INT i=0; i<Found.Num(); i++ )
		{
			FString Temp = FString::Printf( TEXT("%ls") PATH_SEPARATOR TEXT("%ls"), *GSys->CachePath, *Found(i) );
			INT DiffDays = GetFileAgeDays( *Temp );
			if( DiffDays > GSys->PurgeCacheDays )
			{
				debugf( TEXT("Purging outdated file from cache: %ls (%i days old)"), *Temp, DiffDays );
				GFileManager->Delete( *Temp );
			}
		}
	}
	unguard;

	unguard;
}

/*-----------------------------------------------------------------------------
	Guids.
-----------------------------------------------------------------------------*/

//
// Create a new globally unique identifier.
//
CORE_API FGuid appCreateGuid()
{
	guard(appCreateGuid);

	FGuid Result;
	CoCreateGuid( (GUID*)&Result );
	return Result;

	unguard;
}

/*-----------------------------------------------------------------------------
	Command line.
-----------------------------------------------------------------------------*/

// Get startup directory.
CORE_API const TCHAR* appBaseDir()
{
	guard(appBaseDir);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		// Get directory this executable was launched from.
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[256];
			GetModuleFileNameA( hInstance, ACh, ARRAY_COUNT(ACh) );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetModuleFileName( hInstance, Result, ARRAY_COUNT(Result) );
		}
		INT i;
		for( i=appStrlen(Result)-1; i>0; i-- )
			if( Result[i-1]==PATH_SEPARATOR[0] || Result[i-1]=='/' )
				break;
		Result[i]=0;
	}
	return Result;
	unguard;
}

// Get computer name.
CORE_API const TCHAR* appComputerName()
{
	guard(appComputerName);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		DWORD Size=ARRAY_COUNT(Result);
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[ARRAY_COUNT(Result)];
			GetComputerNameA( ACh, &Size );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetComputerName( Result, &Size );
		}
		TCHAR* c;
		TCHAR* d;
		for( c=Result, d=Result; *c!=0; c++ )
			if( appIsAlnum(*c) )
				*d++ = *c;
		*d++ = 0;
	}
	return Result;
	unguard;
}

// Fetches the MAC address and prints it
const TCHAR* GetMACAddress()
{
	guard(GetMACaddress);
	return TEXT("");
	unguard;
}

// Get user name.
const TCHAR* appUserName()
{
	guard(appUserName);
	static TCHAR Result[256]=TEXT("");
	if( !Result[0] )
	{
		DWORD Size=ARRAY_COUNT(Result);
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			ANSICHAR ACh[ARRAY_COUNT(Result)];
			GetUserNameA( ACh, &Size );
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, Result, ARRAY_COUNT(Result) );
		}
		else
#endif
		{
			GetUserName( Result, &Size );
		}
		TCHAR* c;
		TCHAR* d;
		for( c=Result, d=Result; *c!=0; c++ )
			if( appIsAlnum(*c) )
				*d++ = *c;
		*d++ = 0;
	}
	return Result;
	unguard;
}

// Get launch package base name.
const TCHAR* appPackage()
{
	guard(appPackage);
    #if __UE3Make__
	return TEXT("UE3Make");
    #else
    return TEXT("UDKCompress");
    #endif
	unguard;
}

// Get launch package base name.
CORE_API const TCHAR* appPackageExe()
{
	guard(appPackageExe);
    #if __UE3Make__
	return TEXT("UE3Make");
    #else
    return TEXT("UDKCompress");
    #endif
	unguard;
}

/*-----------------------------------------------------------------------------
	App init/exit.
-----------------------------------------------------------------------------*/

//
// Platform specific initialization.
//
#define MMX         0x00800000
#define MMXPLUS     0x00400000
#define SSE         0x02000000
#define SSE2        0x04000000
#define SSE3        0x00000001
#define SSSE3       0x00000200
#define SSE41       0x00080000
#define SSE42       0x00100000
#define SSE4A       0x00000040
#define SSE5        0x00000800
#define A3DNOW      0x80000000
#define A3DNOWEXT   0x40000000

//
// Platform specific initialization.
//
static void DoCPUID()
{
 	int info[4]={-1};
    int infoext[4]={-1};

    unsigned ecx = 0;
    unsigned edx = 0;
    __cpuid(info, 0);
    if( info[0] >= 1 )
    {
        __cpuid(info, 1);
        ecx = info[2];
        edx = info[3];
    }
    __cpuid(info, 0x80000000);
    if( info[0] >= 0x80000001 )
		__cpuid(infoext, 0x80000001);
    if( MMX & edx )
		GIsMMX = 1;
    if( SSE & edx )
		GIsSSE = 1;
    if( SSE2 & edx )
		GIsSSE2 = 1;
    if( SSE3 & ecx )
		GIsSSE3 = 1;
    if( SSSE3 & ecx )
		GIsSSSE3 = 1;
	if( SSE41 & ecx )
		GIsSSE41 = 1;
    if( SSE42 & ecx )
		GIsSSE42 = 1;
    if( SSE5 & infoext[2] )
		GIsSSE5 = 1;
    if( SSE4A & infoext[2] )
		GIsSSE4A = 1;
    if( A3DNOW & infoext[3] )
		GIsA3DNOW = 1;
    if( MMXPLUS & infoext[3] )
		GIsMMXPLUS = 1;
	if( A3DNOWEXT & infoext[3] )
		GIsA3DNOWEXT = 1;
}
void GetCPUBrand()
{
	int CPUInfo[4] = {-1};
	char CPUBrandString[0x40];
	__cpuid(CPUInfo, 0x80000000);
	int nExIds = CPUInfo[0];

	memset(CPUBrandString, 0, sizeof(CPUBrandString));

	// Get the information associated with each extended ID.
	for (int i=0x80000000; i<=nExIds; ++i)
	{
		__cpuid(CPUInfo, i);
		// Interpret CPU brand string.
		if  (i == 0x80000002)
			memcpy(CPUBrandString, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000003)
			memcpy(CPUBrandString + 16, CPUInfo, sizeof(CPUInfo));
		else if  (i == 0x80000004)
			memcpy(CPUBrandString + 32, CPUInfo, sizeof(CPUInfo));
	}
	debugf( NAME_Init, TEXT("CPU Detected: %ls"),ANSI_TO_TCHAR(CPUBrandString));
}

void appPlatformPreInit()
{
	guard(appPlatformPreInit);

	// Check Windows version.
	guard(GetWindowsVersion);
	DWORD dwPlatformId, dwMajorVersion, dwMinorVersion, dwBuildNumber;
#if UNICODE
	if( GUnicode && !GUnicodeOS )
	{
		OSVERSIONINFOA Version;
		Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
		GetVersionExA(&Version);
		dwPlatformId   = Version.dwPlatformId;
		dwMajorVersion = Version.dwMajorVersion;
		dwMinorVersion = Version.dwMinorVersion;
		dwBuildNumber  = Version.dwBuildNumber;
	}
	else
#endif
	{
		OSVERSIONINFO Version;
		Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		GetVersionEx(&Version);
		dwPlatformId   = Version.dwPlatformId;
		dwMajorVersion = Version.dwMajorVersion;
		dwMinorVersion = Version.dwMinorVersion;
		dwBuildNumber  = Version.dwBuildNumber;
	}
	if( dwPlatformId==VER_PLATFORM_WIN32_NT )
	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows NT %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
		GUnicodeOS = 1;
	}
	else if( dwPlatformId==VER_PLATFORM_WIN32_WINDOWS && (dwMajorVersion>4 || dwMinorVersion>=10) )
	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows 98 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
	}
	else if( dwPlatformId==VER_PLATFORM_WIN32_WINDOWS )
	{
		debugf( NAME_Init, TEXT("Detected: Microsoft Windows 95 %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
	}
	else
	{
		debugf( NAME_Init, TEXT("Detected: Windows %u.%u (Build: %u)"), dwMajorVersion, dwMinorVersion, dwBuildNumber );
	}
	unguard;

	if (ParseParam(appCmdLine(), TEXT("nodep")))
		SetProcessDEPPolicy(0);
	else
		SetProcessDEPPolicy(1);

	unguard;
}
BOOL WINAPI CtrlCHandler( DWORD dwCtrlType )
{
	if( !GIsRequestingExit )
	{
		PostQuitMessage( 0 );
		GIsRequestingExit = 1;
	}
	else
	{
		GWarn->Logf( TEXT("CTRL-C has been pressed. Aborted!") );
		_exit(42);
	}
	return 1;
}

#ifndef _DEBUG
//#pragma optimize( "g", off ) // Fix CPU brand string mangling in release mode
#endif
void myPurecallHandler(void)
{
	appMsgf(TEXT("Core: In _purecall_handler."));
   appExit();
}

void appPlatformInit()
{
	guard(appPlatformInit);

	// System initialization.
	GSys = new USystem;
	GSys->AddToRoot();
	for( INT i=0; i<GSys->Suppress.Num(); i++ )
		GSys->Suppress(i).SetFlags( RF_Suppress );

	// Ctrl+C handling.
	SetConsoleCtrlHandler( CtrlCHandler, 1 );

	// Randomize.
	srand( (unsigned)time( NULL ) );

	// Get memory.
	guard(GetMemory);
	MEMORYSTATUS M;
	GlobalMemoryStatus(&M);
	GPhysicalMemory=M.dwTotalPhys;
	unguard;

	// CPU speed.
	guard(CheckCpuSpeed);
	GSecondsPerCycle = 10000;
	unguard;

	// Get CPU info.
	guard(GetCpuInfo);
	SYSTEM_INFO SI;
	GetSystemInfo(&SI);
	GPageSize = SI.dwPageSize;
	check(!(GPageSize&(GPageSize-1)));
	GProcessorCount=SI.dwNumberOfProcessors;
	unguard;

	#if DEBUG_VIRTUALS_ENABLED
	_set_purecall_handler(myPurecallHandler);
	#endif
	unguard;
}
#ifndef _DEBUG
#pragma optimize("", on)
#endif

void appPlatformPreExit()
{
	guard(appPlatformPreExit);
	unguard;
}
void appPlatformExit()
{
	guard(appPlatformExit);
	TimerEnd();
	unguard;
}

//
// Set low precision mode.
//
void appEnableFastMath( UBOOL Enable )
{
	guard(appEnableFastMath);
	//unsigned int control_word;
	//_controlfp_s( &control_word, Enable ? (_PC_24) : (_PC_64), _MCW_PC );
	unguard;
}

#endif
/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
