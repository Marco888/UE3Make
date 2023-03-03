// Launch.cpp : Defines the entry point for the console application.
//

#define __UE3Make__ 1

// Core and Engine
#include "UnEditor.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
#if _MSC_VER
extern "C" {HINSTANCE hInstance; }
#endif
extern "C" {TCHAR GPackage[64] = TEXT("UE3Make"); }

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

		// Parse command line.
#if WIN32
		TCHAR CmdLine[1024], * CmdLinePtr = CmdLine;
		*CmdLinePtr = 0;
		ANSICHAR* Ch = GetCommandLineA();
		while (*Ch && *Ch != ' ')
			Ch++;
		while (*Ch == ' ')
			Ch++;
		while (*Ch)
			*CmdLinePtr++ = *Ch++;
		*CmdLinePtr++ = 0;
#else
		char CmdLine[1024] = "";
		char* CmdLinePtr = CmdLine;
		*CmdLinePtr = 0;
		for (INT i = 1; i < argc; i++)
		{
			if ((strlen(CmdLine) + strlen(argv[i])) > (sizeof(CmdLine) - 1))
				break;
			if (i > 1)
				strcat(CmdLine, " ");
			strcat(CmdLine, argv[i]);
		}
		TCHAR* CommandLine = appStaticString1024();
		appStrcpy(CommandLine, ANSI_TO_TCHAR(CmdLine));
#endif

		// Init engine core.
		appInit(TEXT("UE3Make"), CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1);
		MakeMain();
		appPreExit();
		GIsGuarded = 0;
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
