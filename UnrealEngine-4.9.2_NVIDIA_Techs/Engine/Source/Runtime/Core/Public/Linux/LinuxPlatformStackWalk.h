// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	LinuxStackWalk.h: Linux platform stack walk functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformStackWalk.h"

#undef PLATFORM_SUPPORTS_STACK_SYMBOLS
#define PLATFORM_SUPPORTS_STACK_SYMBOLS 1

struct CORE_API FLinuxPlatformStackWalk : public FGenericPlatformStackWalk
{
	static void ProgramCounterToSymbolInfo( uint64 ProgramCounter, FProgramCounterSymbolInfo& out_SymbolInfo );
	static bool ProgramCounterToHumanReadableString( int32 CurrentCallDepth, uint64 ProgramCounter, ANSICHAR* HumanReadableString, SIZE_T HumanReadableStringSize, FGenericCrashContext* Context = nullptr );
	static void CaptureStackBackTrace( uint64* BackTrace, uint32 MaxDepth, void* Context = nullptr );
};

typedef FLinuxPlatformStackWalk FPlatformStackWalk;
