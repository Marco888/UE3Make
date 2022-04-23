
#include "CorePrivate.h"

#if _WITH_DEBUG_CPU_COUNT_
FCPUFunctionClock* FCPUFunctionClock::FirstCPUClock=NULL;
BYTE FCPUFunctionClock::bWantsDump=0;

FCPUFunctionClock::FCPUFunctionClock( const TCHAR* Name )
	: StatName(Name), Clocks(0), NumCalls(0)
{
	Next = FirstCPUClock;
	FirstCPUClock = this;
}
FCPUFunctionClock::~FCPUFunctionClock()
{
	if( FirstCPUClock==this )
		FirstCPUClock = Next;
	else
	{
		for( FCPUFunctionClock* T=FirstCPUClock; T; T=T->Next )
			if( T->Next==this )
			{
				T->Next = Next;
				break;
			}
	}
}
void FCPUFunctionClock::PrintStats( FOutputDevice* Device )
{
	Device->Log(TEXT("Stack: CPU cycles (number of calls)"));
	for( FCPUFunctionClock* T=FirstCPUClock; T; T=T->Next )
	{
		if( T->NumCalls )
			Device->Logf(TEXT("%04.1f ms (%.3i) %ls"),(GSecondsPerCycle*1000.f*T->Clocks),T->NumCalls,T->StatName);
	}
}
void FCPUFunctionClock::Reset()
{
	if( bWantsDump )
	{
		PrintStats(GLog);
		bWantsDump = 0;
	}
	for( FCPUFunctionClock* T=FirstCPUClock; T; T=T->Next )
	{
		T->Clocks = 0;
		T->NumCalls = 0;
	}
}
#endif
