/*=============================================================================
	UnFile.cpp: ANSI C core.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

// Core includes.
#include "CorePrivate.h"

// ANSI C++ includes.
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#if !MACOSX && !MACOSXPPC
#include <malloc.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#else
#include <wctype.h>
#endif

/*-----------------------------------------------------------------------------
	Time.
-----------------------------------------------------------------------------*/

//
// String timestamp.
// !! Note to self: Move to UnVcWin32.cpp
// !! Make Linux version.
//
#if _MSC_VER
CORE_API FString appTimestamp()
{
	guard(appTimestamp);
	FString Result;
	TCHAR Tmp[1024];
	Tmp[0] = 0;
	if (_wstrdate_s(Tmp, ARRAY_COUNT(Tmp)) == 0)
	{
		Result = Tmp;
		Result += TEXT(" ");
	}
	if (_wstrtime_s(Tmp, ARRAY_COUNT(Tmp)) == 0)
	{
		Result += Tmp;
	}
	return Result;
	unguard;
}
#endif

//
// Get a GMT Ref
//
CORE_API FString appGetGMTRef()
{
#if __PSX2_EE__
#else
	guard(appGetGMTRef);
	struct tm newtime;
	FString GMTRef;
	time_t ltime, gtime;
	FLOAT diff;
	time(&ltime);

	// Standards are amazing --stijn
#ifdef _MSC_VER
	if (gmtime_s(&newtime, &ltime))
#else
	if (gmtime_r(&ltime, &newtime))
#endif
	{
		gtime = mktime(&newtime);
		diff = (ltime - gtime) / 3600;
	}
	else
	{
		diff = 0;
	}
	GMTRef = FString::Printf((diff > 0) ? TEXT("+%1.1f") : TEXT("%1.1f"), diff);
	return GMTRef;
	unguard;
#endif
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

//
// Copy a string with length checking.
//warning: Behavior differs from strncpy; last character is zeroed.
//
TCHAR* appStrncpy( TCHAR* Dest, const TCHAR* Src, INT MaxLen )
{
	guard(appStrncpy);

	if (appStrlen(Src) >= MaxLen)
	{
		wcsncpy(Dest, Src, MaxLen - 1);
		Dest[MaxLen - 1] = 0;
	}
	else
	{
		wcscpy(Dest, Src); // stijn: mem safety ok
	}
	return Dest;

	unguard;
}

//
// Concatenate a string with length checking
//
TCHAR* appStrncat( TCHAR* Dest, const TCHAR* Src, INT MaxLen )
{
	guard(appStrncat);
	INT Len = appStrlen(Dest);
	TCHAR* NewDest = Dest + Len;
	if( (MaxLen-=Len) > 0 )
	{
		appStrncpy( NewDest, Src, MaxLen );
		NewDest[MaxLen-1] = 0;
	}
	return Dest;
	unguard;
}

//
// Standard string functions.
//
// stijn: FIXME: appSprintf is _NOT_ safe and should be avoided at all costs!
CORE_API INT appSprintf( TCHAR* Dest, const TCHAR* Fmt, ... )
{
	int Result=0;
	GET_VARARGS_RESULT(Dest,4096,Fmt,Result);
        return Result;
}
CORE_API INT appSnprintf(TCHAR* Dest, INT Size, const TCHAR* Fmt, ...)
{
	INT Result;
	GET_VARARGS_RESULT(Dest, Size, Fmt, Result);
	return Result;
}

#if _MSC_VER
CORE_API INT appGetVarArgs( TCHAR* Dest, INT Count, const TCHAR* fmt, ... )
{
	va_list ArgPtr;
	va_start( ArgPtr, fmt);
#if UNICODE
	INT Result = _vsnwprintf_s( Dest, 65536, Count, fmt, ArgPtr );
#else
	INT Result = _vsnprintf( Dest, Count, fmt, ArgPtr );
#endif
	va_end( ArgPtr );
	return Result;
}
#endif

    // Changes by Smirftsch for Unicode
CORE_API TCHAR* appStrstr( const TCHAR* String, const TCHAR* Find )
{
	// !!! FIXME: casting away constness here. Need to fix function signature, but it touches a lot of code.  --ryan.
	return (TCHAR *) wcsstr( String, Find );
}

CORE_API TCHAR* appStrchr( const TCHAR* String, int c )
{
	// !!! FIXME: casting away constness here. Need to fix function signature, but it touches a lot of code.  --ryan.
	return (TCHAR *) wcschr( String, c );
}

#if MACOSX || MACOSXPPC
int wcscasecmp(const wchar_t *s1, const wchar_t *s2)
{
        wchar_t c1, c2;

        for (; *s1; s1++, s2++) {
                c1 = towlower(*s1);
                c2 = towlower(*s2);
                if (c1 != c2)
                        return ((int)c1 - c2);
        }
        return (-*s2);
}
int wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n)
{
        wchar_t c1, c2;

        if (n == 0)
                return (0);
        for (; *s1; s1++, s2++) {
                c1 = towlower(*s1);
                c2 = towlower(*s2);
                if (c1 != c2)
                        return ((int)c1 - c2);
                if (--n == 0)
                        return (0);
        }
        return (-*s2);
}
#endif

CORE_API TCHAR* appStrupr( TCHAR* String )
{
#if _MSC_VER
    #if UNICODE
        return _wcsupr( String );
    #else
        return strupr( String );
    #endif
#else
	#if UNICODE
    INT Len=appStrlen(String);
    INT i;
    TCHAR* ConvertString=appStaticString4096();
    appStrcpy(ConvertString,String);
    for (i=0;i<=Len;i++)
        ConvertString[i]=towupper(ConvertString[i]);
    return ConvertString;

    //return ANSI_TO_TCHAR(strupr( TCHAR_TO_ANSI(String) ));; // FIXME!!! ugly hack since no wcsupr for Linux
    #else
        return strupr( String );
    #endif
#endif
}

CORE_API INT appStrtoi( const TCHAR* Start, TCHAR** End, INT Base )
{
#if UNICODE
	return wcstoul( Start, End, Base );
#else
	return strtoul( Start, End, Base );
#endif
}

CORE_API INT appStrnicmp( const TCHAR* A, const TCHAR* B, INT Count )
{
#if _MSC_VER
    #if UNICODE
        return _wcsnicmp( A, B, Count );
    #else
        return strnicmp( A, B, Count );
    #endif
#else
    #if UNICODE
        return wcsncasecmp( A, B, Count );
    #else
        return strnicmp( A, B, Count );
    #endif
#endif
}

CORE_API TCHAR* appStrdup(const TCHAR* Src)
{
#if _MSC_VER
	return _wcsdup(Src);
#else
    return wcsdup(Src);
#endif
}

//
// Gets the extension of a file, such as "PCX".  Returns NULL if none.
// string if there's no extension.
//
CORE_API const TCHAR* appFExt( const TCHAR* fname )
{
	guard(appFExt);

	while( appStrstr(fname,TEXT(":")) )
		fname = appStrstr(fname,TEXT(":"))+1;

	while( appStrstr(fname,TEXT("/")) )
		fname = appStrstr(fname,TEXT("/"))+1;

	while( appStrstr(fname,TEXT(".")) )
		fname = appStrstr(fname,TEXT("."))+1;

	return fname;
	unguard;
}

//
// appFilePathName: chop the file basename off of a file path string
//
CORE_API FString appFilePathName(const TCHAR* InName)
{
	FString Tmp = InName;

	// test all possible path separators starting from the right side
	INT a = Tmp.InStr(PATH_SEPARATOR	, 1);
	INT b = Tmp.InStr(TEXT("/")			, 1);
	INT c = Tmp.InStr(TEXT("\\")		, 1);
	INT d = Tmp.InStr(TEXT(":")			, 1);

	// if we do not find any of them, then InName is just a basename
	if (a + b + c + d == -4)
		return TEXT("");

	// take the rightmost path separator and return the substring to the left of the separator
	INT RightMost = Max(Max(a, b), Max(c, d));

	// make sure we include the separator itself
	return Tmp.Left(RightMost + 1);
}

//
// appFilePathName: return the basename from a file path string
//
CORE_API FString appFileBaseName(const TCHAR* InName)
{
	FString Tmp = InName;

	// test all possible path separators starting from the right side
	INT a = Tmp.InStr(PATH_SEPARATOR	, 1);
	INT b = Tmp.InStr(TEXT("/")			, 1);
	INT c = Tmp.InStr(TEXT("\\")		, 1);
	INT d = Tmp.InStr(TEXT(":")			, 1);

	// if we do not find any of them, then InName is just a basename
	if (a + b + c + d == -4)
		return Tmp;

	// take the rightmost path separator and return the substring to the right of the separator
	INT RightMost = Max(Max(a, b), Max(c, d));

	// make sure we chop off the separator itself. we assume that all separators are 1 character long here...
	return Tmp.Mid(RightMost + 1);
}

/*-----------------------------------------------------------------------------
	Sorting.
-----------------------------------------------------------------------------*/

CORE_API void appQsort( void* Base, INT Num, INT Width, int(CDECL *Compare)(const void* A, const void* B ) )
{
	qsort( Base, Num, Width, Compare );
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
