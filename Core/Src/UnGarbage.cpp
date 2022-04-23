/*=============================================================================
UnGarbage.cpp: Unreal garbage collector.
Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
* Created by Marco
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
Garbage collection.
-----------------------------------------------------------------------------*/

INT GGarbageRefCount = 0;

//
// Serialize the global root set to an archive.
//
void UObject::SerializeRootSet(FArchive& Ar, DWORD KeepFlags, DWORD RequiredFlags)
{
	guard(UObject::SerializeRootSet);
	Ar << GObjRoot;
	for (FObjectIterator It; It; ++It)
	{
		if
			((It->GetFlags() & KeepFlags)
			&& (It->GetFlags()&RequiredFlags) == RequiredFlags)
		{
			UObject* Obj = *It;
			Ar << Obj;
		}
	}
	unguard;
}

//
// Archive for finding unused objects.
//
#define USE_NON_RECURSIVE_GC
class FArchiveTagUsed : public FArchive
{
public:
	FArchiveTagUsed()
		: Context(NULL)
	{
		guard(FArchiveTagUsed::FArchiveTagUsed);
		GGarbageRefCount = 0;

		// Tag all objects as unreachable.
		for (FObjectIterator It; It; ++It)
			It->SetFlags(RF_Unreachable | RF_TagGarbage);

		// Tag all names as unreachable.
		for (INT i = 0; i<FName::GetMaxNames(); i++)
			if (FName::GetEntry(i))
				FName::GetEntry(i)->Flags |= RF_Unreachable;

		unguard;
	}
#ifdef USE_NON_RECURSIVE_GC
	TArray<UObject*> ObjectList;
	void CollectObjects()
	{
		guard(FArchiveTagUsed::CollectObjects);
		while (ObjectList.Num())
		{
			UObject* Obj = ObjectList.Pop();
			Obj->Serialize(*this);
		}
		unguard;
	}
#endif
private:
	FArchive& operator<<(UObject*& Object)
	{
		guardSlow(FArchiveTagUsed << Obj);
		GGarbageRefCount++;

		// Object could be a misaligned pointer.
		// Copy the contents of the pointer into a temporary and work on that.
		UObject* Obj;
		appMemcpy(&Obj, &Object, sizeof(UObject*));

#if DO_GUARD_SLOW
		guard(CheckValid);
		if (Obj)
			check(Obj->IsValid());
		unguard;
#endif
		if (Obj && (Obj->GetFlags() & RF_Unreachable))
		{
			// Only recurse the first time object is claimed.
			guard(TestReach);
			Obj->ClearFlags(RF_Unreachable);

			// Recurse.
			if (Obj->GetFlags() & RF_TagGarbage)
			{
#ifdef USE_NON_RECURSIVE_GC
				ObjectList.AddItem(Obj);
#else
				// Recurse down the object graph.
				UObject* OriginalContext = Context;
				Context = Obj;
				Obj->Serialize(*this);
				Context = OriginalContext;
#endif
			}
			else
			{
				// For debugging.
				debugfSlow(NAME_Log, TEXT("%ls is referenced by %ls"), Obj->GetFullName(), Context ? Context->GetFullName() : NULL);
			}
			unguardf((TEXT("(%ls)"), Obj->GetFullName()));
		}

		// Contents of pointer might have been modified.
		// Copy the results back into the misaligned pointer.
		appMemcpy(&Object, &Obj, sizeof(UObject*));

		return *this;
		unguardSlow;
	}
	FArchive& operator<<(FName& Name)
	{
		guardSlow(FArchiveTagUsed::Name);

		Name.ClearFlags(RF_Unreachable);

		return *this;
		unguardSlow;
	}
	UObject* Context;
};


//
// Purge garbage.
//
void UObject::PurgeGarbage()
{
	guard(UObject::PurgeGarbage);
	GIsCollectingGarbage = 1; //some destroy functions shouldn't be called during this process and possibly crash, so avoid it. Smirftsch
	INT CountBefore = 0, CountPurged = 0;
	if (GNoGC)
	{
		debugf(NAME_DevGarbage, TEXT("Not purging garbage"));
		GIsCollectingGarbage = 0;
		return;
	}
	debugf(NAME_DevGarbage, TEXT("Purging garbage"));

	// Dispatch all Destroy messages.
	guard(DispatchDestroys);
	for (INT i = 0; i<GObjObjects.Num(); i++)
	{
		guard(DispatchDestroy);
		CountBefore += (GObjObjects(i) != NULL);
		if
			(GObjObjects(i)
			&& (GObjObjects(i)->GetFlags() & RF_Unreachable)
			&& (!(GObjObjects(i)->GetFlags() & RF_Native) || GExitPurge))
		{
			debugfSlow(NAME_DevGarbage, TEXT("Garbage collected object %i: %ls"), i, GObjObjects(i)->GetFullName());
			GObjObjects(i)->ConditionalDestroy();
			CountPurged++;
		}
		unguardf((TEXT("(%i: %ls)"), i, GObjObjects(i)->GetFullName()));
	}

	unguard;

	// Purge all unreachable objects.
	//warning: Can't use FObjectIterator here because classes may be destroyed before objects.
	FName DeleteName = NAME_None;
	INT i;
	guard(DeleteGarbage);
	for (i = 0; i<GObjObjects.Num(); i++)
	{
		DeleteName = NAME_Invalid;
		if
			(GObjObjects(i)
			&& (GObjObjects(i)->GetFlags() & RF_Unreachable)
			&& !(GObjObjects(i)->GetFlags() & RF_Native))
		{
			DeleteName = GObjObjects(i)->GetFName();
			delete GObjObjects(i);
		}
	}
	unguardf((TEXT("(%ls, %i)"), *DeleteName, i));

	// Purge all unreachable names.
	guard(Names);
	if (!GExitPurge)
	{
		for (INT i = 0; i < FName::GetMaxNames(); i++)
		{
			FNameEntry* Name = FName::GetEntry(i);
			if
				((Name)
					&& (Name->Flags & RF_Unreachable)
					&& !(Name->Flags & RF_Native))
			{
				debugfSlow(NAME_DevGarbage, TEXT("Garbage collected name %i: %ls"), i, Name->Name);
				FName::DeleteEntry(i);
			}
		}
	}
	unguard;

	debugf(NAME_DevGarbage, TEXT("Garbage: objects: %i->%i; refs: %i"), CountBefore, CountBefore - CountPurged, GGarbageRefCount);
	GIsCollectingGarbage = 0;
	unguard;
}

//
// Delete all unreferenced objects.
//
void UObject::CollectGarbage(DWORD KeepFlags)
{
	guard(UObject::CollectGarbage);
	debugf(NAME_DevGarbage, TEXT("Collecting garbage"));

	// Tag and purge garbage.
	FArchiveTagUsed TagUsedAr;
	SerializeRootSet(TagUsedAr, KeepFlags, RF_TagGarbage);

#ifdef USE_NON_RECURSIVE_GC
	TagUsedAr.CollectObjects();
#endif

	// Purge it.
	PurgeGarbage();

	unguard;
}

//
// Returns whether an object is referenced, not counting the
// one reference at Obj. No side effects.
//

UBOOL UObject::IsReferenced(UObject*& Obj, DWORD KeepFlags, UBOOL IgnoreReference)
{
	guard(UObject::RefCount);

	// Remember it.
	UObject* OriginalObj = Obj;
	if (IgnoreReference)
		Obj = NULL;

	// Tag all garbage.
	FArchiveTagUsed TagUsedAr;
	OriginalObj->ClearFlags(RF_TagGarbage);
	SerializeRootSet(TagUsedAr, KeepFlags, RF_TagGarbage);

#ifdef USE_NON_RECURSIVE_GC
	TagUsedAr.CollectObjects();
#endif

	// Stick the reference back.
	Obj = OriginalObj;

	// Return whether this is tagged.
	return (Obj->GetFlags() & RF_Unreachable) == 0;
	unguard;
}
