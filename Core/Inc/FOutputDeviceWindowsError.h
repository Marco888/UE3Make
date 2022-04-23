/*=============================================================================
	FOutputDeviceWindowsError.h: Windows error message outputter.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// Handle a critical error.
//
class FOutputDeviceWindowsError : public FOutputDeviceError
{
	INT ErrorPos;
	EName ErrorType;
public:
	FOutputDeviceWindowsError()
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
		DebugBreak();
#else
		INT Error = GetLastError();
		if( !GIsCriticalError )
		{
			// First appError.
			GIsCriticalError = 1;
			ErrorType        = Event;
			debugf( NAME_Critical, TEXT("appError called (%i):"), GIsGuarded);
			debugf( NAME_Critical, TEXT("%ls"), Msg );

			// Windows error.
			if (Error !=0)
				debugf( NAME_Critical, TEXT("Windows GetLastError: %ls (%i)"), appGetSystemErrorMessage(Error), Error );

			// Shut down.
			UObject::StaticShutdownAfterError();
			appStrncpy( GErrorHist, Msg, ARRAY_COUNT(GErrorHist) );
			appStrncat( GErrorHist, TEXT("\r\n\r\n"), ARRAY_COUNT(GErrorHist) );
			ErrorPos = appStrlen(GErrorHist);
			if( GIsGuarded )
				appStrncat( GErrorHist, TEXT("History: "), ARRAY_COUNT(GErrorHist) );
			else HandleError();
		}
		else debugf( NAME_Critical, TEXT("Error reentered: %ls"), Msg );

		// Propagate the error or exit.
		if( GIsGuarded )
			throw( 1 );
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
		}
		catch( ... )
		{}
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
