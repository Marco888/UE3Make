/*=============================================================================
	FOutputDeviceFile.h: ANSI file output device.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// ANSI file output device.
//

class FOutputDeviceFile : public FOutputDevice
{
public:
	FOutputDeviceFile()
	: LogAr( NULL )
	, Opened( 0 )
	, Dead( 0 )
	{
	}
	~FOutputDeviceFile()
	{
		if( LogAr )
		{
			Log(NAME_Log, TEXT("Log file closed"));
			delete LogAr;
			LogAr = NULL;
		}
	}
	void Serialize( const TCHAR* Data, enum EName Event )
	{
		static UBOOL Entry=0;
		if( !GIsCriticalError || Entry )
		{
			//if (!FName::SafeSuppressed(Event))
			{
				if( !LogAr && !Dead )
				{
					// Open log file.
                    #if __UE3Make__
                    LogAr = GFileManager->CreateFileWriter( TEXT("UE3Make.log"), FILEWRITE_AllowRead|FILEWRITE_Unbuffered|(Opened?FILEWRITE_Append:0));
                    #else
                    LogAr = GFileManager->CreateFileWriter( TEXT("UDKCompress.log"), FILEWRITE_AllowRead|FILEWRITE_Unbuffered|(Opened?FILEWRITE_Append:0));
                    #endif
					if( LogAr )
					{
						Opened = 1;
#if UNICODE && !FORCE_ANSI_LOG && !FORCE_UTF8_LOG
                        UNICHAR UnicodeBOM = 0;
                        if (sizeof(TCHAR)==4)
                            UnicodeBOM = UTF32_BOM;
                        else UnicodeBOM = UTF16_BOM;
						LogAr->Serialize( &UnicodeBOM, sizeof(UnicodeBOM) );
#endif
						Logf( NAME_Log, TEXT("Log file open, %ls"), *appTimestamp() );
					}
					else Dead = 1;
				}
				if( LogAr && Event!=NAME_Title )
				{
#if FORCE_ANSI_LOG && UNICODE
					WriteAnsi( FName::SafeString(Event) );
					WriteAnsi( TEXT(": ") );
					WriteAnsi( Data );
					WriteAnsi( LINE_TERMINATOR );
#else
					WriteRaw( FName::SafeString(Event) );
					WriteRaw( TEXT(": ") );
					WriteRaw( Data );
					WriteRaw( LINE_TERMINATOR );
#endif
				}
#if _DEBUG
				Flush();
#endif
				if( GLogHook )
					GLogHook->Serialize( Data, Event );
			}
		}
		else
		{
			Entry=1;
			try
			{
				// Ignore errors to prevent infinite-recursive exception reporting.
				Serialize( Data, Event );
			}
			catch( ... )
			{
			}
			Entry=0;
		}
	}
	void Flush()
	{
		if (LogAr)
			LogAr->Flush();
	}
	FArchive* LogAr;
private:
	UBOOL Opened, Dead;
	void WriteRaw(const TCHAR* C)
	{
#if FORCE_ANSI_LOG || FORCE_UTF8_LOG
		static ANSICHAR LogBuf[1024];
		INT Len = appStrlen(C);
		if (Len * 4 < 1023)
		{
# if FORCE_ANSI_LOG
			appToAnsiInPlace(LogBuf, C, Len + 1);
			LogAr->Serialize(const_cast<ANSICHAR*>(LogBuf), Len);
# else
			appToUtf8InPlace(LogBuf, C, 1024);
			LogAr->Serialize(const_cast<ANSICHAR*>(LogBuf), strlen(LogBuf));
# endif
		}
		else
		{
# if FORCE_ANSI_LOG
			ANSICHAR* Tmp = new ANSICHAR[Len + 1];
# else
			ANSICHAR* Tmp = new ANSICHAR[(Len + 1) * 4];
# endif
			if (Tmp)
			{
# if FORCE_ANSI_LOG
				appToAnsiInPlace(Tmp, C, Len + 1);
				LogAr->Serialize(const_cast<ANSICHAR*>(Tmp), Len);
# else
				appToUtf8InPlace(Tmp, C, (Len + 1) * 4);
				LogAr->Serialize(const_cast<ANSICHAR*>(Tmp), strlen(Tmp));
# endif
				delete[] Tmp;
			}
		}
#else
		LogAr->Serialize(const_cast<TCHAR*>(C), appStrlen(C) * sizeof(TCHAR));
#endif
	}
#if FORCE_ANSI_LOG && UNICODE
    void WriteAnsi( const TCHAR* C )
    {
        static BYTE Output[4096];
        INT i=0;
        for(; C[i] && i<4096; ++i )
            Output[i] = C[i];
        LogAr->Serialize( Output, i );
    }
#endif
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
