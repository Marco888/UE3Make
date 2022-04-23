/*=============================================================================
	FMallocAnsi.h: ANSI memory allocator.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#define ADDRESS_CHECKER 0

#if ADDRESS_CHECKER
struct FAddressChecker
{
	const TCHAR* DataTag;
	size_t DataSize;
	QWORD PreData;
};
struct FPostData
{
	QWORD PostData;
};
constexpr QWORD DataConst = 123456789;

#define GET_ADDR_OF(ptr) ((FAddressChecker*)((BYTE*)ptr)-sizeof(FAddressChecker))

inline void VerifyIntegrity(FAddressChecker* A)
{
	/*if (A->PreData != DataConst)
		appErrorf(TEXT("Failure with %ls (%lld vs %lld)"), A->DataTag, A->PreData, DataConst);
	FPostData* B = (FPostData*)(((BYTE*)A) + A->DataSize);
	if (B->PostData != DataConst)
		appErrorf(TEXT("Failure with %ls (%lld vs %lld)"), A->DataTag, B->PostData, DataConst);*/
}

#define guardMem guard
#define unguardMem unguard
#define unguardfMem unguardf
#else
#define guardMem guardSlow
#define unguardMem unguardSlow
#define unguardfMem unguardfSlow
#endif

//
// ANSI C memory allocator.
//
class FMallocAnsi : public FMalloc
{
public:
	// FMalloc interface.
	void* Malloc(size_t Size, const TCHAR* Tag)
	{
		guardMem(FMallocAnsi::Malloc);
		if (Size <= 0)
			appErrorf(TEXT("FMallocAnsi::Malloc with Size=%i Tag: %ls"), Size, Tag);
#if ADDRESS_CHECKER
		FAddressChecker* Ptr = (FAddressChecker*)malloc(Size + sizeof(FAddressChecker) + sizeof(FPostData));
		Ptr->DataTag = Tag;
		Ptr->DataSize = Size + sizeof(FAddressChecker);
		Ptr->PreData = DataConst;
		FPostData* PostPtr = (FPostData*)(((BYTE*)Ptr) + Ptr->DataSize);
		PostPtr->PostData = DataConst;
		return (((BYTE*)Ptr) + sizeof(FAddressChecker));
#else
		void* Ptr = malloc(Size);
		check(Ptr);
		return Ptr;
#endif
		unguardfMem((TEXT("%i %ls"), Size, Tag));
	}
	void* Realloc(void* Ptr, size_t NewSize, const TCHAR* Tag)
	{
		guardMem(FMallocAnsi::Realloc);
		void* Result;
		if( Ptr && NewSize )
		{
#if ADDRESS_CHECKER
			FAddressChecker* A = GET_ADDR_OF(Ptr);
			VerifyIntegrity(A);
			A = (FAddressChecker*)realloc(A, NewSize + sizeof(FAddressChecker) + sizeof(FPostData));
			A->DataTag = Tag;
			A->DataSize = NewSize + sizeof(FAddressChecker);
			FPostData* B = (FPostData*)(((BYTE*)Ptr) + A->DataSize);
			B->PostData = DataConst;
			Result = ((BYTE*)A) + sizeof(FAddressChecker);
#else
			Result = realloc(Ptr, NewSize);
#endif
		}
		else if( NewSize )
		{
#if ADDRESS_CHECKER
			Result = Malloc(NewSize, Tag);
#else
			Result = malloc(NewSize);
#endif
		}
		else
		{
			if( Ptr )
			{
#if ADDRESS_CHECKER
				Free(Ptr);
#else
				free(Ptr);
#endif
			}
			Result = nullptr;
		}
		return Result;
		unguardfMem(( TEXT("%08X %i %ls"), (PTRINT)Ptr, NewSize, Tag ));
	}
	void Free( void* Ptr )
	{
		guardMem(FMallocAnsi::Free);
		if (!Ptr)
			return;

#if ADDRESS_CHECKER
		FAddressChecker* A = GET_ADDR_OF(Ptr);
		VerifyIntegrity(A);
		free(A);
#else
		free(Ptr);
#endif
		unguardMem;
	}
	void DumpAllocs()
	{
		guard(FMallocAnsi::DumpAllocs);
		debugf( NAME_Exit, TEXT("Allocation checking disabled") );
		unguard;
	}
	void HeapCheck()
	{
		guard(FMallocAnsi::HeapCheck);
#if _MSC_VER
		INT Result = _heapchk();
		check(Result!=_HEAPBADBEGIN);
		check(Result!=_HEAPBADNODE);
		check(Result!=_HEAPBADPTR);
		check(Result!=_HEAPEMPTY);
		check(Result==_HEAPOK);
#endif
		unguard;
	}
	void Init()
	{
		guard(FMallocAnsi::Init);
		unguard;
	}
	void Exit()
	{
		guard(FMallocAnsi::Exit);
		unguard;
	}
};

#ifdef ADDRESS_CHECKER
#undef ADDRESS_CHECKER
#endif

#undef guardMem
#undef unguardMem
#undef unguardfMem

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
