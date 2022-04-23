/*=============================================================================
	UnCoreNet.cpp: Core networking support.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FPackageInfo implementation.
-----------------------------------------------------------------------------*/

//
// FPackageInfo constructor.
//
FPackageInfo::FPackageInfo( ULinkerLoad* InLinker )
:	URL				()
,   Linker			( InLinker )
,	Parent			( InLinker ? InLinker->LinkerRoot : NULL )
,	Guid			( InLinker ? InLinker->Summary.Guid : FGuid(0,0,0,0) )
,	FileSize		( InLinker ? InLinker->Loader->TotalSize() : 0 )
,	ObjectBase		( INDEX_NONE )
,	ObjectCount		( INDEX_NONE )
,	NameBase		( INDEX_NONE )
,	NameCount		( INDEX_NONE )
,	LocalGeneration	( 0 )
,	RemoteGeneration( 0 )
,	RemoteNameCount	( 0 )
,	RemoteObjectCount( 0 )
,	PackageFlags	( InLinker ? InLinker->Summary.PackageFlags : 0 )
{
	guard(FPackageInfo::FPackageInfo);
	if( InLinker && *InLinker->Filename && (InLinker->Summary.PackageFlags & PKG_AllowDownload) )
		URL = *InLinker->Filename;
	unguard;
}

#ifdef UTPG_MD5
//elmuerte: MD5 checksum
FString FPackageInfo::ZQ5Purpxfhz()
{
  guard(FPackageInfo::ZQ5Purpxfhz);
  if (Linker) return Linker->Summary.ZQ5Purpxfhz;
  else return TEXT("");
  unguard;
}

FString FPackageInfo::GetFilename()
{
  guard(FPackageInfo::GetFilename);
  if (Linker)
  {
    return Linker->ShortFilename;
  }
  else return TEXT("");
  unguard;
}

void FPackageInfo::SetCheck(UBOOL val)
{
	guard(FPackageInfo::SetCheck);
  if (Linker) Linker->Summary.TableVerified = val;
  unguard;
}

FString FPackageInfo::GetLanguage()
{
	guard(FPackageInfo::GetFilename);
  if (Linker)
  {
    return Linker->FileLang;
  }
  else return TEXT("");
  unguard;
}
#endif

//
// FPackageInfo serializer.
//
CORE_API FArchive& operator<<( FArchive& Ar, FPackageInfo& I )
{
	guard(FPackageInfo<<);
	return Ar << I.Parent << I.Linker;
	unguard;
}

/*-----------------------------------------------------------------------------
	FFieldNetCache implementation.
-----------------------------------------------------------------------------*/

CORE_API FArchive& operator<<( FArchive& Ar, FFieldNetCache& F )
{
	return Ar << F.Field;
}

/*-----------------------------------------------------------------------------
	FClassNetCache implementation.
-----------------------------------------------------------------------------*/

FClassNetCache::FClassNetCache()
{}
FClassNetCache::FClassNetCache( UClass* InClass )
: Class( InClass )
{}
INT FClassNetCache::GetMaxIndex()
{
	guard(FClassNetCache::GetMaxIndex);
	return Fields.Num();
	unguard;
}
INT FClassNetCache::GetMaxPropertyIndex()
{
	guard(FClassNetCache::GetMaxIndex);
	return MaxPropertyIndex;
	unguard;
}
INT FClassNetCache::GetRepValueCount()
{
	guardSlow(FClassNetCache::GetRepValueCount);
	return RepValueCount;
	unguardSlow;
}
INT FClassNetCache::GetRepConditionCount()
{
	guardSlow(FClassNetCache::GetRepConditionCount);
	return RepConditionCount;
	unguardSlow;
}
FFieldNetCache* FClassNetCache::GetFromField( UField* Field )
{
	guard(FClassNetCache::GetFieldIndex);
	INT* Result = FieldIndices.Find(Field);
	return Result ? &Fields(*Result) : NULL;
	unguard;
}
FFieldNetCache* FClassNetCache::GetFromIndex( INT Index )
{
	guard(FClassNetCache::GetFromIndex);
	return Fields.IsValidIndex(Index) ? &Fields(Index) : NULL;
	unguard;
}
CORE_API FArchive& operator<<( FArchive& Ar, FClassNetCache& Cache )
{
	guard(FClassNetCache<<);
	return Ar << Cache.Class << Cache.Fields << Cache.FieldIndices;
	unguard;
}

/*-----------------------------------------------------------------------------
	UPackageMap implementation.
-----------------------------------------------------------------------------*/

//
// General.
//
void UPackageMap::Copy( UPackageMap* Other )
{
	guard(UPackageMap::Copy);
	List              = Other->List;
	LinkerMap         = Other->LinkerMap;
	ClassFieldIndices = Other->ClassFieldIndices;
	NameIndices       = Other->NameIndices;
	MaxObjectIndex    = Other->MaxObjectIndex;
	MaxNameIndex      = Other->MaxNameIndex;
	unguard;
}
UBOOL UPackageMap::SerializeName( FArchive& Ar, FName& Name )
{
	guard(UPackageMap::SerializeName);
	DWORD Index = Name.GetIndex()<NameIndices.Num() ? NameIndices(Name.GetIndex()) : MaxNameIndex;
	Ar.SerializeInt( Index, MaxNameIndex+1 );
	if( Ar.IsLoading() )
	{
		Name = NAME_None;
		if( Index<MaxNameIndex && !Ar.IsError() )
		{
			for( INT i=0; i<List.Num(); i++ )
			{
				FPackageInfo& Info = List(i);
				if( Index < (DWORD)Info.NameCount )
				{
					Name = Info.Linker->NameMap(Index);
					break;
				}
				Index -= Info.NameCount;
			}
		}
		return 1;
	}
	else return Index!=MaxNameIndex;
	unguard;
}
UBOOL UPackageMap::CanSerializeObject( UObject* Obj )
{
	guard(UPackageMap::SerializeObject);
	appErrorf(TEXT("Unexpected UPackageMap::CanSerializeObject"));
	return 1;
	unguard;
}
UBOOL UPackageMap::SerializeObject( FArchive& Ar, UClass* Class, UObject*& Obj )
{
	guard(UPackageMap::SerializeObject);
	appErrorf(TEXT("Unexpected UPackageMap::SerializeObject"));
	return 1;
	unguard;
}

//
// Get a package map's net cache for a class.
//
FClassNetCache* UPackageMap::GetClassNetCache( UClass* Class )
{
	guard(UPackageMap::GetClassNetCache);
	FClassNetCache* Result;
	Result = ClassFieldIndices.Find(Class);
	if( !Result && ObjectToIndex(Class)!=INDEX_NONE )
	{
		Result                    = &ClassFieldIndices.Set( Class, FClassNetCache(Class) );
		Result->RepValueCount     = 0;
		Result->RepConditionCount = 0;
		Result->Fields.Empty( Class->NetFields.Num() );
		{for( INT i=0; i<Class->NetFields.Num(); i++ )
		{
			// Add sandboxed items to net cache.
			UField* Field = Class->NetFields(i);
			if( ObjectToIndex(Field)!=INDEX_NONE )
			{
				UProperty* ItP;
				UFunction* ItF;
				Result->FieldIndices.Set( Field, i );
				if( (ItP=Cast<UProperty>(Field))!=NULL )
				{
					INT ConditionIndex = INDEX_NONE;
					if( ItP->RepOwner==ItP || !ObjectToIndex(ItP->RepOwner) )
						ConditionIndex = Result->RepConditionCount++;
					new(Result->Fields)FFieldNetCache( ItP, i, Result->RepValueCount, ConditionIndex );
					Result->RepValueCount += ItP->ArrayDim;
					Result->MaxPropertyIndex = Result->Fields.Num();
				}
				else if( (ItF=Cast<UFunction>(Field))!=NULL )
				{
					new(Result->Fields)FFieldNetCache( ItF, i, INDEX_NONE, INDEX_NONE );
				}
				else GWarn->Logf(TEXT("Invalid NetField %ls"),Field->GetPathName());
			}
		}}
		Result->Fields.Shrink();
		{for( INT i=0; i<Result->Fields.Num(); i++ )
		{
			// Set condition indices.
			FFieldNetCache& Cache = Result->Fields(i);
			UProperty* P;
			if( (P=Cast<UProperty>(Cache.Field))!=NULL )
			{
				if( Cache.ConditionIndex==INDEX_NONE )
					Cache.ConditionIndex = Result->Fields(*Result->FieldIndices.Find(P->RepOwner)).ConditionIndex;
				Cache.CacheType = ((P->GetOwnerClass()->ClassFlags & CLASS_NativeReplication) ? FNC_NativeProperty : FNC_Property);
			}
			else if( Cast<UFunction>(Cache.Field) )
				Cache.CacheType = FNC_Function;
			else Cache.CacheType = FNC_Invalid;
		}}
	}
	return Result;
	unguard;
}

//
// Add a linker to this linker map.
//
INT UPackageMap::AddLinker( ULinkerLoad* Linker )
{
	guard(UPackageMap::AddLinker);

	// Skip if server only.
	if( Linker->Summary.PackageFlags & PKG_ServerSideOnly )
		return INDEX_NONE;

	// Skip if already on list.
	{for( INT i=0; i<List.Num(); i++ )
		if( List(i).Parent == Linker->LinkerRoot )
			return i;}

	// Add to list.
	INT Index = List.Num();
	new(List)FPackageInfo( Linker );

	// Recurse.
	{for( INT i=0; i<Linker->ImportMap.Num(); i++ )
		if( Linker->ImportMap(i).ClassName==NAME_Package && Linker->ImportMap(i).PackageIndex==0 )
			for( INT j=0; j<UObject::GObjLoaders.Num(); j++ )
				if( UObject::GetLoader(j)->LinkerRoot->GetFName()==Linker->ImportMap(i).ObjectName )
					AddLinker( UObject::GetLoader(j) );}

	return Index;
	unguard;
}

//
// Compute mapping info.
//
void UPackageMap::Compute( UBOOL bOldClientHack )
{
	guard(UPackageMap::Compute);
	{for( INT i=0; i<List.Num(); i++ )
		check(List(i).Linker);}
	NameIndices.Empty( FName::GetMaxNames() );
	NameIndices.Add( FName::GetMaxNames() );
	{for( INT i=0; i<NameIndices.Num(); i++ )
		NameIndices(i) = -1;}
	LinkerMap.Empty();
	MaxObjectIndex = 0;
	MaxNameIndex   = 0;
	{for( INT i=0; i<List.Num(); i++ )
	{
		FPackageInfo& Info    = List(i);
		Info.ObjectBase       = MaxObjectIndex;
		Info.NameBase         = MaxNameIndex;
		Info.ObjectCount      = Info.Linker->ExportMap.Num();
		Info.NameCount        = Info.Linker->NameMap.Num();
		Info.LocalGeneration  = Info.Linker->Summary.Generations.Num();
		if( Info.RemoteGeneration==0 )
			Info.RemoteGeneration = Info.LocalGeneration;
		if( bOldClientHack && Info.RemoteGeneration==7 ) // Fake use 225f version.
			Info.RemoteGeneration = 6;
		if( Info.RemoteGeneration<Info.LocalGeneration )
		{
			Info.ObjectCount  = Min( Info.ObjectCount, Info.Linker->Summary.Generations(Info.RemoteGeneration-1).ExportCount );
			Info.NameCount    = Min( Info.NameCount,   Info.Linker->Summary.Generations(Info.RemoteGeneration-1).NameCount   );
		}
		Info.LocalObjectCount = Info.ObjectCount;
		Info.LocalNameCount = Info.NameCount;
		if( Info.RemoteNameCount )
			Info.NameCount = Min(Info.RemoteNameCount,Info.NameCount);
		if( Info.RemoteObjectCount )
			Info.ObjectCount = Min(Info.RemoteObjectCount,Info.ObjectCount);

		MaxObjectIndex       += Info.ObjectCount;
		MaxNameIndex         += Info.NameCount;
		//debugf( TEXT("** Package %ls: %i objs, %i names, gen %i-%i"), *Info.URL. Info.ObjectCount, Info.NameCount, Info.LocalGeneration, Info.RemoveGeneration );
		for( INT j=0; j<Min(Info.Linker->NameMap.Num(),Info.NameCount); j++ )
			if( NameIndices(Info.Linker->NameMap(j).GetIndex()) == -1 )
				NameIndices(Info.Linker->NameMap(j).GetIndex()) = Info.NameBase + j;
		LinkerMap.Set( Info.Linker, i );
	}}
	unguard;
}

//
// Mapping functions.
//
INT UPackageMap::ObjectToIndex( UObject* Object )
{
	guard(UPackageMap::ObjectToIndex);
	if( Object && Object->GetLinker() && Object->_LinkerIndex!=INDEX_NONE )
	{
		INT* Found = LinkerMap.Find( Object->GetLinker() );
		if( Found )
		{
			FPackageInfo& Info = List( *Found );
			if( Object->_LinkerIndex<Info.ObjectCount )
				return Info.ObjectBase + Object->_LinkerIndex;
		}
	}
	return INDEX_NONE;
	unguard;
}
UBOOL UPackageMap::SupportsPackage( UObject* InOuter )
{
	guard(UPackageMap::SupportsPackage);
	for( INT i=0; i<List.Num(); i++ )
		if( List(i).Parent == InOuter )
			return 1;
	return 0;
	unguard;
}
UObject* UPackageMap::IndexToObject( INT Index, UBOOL Load )
{
	guard(UPackageMap::PairToObject);
	if( Index>=0 )
	{
		for( INT i=0; i<List.Num(); i++ )
		{
			FPackageInfo& Info = List(i);
			if( Index < Info.ObjectCount )
			{
				UObject* Result = Info.Linker->ExportMap(Index)._Object;
				if( !Result && Load )
				{
					UObject::BeginLoad();
					Result = Info.Linker->CreateExport(Index);
					UObject::EndLoad();
				}
				return Result;
			}
			Index -= Info.ObjectCount;
		}
	}
	return NULL;
	unguard;
}
void UPackageMap::Serialize( FArchive& Ar )
{
	guard(UPackageMap::Serialize);
	Super::Serialize( Ar );
	Ar << List << LinkerMap << ClassFieldIndices;
	unguard;
}
IMPLEMENT_CLASS(UPackageMap);

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
