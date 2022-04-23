/*=============================================================================
	FOutputDeviceAnsiError.h: Ansi stdout error output device.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// ANSI stdout output device.
//
class FOutputDeviceAnsiError : public FOutputDeviceError
{
	INT ErrorPos;
	EName ErrorType;
	void LocalPrint( const TCHAR* Str )
	{
#if UNICODE
		wprintf(TEXT("%ls"),Str);
#else
		printf(TEXT("%ls"),Str);
#endif
	}
public:
	FOutputDeviceAnsiError()
	: ErrorPos(0)
	, ErrorType(NAME_None)
	{}
	void Serialize( const TCHAR* Msg, enum EName Event )
	{
#ifdef _DEBUG
		// Just display info and break the debugger.
  		debugf( NAME_Critical, TEXT("appError called while debugging:") );
		debugf( NAME_Critical, Msg );
		UObject::StaticShutdownAfterError();
  		debugf( NAME_Critical, TEXT("Breaking debugger") );
		*(BYTE*)NULL=0;
#else
		if( !GIsCriticalError )
		{
			// First appError.
			GIsCriticalError = 1;
			ErrorType        = Event;
			debugf( NAME_Critical, TEXT("appError called:") );
			debugf( NAME_Critical, Msg );

			#ifndef _WIN32
			// if Linux or OSX use guard blocks to get some details.
			FString GuardBackTrace;
            UnGuardBlock::GetBackTrace(GuardBackTrace);

            TCHAR Buffer[4096]=TEXT("");
            INT i=0;

            // wordwrap for SDL_ShowSimpleMessageBox
            while (GuardBackTrace.Len()>0)
            {
                if ( i == 80 )
                {
                    appStrncat( Buffer, *GuardBackTrace.Left(i), ARRAY_COUNT(Buffer) );
                    appStrncat( Buffer, TEXT(" \n"), ARRAY_COUNT(Buffer) );
                    GuardBackTrace = GuardBackTrace.Mid(i,GuardBackTrace.Len());
                    i=0;
                }
                else if (i == GuardBackTrace.Len())
                {
                    appStrncat( Buffer, *GuardBackTrace, ARRAY_COUNT(Buffer) );
                    appStrncat( Buffer, TEXT(" \n"), ARRAY_COUNT(Buffer) );
                    LocalPrint(Buffer);
                    break;
                }
                else i++;
            }
			#endif

			// Shut down.
			UObject::StaticShutdownAfterError();
			appStrncpy( GErrorHist, Msg, ARRAY_COUNT(GErrorHist) );
			appStrncat( GErrorHist, TEXT("\r\n"), ARRAY_COUNT(GErrorHist) );
			ErrorPos = appStrlen(GErrorHist);
			if( GIsGuarded )
			{
				appStrncat( GErrorHist, TEXT("History"), ARRAY_COUNT(GErrorHist) );
				appStrncat( GErrorHist, TEXT(": \n"), ARRAY_COUNT(GErrorHist) );
				#ifndef _WIN32
				appStrncat( GErrorHist, Buffer, ARRAY_COUNT(GErrorHist) );
				#endif
			}
			else
			{
				HandleError();
			}
		}
		else debugf( NAME_Critical, TEXT("Error reentered: %ls"), Msg );

		// Propagate the error or exit.
		if( GIsGuarded )
		{
			HandleError();
			appRequestExit( 1, Msg );
		}
		else
			appRequestExit( 1, Msg );
#endif
	}
	void HandleError()
	{
		try
		{
			GIsGuarded       = 0;
			GIsRunning       = 0;
			GIsCriticalError = 1;
			GLogHook         = NULL;
			UObject::StaticShutdownAfterError();
			GErrorHist[ErrorType==NAME_FriendlyError ? ErrorPos : ARRAY_COUNT(GErrorHist)-1]=0;
			LocalPrint( GErrorHist );
			LocalPrint( TEXT("\n\nExiting due to error\n") );
			GLog->Log(GErrorHist);
			GLog->Log(TEXT("Exiting due to error"));

			#ifdef SDL2BUILD
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, appToAnsi(GConfig ? *LocalizeError(TEXT("Critical"),TEXT("Window")) : TEXT("Critical Error At Startup")),appToAnsi(GErrorHist), NULL);
            #endif
		}
		catch( ... )
		{}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
