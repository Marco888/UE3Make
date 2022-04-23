/*=============================================================================
FMallocWindows.h: Windows optimized memory allocator.
Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
* Created by Tim Sweeney
=============================================================================*/

#define MEM_TIME(st)

//#define TRACK_MEMORYUSE // Marco: Uncomment this to enable slow memory useage tracker!

#ifdef TRACK_MEMORYUSE
#define MEMTRACK_HASHSIZE 1023
#endif

//
// Optimized Windows virtual memory allocator.
//
class FMallocWindows : public FMalloc
{
private:
	// Counts.
	enum { POOL_COUNT = 49 };
	enum { POOL_MAX = 32768 + 1 };

	// Forward declares.
	struct FFreeMem;
	struct FPoolTable;

	// Memory pool info. 32 bytes.
	struct FPoolInfoBase
	{
		size_t	Bytes;		// Bytes allocated for pool.
		size_t	OsBytes;	// Bytes aligned to page size.
		DWORD   Taken;      // Number of allocated elements in this pool, when counts down to zero can free the entire pool.
		BYTE*       Mem;		// Memory base.
		FPoolTable* Table;		// Index of pool.
		FFreeMem*   FirstMem;   // Pointer to first free memory in this pool.
	};
	typedef TDoubleLinkedList<FPoolInfoBase> FPoolInfo;

	// Information about a piece of free memory. 8 bytes.
	struct FFreeMem
	{
		FFreeMem*	Next;		// Next or MemLastPool[], always in order by pool.
		size_t		Blocks;		// Number of consecutive free blocks here, at least 1.
		FPoolInfo* GetPool()
		{
			return (FPoolInfo*)((size_t)this & 0xffff0000);
		}
	};

	// Pool table.
	struct FPoolTable
	{
		FPoolInfo* FirstPool;
		FPoolInfo* ExaustedPool;
		size_t  BlockSize;
	};

	// Variables.
	FPoolTable  PoolTable[POOL_COUNT], OsTable;
	FPoolInfo*	PoolIndirect[256];
	FPoolTable* MemSizeToPoolTable[POOL_MAX];
	INT      MemInit;
	size_t		OsCurrent, OsPeak, UsedCurrent, UsedPeak, CurrentAllocs, TotalAllocs;
	FTime		MemTime;
	FThreadLock MemMutex;

	// Mem tracker
#ifdef TRACK_MEMORYUSE
	struct FMemTrackMark;

	struct FMemTrackPool
	{
		DWORD Index;
		TCHAR MemTag[256];
		FMemTrackPool* Next;
		FMemTrackMark* List;

		FMemTrackPool(const TCHAR* s)
			: Next(NULL), List(NULL)
		{
			appStrcpy(MemTag, s);
		}
	};
	FMemTrackPool* MemTrack[MEMTRACK_HASHSIZE+1];

	struct FMemTrackMark
	{
		DWORD Index;
		FMemTrackPool* Pool;
		void* ptr;
		size_t size;
		FMemTrackMark* Next;
		FMemTrackMark* AllocNext;

		FMemTrackMark(FMemTrackPool* L, void* p)
			: Pool(L), ptr(p), Next(NULL)
		{
			AllocNext = L->List;
			L->List = this;
		}
	};
	FMemTrackMark* MemPoolTrack[MEMTRACK_HASHSIZE + 1];

	inline FMemTrackPool* FindTagTracker(const TCHAR* Tag, BYTE bAdd)
	{
		DWORD t = appStrihash(Tag) & MEMTRACK_HASHSIZE;
		
		if (MemTrack[t])
		{
			for (FMemTrackPool* tr = MemTrack[t]; tr; tr = tr->Next)
			{
				if (!appStrcmp(tr->MemTag, Tag))
					return tr;
			}
		}
		if(bAdd)
		{
			FMemTrackPool* track = (FMemTrackPool*)malloc(sizeof(FMemTrackPool));
			appStrcpy(track->MemTag, Tag);
			track->List = NULL;
			track->Index = t;

			track->Next = MemTrack[t];
			MemTrack[t] = track;
			return track;
		}
		return NULL;
	}
	inline FMemTrackMark* FindMemTracker(void* ptr, FMemTrackPool* AddList)
	{
		DWORD t = ((DWORD)ptr) & MEMTRACK_HASHSIZE;

		if (MemPoolTrack[t])
		{
			for (FMemTrackMark* tr = MemPoolTrack[t]; tr; tr = tr->Next)
			{
				if (tr->ptr==ptr)
					return tr;
			}
		}
		if (AddList)
		{
			FMemTrackMark* track = (FMemTrackMark*)malloc(sizeof(FMemTrackMark));
			track->Pool = AddList;
			track->ptr = ptr;
			track->AllocNext = AddList->List;
			AddList->List = track;
			track->Index = t;

			track->Next = MemPoolTrack[t];
			MemPoolTrack[t] = track;
			return track;
		}
		return NULL;
	}
	inline void FreeTrack(void* ptr)
	{
		if (GIsRequestingExit) return;
		FMemTrackMark* m = FindMemTracker(ptr, NULL);
		if (m)
		{
			// Unlist mem mark
			if (MemPoolTrack[m->Index] == m)
				MemPoolTrack[m->Index] = m->Next;
			else
			{
				for (FMemTrackMark* tr = MemPoolTrack[m->Index]; tr; tr = tr->Next)
				{
					if (tr->Next == m)
					{
						tr->Next = m->Next;
						break;
					}
				}
			}

			// Unlist mem pool ref.
			if (m->Pool->List == m)
				m->Pool->List = m->AllocNext;
			else
			{
				for (FMemTrackMark* tr = m->Pool->List; tr; tr = tr->AllocNext)
				{
					if (tr->AllocNext == m)
					{
						tr->AllocNext = m->AllocNext;
						break;
					}
				}
			}

			// Unlist mem tag if empty!
			if (!m->Pool->List)
			{
				FMemTrackPool* tp = m->Pool;
				if (MemTrack[tp->Index] == tp)
					MemTrack[tp->Index] = tp->Next;
				else
				{
					for (FMemTrackPool* tr = MemTrack[tp->Index]; tr; tr = tr->Next)
					{
						if (tr->Next == tp)
						{
							tr->Next = tp->Next;
							break;
						}
					}
				}
				free(tp);
			}
			free(m);
		}
	}
	inline void AddTrack(const TCHAR* Tag, void* ptr, size_t s)
	{
		if (GIsRequestingExit) return;

		if (!Tag)
			Tag = TEXT("NULL");
		DWORD t = ((DWORD)(ptr)) & MEMTRACK_HASHSIZE;
		FMemTrackPool* track = FindTagTracker(Tag, 1);
		FindMemTracker(ptr, track)->size = s;
	}

	struct FOutTrackInfo
	{
		FMemTrackPool* Pool;
		INT size;
	};
	static QSORT_RETURN TrackSorts(const FOutTrackInfo* A, const FOutTrackInfo* B)
	{
		return (B->size - A->size);
	}
	inline void TrackOutputStats()
	{
		DWORD t;
		FMemTrackPool* p;
		FMemTrackMark* m;
		size_t s;
		DWORD ix = 0;
		DWORD Count = 0;

		for (t = 0; t < (MEMTRACK_HASHSIZE + 1); ++t)
		{
			if (!MemTrack[t])
				continue;
			for (p = MemTrack[t]; p; p = p->Next)
			{
				++Count;
			}
		}
		FOutTrackInfo* List = (FOutTrackInfo*)malloc(sizeof(FOutTrackInfo) * Count);
		for (t = 0; t < (MEMTRACK_HASHSIZE + 1); ++t)
		{
			if (!MemTrack[t])
				continue;
			for (p = MemTrack[t]; p; p = p->Next)
			{
				size_t s = 0;
				for (m = p->List; m; m = m->AllocNext)
					s += m->size;
				List[ix].Pool = p;
				List[ix].size = (INT)s;
				++ix;
			}
		}
		appQsort(List, Count, sizeof(FOutTrackInfo), (QSORT_COMPARE)TrackSorts);
		for (ix = 0; ix < Count; ++ix)
		{
			if (List[ix].size <= 300)
				break;
			debugf(TEXT("%i \t %ls"), List[ix].size, List[ix].Pool->MemTag);
		}
		INT RemainSize = 0;
		INT RemainCount = (Count-ix);
		for (; ix < Count; ++ix)
		{
			RemainSize += List[ix].size;
		}
		debugf(TEXT("%i items with less then 300b of size taking up: %i bytes"), RemainCount, RemainSize);
		free(List);
	}
#endif

	// Implementation.
	void OutOfMemory(FString Message)
	{
		guardSlow(OutOfMemory);
		GWarn->Logf(TEXT("Out of Memory occured in %ls with (%#x)"), *Message, GetLastError());
		appErrorf(*LocalizeError(TEXT("OutOfMemory"), TEXT("Core")));
		unguardSlow;
	}
	FPoolInfo* CreateIndirect()
	{
		guardSlow(FMallocWindows::CreateIndirect);
		FPoolInfo* Indirect = (FPoolInfo*)VirtualAlloc(NULL, 256 * sizeof(FPoolInfo), MEM_COMMIT, PAGE_READWRITE);
		if (!Indirect)
			OutOfMemory(TEXT("FPoolInfo* CreateIndirect()"));
		return Indirect;
		unguardSlow;
	}

	inline void* MallocInner(size_t Size, const TCHAR* Tag)
	{
		guardSlow(MallocInner);
		checkSlow(Size > 0);
		checkSlow(MemInit);
		STAT(++GMemStats.AllocCount.Count);

		FFreeMem* Free;
		if (Size < POOL_MAX)
		{
			// Allocate from pool.
			FPoolTable* Table = MemSizeToPoolTable[Size];
			checkSlow(Size <= Table->BlockSize);
			FPoolInfo* Pool = Table->FirstPool;
			if (!Pool)
			{
				// Must create a new pool.
				size_t Blocks = 65536 / Table->BlockSize;
				size_t Bytes = Blocks * Table->BlockSize;
				checkSlow(Blocks >= 1);
				checkSlow(Blocks * Table->BlockSize <= Bytes);

				// Allocate memory.
				Free = (FFreeMem*)VirtualAlloc(NULL, Bytes, MEM_COMMIT, PAGE_READWRITE);
				if (!Free)
					OutOfMemory(TEXT("void* Malloc(size_t Size, const TCHAR* Tag), Size<POOL_MAX"));

				// Create pool in the indirect table.
				FPoolInfo*& Indirect = PoolIndirect[((size_t)Free >> 24)];
				if (!Indirect)
					Indirect = CreateIndirect();
				Pool = &Indirect[((size_t)Free >> 16) & 255];

				// Init pool.
				Pool->Link(Table->FirstPool);
				Pool->Mem = (BYTE*)Free;
				Pool->Bytes = Bytes;
				Pool->OsBytes = Align(Bytes, GPageSize);
				STAT(OsPeak = Max(OsPeak, OsCurrent += Pool->OsBytes));
				Pool->Table = Table;
				Pool->Taken = 0;
				Pool->FirstMem = Free;

				// Create first free item.
				Free->Blocks = Blocks;
				Free->Next = NULL;
			}

			// Pick first available block and unlink it.
			Pool->Taken++;
			checkSlow(Pool->FirstMem);
			checkSlow(Pool->FirstMem->Blocks > 0);
			Free = (FFreeMem*)((BYTE*)Pool->FirstMem + --Pool->FirstMem->Blocks * Table->BlockSize);
			if (Pool->FirstMem->Blocks == 0)
			{
				Pool->FirstMem = Pool->FirstMem->Next;
				if (!Pool->FirstMem)
				{
					// Move to exausted list.
					Pool->Unlink();
					Pool->Link(Table->ExaustedPool);
				}
			}
			STAT(UsedPeak = Max(UsedPeak, UsedCurrent += Table->BlockSize));
		}
		else
		{
			// Use OS for large allocations.
			size_t AlignedSize = Align(Size, GPageSize);
			Free = (FFreeMem*)VirtualAlloc(NULL, AlignedSize, MEM_COMMIT, PAGE_READWRITE);
			if (!Free)
				OutOfMemory(TEXT("void* Malloc( size_t Size, const TCHAR* Tag )"));
			checkSlow(!((size_t)Free & 65535));

			// Create indirect.
			FPoolInfo*& Indirect = PoolIndirect[((size_t)Free >> 24)];
			if (!Indirect)
				Indirect = CreateIndirect();

			// Init pool.
			FPoolInfo* Pool = &Indirect[((size_t)Free >> 16) & 255];
			Pool->Mem = (BYTE*)Free;
			Pool->Bytes = Size;
			Pool->OsBytes = AlignedSize;
			Pool->Table = &OsTable;
			STAT(OsPeak = Max(OsPeak, OsCurrent += AlignedSize));
			STAT(UsedPeak = Max(UsedPeak, UsedCurrent += Size));
		}
#ifdef TRACK_MEMORYUSE
		AddTrack(Tag, Free, Size);
#endif
		return Free;
		unguardSlow;
	}
	inline void FreeInner(void* Ptr)
	{
		guardSlow(FreeInner);
		STAT(--GMemStats.AllocCount.Count);

#ifdef TRACK_MEMORYUSE
		FreeTrack(Ptr);
#endif

		// Windows version.
		FPoolInfo* Pool = NULL;
		// FMallocWindows crashes on deallocating non allocs.
		if (PoolIndirect[(size_t)Ptr >> 24] == NULL)
		{
			GWarn->Logf(TEXT("PoolIndirect[(size_t)Ptr>>24]==NULL"));
			return;
		}
		Pool = &PoolIndirect[(size_t)Ptr >> 24][((size_t)Ptr >> 16) & 255];

		if (Pool->Table != &OsTable)
		{
			// If this pool was exausted, move to available list.
			if (!Pool->FirstMem)
			{
				Pool->Unlink();
				Pool->Link(Pool->Table->FirstPool);
			}

			// Free a pooled allocation.
			FFreeMem* Free = (FFreeMem*)Ptr;
			Free->Blocks = 1;
			Free->Next = Pool->FirstMem;
			Pool->FirstMem = Free;
			STAT(UsedCurrent -= Pool->Table->BlockSize);

			// Free this pool.
			checkSlow(Pool->Taken >= 1);
			if (--Pool->Taken == 0)
			{
				// Free the OS memory.
				Pool->Unlink();
				verify(VirtualFree(Pool->Mem, 0, MEM_RELEASE) != 0);
				STAT(OsCurrent -= Pool->OsBytes);
			}
		}
		else
		{
			// Free an OS allocation.
			checkSlow(!((INT)Ptr & 65535));
			STAT(UsedCurrent -= Pool->Bytes);
			STAT(OsCurrent -= Pool->OsBytes);
			verify(VirtualFree(Ptr, 0, MEM_RELEASE) != 0);
		}
		unguardfSlow((TEXT("(Ptr=0x%08x)"), Ptr));
	}

public:
	// FMalloc interface.
	FMallocWindows()
		: MemInit(0)
		, OsCurrent(0)
		, OsPeak(0)
		, UsedCurrent(0)
		, UsedPeak(0)
		, CurrentAllocs(0)
		, TotalAllocs(0)
		, MemTime(0.0)
	{}
	void* Malloc(size_t Size, const TCHAR* Tag)
	{
		guard(FMallocWindows::Malloc);
		FScopeThread Scope(MemMutex);
		STAT(++GMemStats.AllocCounter.Calls);
		STAT(FStatTimerScope MScope(GMemStats.AllocTime));
		return MallocInner(Size,Tag);
		unguard;
	}
	void* Realloc(void* Ptr, size_t NewSize, const TCHAR* Tag)
	{
		guard(FMallocWindows::Realloc);
		FScopeThread Scope(MemMutex);
		STAT(++GMemStats.AllocCounter.Calls);
		STAT(FStatTimerScope MScope(GMemStats.AllocTime));
		checkSlow(MemInit);
		check(NewSize >= 0);
		void* NewPtr = Ptr;
		if (Ptr && NewSize)
		{
			checkSlow(MemInit);
			FPoolInfo* Pool = &PoolIndirect[(size_t)Ptr >> 24][((size_t)Ptr >> 16) & 255];
			if (Pool->Table != &OsTable)
			{
				// Allocated from pool, so grow or shrink if necessary.
				if (NewSize>Pool->Table->BlockSize || MemSizeToPoolTable[NewSize] != Pool->Table)
				{
					NewPtr = MallocInner(NewSize, Tag);
					appMemcpy(NewPtr, Ptr, Min(NewSize, Pool->Table->BlockSize));
					FreeInner(Ptr);
				}
			}
			else
			{
				// Allocated from OS.
				checkSlow(!((size_t)Ptr & 65535));
				if (NewSize>Pool->OsBytes || NewSize * 3<Pool->OsBytes * 2)
				{
					// Grow or shrink.
					NewPtr = MallocInner(NewSize, Tag);
					appMemcpy(NewPtr, Ptr, Min(NewSize, Pool->Bytes));
					FreeInner(Ptr);
				}
				else
				{
					// Keep as-is, reallocation isn't worth the overhead.
					Pool->Bytes = NewSize;
				}
			}
		}
		else if (NewSize)
		{
			NewPtr = MallocInner(NewSize, Tag);
		}
		else
		{
			if (Ptr)
				FreeInner(Ptr);
			NewPtr = NULL;
		}
		return NewPtr;
		unguardf((TEXT("%08X %i %ls"), (size_t)Ptr, NewSize, Tag));
	}
	void Free(void* Ptr)
	{
		guard(FMallocWindows::Free);
		if (Ptr)
		{
			FScopeThread Scope(MemMutex);
			STAT(++GMemStats.AllocCounter.Calls);
			STAT(FStatTimerScope MScope(GMemStats.AllocTime));
			FreeInner(Ptr);
		}
		unguardf((TEXT("(Ptr=0x%08x)"), Ptr));
	}
	void DumpAllocs()
	{
		guard(FMallocWindows::DumpAllocs);
		//FScopeThread Scope(MemMutex); <- TODO Marco: why does this crash UnrealEd on exit?
		FMallocWindows::HeapCheck();

		STAT(debugf(TEXT("Memory Allocation Status")));
		STAT(debugf(TEXT("Curr Memory % 5.3fM / % 5.3fM"), UsedCurrent / 1024.0 / 1024.0, OsCurrent / 1024.0 / 1024.0));
		STAT(debugf(TEXT("Peak Memory % 5.3fM / % 5.3fM"), UsedPeak / 1024.0 / 1024.0, OsPeak / 1024.0 / 1024.0));
		STAT(debugf(TEXT("Allocs      % 6i Current / % 6i Total"), CurrentAllocs, TotalAllocs));
		MEM_TIME(debugf("Seconds     % 5.3f", MemTime));
		MEM_TIME(debugf("MSec/Allc   % 5.5f", 1000.0 * MemTime / MemAllocs));

#ifdef TRACK_MEMORYUSE
		if (!GIsRequestingExit)
		{
			debugf(TEXT("Pool sizes: (bytes - tag)====================================="));
			TrackOutputStats();
			debugf(TEXT("=============================================================="));
		}
#endif

#if STATS
		if (ParseParam(appCmdLine(), TEXT("MEMSTAT")))
		{
			debugf(TEXT("Block Size Num Pools Cur Allocs Total Allocs Mem Used Mem Waste Efficiency"));
			debugf(TEXT("---------- --------- ---------- ------------ -------- --------- ----------"));
			size_t TotalPoolCount = 0;
			size_t TotalAllocCount = 0;
			size_t TotalMemUsed = 0;
			size_t TotalMemWaste = 0;
			for (INT i = 0; i<POOL_COUNT; i++)
			{
				FPoolTable* Table = &PoolTable[i];
				INT PoolCount = 0;
				size_t AllocCount = 0;
				size_t MemUsed = 0;
				for (INT j = 0; j<2; j++)
				{
					for (FPoolInfo* Pool = (j ? Table->FirstPool : Table->ExaustedPool); Pool; Pool = Pool->Next)
					{
						PoolCount++;
						AllocCount += Pool->Taken;
						MemUsed += Pool->Bytes;
						size_t FreeCount = 0;
						for (FFreeMem* Free = Pool->FirstMem; Free; Free = Free->Next)
							FreeCount += Free->Blocks;
					}
				}
				size_t MemWaste = MemUsed - AllocCount*Table->BlockSize;
				debugf
					(
					TEXT("% 10i % 9i % 10i % 11iK % 7iK % 8iK % 9.2f%%"),
					Table->BlockSize,
					PoolCount,
					AllocCount,
					0,
					MemUsed / 1024,
					MemWaste / 1024,
					MemUsed ? 100.0 * MemUsed / (MemUsed + MemWaste) : 100.0
					);
				TotalPoolCount += PoolCount;
				TotalAllocCount += AllocCount;
				TotalMemUsed += MemUsed;
				TotalMemWaste += MemWaste;
			}
			debugf
				(
				TEXT("BlkOverall % 9i % 10i % 11iK % 7iK % 8iK % 9.2f%%"),
				TotalPoolCount,
				TotalAllocCount,
				0,
				TotalMemUsed / 1024,
				TotalMemWaste / 1024,
				TotalMemUsed ? 100.0 * TotalMemUsed / (TotalMemUsed + TotalMemWaste) : 100.0
				);
		}
#endif
		unguard;
	}
	void HeapCheck()
	{
		guard(FMallocWindows::HeapCheck);
		for (INT i = 0; i<POOL_COUNT; i++)
		{
			FPoolTable* Table = &PoolTable[i];
			FPoolInfo** PoolPtr;
			for (PoolPtr = &Table->FirstPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next)
			{
				FPoolInfo* Pool = *PoolPtr;
				check(Pool->PrevLink == PoolPtr);
				check(Pool->FirstMem);
				for (FFreeMem* Free = Pool->FirstMem; Free; Free = Free->Next)
					check(Free->Blocks>0);
			}
			for (PoolPtr = &Table->ExaustedPool; *PoolPtr; PoolPtr = &(*PoolPtr)->Next)
			{
				FPoolInfo* Pool = *PoolPtr;
				check(Pool->PrevLink == PoolPtr);
				check(!Pool->FirstMem);
			}
		}
		unguard;
	}
	void Init()
	{
		guard(FMallocWindows::Init);
		check(!MemInit);
		MemInit = 1;

#ifdef TRACK_MEMORYUSE
		appMemzero(MemTrack, sizeof(FMemTrackPool*) * (MEMTRACK_HASHSIZE + 1));
		appMemzero(MemPoolTrack, sizeof(FMemTrackMark*) * (MEMTRACK_HASHSIZE + 1));
#endif

		// Get OS page size.
		SYSTEM_INFO SI;
		GetSystemInfo(&SI);
		GPageSize = SI.dwPageSize;
		check(!(GPageSize&(GPageSize - 1)));

		// Init tables.
		OsTable.FirstPool = NULL;
		OsTable.ExaustedPool = NULL;
		OsTable.BlockSize = 0;
		DWORD i;
		for (i = 0; i<POOL_COUNT; i++)
		{
			PoolTable[i].FirstPool = NULL;
			PoolTable[i].ExaustedPool = NULL;
			PoolTable[i].BlockSize = (4 + (i & 3)) << (1 + (i >> 2));
		}
		for (i = 0; i<POOL_MAX; i++)
		{
			DWORD Index;
			for (Index = 0; PoolTable[Index].BlockSize<i; Index++);
			checkSlow(Index<POOL_COUNT);
			MemSizeToPoolTable[i] = &PoolTable[Index];
		}
		for (i = 0; i<256; i++)
		{
			PoolIndirect[i] = NULL;
		}
		check(POOL_MAX - 1 == PoolTable[POOL_COUNT - 1].BlockSize);
		unguard;
	}
	void Exit()
	{}
};

/*-----------------------------------------------------------------------------
The End.
-----------------------------------------------------------------------------*/
