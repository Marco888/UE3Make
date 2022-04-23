/*=============================================================================
	UnCorSc.cpp: UnrealScript execution and support code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Description:
	UnrealScript execution and support code.

Revision history:
	* Created by Tim Sweeney

=============================================================================*/

#include "CorePrivate.h"

/*-----------------------------------------------------------------------------
	FFrame implementation.
-----------------------------------------------------------------------------*/

//
// Error or warning handler.
//

FArchive& operator<<(FArchive& Ar, FPushedState& PushedState)
{
	INT Offset = Ar.IsSaving() ? PushedState.Code - &PushedState.Node->Script(0) : INDEX_NONE;
	Ar << PushedState.State << PushedState.Node << Offset;
	if (Offset != INDEX_NONE)
	{
		PushedState.Code = &PushedState.Node->Script(Offset);
	}
	return Ar;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
