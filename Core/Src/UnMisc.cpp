/*=============================================================================
	UnMisc.cpp: Various core platform-independent functions.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

// Core includes.
#include "CorePrivate.h"
#include <atomic>
static const char *sha2_hex_digits = "0123456789abcdef";

/*-----------------------------------------------------------------------------
	FOutputDevice implementation.
-----------------------------------------------------------------------------*/

void FOutputDevice::Log( EName Event, const TCHAR* Str )
{
	if( !FName::SafeSuppressed(Event) )
		Serialize( Str, Event );
}
void FOutputDevice::Log( const TCHAR* Str )
{
	if( !FName::SafeSuppressed(NAME_Log) )
		Serialize( Str, NAME_Log );
}
void FOutputDevice::Log( const FString& S )
{
	if( !FName::SafeSuppressed(NAME_Log) )
		Serialize( *S, NAME_Log );
}
void FOutputDevice::Log( enum EName Type, const FString& S )
{
	if( !FName::SafeSuppressed(Type) )
		Serialize( *S, Type );
}
void FOutputDevice::Logf( EName Event, const TCHAR* Fmt, ... )
{
	//if( !FName::SafeSuppressed(Event) )
	{
		INT		BufSize = 1024;
		TCHAR* Buffer = NULL;
		INT		Result = -1;

		while (Result == -1)
		{
			Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, BufSize * sizeof(TCHAR), TEXT("")));
			if (!Buffer)
			{
				Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, 1 * sizeof(TCHAR), TEXT("")));
				Result = 0;
				break;
			}
			GET_VARARGS_RESULT(Buffer, BufSize - 1, Fmt, Result);
			if (Result == -1
#ifndef _MSC_VER
				&& errno == EILSEQ
#endif
				)
			{
				appErrorf(TEXT("Illegal FOutputDevice::Logf (error: %ls)"), appFromAnsi(strerror(errno)));
				Result = 0;
				break;
			}
			BufSize <<= 1;
		}

		if (Buffer)
		{
			Buffer[Result] = 0;
			Serialize(Buffer, Event);
			appFree(Buffer);
		}
	}
}
void FOutputDevice::Logf( const TCHAR* Fmt, ... )
{
	//if( !FName::SafeSuppressed(NAME_Log) )
	{
		INT		BufSize = 1024;
		TCHAR*	Buffer	= NULL;
		INT		Result	= -1;

		while (Result == -1)
		{
			Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, BufSize * sizeof(TCHAR), TEXT("")));
			if (!Buffer)
			{
				Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, 1 * sizeof(TCHAR), TEXT("")));
				Result = 0;
				break;
			}
			GET_VARARGS_RESULT(Buffer, BufSize - 1, Fmt, Result);
			if (Result == -1
#ifndef _MSC_VER
				&& errno == EILSEQ
#endif
				)
			{
				appErrorf(TEXT("Illegal FOutputDevice::Logf (error: %ls)"), appFromAnsi(strerror(errno)));
				Result = 0;
				break;
			}
			BufSize <<= 1;
		}

		if (Buffer)
		{
			Buffer[Result] = 0;
			Serialize(Buffer, NAME_Log);
			appFree(Buffer);
		}
	}
}

inline void FOutputDevice::show_pure_virtual_call(const char *Message)
{
	debugf(TEXT("Pure virtual call: %ls"),ANSI_TO_TCHAR(Message));
	appMsgf(TEXT("Pure virtual call: %ls"),ANSI_TO_TCHAR(Message));
}

/*-----------------------------------------------------------------------------
	FArray implementation.
-----------------------------------------------------------------------------*/

void FArray::Realloc( INT ElementSize )
{
	guard(FArray::Realloc);
    if ((!Data && ((ArrayMax*ElementSize) == 0)) || GExitPurge)
        return;  // don't call appRealloc (might be static constructor and we'd call into FMallocError then).
	Data = appRealloc( Data, ArrayMax*ElementSize, TEXT("FArray") );
	unguardf(( TEXT("%i*%i"), ArrayMax, ElementSize ));
}

void FArray::Remove( INT Index, INT Count, INT ElementSize )
{
	guardSlow(FArray::Remove);
	if( Count )
	{
		appMemmove
		(
			(BYTE*)Data + (Index      ) * ElementSize,
			(BYTE*)Data + (Index+Count) * ElementSize,
			(ArrayNum - Index - Count ) * ElementSize
		);
		ArrayNum -= Count;
		if
		(	(3*ArrayNum<2*ArrayMax || (ArrayMax-ArrayNum)*ElementSize>=16384)
		&&	(ArrayMax-ArrayNum>64 || ArrayNum==0) )
		{
			ArrayMax = ArrayNum;
			Realloc( ElementSize );
		}
	}
	checkSlow(ArrayNum>=0);
	checkSlow(ArrayMax>=ArrayNum);
	unguardSlow;
}

void FConfigCache::GetArray(const TCHAR* Section, const TCHAR* Key, TArray<FString>* List, const TCHAR* Filename)
{
	List->Empty();
	TMultiMap<FString, FString>* MM = GetSectionPrivate(Section, 0, 1, Filename);
	if (MM) MM->MultiFind(Key, *List);
}

/*-----------------------------------------------------------------------------
	FString implementation.
-----------------------------------------------------------------------------*/

FString FString::Chr( TCHAR Ch )
{
	guardSlow(FString::Chr);
	TCHAR Temp[2]={Ch,0};
	return FString(Temp);
	unguardSlow;
}

FString FString::LeftPad( INT ChCount )
{
	guardSlow(FString::LeftPad);
	INT Pad = ChCount - Len();
	if( Pad > 0 )
	{
		TCHAR* Ch = (TCHAR*)appAlloca((Pad+1)*sizeof(TCHAR));
		INT i;
		for( i=0; i<Pad; i++ )
			Ch[i] = ' ';
		Ch[i] = 0;
		return FString(Ch) + *this;
	}
	else return *this;
	unguardSlow;
}
FString FString::RightPad( INT ChCount )
{
	guardSlow(FString::RightPad);
	INT Pad = ChCount - Len();
	if( Pad > 0 )
	{
		TCHAR* Ch = (TCHAR*)appAlloca((Pad+1)*sizeof(TCHAR));
		INT i;
		for( i=0; i<Pad; i++ )
			Ch[i] = ' ';
		Ch[i] = 0;
		return *this + (const TCHAR *) Ch;
	}
	else return *this;
	unguardSlow;
}
FString::FString( BYTE Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( SBYTE Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( _WORD Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( SWORD Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( INT Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( DWORD Arg, INT Digits )
: TArray<TCHAR>()
{
}
FString::FString( FLOAT Arg, INT Digits, INT RightDigits, UBOOL LeadZero )
: TArray<TCHAR>()
{
}

#ifndef GET_VARARGS_LENGTH
FString FString::Printf( const TCHAR* Fmt, ... )
{
	INT		BufSize = 1024;
	TCHAR*	Buffer	= NULL;
	INT		Result	= -1;

	while (Result == -1)
	{
		Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, BufSize * sizeof(TCHAR), TEXT("")));
		if (!Buffer)
		{
			Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, 1 * sizeof(TCHAR), TEXT("")));
			Result = 0;
			break;
		}
		GET_VARARGS_RESULT(Buffer, BufSize - 1, Fmt, Result);
		if (Result == -1
#ifndef _MSC_VER
			&& errno == EILSEQ
#endif
			)
		{
			GLog->Logf(TEXT("Illegal FString::Printf (error: %ls)"), appFromAnsi(strerror(errno)));
			Result = 0;
			break;
		}
		BufSize <<= 1;
	}

	if (Buffer)
	{
		Buffer[Result] = 0;
		FString ResultStr(Buffer);
		appFree(Buffer);
		return ResultStr;
	}
	return FString(TEXT(""));
}
#else
FString FString::Printf( const TCHAR* Fmt, ... )
{
	FString Result;
	va_list ArgList;
	INT Length;

	va_start(ArgList, Fmt);
	GET_VARARGS_LENGTH( Length, Fmt, ArgList);
	if ( Length > 0 )
	{
		Result.GetCharArray().SetSize( Length + 1 );
		if ( Result.GetData() )
			PRINT_VARARGS_LENGTH(Result.GetData(),Length,Fmt,ArgList);
	}
	va_end(ArgList);
	return Result;
}
#endif

FString FString::Printf(const TCHAR* Fmt, va_list ArgPtr)
{
	INT		BufSize = 1024;
	TCHAR*	Buffer	= NULL;
	INT		Result	= -1;

	while (Result == -1)
	{
		Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, BufSize * sizeof(TCHAR), TEXT("")));

		if (!Buffer)
		{
			Buffer = reinterpret_cast<TCHAR*>(appRealloc(Buffer, 1 * sizeof(TCHAR), TEXT("")));
			Result = 0;
			break;
		}

#ifdef _MSC_VER
		Result = _vsnwprintf(Buffer, BufSize, Fmt, ArgPtr);
#else
		Result = vswprintf(Buffer, BufSize, Fmt, ArgPtr);
#endif

		if (Result == -1
#ifndef _MSC_VER
			&& errno == EILSEQ
#endif
			)
		{
			GLog->Logf(TEXT("Illegal FString::Printf (error: %ls)"), appFromAnsi(strerror(errno)));
			Result = 0;
			break;
		}
		BufSize <<= 1;
	}

	if (Buffer)
	{
		Buffer[Result] = 0;
		FString ResultStr(Buffer);
		appFree(Buffer);
		return ResultStr;
	}
	return FString(TEXT(""));
}
FArchive& operator<<( FArchive& Ar, FString& A )
{
	guard(FString<<);
	A.CountBytes( Ar );
	INT SaveNum = appIsPureAnsi(*A) ? A.Num() : -A.Num();
	Ar << SaveNum;

	if( Ar.IsLoading() )
	{
		A.ArrayMax = A.ArrayNum = Abs(SaveNum);
		A.Realloc( sizeof(TCHAR) );
		if( SaveNum>=0 )
			for( INT i=0; i<A.Num(); i++ )
				{ANSICHAR ACh; Ar << *(BYTE*)&ACh; A(i)=FromAnsi(ACh);}
#if __GNUG__ && UNICODE
		else
			for( INT i=0; i<A.Num(); i++ )
				{UNICHAR UCh=0; Ar.ByteOrderSerialize( ((BYTE*)&UCh)+2, 2 ); A(i)=FromUnicode(UCh);}
#else
		else
			for( INT i=0; i<A.Num(); i++ )
				{UNICHAR UCh; Ar << UCh; A(i)=FromUnicode(UCh);}
#endif
		if( A.Num()==1 )
			A.Empty();
	}
	else
	{
		if( SaveNum>=0 )
			for( INT i=0; i<A.Num(); i++ )
				{ANSICHAR ACh=ToAnsi(A(i)); Ar << *(BYTE*)&ACh;}
#if __GNUG__ && UNICODE
		else
			for( INT i=0; i<A.Num(); i++ )
				{UNICHAR UCh=ToUnicode(A(i)); Ar.ByteOrderSerialize( ((BYTE*)&UCh)+2, 2 );}
#else
		else
			for( INT i=0; i<A.Num(); i++ )
				{UNICHAR UCh=ToUnicode(A(i)); Ar << UCh;}
#endif
	}
	return Ar;
	unguard;
}
CORE_API FArchive& operator<<( FArchive& Ar, FTime& T )
{
	union {struct FDoubleIEEE {QWORD man:52; QWORD exp:11; QWORD sgn:1;} s; QWORD m;} D;
	union {struct FFloatIEEE  {QWORD man:23; QWORD exp:8;  QWORD sgn:1;} s; FLOAT m;} F;
	F.m=T.GetFloat();
	D.s.man=F.s.man; D.s.exp=F.s.exp; D.s.sgn=F.s.sgn;
	Ar.ByteOrderSerialize( &D.m, sizeof(D.m) );
	F.s.man=D.s.man; F.s.exp=D.s.exp; F.s.sgn=D.s.sgn;
	T=FTime(F.m);
	return Ar;
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

//
// Returns whether the string is pure ANSI.
//
CORE_API UBOOL appIsPureAnsi( const TCHAR* Str )
{
/*
#if UNICODE
 if( !GIsOnStrHack )
  return 1;
#endif
 for( ; *Str; Str++ )
  if( *Str<0 || *Str>0xff )
   return 0;
 return 1;
*/
#if UNICODE
	for( ; *Str; Str++ )
		if( *Str<0 || *Str>0xff )
			return 0;
#endif
	return 1;

}

//
// Failed assertion handler.
//warning: May be called at library startup time.
//
void VARARGS appFailAssert( const ANSICHAR* Expr, const ANSICHAR* File, INT Line )
{
	appErrorf( TEXT("Assertion failed: %ls [File:%ls] [Line: %i]"), appFromAnsi(Expr), appFromAnsi(File), Line );
	//debugf( TEXT("Assertion failed: %ls [File:%ls] [Line: %i]"), appFromAnsi(Expr), appFromAnsi(File), Line );
}

//
// Get a static result string.
//
TCHAR* appStaticString1024()
{
	static TCHAR Results[256][1024];
	static std::atomic<INT> Count;
	TCHAR* Result = Results[Count++ & 255];
	*Result = 0;
	return Result;
}
TCHAR* appStaticString4096()
{
	static TCHAR Results[256][4096];
	static std::atomic<INT> Count;
	TCHAR* Result = Results[Count++ & 255];
	*Result = 0;
	return Result;
}

//
// Get an ANSI static result string.
//
ANSICHAR* appAnsiStaticString1024()
{
    static char Results[256][1024];
	static std::atomic<INT> Count;
	char* Result = Results[Count++ & 255];
	*Result = 0;
	return Result;
}
ANSICHAR* appAnsiStaticString4096()
{
    static char Results[256][4096];
	static std::atomic<INT> Count;
	char* Result = Results[Count++ & 255];
	*Result = 0;
	return Result;
}

ANSICHAR* appAnsiStaticString8192()
{
    static char Results[256][8192];
	static std::atomic<INT> Count;
	char* Result = Results[Count++ & 255];
	*Result = 0;
	return Result;
}

TCHAR* appStaticString(size_t Len)
{
	static TCHAR* Results[256] = {NULL};
	static std::atomic<INT> Count;
	TCHAR*& Result = Results[Count++ & 255];
	if (Result)
		appFree(Result);
	Result = reinterpret_cast<TCHAR*>(appMalloc(Len * sizeof(TCHAR), TEXT("StaticString")));
	return Result;
}

ANSICHAR* appAnsiStaticString(size_t Len)
{
	static ANSICHAR* Results[256] = {NULL};
	static std::atomic<INT> Count;
	ANSICHAR*& Result = Results[Count++ & 255];
	if (Result)
		appFree(Result);
	Result = reinterpret_cast<ANSICHAR*>(appMalloc(Len, TEXT("AnsiStaticString")));
	return Result;
}
//
// Find string in string, case insensitive, requires non-alphanumeric lead-in.
//
const TCHAR* appStrfind( const TCHAR* Str, const TCHAR* Find )
{
	guard(appStrfind);
	UBOOL Alnum  = 0;
	TCHAR f      = appToUpper(*Find);
	INT   Length = appStrlen(Find++)-1;
	TCHAR c      = *Str++;
	while( c )
	{
		if( c>='a' && c<='z' )
			c += 'A'-'a';
		if( !Alnum && c==f && !appStrnicmp(Str,Find,Length) )
			return Str-1;
		Alnum = (c>='A' && c<='Z') || (c>='0' && c<='9');
		c = *Str++;
	}
	return NULL;
	unguard;
}

//
// Returns a certain number of spaces.
// Only one return value is valid at a time.
//
const TCHAR* appSpc(INT Num)
{
	guard(spc);
	static TCHAR Spacing[256];
	static INT OldNum = -1;
	static UBOOL Initialized = 0;
	Num = Clamp(Num, 0, (INT)ARRAY_COUNT(Spacing));
	if (Num != OldNum || !Initialized)
	{
		Initialized = 1;
		for (OldNum = 0; OldNum<Num; OldNum++)
			Spacing[OldNum] = ' ';
		Spacing[Num] = 0;
	}
	return Spacing;
	unguard;
}
//
// Trim spaces from an asciiz string by zeroing them.
//
void appTrimSpaces( ANSICHAR* String )
{
	guard(appTrimSpaces);
	// Find 0 terminator.
	INT t=0;
	while( (String[t]!=0 ) && (t< 1024) ) t++;
	if (t>0) t--;
	// Zero trailing spaces.
	while( (String[t]==32) && (t>0) )
	{
		String[t]=0;
		t--;
	}
	unguard;
}

void appStripSpaces( TCHAR* T )
{
	guard(appStripSpaces);
	if (!T)
		return;
	TCHAR* ReadOffset = T;
	while( *ReadOffset )
	{
		TCHAR* Offset = ReadOffset+1;
		while( *ReadOffset==' ' )
		{
			*T = *Offset;
			Offset++;
			ReadOffset++;
		}
		ReadOffset = Offset;
		T++;
		if( T!=Offset )
		*T = *Offset;
	}
	*T = 0;
	unguard;
}

void appStripChars( TCHAR* T, const TCHAR* ClearChars )
{
	guard(appStripChars);
	if (!T)
		return;
	TCHAR* ReadOffset = T;
	while( *ReadOffset )
	{
		TCHAR* Offset = ReadOffset+1;
		while( *ReadOffset==*ClearChars )
		{
			*T = *Offset;
			Offset++;
			ReadOffset++;
		}
		ReadOffset = Offset;
		T++;
		if( T!=Offset )
		*T = *Offset;
	}
	*T = 0;
	unguard;
}

void appStripBadChrs( TCHAR* T )
{
	guard(appStripBadChrs);
	if (!T)
		return;
	TCHAR* ReadOffset = T;
	while( *ReadOffset )
	{
		TCHAR* Offset = ReadOffset+1;
		while( *ReadOffset && !appIsAlnum(*ReadOffset) )
		{
			*T = *Offset;
			Offset++;
			ReadOffset++;
		}
		ReadOffset = Offset;
		T++;
		if( T!=Offset )
		*T = *Offset;
	}
	*T = 0;
	unguard;
}

INT appStrPrefix( const TCHAR* Str, const TCHAR* Prefix )
{
    for(;;)
    {
        if( !*Prefix )
            return 0;

        TCHAR A = appToUpper(*Str);
        TCHAR B = appToUpper(*Prefix);

        if( A != B )
            return( B - A );

        Str++;
        Prefix++;
    }

	return 0;
}
// --- gam

/*-----------------------------------------------------------------------------
	CRC functions.
-----------------------------------------------------------------------------*/

// CRC 32 polynomial.
#define CRC32_POLY 0x04c11db7
CORE_API DWORD GCRCTable[256];

//
// CRC32 computer based on CRC32_POLY.
//
DWORD appMemCrc( const void* InData, INT Length, DWORD CRC )
{
	guardSlow(appMemCrc);
	BYTE* Data = (BYTE*)InData;
	CRC = ~CRC;
	for( INT i=0; i<Length; i++ )
		CRC = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ Data[i]];
	return ~CRC;
	unguardSlow;
}

//
// String CRC.
//
DWORD appStrCrc( const TCHAR* Data )
{
	guardSlow(appStrCrc);
	INT Length = appStrlen( Data );
	DWORD CRC = 0xFFFFFFFF;
	for( INT i=0; i<Length; i++ )
	{
		TCHAR C   = Data[i];
		INT   CL  = (C&255);
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CL];;
#if UNICODE
		INT   CH  = (C>>8)&255;
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CH];;
#endif
	}
	return ~CRC;
	unguardSlow;
}

//
// String CRC, case insensitive.
//
DWORD appStrCrcCaps( const TCHAR* Data )
{
	guardSlow(appStrCrcCaps);
	INT Length = appStrlen( Data );
	DWORD CRC = 0xFFFFFFFF;
	for( INT i=0; i<Length; i++ )
	{
		TCHAR C   = appToUpper(Data[i]);
		INT   CL  = (C&255);
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CL];
#if UNICODE
		INT   CH  = (C>>8)&255;
		CRC       = (CRC << 8) ^ GCRCTable[(CRC >> 24) ^ CH];
#endif
	}
	return ~CRC;
	unguardSlow;
}

//
// Returns smallest N such that (1<<N)>=Arg.
// Note: appCeilLogTwo(0)=0 because (1<<0)=1 >= 0.
//
static BYTE GLogs[257];

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

CORE_API BYTE appCeilLogTwo( DWORD Arg )
{
#ifdef _WIN32
    if( --Arg == MAXDWORD )
		return 0;
	BYTE Shift = Arg<=0x10000 ? (Arg<=0x100?0:8) : (Arg<=0x1000000?16:24);
	return Shift + GLogs[Arg>>Shift];
#else
    #if __has_builtin(__builtin_clzll) //use compiler if possible
        return ((sizeof(unsigned long long) * 8 - 1) - __builtin_clzll(Arg)) + (!!(Arg & (Arg - 1)));
    #endif

    if (Arg<=0L		) return 0;
	if (Arg<=1L		) return 0;
	if (Arg<=2L		) return 1;
	if (Arg<=4L		) return 2;
	if (Arg<=8L		) return 3;
	if (Arg<=16L	    ) return 4;
	if (Arg<=32L	    ) return 5;
	if (Arg<=64L 	    ) return 6;
	if (Arg<=128L     ) return 7;
	if (Arg<=256L     ) return 8;
	if (Arg<=512L     ) return 9;
	if (Arg<=1024L    ) return 10;
	if (Arg<=2048L    ) return 11;
	if (Arg<=4096L    ) return 12;
	if (Arg<=8192L    ) return 13;
	if (Arg<=16384L   ) return 14;
	if (Arg<=32768L   ) return 15;
	if (Arg<=65536L   ) return 16;
	else			  return 0;
#endif
}

/*-----------------------------------------------------------------------------
	MD5 functions, adapted from MD5 RFC by Brandon Reinhart
-----------------------------------------------------------------------------*/

//
// Constants for MD5 Transform.
//

enum {S11=7};
enum {S12=12};
enum {S13=17};
enum {S14=22};
enum {S21=5};
enum {S22=9};
enum {S23=14};
enum {S24=20};
enum {S31=4};
enum {S32=11};
enum {S33=16};
enum {S34=23};
enum {S41=6};
enum {S42=10};
enum {S43=15};
enum {S44=21};

static BYTE PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0
};

//
// Basic MD5 transformations.
//
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

//
// Rotates X left N bits.
//
#define ROTLEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

//
// Rounds 1, 2, 3, and 4 MD5 transformations.
// Rotation is seperate from addition to prevent recomputation.
//
#define FF(a, b, c, d, x, s, ac) { \
	(a) += F ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define GG(a, b, c, d, x, s, ac) { \
	(a) += G ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define HH(a, b, c, d, x, s, ac) { \
	(a) += H ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

#define II(a, b, c, d, x, s, ac) { \
	(a) += I ((b), (c), (d)) + (x) + (DWORD)(ac); \
	(a) = ROTLEFT ((a), (s)); \
	(a) += (b); \
}

//
// MD5 initialization.  Begins an MD5 operation, writing a new context.
//
CORE_API void appMD5Init( FMD5Context* context )
{
	context->count[0] = context->count[1] = 0;
	// Load magic initialization constants.
	context->state[0] = 0x67452301;
	context->state[1] = 0xefcdab89;
	context->state[2] = 0x98badcfe;
	context->state[3] = 0x10325476;
    appMemzero(context->buffer, sizeof (context->buffer));
}

//
// MD5 block update operation.  Continues an MD5 message-digest operation,
// processing another message block, and updating the context.
//
CORE_API void appMD5Update( FMD5Context* context, BYTE* input, INT inputLen )
{
	INT i, index, partLen;

	// Compute number of bytes mod 64.
	index = (INT)((context->count[0] >> 3) & 0x3F);

	// Update number of bits.
	if ((context->count[0] += ((DWORD)inputLen << 3)) < ((DWORD)inputLen << 3))
		context->count[1]++;
	context->count[1] += ((DWORD)inputLen >> 29);

	partLen = 64 - index;

	// Transform as many times as possible.
	if (inputLen >= partLen) {
		appMemcpy( &context->buffer[index], input, partLen );
		appMD5Transform( context->state, context->buffer );
		for (i = partLen; i + 63 < inputLen; i += 64)
			appMD5Transform( context->state, &input[i] );
		index = 0;
	} else
		i = 0;

	// Buffer remaining input.
	appMemcpy( &context->buffer[index], &input[i], inputLen-i );
}

//
// MD5 finalization. Ends an MD5 message-digest operation, writing the
// the message digest and zeroizing the context.
// Digest is 16 BYTEs.
//
CORE_API void appMD5Final ( BYTE* digest, FMD5Context* context )
{
	BYTE bits[8];
	INT index, padLen;

	// Save number of bits.
	appMD5Encode( bits, context->count, 8 );

	// Pad out to 56 mod 64.
	index = (INT)((context->count[0] >> 3) & 0x3f);
	padLen = (index < 56) ? (56 - index) : (120 - index);
	appMD5Update( context, PADDING, padLen );

	// Append length (before padding).
	appMD5Update( context, bits, 8 );

	// Store state in digest
	appMD5Encode( digest, context->state, 16 );

	// Zeroize sensitive information.
	appMemset( context, 0, sizeof(*context) );
}

//
// MD5 basic transformation. Transforms state based on block.
//
CORE_API void appMD5Transform( DWORD* state, BYTE* block )
{
	DWORD a = state[0], b = state[1], c = state[2], d = state[3], x[16];

	appMD5Decode( x, block, 64 );

	// Round 1
	FF (a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
	FF (d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
	FF (c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
	FF (b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
	FF (a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
	FF (d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
	FF (c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
	FF (b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
	FF (a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
	FF (d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
	FF (c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
	FF (b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
	FF (a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
	FF (d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
	FF (c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
	FF (b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

	// Round 2
	GG (a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
	GG (d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
	GG (c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
	GG (b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
	GG (a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
	GG (d, a, b, c, x[10], S22,  0x2441453); /* 22 */
	GG (c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
	GG (b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
	GG (a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
	GG (d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
	GG (c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
	GG (b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
	GG (a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
	GG (d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
	GG (c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
	GG (b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

	// Round 3
	HH (a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
	HH (d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
	HH (c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
	HH (b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
	HH (a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
	HH (d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
	HH (c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
	HH (b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
	HH (a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
	HH (d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
	HH (c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
	HH (b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
	HH (a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
	HH (d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
	HH (c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
	HH (b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

	// Round 4
	II (a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
	II (d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
	II (c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
	II (b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
	II (a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
	II (d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
	II (c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
	II (b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
	II (a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
	II (d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
	II (c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
	II (b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
	II (a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
	II (d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
	II (c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
	II (b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;

	// Zeroize sensitive information.
	appMemset( x, 0, sizeof(x) );
}

//
// Encodes input (DWORD) into output (BYTE).
// Assumes len is a multiple of 4.
//
CORE_API void appMD5Encode( BYTE* output, DWORD* input, INT len )
{
	INT i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) {
		output[j] = (BYTE)(input[i] & 0xff);
		output[j+1] = (BYTE)((input[i] >> 8) & 0xff);
		output[j+2] = (BYTE)((input[i] >> 16) & 0xff);
		output[j+3] = (BYTE)((input[i] >> 24) & 0xff);
	}
}

//
// Decodes input (BYTE) into output (DWORD).
// Assumes len is a multiple of 4.
//
CORE_API void appMD5Decode( DWORD* output, BYTE* input, INT len )
{
	INT i, j;

	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((DWORD)input[j]) | (((DWORD)input[j+1]) << 8) |
		(((DWORD)input[j+2]) << 16) | (((DWORD)input[j+3]) << 24);
}

// return the MD5 of a string
CORE_API FString appMD5String( FString InString )
{
	FMD5Context PContext;
  BYTE Digest[16];
	appMD5Init( &PContext );
  // make sure we use single bytes
  BYTE tmp[4096];
	InString = InString.Mid(0, sizeof(tmp));
  INT j;
  for (j=0; j < InString.Len(); j++)
  {
		tmp[j] = BYTE(InString[j]);
  }
  appMD5Update( &PContext, (BYTE *) &tmp, InString.Len() );
  appMD5Final( Digest, &PContext );
  InString = TEXT(""); // make empty
  for (j=0; j<16; j++) InString += FString::Printf(TEXT("%02x"), Digest[j]);
	return InString;
}

// SHA-2 256
CORE_API FString GenHash(FString InString)
{
	SHA256_CTX ctx256;
	BYTE Digest[SHA256_DIGEST_STRING_LENGTH];
	SHA256_Init(&ctx256);

	BYTE tmp[16384];
	InString = InString.Mid(0, sizeof(tmp));
	INT j;
	for (j=0; j < InString.Len(); j++)
	{
		tmp[j] = BYTE(InString[j]);
	}
	SHA256_Update( &ctx256, (BYTE *) &tmp, InString.Len());
	SHA256_Final( Digest, &ctx256);

	FString outSHA;
	for (INT i=0; i<SHA256_DIGEST_LENGTH; i++)
		outSHA += FString::Printf(TEXT("%02x"), Digest[i]);

	//GLog->Logf(TEXT("HASH SHA: %ls"), *outSHA);

	return outSHA;
}

CORE_API void SHA256_Init(SHA256_CTX* context) {
	if (context == (SHA256_CTX*)0) {
		return;
	}
	MEMCPY_BCOPY(context->state, sha256_initial_hash_value, SHA256_DIGEST_LENGTH);
	MEMSET_BZERO(context->buffer, SHA256_BLOCK_LENGTH);
	context->bitcount = 0;
}

CORE_API void SHA256_Transform(SHA256_CTX* context, const UINT* data) {
	UINT	a, b, c, d, e, f, g, h, s0, s1;
	UINT	T1, T2, *W256;
	int		j;

	W256 = (UINT*)context->buffer;

	/* Initialize registers with the prev. intermediate value */
	a = context->state[0];
	b = context->state[1];
	c = context->state[2];
	d = context->state[3];
	e = context->state[4];
	f = context->state[5];
	g = context->state[6];
	h = context->state[7];

	j = 0;
	do {
		/* Copy data while converting to host byte order */
		REVERSE32(*data++,W256[j]);
		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + ShaCh(e, f, g) + K256[j] + W256[j];
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 16);

	do {
		/* Part of the message block expansion: */
		s0 = W256[(j+1)&0x0f];
		s0 = sigma0_256(s0);
		s1 = W256[(j+14)&0x0f];
		s1 = sigma1_256(s1);

		/* Apply the SHA-256 compression function to update a..h */
		T1 = h + Sigma1_256(e) + ShaCh(e, f, g) + K256[j] +
		     (W256[j&0x0f] += s1 + W256[(j+9)&0x0f] + s0);
		T2 = Sigma0_256(a) + Maj(a, b, c);
		h = g;
		g = f;
		f = e;
		e = d + T1;
		d = c;
		c = b;
		b = a;
		a = T1 + T2;

		j++;
	} while (j < 64);

	/* Compute the current intermediate hash value */
	context->state[0] += a;
	context->state[1] += b;
	context->state[2] += c;
	context->state[3] += d;
	context->state[4] += e;
	context->state[5] += f;
	context->state[6] += g;
	context->state[7] += h;

	/* Clean up */
	a = b = c = d = e = f = g = h = T1 = T2 = 0;
}

CORE_API void SHA256_Update(SHA256_CTX* context, const BYTE *data, size_t len) {
	unsigned int	freespace, usedspace;

	if (len == 0) {
		/* Calling with no data is valid - we do nothing */
		return;
	}

	/* Sanity check: */
	check(context != (SHA256_CTX*)0 && data != (BYTE*)0);

	usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
	if (usedspace > 0) {
		/* Calculate how much free space is available in the buffer */
		freespace = SHA256_BLOCK_LENGTH - usedspace;

		if (len >= freespace) {
			/* Fill the buffer completely and process it */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, freespace);
			context->bitcount += freespace << 3;
			len -= freespace;
			data += freespace;
			SHA256_Transform(context, (UINT*)context->buffer);
		} else {
			/* The buffer is not yet full */
			MEMCPY_BCOPY(&context->buffer[usedspace], data, len);
			context->bitcount += len << 3;
			/* Clean up: */
			usedspace = freespace = 0;
			return;
		}
	}
	while (len >= SHA256_BLOCK_LENGTH) {
		/* Process as many complete blocks as we can */
		SHA256_Transform(context, (UINT*)data);
		context->bitcount += SHA256_BLOCK_LENGTH << 3;
		len -= SHA256_BLOCK_LENGTH;
		data += SHA256_BLOCK_LENGTH;
	}
	if (len > 0) {
		/* There's left-overs, so save 'em */
		MEMCPY_BCOPY(context->buffer, data, len);
		context->bitcount += len << 3;
	}
	/* Clean up: */
	usedspace = freespace = 0;
}

CORE_API void SHA256_Final(BYTE digest[], SHA256_CTX* context) {
	UINT	*d = (UINT*)digest;
	UINT	usedspace;

	/* Sanity check: */
	check(context != (SHA256_CTX*)0);

	/* If no digest buffer is passed, we don't bother doing this: */
	if (digest != (BYTE*)0) {
		usedspace = (context->bitcount >> 3) % SHA256_BLOCK_LENGTH;
		REVERSE64(context->bitcount,context->bitcount);
		if (usedspace > 0) {
			/* Begin padding with a 1 bit: */
			context->buffer[usedspace++] = 0x80;

			if (usedspace <= SHA256_SHORT_BLOCK_LENGTH) {
				/* Set-up for the last transform: */
				MEMSET_BZERO(&context->buffer[usedspace], SHA256_SHORT_BLOCK_LENGTH - usedspace);
			} else {
				if (usedspace < SHA256_BLOCK_LENGTH) {
					MEMSET_BZERO(&context->buffer[usedspace], SHA256_BLOCK_LENGTH - usedspace);
				}
				/* Do second-to-last transform: */
				SHA256_Transform(context, (UINT*)context->buffer);

				/* And set-up for the last transform: */
				MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);
			}
		} else {
			/* Set-up for the last transform: */
			MEMSET_BZERO(context->buffer, SHA256_SHORT_BLOCK_LENGTH);

			/* Begin padding with a 1 bit: */
			*context->buffer = 0x80;
		}
		/* Set the bit count: */
		*(QWORD*)&context->buffer[SHA256_SHORT_BLOCK_LENGTH] = context->bitcount;

		/* Final transform: */
		SHA256_Transform(context, (UINT*)context->buffer);
		{
			/* Convert TO host byte order */
			int	j;
			for (j = 0; j < 8; j++) {
				REVERSE32(context->state[j],context->state[j]);
				*d++ = context->state[j];
			}
		}
	}

	/* Clean up state data: */
	MEMSET_BZERO(context, sizeof(SHA256_CTX));
	usedspace = 0;
}

CORE_API char *SHA256_End(SHA256_CTX* context, char buffer[]) {
	BYTE	digest[SHA256_DIGEST_LENGTH], *d = digest;
	int		i;

	/* Sanity check: */
	check(context != (SHA256_CTX*)0);

	if (buffer != (char*)0) {
		SHA256_Final(digest, context);

		for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
			*buffer++ = sha2_hex_digits[(*d & 0xf0) >> 4];
			*buffer++ = sha2_hex_digits[*d & 0x0f];
			d++;
		}
		*buffer = (char)0;
	} else {
		MEMSET_BZERO(context, sizeof(SHA256_CTX));
	}
	MEMSET_BZERO(digest, SHA256_DIGEST_LENGTH);
	return buffer;
}

CORE_API char* SHA256_Data(const BYTE* data, size_t len, char digest[SHA256_DIGEST_STRING_LENGTH]) {
	SHA256_CTX	context;
	SHA256_Init(&context);
	SHA256_Update(&context, data, len);
	return SHA256_End(&context, digest);
}

/*-----------------------------------------------------------------------------
	Exceptions.
-----------------------------------------------------------------------------*/

//
// Throw a string exception with a message.
//
CORE_API void VARARGS appThrowf( const TCHAR* Fmt, ... )
{
	static TCHAR TempStr[4096];
	GET_VARARGS(TempStr,ARRAY_COUNT(TempStr),Fmt);
	throw( TempStr );
	#if UNICODE
    wprintf(TEXT("Console Error Logging: %ls \n"),TempStr);
    #else
    printf("Console Error Logging: %ls \n",TempStr);
    #endif
}

/*-----------------------------------------------------------------------------
	Parameter parsing.
-----------------------------------------------------------------------------*/

//
// Get a string from a text string.
//
CORE_API UBOOL Parse
(
	const TCHAR* Stream,
	const TCHAR* Match,
	TCHAR*		 Value,
	INT			 MaxLen
)
{
	guard(ParseString);

	const TCHAR* Found = appStrfind(Stream,Match);
	const TCHAR* Start;

	if( Found )
	{
		Start = Found + appStrlen(Match);
		if( *Start == '\x22' )
		{
			// Quoted string with spaces.
			appStrncpy( Value, Start+1, MaxLen );
			Value[MaxLen-1]=0;
			TCHAR* Temp = appStrstr( Value, TEXT("\x22") );
			if( Temp != NULL )
				*Temp=0;
		}
		else if (*Start == '(')
		{
			// bracketed string.
			appStrncpy(Value, Start, MaxLen);
			Value[MaxLen - 1] = 0;
			TCHAR* Temp = appStrstr(Value, TEXT(")"));
			if (Temp != NULL)
				*(Temp+1) = 0;
		}
		else
		{
			// Non-quoted string without spaces.
			appStrncpy( Value, Start, MaxLen );
			Value[MaxLen-1]=0;
			TCHAR* Temp;
			Temp = appStrstr( Value, TEXT(" ")  ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\r") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\n") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT("\t") ); if( Temp ) *Temp=0;
			Temp = appStrstr( Value, TEXT(",")  ); if( Temp ) *Temp=0;
		}
		return 1;
	}
	else return 0;
	unguard;
}

//
// See if a command-line parameter exists in the stream.
//
UBOOL CORE_API ParseParam( const TCHAR* Stream, const TCHAR* Param )
{
	guard(GetParam);
	/*
    TCHAR* UpStream=appStrupr((TCHAR*)Stream);
    TCHAR* UpParam=appStrupr((TCHAR*)Param);
    TCHAR* Result = NULL;
    if (UpStream[0]=='-' || UpStream[0]=='/')
        Result= wcsstr(UpStream+1,UpParam);
    else
        Result= wcsstr(UpStream,UpParam);
	if (Result)
        return 1;
    return 0;
    */
	const TCHAR* Start = Stream;
	if( *Stream )
		while( (Start=appStrfind(Start+1,Param)) != NULL )
			if( Start>Stream && (Start[-1]=='-' || Start[-1]=='/') )
				return 1;
	return 0;

	unguard;
}

//
// Parse a string.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FString& Value )
{
	guard(FString::Parse);
	TCHAR Temp[4096]=TEXT("");
	if( ::Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
	{
		Value = Temp;
		return 1;
	}
	else return 0;
	unguard;
}

//
// Parse a quadword.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value )
{
	guard(ParseQWORD);
	return Parse( Stream, Match, *(SQWORD*)&Value );
	unguard;
}

//
// Parse a signed quadword.
//
UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value )
{
	guard(ParseSQWORD);
	TCHAR Temp[4096]=TEXT(""), *Ptr=Temp;
	if( ::Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
	{
		Value = 0;
		UBOOL Negative = (*Ptr=='-');
		Ptr += Negative;
		while( *Ptr>='0' && *Ptr<='9' )
			Value = Value*10 + *Ptr++ - '0';
		if( Negative )
			Value = -Value;
		return 1;
	}
	else return 0;
	unguard;
}

//
// Get an object from a text stream.
//
CORE_API UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, UClass* Class, UObject*& DestRes, UObject* InParent )
{
	guard(ParseUObject);
	TCHAR TempStr[256];
	if( !Parse( Stream, Match, TempStr, NAME_SIZE ) )
	{
		return 0;
	}
	else if( appStricmp(TempStr,TEXT("NONE"))==0 )
	{
		DestRes = NULL;
		return 1;
	}
	else
	{
		// Look this object up.
		UObject* Res;
		Res = UObject::StaticFindObject( Class, InParent, TempStr );
		if( !Res )
			return 0;
		DestRes = Res;
		return 1;
	}
	unguard;
}

//
// Get a name.
//
CORE_API UBOOL Parse
(
	const TCHAR* Stream,
	const TCHAR* Match,
	FName& Name
)
{
	guard(ParseFName);
	TCHAR TempStr[NAME_SIZE];

	if( !Parse(Stream,Match,TempStr,NAME_SIZE) )
		return 0;
	Name = FName( TempStr );

	return 1;
	unguard;
}

//
// Get a DWORD.
//
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value )
{
	guard(ParseDWORD);

	const TCHAR* Temp = appStrfind(Stream,Match);
	TCHAR* End;
	if( Temp==NULL )
		return 0;
	Value = appStrtoi( Temp + appStrlen(Match), &End, 10 );

	return 1;
	unguard;
}

//
// Get a byte.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value )
{
	guard(ParseBYTE);

	const TCHAR* Temp = appStrfind(Stream,Match);
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (BYTE)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);

	unguard;
}

//
// Get a signed byte.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value )
{
	guard(ParseCHAR);
	const TCHAR* Temp = appStrfind(Stream,Match);
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
	unguard;
}

//
// Get a word.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, _WORD& Value )
{
	guard(ParseWORD);
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (_WORD)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
	unguard;
}

//
// Get a signed word.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value )
{
	guard(ParseSWORD);
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Temp += appStrlen( Match );
	Value = (SWORD)appAtoi( Temp );
	return Value!=0 || appIsDigit(Temp[0]);
	unguard;
}

//
// Get a floating-point number.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value )
{
	guard(ParseFLOAT);
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Value = appAtof( Temp+appStrlen(Match) );
	return 1;
	unguard;
}

//
// Get a signed double word.
//
UBOOL CORE_API Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value )
{
	guard(ParseINT);
	const TCHAR* Temp = appStrfind( Stream, Match );
	if( Temp==NULL )
		return 0;
	Value = appAtoi( Temp + appStrlen(Match) );
	return 1;
	unguard;
}

//
// Get a boolean value.
//
UBOOL CORE_API ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff )
{
	guard(ParseUBOOL);
	TCHAR TempStr[16];
	if( Parse( Stream, Match, TempStr, 16 ) )
	{
		OnOff
		=	!appStricmp(TempStr,TEXT("On"))
		||	!appStricmp(TempStr,TEXT("True"))
		||	!appStricmp(TempStr,GTrue)
		||	!appStricmp(TempStr,TEXT("1"))
		||	!appStricmp(TempStr,TEXT("2"));
		return 1;
	}
	else return 0;
	unguard;
}

//
// Get a globally unique identifier.
//
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid )
{
	guard(ParseGUID);

	TCHAR Temp[256];
	if( !Parse( Stream, Match, Temp, ARRAY_COUNT(Temp) ) )
		return 0;

	Guid.A = Guid.B = Guid.C = Guid.D = 0;
	if( appStrlen(Temp)==32 )
	{
		TCHAR* End;
		Guid.D = appStrtoi( Temp+24, &End, 16 ); Temp[24]=0;
		Guid.C = appStrtoi( Temp+16, &End, 16 ); Temp[16]=0;
		Guid.B = appStrtoi( Temp+8,  &End, 16 ); Temp[8 ]=0;
		Guid.A = appStrtoi( Temp+0,  &End, 16 ); Temp[0 ]=0;
	}
	return 1;

	unguard;
}

//
// Sees if Stream starts with the named command.  If it does,
// skips through the command and blanks past it.  Returns 1 of match,
// 0 if not.
//
CORE_API UBOOL ParseCommand
(
	const TCHAR** Stream,
	const TCHAR*  Match
)
{
	guard(ParseCommand);

	while( (**Stream==' ')||(**Stream==9) )
		(*Stream)++;

	if( appStrnicmp(*Stream,Match,appStrlen(Match))==0 )
	{
		*Stream += appStrlen(Match);
		if( !appIsAlnum(**Stream) )
		{
			while ((**Stream==' ')||(**Stream==9)) (*Stream)++;
			return 1; // Success.
		}
		else
		{
			*Stream -= appStrlen(Match);
			return 0; // Only found partial match.
		}
	}
	else return 0; // No match.
	unguard;
}

//
// Get next command.  Skips past comments and cr's.
//
CORE_API void ParseNext( const TCHAR** Stream )
{
	guard(ParseNext);

	// Skip over spaces, tabs, cr's, and linefeeds.
	SkipJunk:
	while( **Stream==' ' || **Stream==9 || **Stream==13 || **Stream==10 )
		++*Stream;

	if( **Stream==';' )
	{
		// Skip past comments.
		while( **Stream!=0 && **Stream!=10 && **Stream!=13 )
			++*Stream;
		goto SkipJunk;
	}

	// Upon exit, *Stream either points to valid Stream or a nul.
	unguard;
}

//
// Grab the next space-delimited string from the input stream.
// If quoted, gets entire quoted string.
//
CORE_API UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape )
{
	guard(ParseToken);
	INT Len=0;

	// Skip spaces and tabs.
	while( *Str==' ' || *Str==9 )
		Str++;
	if( *Str == 34 )
	{
		// Get quoted string.
		Str++;
		while( *Str && *Str!=34 && (Len+1)<MaxLen )
		{
			TCHAR c = *Str++;
			if( c=='\\' && UseEscape )
			{
				// Get escape.
				c = *Str++;
				if( !c )
					break;
			}
			if( (Len+1)<MaxLen )
				Result[Len++] = c;
		}
		if( *Str==34 )
			Str++;
	}
	else
	{
		// Get unquoted string.
		for( ; *Str && *Str!=' ' && *Str!=9; Str++ )
			if( (Len+1)<MaxLen )
				Result[Len++] = *Str;
	}
	Result[Len]=0;
	return Len!=0;
	unguard;
}
CORE_API UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape )
{
	TCHAR Buffer[1024];
	if( ParseToken( Str, Buffer, ARRAY_COUNT(Buffer), UseEscape ) )
	{
		Arg = Buffer;
		return 1;
	}
	return 0;
}
CORE_API FString ParseToken( const TCHAR*& Str, UBOOL UseEscape )
{
	FString Result;
	if( ParseToken( Str, Result, UseEscape ) )
		return Result;
	else
		return TEXT("");
}

//
// Get a line of Stream (everything up to, but not including, CR/LF.
// Returns 0 if ok, nonzero if at end of stream and returned 0-length string.
//
CORE_API UBOOL ParseLine
(
	const TCHAR**	Stream,
	TCHAR*			Result,
	INT				MaxLen,
	UBOOL			Exact
)
{
	guard(ParseLine);
	UBOOL GotStream=0;
	UBOOL IsQuoted=0;
	UBOOL Ignore=0;

	*Result=0;
	while( **Stream!=0 && **Stream!=10 && **Stream!=13 && --MaxLen>0 )
	{
		// Start of comments.
		if( !IsQuoted && !Exact && (*Stream)[0]=='/' && (*Stream)[1]=='/' )
			Ignore = 1;

		// Command chaining.
		if( !IsQuoted && !Exact && **Stream=='|' )
			break;

		// Check quoting.
		IsQuoted = IsQuoted ^ (**Stream==34);
		GotStream=1;

		// Got stuff.
		if( !Ignore )
			*(Result++) = *((*Stream)++);
		else
			(*Stream)++;
	}
	if( Exact )
	{
		// Eat up exactly one CR/LF.
		if( **Stream == 13 )
			(*Stream)++;
		if( **Stream == 10 )
			(*Stream)++;
	}
	else
	{
		// Eat up all CR/LF's.
		while( **Stream==10 || **Stream==13 || **Stream=='|' )
			(*Stream)++;
	}
	*Result=0;
	return **Stream!=0 || GotStream;
	unguard;
}
CORE_API UBOOL ParseLine
(
	const TCHAR**	Stream,
	FString&		Result,
	UBOOL			Exact
)
{
	guard(ParseLine);
	UBOOL GotStream = 0;
	UBOOL IsQuoted = 0;
	UBOOL Ignore = 0;

	Result.Empty();
	while (**Stream && **Stream != '\n' && **Stream != '\r')
	{
		// Start of comments.
		if (!IsQuoted && !Exact && (*Stream)[0] == '/' && (*Stream)[1] == '/')
			Ignore = 1;

		// Command chaining.
		if (!IsQuoted && !Exact && **Stream == '|')
			break;

		// Check quoting.
		IsQuoted = IsQuoted ^ (**Stream == 34);
		GotStream = 1;

		// Got stuff.
		if (!Ignore)
			Result += *((*Stream)++);
		else
			(*Stream)++;
	}
	if (Exact)
	{
		// Eat up exactly one CR/LF.
		if (**Stream == 13)
			(*Stream)++;
		if (**Stream == 10)
			(*Stream)++;
	}
	else
	{
		// Eat up all CR/LF's.
		while (**Stream == 10 || **Stream == 13 || **Stream == '|')
			(*Stream)++;
	}
	return **Stream != 0 || GotStream;
	unguard;
}

/*----------------------------------------------------------------------------
	String substitution.
----------------------------------------------------------------------------*/

CORE_API FString appFormat( FString Src, const TMultiMap<FString,FString>& Map )
{
	guard(appFormat);
	FString Result;
	for( INT Toggle=0; ; Toggle^=1 )
	{
		INT Pos=Src.InStr(TEXT("%")), NewPos=Pos>=0 ? Pos : Src.Len();
		FString Str = Src.Left( NewPos );
		if( Toggle )
		{
			const FString* Ptr = Map.Find( Str );
			if( Ptr )
				Result += *Ptr;
			else if( NewPos!=Src.Len() )
				Result += US + TEXT("%") + Str + TEXT("%");
			else
				Result += US + TEXT("%") + Str;
		}
		else Result += Str;
		Src = Src.Mid( NewPos+1 );
		if( Pos<0 )
			break;
	}
	return Result;
	unguard;
}


/*-----------------------------------------------------------------------------
	High level file functions.
-----------------------------------------------------------------------------*/


//
// Update file modification time.
//
CORE_API UBOOL appUpdateFileModTime( TCHAR* Filename )
{
	guard(appUpdateFileModTime);
	FArchive* Ar = GFileManager->CreateFileWriter(Filename,FILEWRITE_Append,GNull);
	if( Ar )
	{
		delete Ar;
		return 1;
	}
	return 0;
	unguard;
}

//
// Load a binary file to a dynamic array.
//
CORE_API UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename, FFileManager* FileManager )
{
	guard(appLoadFileToArray);
	FArchive* Reader = FileManager->CreateFileReader( Filename );
	if( !Reader )
	{
		debugf(TEXT("Can't create FileReader for %ls"),Filename);
		return 0;
	}
	INT Size = Reader->TotalSize();
	Result.Empty(Size);
	Result.Add(Size);
	Reader->Serialize(&Result(0), Size);
	UBOOL Success = Reader->Close();
	delete Reader;
	return Success;
	unguard;
}

//
// Load a text file to an FString.
// Supports all combination of ANSI/Unicode files and platforms.
//
CORE_API UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename, FFileManager* FileManager )
{
	guard(appLoadFileToString);
	FArchive* Reader = FileManager->CreateFileReader( Filename );
	if( !Reader )
		return 0;
	INT Size = Reader->TotalSize();
	TArray<ANSICHAR> Ch( Size+2 );
	Reader->Serialize( &Ch(0), Size );
	UBOOL Success = Reader->Close();
	delete Reader;
	Ch( Size+0 )=0;
	Ch( Size+1 )=0;
	TArray<TCHAR>& ResultArray = Result.GetCharArray();
	ResultArray.Empty();
	//debugf(TEXT("Filename %s with Ch(0) %x, Ch(1) %x, Size %i"),Filename,(BYTE)Ch(0),(BYTE)Ch(1),Size);
	if( Size>=2 && !(Size&1) && (BYTE)Ch(0)==0xff && (BYTE)Ch(1)==0xfe && (BYTE)Ch(2)==0x00 && (BYTE)Ch(3)==0x00 )
	{
		// UTF32LE
		//debugf(TEXT("UTF32LE Filename %s with Ch(0) %x, Ch(1) %x, Ch(2) %x, Ch(3) %x, Size %i"),Filename,(BYTE)Ch(0),(BYTE)Ch(1),(BYTE)Ch(2),(BYTE)Ch(3),Size);
		ResultArray.Add( Size / 4);
		for( INT i=0; i<ResultArray.Num()-1; i++ )
			ResultArray( i ) = FromUnicode( (_WORD)(ANSICHARU)Ch(i*4+4) + (_WORD)(ANSICHARU)Ch(i*4+5)*256 );
	}
	else if( Size>=2 && !(Size&1) && (BYTE)Ch(0)==0x00 && (BYTE)Ch(1)==0x00 && (BYTE)Ch(2)==0x0fe && (BYTE)Ch(3)==0xff )
	{
		// UTF32BE
		//debugf(TEXT("UTF32BE Filename %s with Ch(0) %x, Ch(1) %x, Ch(2) %x, Ch(3) %x, Size %i"),Filename,(BYTE)Ch(0),(BYTE)Ch(1),(BYTE)Ch(2),(BYTE)Ch(3),Size);
		ResultArray.Add( Size / 4);
		for( INT i=0; i<ResultArray.Num()-1; i++ )
			ResultArray( i ) = FromUnicode( (_WORD)(ANSICHARU)Ch(i*4+7) + (_WORD)(ANSICHARU)Ch(i*4+6)*256 );
	}
	else if( Size>=2 && !(Size&1) && (BYTE)Ch(0)==0xff && (BYTE)Ch(1)==0xfe )
	{
		// UTF16LE Intel byte order.
		ResultArray.Add( Size / 2);
		for( INT i=0; i<ResultArray.Num()-1; i++ )
			ResultArray( i ) = FromUnicode( (_WORD)(ANSICHARU)Ch(i*2+2) + (_WORD)(ANSICHARU)Ch(i*2+3)*256 );
	}
	else if( Size>=2 && !(Size&1) && (BYTE)Ch(0)==0xfe && (BYTE)Ch(1)==0xff )
	{
		// UTF16BE non-Intel byte order.
		ResultArray.Add( Size / 2);
		for( INT i=0; i<ResultArray.Num()-1; i++ )
			ResultArray( i ) = FromUnicode( (_WORD)(ANSICHARU)Ch(i*2+3) + (_WORD)(ANSICHARU)Ch(i*2+2)*256 );
	}
	else if (Size >= 3 && (BYTE)Ch(0) == 0xef && (BYTE)Ch(1) == 0xbb && (BYTE)Ch(2) == 0xbf)
	{
		// UTF-8
		ResultArray.Add(Size - 3);
		appFromUtf8InPlace(&ResultArray(0), &Ch(3), ResultArray.Num());
	}
	else
	{
		// ASCII in system locale
		ResultArray.Add(Size + 1);
		appFromAnsiInPlace(&ResultArray(0), &Ch(0), ResultArray.Num());
	}
	ResultArray.Last() = 0;
	return Success;
	unguard;
}

//
// Save a binary array to a file.
//
CORE_API UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename, FFileManager* FileManager )
{
	guard(appSaveArrayToFile);
	FArchive* Ar = FileManager->CreateFileWriter( Filename );
	if( !Ar )
		return 0;
	Ar->Serialize( const_cast<BYTE*>(&Array(0)), Array.Num() );
	delete Ar;
	return 1;
	unguard;
}

//
// Write the FString to a file.
// Supports all combination of ANSI/Unicode files and platforms.
//
CORE_API UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename, FFileManager* FileManager )
{
	guard(appSaveStringToFile);
	if( !String.Len() )
		return 0;
	FArchive* Ar = FileManager->CreateFileWriter( Filename );
	if( !Ar )
		return 0;
	UBOOL SaveAsUnicode=0, Success=1;
#if UNICODE
	for( INT i=0; i<String.Len(); i++ )
	{
		if( (*String)[i] != (TCHAR)(ANSICHARU)ToAnsi((*String)[i]) )
		{
		    UNICHAR BOM =0x00;
		    if (sizeof(TCHAR)==2) //Windows 2bit wchar_t UTF16
                BOM = UTF16_BOM;
		    if (sizeof(TCHAR)==4) //Linux 4bit wchar_t UTF32
                BOM = UTF32_BOM;

			Ar->Serialize( &BOM, sizeof(BOM) );
			SaveAsUnicode = 1;
			break;
		}
	}
#endif

	if( SaveAsUnicode || sizeof(TCHAR)==1 )
	{
		Ar->Serialize( const_cast<TCHAR*>(*String), String.Len()*sizeof(TCHAR) );
	}
	else
	{
		TArray<ANSICHAR> AnsiBuffer(String.Len()+1);
		appToAnsiInPlace(&AnsiBuffer(0), &String[0], String.Len()+1);
		Ar->Serialize( const_cast<ANSICHAR*>(&AnsiBuffer(0)), String.Len() );
	}
	delete Ar;
	if( !Success )
		GFileManager->Delete( Filename );
	return Success;
	unguard;
}

/*-----------------------------------------------------------------------------
	Files.
-----------------------------------------------------------------------------*/

static TMap<FString, FString>* FileList = NULL;

//
// Find a file.
//
UBOOL appFindPackageFile( const TCHAR* InIn, TCHAR* Out )
{
	guard(appFindPackageFile);

	if (!FileList)
	{
		FileList = new TMap<FString, FString>;
		INT i,j,z;

		TArray<FString> LookDirs = GSys->Paths;
		for (i = 0; i < LookDirs.Num(); ++i)
		{
			TArray<FString> Dirs = GFileManager->FindFiles(*(LookDirs(i) * TEXT("*")), 0, 1);
			for (j = 0; j < Dirs.Num(); ++j)
				new(LookDirs)FString(LookDirs(i) * Dirs(j));
		}

		for (i = 0; i < LookDirs.Num(); ++i)
		{
			for (j = 0; j < GSys->Extensions.Num(); ++j)
			{
				TArray<FString> Files = GFileManager->FindFiles(*(LookDirs(i) * TEXT("*.") + GSys->Extensions(j)), 1, 0);
				for (z = 0; z < Files.Num(); ++z)
					FileList->Set(*Files(z).GetFilenameOnly(), *(LookDirs(i) * Files(z)));
			}
		}
	}

	const TCHAR* Start = InIn;
	const TCHAR* S = Start;
	while (*S)
	{
		if (*S == '/' || *S == '\\')
			Start = S + 1;
		++S;
	}
	const TCHAR* End = S;
	while (*S)
	{
		if (*S == '.')
			End = S;
		++S;
	}

	FString Lookup(Start, End);
	FString* Res = FileList->Find(*Lookup);
	if (Res)
	{
		appStrcpy(Out, **Res);
		return 1;
	}
	return 0;
	unguard;
}

static TArray<FString> CachedFileNames[256];
INT bCached=0;

CORE_API UBOOL appfindFileCaseInsensitive(const TCHAR* InFileName, TCHAR* OutFileName)
{
    guard(appfindFileCaseInsensitive);
	INT i = 0,j = 0;

	if (!bCached)
	{
		bCached = GSys->Paths.Num();
		for( i=0; i<GSys->Paths.Num(); i++ )
		{
			const TCHAR* Path = *GSys->Paths(i);
			TCHAR Temp[256]=TEXT("");

			// Strip path part
			appMemzero(&Temp,sizeof(Temp));
			INT l=appStrlen(Path);
			for( j=0; j<l; j++ )
			{
				if( Path[j]=='*' )
					break;
				Temp[j] = Path[j];
			}
			appStrcat( Temp, TEXT("*.*") ); // make sure to check any filetype

			CachedFileNames[i] = GFileManager->FindFiles( Temp, 1, 0 );
		}
	}

	for( i=0; i<bCached; ++i )
	{
		for( j=0; j<CachedFileNames[i].Num(); j++ )
		{
			if( !appStricmp(InFileName,*CachedFileNames[i](j)))
			{
				appStrcat( OutFileName, *CachedFileNames[i](j) );
				return 1;
			}
		}
	}
	return 0;
	unguard;
}
//
// Create a temporary file.
//
CORE_API void appCreateTempFilename( const TCHAR* Path, TCHAR* Result256 )
{
	guard(appCreateTempFilename);
	static INT i=0;
	do
		appSprintf( Result256, TEXT("%ls%ls%04X.tmp"), Path, PATH_SEPARATOR, i++ );
	while( GFileManager->FileSize(Result256)>0 );
	unguard;
}

//elmuerte: check program to execute, only allow local commands --
CORE_API UBOOL appCheckValidCmd( const TCHAR* URL )
{
#ifdef WIN32
#define PATH_FROM		TEXT("/")
#define PATH_TO			TEXT("\\")
#else
#define PATH_FROM		TEXT("\\")
#define PATH_TO			TEXT("/")
#endif

	FString BaseUrl = GFileManager->GetDefaultDirectory();
	FString valurl = FString::Printf(TEXT("%ls"), URL);

	// DOS'ify url
	INT i = valurl.InStr(PATH_FROM);
	while (i > -1)
	{
		valurl = valurl.Left(i)+PATH_TO+valurl.Mid(i+1);
		//debugf(TEXT("=> %i %ls"), i, *valurl);
		i = valurl.InStr(PATH_FROM);
	}
	// resolve "..\\"
	// not perfect..., but well enough
	// breaks on "../foo/../bar/../quux"
	i = valurl.InStr(TEXT("..")); // take the (back)slash for granted
	while (i > -1)
	{
		valurl = valurl.Left(i)+valurl.Mid(i+3);
		INT j = BaseUrl.InStr(PATH_TO, true);
		if (j > -1) BaseUrl = BaseUrl.Left(j);
		else {
			debugf(NAME_Warning, TEXT("Invalid commendline: %ls"), URL);
			return false;
		}
		//debugf(TEXT("-> %i %ls\t%ls"), i, *valurl, *BaseUrl);
		i = valurl.InStr(TEXT(".."));
	}

	valurl = BaseUrl+PATH_TO+valurl;
	//debugf(TEXT("Program to execute: %ls"), *valurl);

#ifdef WIN32
	FString tmp = valurl.Locs();
	if (tmp.InStr(GFileManager->GetDefaultDirectory().Locs()) != 0)
#else
	if (valurl.InStr(GFileManager->GetDefaultDirectory()) != 0)
#endif
	{
		debugf(NAME_Warning, TEXT("!!! Possible security breach"));
		debugf(NAME_Warning, TEXT("!!! Tried to execute a program outside the base directory: %ls"), URL);
		debugf(NAME_Warning, TEXT("!!! Execution refused"), URL);
		return false;
	}

	if (GFileManager->FileSize(*valurl) < 0)
	{
		debugf(NAME_Warning, TEXT("\"%ls\" doesn't exists, unable to create process"), *valurl);
		return false;
	}

	return true;
}
// -- elmuerte

/*-----------------------------------------------------------------------------
	Init and Exit.
-----------------------------------------------------------------------------*/

//
// General initialization.
//
static TCHAR GCmdLine[1024]=TEXT("");

CORE_API const TCHAR* appCmdLine()
{
	return GCmdLine;
}
CORE_API void appInit( const TCHAR* InPackage, const TCHAR* InCmdLine, FMalloc* InMalloc, FOutputDevice* InLog, FOutputDeviceError* InError, FFeedbackContext* InWarn, FFileManager* InFileManager, FConfigCache*(*ConfigFactory)(), UBOOL RequireConfig )
{
	guard(appInit);

	// Platform specific pre-init.
	appPlatformPreInit();

	// stijn: please please please do not use this code. This is the worst piece
	// of crap I've ever seen in a C++ program
//#if _MSC_VER
//	#ifdef UTPG_HOOK_SCAN
//	//SetMemSearchString("ReadProcessMemory\0", "Core.dll\0");
//	SetMemSearchString("GetProcAddress\0", "Core.dll\0");
//	#endif
//#endif

	GFileManager = InFileManager;

	// Init CRC table.
    for( DWORD iCRC=0; iCRC<256; iCRC++ )
		for( DWORD c=iCRC<<24, j=8; j!=0; j-- )
			GCRCTable[iCRC] = c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);

	// Init log table.
	{for( INT i=0,e=-1,c=0; i<=256; i++ )
	{
		GLogs[i] = e+1;
		if( !i || ++c>=(1<<e) )
			c=0, e++;
	}}
	// Command line.
		if( *InCmdLine=='\"' )
		{
			InCmdLine++;
			while( *InCmdLine && *InCmdLine!='\"' )
				InCmdLine++;
			if( *InCmdLine )
				InCmdLine++;
		}
		while( *InCmdLine && *InCmdLine!=' ' )
			InCmdLine++;
		if( *InCmdLine )
			InCmdLine++;
    appStrcpy( GCmdLine, InCmdLine );

    //wprintf(TEXT("GCmdLine %ls %i,%i"),GCmdLine,appStrlen(GCmdLine), appAtoi(GCmdLine));
	//printf(TEXT("GCmdLine %ls %i,%i"),InCmdLine,appStrlen(GCmdLine), appAtoi(InCmdLine));

	// Error history.
	appStrcpy( GErrorHist, TEXT("General protection fault!\r\n\r\nHistory: ") );

	// Subsystems.
	GLog         = InLog;
	GError       = InError;
	GWarn        = InWarn;

	// Memory allocator.
	// GMalloc = InMalloc;
	// GMalloc->Init();

	// vogel: moved here to ensure that the FileManager is inited before useage
#if __GNUG__
	GFileManager->Init( !ParseParam( appCmdLine(), TEXT("NOHOMEDIR") ) );
#else
	GFileManager->Init(1);
#endif

	// Init names.
	FName::StaticInit();

	#ifdef UTPG_MD5
	TK5Ahisl = new UZQ5Gnoyr();
	#endif

	// Switch into executable's directory.
	GFileManager->SetDefaultDirectory( appBaseDir() );

	// Command line.
	debugf( NAME_Init, TEXT("Version: %i"), ENGINE_VERSION);
	debugf( NAME_Init, TEXT("Compiled: %ls %ls"), appFromAnsi(__DATE__), appFromAnsi(__TIME__) );
	debugf( NAME_Init, TEXT("Command line: %ls"), appCmdLine() );
	debugf( NAME_Init, TEXT("Base directory: %ls"), appBaseDir() );
	debugf( NAME_Init, TEXT("Character set: %ls"), sizeof(TCHAR)==1 ? TEXT("ANSI") : TEXT("Unicode") );

	// Ini.
	appSprintf(GIni, TEXT("%s.ini"), InPackage);

	if( GFileManager->FileSize(GIni)<0 && RequireConfig )
	{
		// Create Package.ini from default.ini.
		appErrorf(TEXT("Missing %s"), GIni);
	}

 	// Init config.
	GConfig = ConfigFactory();
	GConfig->Init( GIni, InPackage, RequireConfig );

	// Safe mode.
	if( ParseParam(appCmdLine(),TEXT("safe")) || appStrfind(appCmdLine(),TEXT("READINI=")) )
		GSafeMode = 1;

	// Object initialization.
	UObject::StaticInit();

	// Memory initalization.

	if (!GIsEditor)
		GMem.Init( 65536 );
	else GMem.Init( 262144 );

	// Platform specific init.
	appPlatformInit();
	unguard;
}

//
// Pre-shutdown.
// Called from within guarded exit code, only during non-error exits.
//
CORE_API void appPreExit()
{
	guard(appPreExit);

	debugf( NAME_Exit, TEXT("Preparing to exit.") );
	appPlatformPreExit();
	GMem.Exit();
	UObject::StaticExit();

	unguard;
}

//
// Shutdown.
// Called outside guarded exit code, during all exits (including error exits).
//
void appExit()
{
	guard(appExit);
	debugf( NAME_Exit, TEXT("Exiting.") );
	appPlatformExit();
	if( GConfig )
	{
        GConfig->Flush(0);
		GConfig->Exit();
		delete GConfig;
		GConfig = NULL;
	}
	FName::StaticExit();
	if( !GIsCriticalError )
		GMalloc->DumpAllocs();
	unguard;
}

/*-----------------------------------------------------------------------------
	USystem.
-----------------------------------------------------------------------------*/

void USystem::StaticConstructor()
{
	guard(USystem::StaticConstructor);
	unguard;
}

#ifdef _DEBUG
#pragma warning (push)
#pragma warning (disable : 4717)
static void Recurse()
{
	guard(Recurse);
	if (1)
		Recurse();
	unguard;
}
#pragma warning (pop)
#endif

UBOOL USystem::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (ParseCommand(&Cmd, TEXT("MEMSTAT")))
	{
		DumpMemStat(Ar);
		return 1;
	}
	else if (ParseCommand(&Cmd, TEXT("CONFIGHASH")))
	{
		GConfig->Dump(Ar);
		return 1;
	}
	else if (ParseCommand(&Cmd, TEXT("OS")))
	{
		Ar.Log(GetOS());
		return 1;
	}
	else if (ParseCommand(&Cmd, TEXT("RELAUNCH")))
	{
		GLog->Logf(NAME_Exit, TEXT("Relaunch: %ls"), Cmd);
		GConfig->Flush(0);
		appRelaunch(Cmd);
		appRequestExit(0, TEXT("RELAUNCH"));
		return 1;
	}
#ifdef _DEBUG
	else if (ParseCommand(&Cmd, TEXT("DEBUG")))
	{
		if (ParseCommand(&Cmd, TEXT("CRASH")))
		{
			appErrorf(TEXT("%ls"), TEXT("Unreal crashed at your request"));
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("GPF")))
		{
			Ar.Log(TEXT("Unreal crashing with voluntary GPF"));
			*(volatile int*)NULL = 123;
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("RECURSE")))
		{
			Ar.Logf(TEXT("Recursing"));
			Recurse();
			return 1;
		}
		else if (ParseCommand(&Cmd, TEXT("EATMEM")))
		{
			Ar.Log(TEXT("Eating up all available memory"));
			while (1)
			{
				void* Eat = appMalloc(65536, TEXT("EatMem"));
				memset(Eat, 0, 65536);
			}
			return 1;
		}
		else return 0;
	}
#endif
	else return 0;
}

IMPLEMENT_CLASS(USystem);

/*-----------------------------------------------------------------------------
	String conversion.
-----------------------------------------------------------------------------*/

// TCHAR to ANSICHAR.
CORE_API const ANSICHAR* appToAnsi( const TCHAR* Str )
{
	guard(appToAnsi);
	if( !Str )
		return NULL;

	INT Len = appStrlen(Str) + 1;
	ANSICHAR* ACh = appAnsiStaticString(Len);
	if (ACh)
		appToAnsiInPlace(ACh, Str, Len);
	return ACh;
	unguard;
}

// TCHAR to UNICHAR.
CORE_API const UNICHAR* appToUnicode( const TCHAR* Str )
{
	guard(appToUnicode);
	if (!Str)
		return NULL;
	UNICHAR* UCh = reinterpret_cast<UNICHAR*>(appStaticString1024()); // stijn: memory safety OK
	appToUnicodeInPlace(UCh, Str, 1024 / sizeof(UNICHAR) * sizeof(TCHAR));
	return UCh;
	unguard;
}

// ANSICHAR to TCHAR.
CORE_API const TCHAR* appFromAnsi( const ANSICHAR* ACh )
{
	guardSlow(appFromAnsi);
	if( !ACh )
		return NULL;

	size_t Len = strlen(ACh) + 1;
	TCHAR* Ch = appStaticString(Len);
	if (Ch)
		appFromAnsiInPlace(Ch, ACh, Len);
	return Ch;
	unguardSlow;
}

// UNICHAR to TCHAR.
CORE_API const TCHAR* appFromUnicode( const UNICHAR* UCh )
{
	guardSlow(appFromUnicode);
	if( !UCh )
		return NULL;
	TCHAR* Ch = appStaticString1024(); // stijn: memory safety OK
	appFromUnicodeInPlace(Ch, UCh, 1024);
	return Ch;
	unguardSlow;
}

/*-----------------------------------------------------------------------------
	Error handling.
-----------------------------------------------------------------------------*/

//
// Unwind the stack.
//
CORE_API void VARARGS appUnwindf( const TCHAR* Fmt, ... )
{
	GIsCriticalError = 1;

	TCHAR TempStr[4096];
	GET_VARARGS( TempStr, ARRAY_COUNT(TempStr), Fmt );

	static INT Count=0;
	if( Count++ )
		appStrncat( GErrorHist, TEXT(" <- "), ARRAY_COUNT(GErrorHist) );
	appStrncat( GErrorHist, TempStr, ARRAY_COUNT(GErrorHist) );

	debugf( NAME_Critical, TempStr );
}


#ifndef _MSC_VER
thread_local INT UnGuardBlock::GuardIndex = 0;
thread_local const TCHAR* UnGuardBlock::GuardTexts[MAX_RYANS_HACKY_GUARD_BLOCKS];

void UnGuardBlock::GetBackTrace(FString &str)
{
	INT i = MAX_RYANS_HACKY_GUARD_BLOCKS;
	if (GuardIndex < i)
		i = GuardIndex;

	if (!i)
		str = TEXT("(no backtrace available.)");
	else
	{
		str = GuardTexts[i];
		while ( --i > 0 )
		{
			str += TEXT(" <- ");
			str += GuardTexts[i];
		}
	}
}
#endif

/*-----------------------------------------------------------------------------
	Parsing conditional param.
-----------------------------------------------------------------------------*/

inline INT RoundToInt(DOUBLE d)
{
	return d < 0.f ? ((INT)(d - 0.5)) : ((INT)(d + 0.5));
}
inline BYTE fromHEX(TCHAR c)
{
	return (c >= '0' && c <= '9') ? (c - '0') : (c >= 'a' && c <= 'f') ? (c - 'a' + 10) : (c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 0;
}

struct FObjValueParser : public FOutputDevice
{
	static DOUBLE LastValueCache;

	DOUBLE ResultValue;
	BYTE bError;
private:
	struct FPropPair
	{
		DOUBLE Value;
		BYTE Action,Pass;

		FPropPair(DOUBLE v, BYTE a, BYTE PreOperator)
			: Action(a), Pass(a<=2)
		{
			if (PreOperator == 200)
				Value = (~RoundToInt(v) & MAXINT);
			else if (PreOperator == 202)
				Value = -v;
			else Value = v;
		}
	};
	UObject* Object;
	const TCHAR* S;
	const TCHAR* End;
	DOUBLE DigitResult;

	void Serialize(const TCHAR* V, EName Event)
	{
		GWarn->Log(NAME_ExecWarning, V);
		S = End;
		bError = 1;
	}
	inline BYTE IsDigitNext()
	{
		if (!appIsDigit(*S))
			return 0;

		const TCHAR* T = S;
		BYTE bFloat = 0;

		while (T < End)
		{
			if (appIsDigit(*T))
				++T;
			else if (*T == '.' || *T == 'f')
			{
				++T;
				bFloat = 1;
			}
			else if (*T == 'e' && (T[1] == '+' || T[1] == '-'))
			{
				T+=2;
				bFloat = 1;
			}
			else break;
		}

		if (bFloat)
			DigitResult = appAtof(S);
		else DigitResult = appAtoi(S);
		S = T;
		return 1;
	}
	inline BYTE IsCommandNext(BYTE bFirstExpr)
	{
		if (*S == '~')
		{
			++S;
			return 200; // Not
		}
		else if (*S == '+')
		{
			++S;
			return bFirstExpr ? 201 : 1; // Add
		}
		else if (*S == '-')
		{
			++S;
			return bFirstExpr ? 202 : 2; // Sub
		}
		else if (*S == '|')
		{
			++S;
			return 3; // Or
		}
		else if (*S == '^')
		{
			++S;
			return 4; // XOr
		}
		else if (S[0] == '*' && S[1] == '*')
		{
			S+=2;
			return 10; // Pow
		}
		else if (*S == '*')
		{
			++S;
			return 5; // Multi
		}
		else if (*S == '/')
		{
			++S;
			return 6; // Div
		}
		else if (*S == '<' && *(S+1) == '<')
		{
			S+=2;
			return 7; // Lshift
		}
		else if (*S == '>' && *(S + 1) == '>')
		{
			S += 2;
			return 8; // Rshift
		}
		else if (*S == '&')
		{
			++S;
			return 9; // And
		}
		return 255;
	}
	inline BYTE ParseVariable()
	{
		TCHAR* Str = const_cast<TCHAR*>(S);
		TCHAR* Start = Str;
		BYTE bMultiExpr = 0;

		while (Str < End && (appIsAlnum(*Str) || *Str == '.' || *Str =='_'))
		{
			if (*Str == '.')
				bMultiExpr = 1;
			++Str;
		}
		S = Str;

		UField* Field = NULL;
		UClass* UseObject = NULL;

		if (!bMultiExpr) // Something simple from this class only.
		{
			TCHAR Temp = *Str;
			*Str = 0;
			if (!appStrcmp(Start, TEXT("LAST_VALUE")))
			{
				*Str = Temp;
				DigitResult = LastValueCache;
				return 1;
			}
			FName Expr(Start, FNAME_Find);
			*Str = Temp;
			if (Expr == NAME_None)
				return 0;

			Field = FindField<UField>(Object->GetClass(), *Expr);
			UseObject = Object->GetClass();

			if (!Field)
				Field = FindObject<UConst>(ANY_PACKAGE, *Expr, 1);
			if (!Field)
			{
				// Find any enum containing that name!
				for (TObjectIterator<UEnum> It; It; ++It)
				{
					INT Res = It->Names.FindItemIndex(Expr);
					if (Res >= 0)
					{
						DigitResult = Res;
						return 1;
					}
				}
				return 0;
			}
		}
		else
		{
			TCHAR Temp = *Str;
			*Str = 0;
			FString FStr(Start);
			*Str = Temp;
			INT i = FStr.InStr(TEXT("."));
			FString FClass = FStr.Left(i);
			FStr = FStr.Mid(i+1);

			UseObject = FindObject<UClass>(ANY_PACKAGE, *FClass);
			if (!UseObject)
				return 0;
			Field = FindField<UField>(UseObject, *FStr);
			if (!Field)
				return 0;
		}

		if (Field->GetClass() == UConst::StaticClass())
		{
			const TCHAR* Value = *((UConst*)Field)->Value;
			while (*Value == ' ' || *Value == '\t') // Skip whitespaces.
				++Value;

			if (Value[0] == '0' && Value[1] == 'x')
			{
				// Convert from HEX
				Value += 2;
				INT ResultInt = 0;
				while (*Value && *Value!=' ')
				{
					ResultInt = (ResultInt<<4) | fromHEX(*Value);
					++Value;
				}
				DigitResult = ResultInt;
				return 1;
			}
			const TCHAR* Test = Value;
			BYTE bFloat = 0;
			while (*Test)
			{
				if (*Test == '.')
					bFloat = 1;
				++Test;
			}
			if (bFloat)
				DigitResult = appAtof(Value);
			else DigitResult = appAtoi(Value);
		}
		else if (Field->GetClass() == UIntProperty::StaticClass() || Field->GetClass() == UFloatProperty::StaticClass() || Field->GetClass() == UByteProperty::StaticClass())
		{
			UProperty* P = (UProperty*)Field;
			BYTE* Data = ((BYTE*)UseObject->GetDefaultObject()) + P->Offset;
			if (Field->GetClass() == UIntProperty::StaticClass())
				DigitResult = *(INT*)Data;
			else if (Field->GetClass() == UFloatProperty::StaticClass())
				DigitResult = *(FLOAT*)Data;
			else if (Field->GetClass() == UByteProperty::StaticClass())
				DigitResult = *(BYTE*)Data;
		}
		else return 0;

		return 1;
	}
	DOUBLE ParseNextSegment(BYTE Depth)
	{
		TArray<FPropPair> Commands;
		BYTE PrevCommand = 255;
		BYTE PreOp = 255;

		while (S < End)
		{
			// Skip whitespaces
			while (*S == ' ' || *S == '\t')
				++S;

			if (S >= End)
				break;

			// Parse next token.
			if (*S == '(')
			{
				++S;
				DOUBLE Value = ParseNextSegment(Depth+1);
				if (bError)
					return 0.f;
				if (PrevCommand == 254)
				{
					Logf(TEXT("ParseCommand error: Invalid operator between values (at %ls)!"), S);
					return 0.f;
				}
				new (Commands) FPropPair(Value, PrevCommand, PreOp);
				PreOp = 255;
				PrevCommand = 254;
				continue;
			}
			else if (*S == ')')
			{
				++S;
				break;
			}

			const TCHAR* Start = S;
			if (IsDigitNext())
			{
				if (PrevCommand == 254)
				{
					Logf(TEXT("ParseCommand error: Invalid operator between values (at %ls)!"), Start);
					return 0.f;
				}
				new (Commands) FPropPair(DigitResult, PrevCommand, PreOp);
				PreOp = 255;
				PrevCommand = 254;
			}
			else
			{
				BYTE Res = IsCommandNext(Commands.Num()==0 || PrevCommand<250);
				if (Res < 255)
				{
					if (Res >= 200)
						PreOp = Res;
					else PrevCommand = Res;
				}
				else if(ParseVariable())
				{
					if (PrevCommand == 254)
					{
						Logf(TEXT("ParseCommand error: Invalid operator between values (at %ls)!"), Start);
						return 0.f;
					}
					new (Commands) FPropPair(DigitResult, PrevCommand, PreOp);
					PreOp = 255;
					PrevCommand = 254;
				}
				else
				{
					Logf(TEXT("ParseCommand error: Invalid command or expression (at %ls)!"), Start);
					return 0.f;
				}
			}
		}

		if (Commands.Num() == 0)
			return 0.f;

		for (BYTE Pass = 0; Pass < 2; ++Pass)
		{
			for (INT i = 1; i < Commands.Num(); ++i)
			{
				if (Commands(i).Pass != Pass) continue;
				DOUBLE Value = Commands(i - 1).Value;
				switch (Commands(i).Action)
				{
				case 1:
					Value += Commands(i).Value;
					break;
				case 2:
					Value -= Commands(i).Value;
					break;
				case 3:
					Value = RoundToInt(Value) | RoundToInt(Commands(i).Value);
					break;
				case 4:
					Value = RoundToInt(Value) ^ RoundToInt(Commands(i).Value);
					break;
				case 5:
					Value *= Commands(i).Value;
					break;
				case 6:
					Value /= Commands(i).Value;
					break;
				case 7:
					Value = RoundToInt(Value) << RoundToInt(Commands(i).Value);
					break;
				case 8:
					Value = RoundToInt(Value) >> RoundToInt(Commands(i).Value);
					break;
				case 9:
					Value = RoundToInt(Value) & RoundToInt(Commands(i).Value);
					break;
				case 10:
					Value = appPow(Value, Commands(i).Value);
					break;
				}
				Commands(i - 1).Value = Value;
				Commands.Remove(i--);
			}
		}
		return Commands(0).Value;
	}
public:
	FObjValueParser(UObject* InObj, const TCHAR* St, const TCHAR* En)
		: bError(0), Object(InObj), S(St), End(En)
	{
		ResultValue = ParseNextSegment(0);
	}
};

DOUBLE FObjValueParser::LastValueCache = 0.f;

UBOOL UObject::ParsePropertyValue(void* Result, const TCHAR** Str, BYTE bIntValue)
{
	guard(UObject::ParsePropertyValue);
	// Skip whitespaces at first.
	while (**Str == ' ' || **Str == '\t')
		++*Str;

	// Check if float/int value only
	const TCHAR* S = *Str;
	const TCHAR* End = *Str;
	UBOOL bNonDigit = 0;
	UBOOL bIsHEX = 0;

	{
		if (*S == '+' || *S == '-') // Skip positive or negative sign at start.
			++S;

		BYTE bDigits = 0;
		BYTE bEscapeFloat = 0;
		BYTE bDepth = 0;
		BYTE bHasSpaces = 0;
		while (1)
		{
			if (*S == ')')
			{
				if (bDepth == 0)
				{
					End = S;
					break;
				}
				--bDepth;
			}
			if (*S == '(')
			{
				bNonDigit = 1;
				++bDepth;
				++S;
				continue;
			}
			if (!*S || *S == ',' || *S == '\n' || *S == '\r' || *S == '}' || *S == ';')
			{
				End = S;
				break;
			}

			if (*S == ' ' || *S == '\t')
			{
				if (!bHasSpaces)
				{
					End = S;
					bHasSpaces = 1;
				}
				else if(bDigits)
					bNonDigit = 1;
				++S;
				continue;
			}
			if (!bDigits && !bNonDigit)
			{
				if (S[0] == '0' && S[1] == 'x')
				{
					bIsHEX = TRUE;
					bDigits = TRUE;
					S += 2;
					while (appIsDigit(*S) || (*S>='a' && *S<='f') || (*S >= 'A' && *S <= 'F'))
						++S;
					End = S;
					continue;
				}
			}

			if (bEscapeFloat)
				bNonDigit = 1;
			if (*S == 'f')
				bEscapeFloat = 1;
			else if(*S == 'e' && (S[1] == '+' || S[1] == '-')) // exponent float.
				++S;
			else if (!appIsDigit(*S) && *S != '.')
				bNonDigit = 1;
			else if(bHasSpaces) // Do not allow additional digits after space.
				bDigits = 1;
			++S;
		}
	}

	if (!bNonDigit)
	{
		TCHAR* EChr = const_cast<TCHAR*>(End);
		TCHAR REChr = *EChr;
		*EChr = 0;

		if (bIsHEX && bIntValue != 2)
		{
			INT ResultInt = 0;
			S = *Str;
			UBOOL bNegative = (*S == '-');

			if (*S == '+' || *S == '-') // Skip positive or negative sign at start.
				++S;
			while (*S == ' ' || *S == '\t')
				++S;

			// Convert from HEX
			S += 2;
			while (*S && *S != ' ')
			{
				ResultInt = (ResultInt << 4) | fromHEX(*S);
				++S;
			}
			if (bNegative)
				ResultInt = -ResultInt;
			if (bIntValue == 1)
				*(INT*)Result = ResultInt;
			else *(FLOAT*)Result = ResultInt;
		}
		else
		{
			if (bIntValue == 1)
				*(INT*)Result = appAtoi(*Str);
			else if (bIntValue == 2)
				*(FString*)Result = *Str;
			else *(FLOAT*)Result = appAtof(*Str);
		}

		// Skip stream to end of digits
		*EChr = REChr;
		*Str = End;

		return 1;
	}

	FObjValueParser Parse(this, *Str, End);
	*Str = End;

	if (Parse.bError)
		return 0;

	FObjValueParser::LastValueCache = Parse.ResultValue;

	if (bIntValue==1)
		*(INT*)Result = RoundToInt(Parse.ResultValue);
	else if (bIntValue == 2)
	{
		TCHAR ResStr[64]=TEXT("");
		appSprintf(ResStr, TEXT("%1.10f"), Parse.ResultValue);
		TCHAR* T = ResStr;
		while (*T && *T != '.') // Skip until decimal point.
			++T;
		if (*T == '.')
		{
			TCHAR* Dec = T++;
			TCHAR* NZero = NULL;
			while (*T)
			{
				if (*T != '0') // Find final trailing non-zero
					NZero = T;
				++T;
			}
			if (!NZero)
				*Dec = 0; // Remove decimal sign.
			else *(NZero + 1) = 0; // Remove any additional zeroes.
		}
		*(FString*)Result = ResStr;
	}
	else *(FLOAT*)Result = (FLOAT)Parse.ResultValue;
	return 1;
	unguardobj;
}

/** 
 * Removes the executable name from a commandline, denoted by parentheses.
 */
const TCHAR* RemoveExeName(const TCHAR* CmdLine)
{
    guard(RemoveExeName);
	// Skip over executable that is in the commandline
	if( *CmdLine=='\"' )
	{
		CmdLine++;
		while( *CmdLine && *CmdLine!='\"' )
		{
			CmdLine++;
		}
		if( *CmdLine )
		{
			CmdLine++;
		}
	}
	while( *CmdLine && *CmdLine!=' ' )
	{
		CmdLine++;
	}
	// skip over any spaces at the start, which Vista likes to toss in multiple
	while (*CmdLine == ' ')
	{
		CmdLine++;
	}
	return CmdLine;
    unguard;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
