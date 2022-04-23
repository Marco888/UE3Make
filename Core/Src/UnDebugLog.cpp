
#include "CorePrivate.h"

#if DEBUG_STACK_ENABLED
FGameTrackerLog::FGameTrackerLog()
	: FArch(NULL), Num(0)
{
}
FGameTrackerLog::~FGameTrackerLog()
{
	if( FArch )
		delete FArch;
}
void FGameTrackerLog::Serialize( const TCHAR* V, EName Event )
{
	static TCHAR StackLine[64];
	if( !FArch )
	{
		FArch = GFileManager->CreateFileWriter(TEXT("UnrealDebug.log"));
		for( INT i=0; i<64; ++i )
			StackLine[i] = '.';
		if( !FArch )
			return;
	}

	StackLine[StackNum] = 0;
	FString Line = FString::Printf(TEXT("%i: %ls%ls\r\n"),Num,StackLine,V);
	StackLine[StackNum] = '.';
	FArch->Serialize( const_cast<TCHAR*>(*Line), Line.Len()*sizeof(TCHAR) );
	FArch->Flush();
}
void FGameTrackerLog::Restart()
{
	++Num;
	if( FArch && (Num & 16) )
		FArch->Seek(0);
}
CORE_API FGameTrackerLog SLog;
#endif
