/*=============================================================================
	UnBuild.h: Unreal build settings.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/


/*-----------------------------------------------------------------------------
	Major build options.
-----------------------------------------------------------------------------*/

#if _WIN64 == 1 || __LP64__ || __x86_64__ || __powerpc64__ || __aarch64__ || __ARM_ARCH_8__ || _M_X64
#define BUILD_64 1
#else
#define BUILD_64 0
#endif

// Whether to turn off all checks.
#ifndef DO_CHECK
#define DO_CHECK 1
#endif

// Whether to track call-stack errors.
#ifndef DO_GUARD
#define DO_GUARD 1
#endif

#ifndef DEBUG_STACK_ENABLED
#define DEBUG_STACK_ENABLED 0
#endif

#ifndef DEBUG_VIRTUALS_ENABLED
#define DEBUG_VIRTUALS_ENABLED 0
#endif

#if DEBUG_VIRTUALS_ENABLED
#define _VF_BASE { FOutputDevice::show_pure_virtual_call(__FUNCSIG__); }
#define _VF_BASE_RET(type) { FOutputDevice::show_pure_virtual_call(__FUNCSIG__); return type; }
#else
#define _VF_BASE = 0
#define _VF_BASE_RET(type) = 0
#endif

#ifndef DEBUG_USCRIPT_CPP_MISMATCHES
#define DEBUG_USCRIPT_CPP_MISMATCHES 0
#endif


// Useful option if some lib fails to load.
#ifndef LOOKING_FOR_UNDEFINED_SYMBOLS
#define LOOKING_FOR_UNDEFINED_SYMBOLS 0
#endif

// Whether to track call-stack errors in performance critical routines.
#ifndef DO_GUARD_SLOW
#define DO_GUARD_SLOW 0
#endif

// Whether to perform CPU-intensive timing of critical loops.
#ifndef DO_CLOCK_SLOW
#define DO_CLOCK_SLOW 0
#endif

// Whether to gather performance statistics.
#ifndef STATS
#define STATS 1
#endif

// Whether to use Intel assembler code.
#ifndef ASM
#define ASM 0
#endif

// Whether to use 3DNow! assembler code.
#ifndef ASM3DNOW
#define ASM3DNOW 0
#endif

//RTNP build for binding with UPak
#ifndef UPAK_Build
#define UPAK_Build 1
#endif

// Disable/enable multi-threading for some render tasks.
#ifndef DISABLE_MULTITHREADING
#ifdef __LINUX__
#define DISABLE_MULTITHREADING 1
#else
#define DISABLE_MULTITHREADING 1
#endif
#endif


#ifdef _MSC_VER
	//elmuerte: enabled hook scanning routines
	//#ifndef UTPG_HOOK_SCAN
	//#define UTPG_HOOK_SCAN 1
	//#endif

	//elmuerte: winxp sp2 firewall hack (backported from UT2004)
	#ifndef UTPG_WINXP_FIREWALL
	#define UTPG_WINXP_FIREWALL 1
	#endif

	#define HUNG_SAFE_BUILD 0
	#define _WITH_DEBUG_CPU_COUNT_ 0
	#define INCLUDE_DEP_POLICY_CHANGE 0
	#define _ALLOW_RTCc_IN_STL 1

	// Use FMallocAnsi instead of FMallocWindows
	#if BUILD_64
		// FMallocWindows does crash in 64b Windows yet, Stijn assumes FMallocAnsi maybe even faster nowadays too. TODO: Benchmarking
		#define MALLOC_ANSI 1
	#endif
#endif

#ifdef __LINUX__
//	#define SDL2BUILD 1
#elif MACOSX
    #define SDL2BUILD 1
#endif


/*-----------------------------------------------------------------------------
	Constants.
-----------------------------------------------------------------------------*/

// Boundary to align class properties on.
#define INT_ALIGNMENT 4

#if BUILD_64
#define VECTOR_ALIGNMENT 16
#define OBJECT_ALIGNMENT 16 // needs to be >= VECTOR_ALIGNMENT
#define POINTER_ALIGNMENT 8
#elif __LINUX_X86__
#define VECTOR_ALIGNMENT 16
#define OBJECT_ALIGNMENT 16 // needs to be >= VECTOR_ALIGNMENT
#define POINTER_ALIGNMENT 4
#elif !defined(__LINUX_ARM__)
#define VECTOR_ALIGNMENT 4
#define OBJECT_ALIGNMENT 4
#define POINTER_ALIGNMENT 4
#else
// stijn: ARM does NOT want unaligned mem accesses. This means that even with
// tight packing, structs like FTime will be aligned on 8-byte boundaries
#define VECTOR_ALIGNMENT 16
#define OBJECT_ALIGNMENT 16
#define POINTER_ALIGNMENT 4
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
