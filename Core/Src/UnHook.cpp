/*=============================================================================
	UnHook: Memory scanning for hooks

Revision history:
	* Created by Michiel Hendriks
=============================================================================*/

/*-----------------------------------------------------------------------------
	Hook detection routine
-----------------------------------------------------------------------------*/
// only works for Windows
#if _MSC_VER
#include "windows.h"
#endif
#include "CorePrivate.h"

#ifdef UTPG_HOOK_SCAN
#define MAX_SCANTIME 0.005f

// elmuerte: find hook strings, must be UPPERCASE
char *MemScanStr1				= "\0"; //"GETPROCADDRESS\0";
char *MemScanStr2				= "\0"; //"CORE.DLL\0";

inline UBOOL findInMem(char *p, char *pend, char *find, size_t findLen)
{
	char *sptr0, *sptr1;
	char *mptr0, *mptr1;
	char c, d;
	size_t	length;

	if(findLen <= 0) return 0;
	if(p == NULL) return 0;
	if(pend == NULL) return 0;
	if(find == NULL) return 0;

	sptr0 = p;
	mptr0 = find;

	if (mptr0 > p && mptr0 < pend)
	{
		//debugf(TEXT("THIS IS ME"));
		return 0;
	}

	while(sptr0 < pend-findLen)
	{
		c = *sptr0;
		if (c > 97) c -= 32;
		d = *mptr0;
		if (d > 97) d -= 32;
		if(c == *mptr0)
		{
			sptr1 = sptr0 + 1;
			mptr1 = mptr0 + 1;
			length = findLen;
			if(sptr1 >= pend-findLen) return 0;
			while(--length > 0)
			{
				c = *sptr1++;
				if (c > 97) c -= 32;
				d = *mptr1++;
				if (d > 97) d -= 32;
				if(c != d) break;
			}
			if(length == 0) return 1;
		}
		sptr0++;
	}
	return 0;
}

inline UBOOL scanSinglePage(FOutputDevice& Ar, MEMORY_BASIC_INFORMATION* mbi)
{
	int iUnReadable = 0;
	iUnReadable |= (mbi->State == MEM_FREE);
	iUnReadable |= (mbi->State == MEM_RESERVE);
	iUnReadable |= (mbi->Protect & PAGE_WRITECOPY);
	iUnReadable |= (mbi->Protect & PAGE_EXECUTE);
	iUnReadable |= (mbi->Protect & PAGE_GUARD);
	iUnReadable |= (mbi->Protect & PAGE_NOACCESS);
	if (iUnReadable) return 0;
	//if (mbi->Type != MEM_MAPPED) return 0; // hook is mapped (?)
	//if (mbi->Type == MEM_PRIVATE) return 0; // helios hook 4.2 is private, others where mapped
	if (mbi->Type == MEM_IMAGE) return 0; // dll image?

	char srcString[33];
	char *p, *pend;

	strncpy((char *) srcString, MemScanStr1, 32);
	size_t iLen = strlen(MemScanStr1);
	p = (char *) mbi->BaseAddress;
	pend = p + mbi->RegionSize;

	if (findInMem(p, pend, srcString, iLen))
	{
		strncpy((char *) srcString, MemScanStr2, 32);
		size_t iLen = strlen(MemScanStr2);
		p = (char *) mbi->BaseAddress;
		pend = p + mbi->RegionSize;
		if (findInMem(p, pend, srcString, iLen))
		{
			// don't show this message, unless started with -debug
			if (ParseParam( appCmdLine(), TEXT("DEBUG") ))
				Ar.Logf(TEXT("Memory corruption in page %08lx"), mbi->BaseAddress);
			return 1; // dont' keep scanning!!
		}
	}

	return 0;
}

UBOOL MemPageScan(FOutputDevice& Ar)
{
	MEMORY_BASIC_INFORMATION mbi;

	static LPVOID lpMaximumApplicationAddress;
	static LPVOID lpMem = 0;

	if (strlen(MemScanStr1) == 0) return 1;
	if (strlen(MemScanStr2) == 0) return 1;

	if (!lpMaximumApplicationAddress)
	{
		SYSTEM_INFO	si;
		GetSystemInfo(&si);
		lpMaximumApplicationAddress = si.lpMaximumApplicationAddress;
	}

	float x = appSeconds().GetFloat();
	while ((lpMem < lpMaximumApplicationAddress))
	{
		VirtualQuery(lpMem, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		if (scanSinglePage(Ar, &mbi)) return 1;
		lpMem = (LPVOID)((uintptr_t)mbi.BaseAddress + (uintptr_t)mbi.RegionSize);

		if (appSeconds().GetFloat() - x > MAX_SCANTIME) break;
	}

	if (lpMem >= lpMaximumApplicationAddress)
	{
		lpMem = 0;
		//debugf(TEXT("done!"));
	}


	// TODO: remove
	//x = appSeconds().GetFloat() - x;
	//debugf(TEXT("Took: %f seconds; %08lx %08lx"), x, lpMem, lpMaximumApplicationAddress);

	return 0;
}

void SetMemSearchString(char *str1, char *str2)
{
	MemScanStr1 = (str1);
	MemScanStr2 = (str2);
}
#endif


/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
