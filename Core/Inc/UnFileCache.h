/*=============================================================================
	UnFileCache.h: Unreal local file cache handler
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Marco
=============================================================================*/
#include "FConfigCacheIni.h"

class CORE_API GameCacheFolderIni
{
private:
	FConfigFile CacheFile;
	FConfigSection* CacheSection;
	FString IniName;

public:
	GameCacheFolderIni()
	{
		IniName = FString::Printf(TEXT("%ls") PATH_SEPARATOR TEXT("cache.ini"), *GSys->CachePath);
		CacheFile.Read(*IniName);
		CacheSection = CacheFile.Find(TEXT("Cache"));
		if( !CacheSection )
		{
			CacheSection = new FConfigSection;
			CacheFile.Set(TEXT("Cache"),*CacheSection);
			CacheFile.Dirty = 1;
		}
		else if( !GIsEditor ) // Validate cache entries
		{
			GLog->Logf(NAME_Init,TEXT("Validating cache.ini entries..."));
			FString S;
			INT Valid=0,Purged=0;
			for( TMultiMap<FString,FString>::TIterator It(*CacheSection); It; ++It )
			{
				S = FString::Printf(TEXT("%ls") PATH_SEPARATOR TEXT("%ls%ls"),*GSys->CachePath,*It.Key(),*GSys->CacheExt);
				TArray<FString> A = GFileManager->FindFiles(*S,1,0);
				if( A.Num() )
				{
					++Valid; // File found
				}
				else
				{
					It.RemoveCurrent(); // File not found, remove item.
					CacheFile.Dirty = 1;
					++Purged;
				}
			}
			if (Purged > 0)
				GLog->Logf(NAME_Init,TEXT("Found %i valid entries, purged %i entries."),Valid,Purged);
			if( CacheFile.Dirty )
				SaveConfig();
		}
	}
	void SaveConfig()
	{
		CacheFile.Write(*IniName);
	}
	const TCHAR* GetIniFileName()
	{
		return *IniName;
	}
	void AddCacheFile( const TCHAR* GUID, const TCHAR* FileName )
	{
		if( CacheSection->Find(GUID) )
			return;
		CacheSection->Set(GUID,FileName);
		CacheFile.Dirty = 1;
	}
	const TCHAR* FindMatchingFile( const TCHAR* FileName )
	{
		INT iFind;
		FString S;
		const TCHAR* Result=NULL;
		for( TMultiMap<FString,FString>::TIterator It(*CacheSection); It; ++It )
		{
			S = It.Value();
			iFind = S.InStr(TEXT("."));
			if( iFind>0 )
				S = S.Left(iFind);
			if( S==FileName )
			{
				S = FString::Printf(TEXT("%ls") PATH_SEPARATOR TEXT("%ls%ls"),*GSys->CachePath,*It.Key(),*GSys->CacheExt);
				if( GFileManager->FileSize(*S)>0 ) // Ensure the file exists.
					Result = *It.Key();
				else // If not found, remove this old entry.
				{
					It.RemoveCurrent();
					CacheFile.Dirty = 1;
				}
			}
		}
		if( Result )
			GLog->Logf(TEXT("Using cache file for package '%ls'"),FileName);
		return Result;
	}
};


/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
