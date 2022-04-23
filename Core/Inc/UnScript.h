/*=============================================================================
	UnScript.h: UnrealScript execution engine.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	FFrame implementation.
-----------------------------------------------------------------------------*/

inline FFrame::FFrame( UObject* InObject )
:	Node		(InObject ? InObject->GetClass() : NULL)
,	Object		(InObject)
,	Code		(NULL)
,	Locals		(NULL)
{}

/*-----------------------------------------------------------------------------
	FStateFrame implementation.
-----------------------------------------------------------------------------*/

inline FStateFrame::FStateFrame( UObject* InObject )
:	FFrame		( InObject )
,	CurrentFrame( NULL )
,	StateNode	( InObject->GetClass() )
,	ProbeMask	( ~(QWORD)0 )
,	LatentAction( 0 )
{}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
