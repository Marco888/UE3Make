/*=============================================================================
	UnMD5: Unreal MD5 lookup table

Revision history:
	* Created by Michiel Hendriks
=============================================================================*/

#include "CorePrivate.h"
//#include "CoreClasses.h"
#ifdef UTPG_MD5

/*-----------------------------------------------------------------------------
	FMD5Record implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, FZQ5Erpbeq& record )
{
  guard(FArchive<<FZQ5Erpbeq);
  Ar << record.filename;
  Ar << record.guid;
  Ar << record.generation;
  Ar << record.ZQ5Purpxfhz;
  Ar << record.filetime;
	Ar << record.language;
  return Ar;
  unguard;
}

/*-----------------------------------------------------------------------------
	UMD5Table implementation.
-----------------------------------------------------------------------------*/

FArchive& operator<<( FArchive& Ar, UZQ5Gnoyr& table )
{
  guard(FArchive<<UZQ5Gnoyr);
  for (INT i = 0; i < table.List.Num(); i++)
  {
		//debugf( TEXT("<< FILE=%ls GUID=%ls GEN=%d MD5=%ls LANG=%ls"), *table.List(i)->filename, table.List(i)->guid.String(), table.List(i)->generation, *table.List(i)->ZQ5Purpxfhz, *table.List(i)->language);
    Ar << *table.List(i);
  }
  return Ar;
  unguard;
}

void UZQ5Gnoyr::addRecord(FZQ5Erpbeq* record)
{
  guard(UZQ5Gnoyr::addRecord);
  if (record->filename != TEXT("") && record->ZQ5Purpxfhz != TEXT("")) List.AddItem(record);
  else debugf(NAME_DevMD5, TEXT("Tried to add a MD5Record without filename or checksum"));
	//debugf( TEXT("Added record FILE=%ls GUID=%ls GEN=%d MD5=%ls LANG=%ls"), *record->filename, record->guid.String(), record->generation, *record->ZQ5Purpxfhz, *record->language);
  unguard;
}

void UZQ5Gnoyr::removeRecord(INT i)
{
  guard(UZQ5Gnoyr::removeRecord);
  List.Remove(i);
  unguard;
}

FZQ5Erpbeq* UZQ5Gnoyr::getRecord(INT i)
{
  guard(UZQ5Gnoyr::getRecord);
  return List(i);
  unguard;
}

FString UZQ5Gnoyr::lookupZQ5(FString inFilename, FGuid InGuid, INT InGeneration, FString InLanguage)
{
  guard(UZQ5Gnoyr::lookupZQ5);
  for ( INT i = 0; i < List.Num(); i++)
  {
    if ((List(i)->guid == InGuid)
      && (List(i)->generation == InGeneration))
    {
			if (InLanguage == List(i)->language)
				return List(i)->ZQ5Purpxfhz;
    }
  }
  debugf(NAME_DevMD5, TEXT("No MD5 record for %ls GUID=%ls GEN=%i LANG=%ls"), *inFilename, InGuid.String(), InGeneration, *InLanguage);
  return TEXT("");
  unguard;
}

FZQ5Erpbeq* UZQ5Gnoyr::lookupZQ5Erpbeq(FString inFilename, FGuid InGuid, INT InGeneration, FString InLanguage)
{
  guard(UZQ5Gnoyr::lookupZQ5);
  for ( INT i = 0; i < List.Num(); i++)
  {
    if ((List(i)->guid == InGuid)
      && (List(i)->generation == InGeneration))
    {
			if (InLanguage == List(i)->language)
				return List(i);
    }
  }
	debugf(NAME_DevMD5, TEXT("No MD5 record for %ls GUID=%ls GEN=%i LANG=%ls"), *inFilename, InGuid.String(), InGeneration, *InLanguage);
  return NULL;
  unguard;
}

FString UZQ5Gnoyr::lookupZQ5filename(FString inFilename, INT InGeneration, FString InLanguage)
{
  guard(UZQ5Gnoyr::lookupMD5);
  for ( INT i = 0; i < List.Num(); i++)
  {
    if ((List(i)->filename == inFilename)
      && (List(i)->generation == InGeneration))
    {
			if (InLanguage == List(i)->language)
				return List(i)->ZQ5Purpxfhz;
    }
  }
  return TEXT("");
  unguard;
}



/*-----------------------------------------------------------------------------
	xmd5 implementation
-----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(Uxmd5);

void Uxmd5::execxFileMD5( FFrame& Stack, RESULT_DECL )
{
	guardSlow(UObject::execxFileMD5);

	P_GET_STR(inFile);
	P_GET_STR_REF(outMD5);
	P_FINISH;

	*outMD5 = TEXT("");
	*(INT*)Result = 0;

#ifdef UTPG_MD5
	FString newBase = FString::Printf(TEXT("%ls..%ls"), appBaseDir(), PATH_SEPARATOR);
	GFileManager->SetDefaultDirectory(*newBase);

	if (inFile.InStr(TEXT("..")) > -1)
	{
		*(INT*)Result = -2;
		debugf(TEXT("xFileMD5: \"..\" is not allowed"));
		return;
	}

	inFile = FString::Printf(TEXT("%ls%ls%ls"), *GFileManager->GetDefaultDirectory(), PATH_SEPARATOR, *inFile);
	debugf(TEXT("xFileMD5: \"%ls\""), *inFile);

	FArchive* ar = GFileManager->CreateFileReader(*inFile);

	if (!ar)
	{
		*(INT*)Result = -1;
		debugf(TEXT("xFileMD5: file does not exist \"%ls\""), *inFile);
		return;
	}

	FMD5Context PContext;
	appMD5Init( &PContext );

	ar->Seek(0);
	BYTE sip[MD5CHUNK];
	INT pos = 0;

	while (ar->TotalSize()-pos > MD5CHUNK)
	{
		ar->Serialize(&sip, MD5CHUNK);
		appMD5Update( &PContext, (BYTE*) &sip, MD5CHUNK);
		pos += MD5CHUNK;
	}
	pos = ar->TotalSize()-pos;
	if (pos > 0)
	{
		ar->Serialize(&sip, pos);
		appMD5Update( &PContext, (BYTE*) &sip, pos);
	}

	BYTE Digest[16];
	appMD5Final( Digest, &PContext );
	for (INT j=0; j<16; j++) *outMD5 += FString::Printf(TEXT("%02x"), Digest[j]);

	*(INT*)Result = ar->TotalSize();
	delete ar;
#endif

	unguardexecSlow;
}
IMPLEMENT_FUNCTION( Uxmd5, 4004, execxFileMD5 );

#if __STATIC_LINK
Uxmd5NativeInfo GCoreUxmd5Natives[] =
{
	MAP_NATIVE(Uxmd5, execxFileMD5)
	{NULL, NULL}
};
#endif

#endif
/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
