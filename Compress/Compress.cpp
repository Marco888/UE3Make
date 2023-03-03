// Compress.cpp : Defines the entry point for the console application.
//

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

#define __UE3Make__ 0
#include "Core.h"
#include "FCodec.h"

// General.
#if _MSC_VER
extern "C" {HINSTANCE hInstance; }
#endif
extern "C" {TCHAR GPackage[64] = TEXT("UDKCompress"); }

// Log.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
#include "FFeedbackContextAnsi.h"
FFeedbackContextAnsi Warn;

// File manager.
#if WIN32
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;

#if 1
#include "FMallocAnsi.h"
FMallocAnsi Malloc;
#else
#include "FMallocWindows.h"
FMallocWindows Malloc;
#endif

#elif __PSX2_EE__
#include "FFileManagerPSX2.h"
FFileManagerPSX2 FileManager;
#include "FMallocAnsi.h"
FMallocAnsi Malloc;
#elif __GNUG__
#include "FFileManagerLinux.h"
FFileManagerLinux FileManager;
#include "FMallocAnsi.h"
FMallocAnsi Malloc;
#else
#include "FFileManagerAnsi.h"
FFileManagerAnsi FileManager;
#endif

void CompressFile(FString FileName)
{
    guard(CompressFile);

    GWarn->Logf(NAME_Heading, TEXT("Compressing %s"), *FileName);

    FString CompressFile = FileName + TEXT(".uz2");
    INT FileSize = GFileManager->FileSize(*FileName);

    FArchive* CFileAr = GFileManager->CreateFileReader(*FileName);
    if (!CFileAr)
    {
        GWarn->Logf(NAME_Heading, TEXT("Failed to read %s!"), *FileName);
        return;
    }
        
    FArchive* UFileAr = GFileManager->CreateFileWriter(*CompressFile);
    if (!UFileAr)
    {
        GWarn->Logf(NAME_Heading, TEXT("Failed to write %s!"), *CompressFile);
        return;
    }

    INT iSig = 0x0000162E;
    FString FSig = appFileBaseName(*FileName);
    *UFileAr << iSig;
    *UFileAr << FSig;

    FCodecFull Codec;
    Codec.AddCodec(new FCodecRLE);
    Codec.AddCodec(new FCodecBWT);
    Codec.AddCodec(new FCodecMTF);
    Codec.AddCodec(new FCodecRLE);
    Codec.AddCodec(new FCodecHuffman);
    Codec.Encode(*CFileAr, *UFileAr);

    delete CFileAr;
    delete UFileAr;
    
    INT CompressedFileSize = GFileManager->FileSize(*CompressFile);
    GWarn->Logf(NAME_Heading, TEXT("Compressed %s -> %s (-%d%%)\n\n"), *FileName, *CompressFile, static_cast<int>((1.f - (static_cast<float>(CompressedFileSize) / static_cast<float>(FileSize))) * 100));
    
    unguard;
}

void DecompressFile(FString FileName)
{
    guard(DecompressFile);
    
    GWarn->Logf(NAME_Heading, TEXT("Decompressing %s"), *FileName);

    FString UncompressFile = FileName.FStringReplace(TEXT(".uz2"), TEXT(""));
    INT FileSize = GFileManager->FileSize(*FileName);

    FArchive* CFileAr = GFileManager->CreateFileReader(*FileName);
    if (!CFileAr)
    {
        GWarn->Logf(NAME_Heading, TEXT("Failed to read %s!"), *FileName);
        return;
    }
        
    FArchive* UFileAr = GFileManager->CreateFileWriter(*UncompressFile);
    if (!UFileAr)
    {
        GWarn->Logf(NAME_Heading, TEXT("Failed to write %s!"), *UncompressFile);
        return;
    }

    INT Signature;
    FString OrigFilename;

    *CFileAr << Signature;
    *CFileAr << OrigFilename;

    FCodecFull Codec;
    Codec.AddCodec(new FCodecRLE);
    Codec.AddCodec(new FCodecBWT);
    Codec.AddCodec(new FCodecMTF);
    Codec.AddCodec(new FCodecRLE);
    Codec.AddCodec(new FCodecHuffman);
    Codec.Decode(*CFileAr, *UFileAr);

    delete CFileAr;
    delete UFileAr;

    GWarn->Logf(NAME_Heading, TEXT("Decompressed %s -> %s\n\n"), *FileName, *UncompressFile);
    
    unguard;
}

void CompressMain(const TArray<TCHAR*> CmdLine)
{
	guard(CompressMain);
	for( INT i = 0; i < CmdLine.Num(); i++ )
	{
        const FString FileExt = FString(appFExt(CmdLine(i)));
        if( FileExt != TEXT("uz2") )
            CompressFile(FString(CmdLine(i)));
        else if( FileExt == TEXT("uz2") )
            DecompressFile(FString(CmdLine(i)));
	}
	unguard;
}

int main(int argc, char* argv[])
{
	// stijn: needs to be initialized early now because we use FStrings everywhere
	Malloc.Init();
	GMalloc = &Malloc;
	
	INT ErrorLevel = 0;
	GIsStarted = 1;

#ifndef _DEBUG
	try
#endif
	{
		GIsGuarded = 1;

#if !_MSC_VER
		// Set module name.
		//appStrcpy( GModule, "ucc" );
		// GModule < PATH_MAX
		appStrncpy(GModule, ANSI_TO_TCHAR(argv[0]), ARRAY_COUNT(GModule));
#endif
        if( argc > 1 )
        {
            // Parse command line.
            TArray<TCHAR*> CmdLine;

            // Init engine core.
            appInit(TEXT("UDKCompress"), GetCommandLine(), &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 0);
            
            for( INT i = 1; i < argc; i++ )
                CmdLine.AddItem(ANSI_TO_TCHAR(argv[i]));
            
            CompressMain(CmdLine);
            appPreExit();
            GIsGuarded = 0;
        }
        else
        {
            Warn.Log(NAME_Title, TEXT("Critical Warning!"));
            appPreExit();
            GIsGuarded = 0;
            wprintf(TEXT("Critical Warning: Must pass a argument!\n\n"));
        }
	}
#ifndef _DEBUG
	catch( ... )
	{
		Warn.Log(NAME_Title, TEXT("Critical Error!"));

		// Crashed.
		ErrorLevel = 1;
		GIsGuarded = 0;
		Error.HandleError();
		wprintf(TEXT("Critical Error: %ls\n\n"), GErrorHist);
	}
#endif
	appExit();
	GIsStarted = 0;
	system("pause");
	return ErrorLevel;
}
