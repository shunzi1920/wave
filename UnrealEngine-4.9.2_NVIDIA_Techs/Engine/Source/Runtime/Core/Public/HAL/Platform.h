// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


// define all other platforms to be zero
//@port Define the platform here to be zero when compiling for other platforms
#if !defined(PLATFORM_WINDOWS)
	#define PLATFORM_WINDOWS 0
#endif
#if !defined(PLATFORM_XBOXONE)
	#define PLATFORM_XBOXONE 0
#endif
#if !defined(PLATFORM_MAC)
	#define PLATFORM_MAC 0
#endif
#if !defined(PLATFORM_PS4)
	#define PLATFORM_PS4 0
#endif
#if !defined(PLATFORM_IOS)
	#define PLATFORM_IOS 0
#endif
#if !defined(PLATFORM_ANDROID)
	#define PLATFORM_ANDROID 0
#endif
#if !defined(PLATFORM_ANDROID_ARM)
	#define PLATFORM_ANDROID_ARM 0
#endif
#if !defined(PLATFORM_ANDROID_ARM64)
	#define PLATFORM_ANDROID_ARM64 0
#endif
#if !defined(PLATFORM_ANDROID_X86)
	#define PLATFORM_ANDROID_X86 0
#endif
#if !defined(PLATFORM_ANDROID_X64)
	#define PLATFORM_ANDROID_X64 0
#endif
#if !defined(PLATFORM_ANDROIDGL4)
	#define PLATFORM_ANDROIDGL4 0
#endif
#if !defined(PLATFORM_ANDROIDES31)
	#define PLATFORM_ANDROIDES31 0
#endif
#if !defined(PLATFORM_WINRT)
	#define PLATFORM_WINRT 0
#endif
#if !defined(PLATFORM_WINRT_ARM)
	#define PLATFORM_WINRT_ARM	0
#endif
#if !defined(PLATFORM_APPLE)
	#define PLATFORM_APPLE 0
#endif
#if !defined(PLATFORM_HTML5)
	#define PLATFORM_HTML5 0
#endif
#if !defined(PLATFORM_LINUX)
	#define PLATFORM_LINUX 0
#endif

// Platform specific compiler pre-setup.
#if PLATFORM_WINDOWS
	#include "Windows/WindowsPlatformCompilerPreSetup.h"
#elif PLATFORM_PS4
	#include "PS4/PS4PlatformCompilerPreSetup.h"
#elif PLATFORM_XBOXONE
	#include "XboxOne/XboxOnePlatformCompilerPreSetup.h"
#elif PLATFORM_MAC
	#include "Mac/MacPlatformCompilerPreSetup.h"
#elif PLATFORM_IOS
	#include "IOS/IOSPlatformCompilerPreSetup.h"
#elif PLATFORM_ANDROID
	#include "Android/AndroidPlatformCompilerPreSetup.h"
#elif PLATFORM_WINRT_ARM || PLATFORM_WINRT
	#include "WinRT/WinRTPlatformCompilerPreSetup.h"
#elif PLATFORM_HTML5
	#include "HTML5/HTML5PlatformCompilerPreSetup.h"
#elif PLATFORM_LINUX
	#include "Linux/LinuxPlatformCompilerPreSetup.h"
#else
	#error Unknown Compiler
#endif

// Generic compiler pre-setup.
#include "GenericPlatform/GenericPlatformCompilerPreSetup.h"

#if PLATFORM_APPLE
	#include <stddef.h> // needed for size_t
#endif

#include "GenericPlatform/GenericPlatform.h"

//---------------------------------------------------------
// Identify the current platform and include that header
//---------------------------------------------------------

//@port Identify the platform here and include the platform header to setup the platform types, etc
#if PLATFORM_WINDOWS
	#include "Windows/WIndowsPlatform.h"
#elif PLATFORM_PS4
	#include "PS4/PS4Platform.h"
#elif PLATFORM_XBOXONE
	#include "XboxOne/XboxOnePlatform.h"
#elif PLATFORM_MAC
	#include "Mac/MacPlatform.h"
#elif PLATFORM_IOS
	#include "IOS/IOSPlatform.h"
#elif PLATFORM_ANDROID
	#include "Android/AndroidPlatform.h"
#elif PLATFORM_WINRT_ARM
	#include "WinRT/WinRTARMPlatform.h"
#elif PLATFORM_WINRT
	#include "WinRT/WinRTPlatform.h"
#elif PLATFORM_HTML5
	#include "HTML5/HTML5Platform.h"
#elif PLATFORM_LINUX
	#include "Linux/LinuxPlatform.h"
#else
	#error Unknown Compiler
#endif

//------------------------------------------------------------------
// Finalize define setup
//------------------------------------------------------------------

// Base defines, must define these for the platform, there are no defaults
#ifndef PLATFORM_DESKTOP
	#error "PLATFORM_DESKTOP must be defined"
#endif
#ifndef PLATFORM_64BITS
	#error "PLATFORM_64BITS must be defined"
#endif

// Base defines, these have defaults
#ifndef PLATFORM_LITTLE_ENDIAN
	#define PLATFORM_LITTLE_ENDIAN				0
#endif
#ifndef PLATFORM_EXCEPTIONS_DISABLED
	#define PLATFORM_EXCEPTIONS_DISABLED		!PLATFORM_DESKTOP
#endif
#ifndef PLATFORM_SEH_EXCEPTIONS_DISABLED
	#define PLATFORM_SEH_EXCEPTIONS_DISABLED	0
#endif
#ifndef PLATFORM_SUPPORTS_PRAGMA_PACK
	#define PLATFORM_SUPPORTS_PRAGMA_PACK		0
#endif
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS
	#define PLATFORM_ENABLE_VECTORINTRINSICS	0
#endif
#ifndef PLATFORM_HAS_CPUID
	#if defined(_M_IX86) || defined(__i386__)
		#define PLATFORM_HAS_CPUID				1
	#else
		#define PLATFORM_HAS_CPUID				0
	#endif
#endif	
#ifndef PLATFORM_ENABLE_VECTORINTRINSICS_NEON
	#define PLATFORM_ENABLE_VECTORINTRINSICS_NEON	0
#endif
#ifndef PLATFORM_USE_LS_SPEC_FOR_WIDECHAR
	#define PLATFORM_USE_LS_SPEC_FOR_WIDECHAR	1
#endif
#ifndef PLATFORM_USE_SYSTEM_VSWPRINTF
	#define PLATFORM_USE_SYSTEM_VSWPRINTF		1
#endif
#ifndef PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG
	#define PLATFORM_COMPILER_DISTINGUISHES_INT_AND_LONG			0
#endif
#ifndef PLATFORM_COMPILER_HAS_GENERIC_KEYWORD
	#define PLATFORM_COMPILER_HAS_GENERIC_KEYWORD	0
#endif
#ifndef PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS
	#define PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS	1
#endif
#ifndef PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
	#define PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES	1
#endif
#ifndef PLATFORM_COMPILER_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS
	#define PLATFORM_COMPILER_HAS_DEFAULT_FUNCTION_TEMPLATE_ARGUMENTS	1
#endif
#ifndef PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS
	#define PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS	1
#endif
#ifndef PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION
	#define PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION 0
#endif
#ifndef PLATFORM_COMPILER_HAS_TCHAR_WMAIN
	#define PLATFORM_COMPILER_HAS_TCHAR_WMAIN 0
#endif
#ifndef PLATFORM_TCHAR_IS_4_BYTES
	#define PLATFORM_TCHAR_IS_4_BYTES			0
#endif
#ifndef PLATFORM_HAS_BSD_TIME
	#define PLATFORM_HAS_BSD_TIME				1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKETS
	#define PLATFORM_HAS_BSD_SOCKETS			1
#endif
#ifndef PLATFORM_HAS_BSD_IPV6_SOCKETS
	#define PLATFORM_HAS_BSD_IPV6_SOCKETS			0
#endif
#ifndef PLATFORM_USE_PTHREADS
	#define PLATFORM_USE_PTHREADS				1
#endif
#ifndef PLATFORM_MAX_FILEPATH_LENGTH
	#define PLATFORM_MAX_FILEPATH_LENGTH		128
#endif
#ifndef PLATFORM_SUPPORTS_TEXTURE_STREAMING
	#define PLATFORM_SUPPORTS_TEXTURE_STREAMING	1
#endif
#ifndef PLATFORM_SUPPORTS_VIRTUAL_TEXTURES
	#define PLATFORM_SUPPORTS_VIRTUAL_TEXTURES		0
#endif
#ifndef PLATFORM_REQUIRES_FILESERVER
	#define PLATFORM_REQUIRES_FILESERVER		0
#endif
#ifndef PLATFORM_SUPPORTS_MULTITHREADED_GC
	#define PLATFORM_SUPPORTS_MULTITHREADED_GC	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_IOCTL	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_SELECT	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS	0
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_GETHOSTNAME	1
#endif
#ifndef PLATFORM_HAS_BSD_SOCKET_FEATURE_CLOSE_ON_EXEC
	#define PLATFORM_HAS_BSD_SOCKET_FEATURE_CLOSE_ON_EXEC	0
#endif
#ifndef PLATFORM_HAS_NO_EPROCLIM
	#define PLATFORM_HAS_NO_EPROCLIM			0
#endif
#ifndef PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS
	#define PLATFORM_USES_MICROSOFT_LIBC_FUNCTIONS 0
#endif

#ifndef PLATFORM_SUPPORTS_DRAW_MESH_EVENTS
	#define PLATFORM_SUPPORTS_DRAW_MESH_EVENTS	1
#endif

#ifndef PLATFORM_USES_ES2
	#define PLATFORM_USES_ES2					0
#endif

#ifndef PLATFORM_SUPPORTS_TBB
	#define PLATFORM_SUPPORTS_TBB 0
#endif

#ifndef PLATFORM_SUPPORTS_JEMALLOC
	#define PLATFORM_SUPPORTS_JEMALLOC 0
#endif

#ifndef PLATFORM_CAN_SUPPORT_EDITORONLY_DATA
	#define PLATFORM_CAN_SUPPORT_EDITORONLY_DATA 0
#endif

#ifndef PLATFORM_SUPPORTS_NAMED_PIPES
	#define PLATFORM_SUPPORTS_NAMED_PIPES		0
#endif

#ifndef PLATFORM_USES_FIXED_RHI_CLASS
	#define PLATFORM_USES_FIXED_RHI_CLASS		0
#endif

#ifndef PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS
	#define PLATFORM_SUPPORTS_MULTIPLE_NATIVE_WINDOWS	1
#endif

#ifndef PLATFORM_HAS_TOUCH_MAIN_SCREEN
	#define PLATFORM_HAS_TOUCH_MAIN_SCREEN		0
#endif

#ifndef PLATFORM_HAS_128BIT_ATOMICS
	#define PLATFORM_HAS_128BIT_ATOMICS 0
#endif

#ifndef PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING
	#define PLATFORM_USES_ANSI_STRING_FOR_EXTERNAL_PROFILING 1
#endif

#ifndef PLATFORM_RHITHREAD_DEFAULT_BYPASS
	#define PLATFORM_RHITHREAD_DEFAULT_BYPASS					1
#endif

#ifndef PLATFORM_CAN_TOGGLE_RHITHREAD_IN_SHIPPING
	#define PLATFORM_CAN_TOGGLE_RHITHREAD_IN_SHIPPING			0
#endif

// deprecated, do not use
#define PLATFORM_HAS_THREADSAFE_RHIGetRenderQueryResult	#
#define PLATFORM_SUPPORTS_RHI_THREAD #
#define PLATFORM_RHI_USES_CONTEXT_OBJECT # // deprecated, do not use; all platforms must use a context object
#define PLATFORM_SUPPORTS_PARALLEL_RHI_EXECUTE # // deprecated, do not use; see GRHISupportsParallelRHIExecute


//These are deprecated old defines that we want to make sure are not used
#define CONSOLE (#)
#define MOBILE (#)
#define PLATFORM_CONSOLE (#)

// These is computed, not predefined
#define PLATFORM_32BITS					(!PLATFORM_64BITS)

// not supported by the platform system yet or maybe ever
#define PLATFORM_VTABLE_AT_END_OF_CLASS 0 

#ifndef VARARGS
	#define VARARGS					/* Functions with variable arguments */
#endif
#ifndef CDECL
	#define CDECL	    			/* Standard C function */
#endif
#ifndef STDCALL
	#define STDCALL					/* Standard calling convention */
#endif
#ifndef FORCEINLINE
	#define FORCEINLINE 			/* Force code to be inline */
#endif
#ifndef FORCENOINLINE
	#define FORCENOINLINE 			/* Force code to NOT be inline */
#endif
#ifndef RESTRICT
	#define RESTRICT __restrict		/* no alias hint */
#endif

#ifndef ASSUME						/* Hints compiler that expression is true; generally restricted to comparisons against constants */
	#define ASSUME(...) 
#endif

// Optimization macros (uses __pragma to enable inside a #define).
#ifndef PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_DISABLE_OPTIMIZATION_ACTUAL
	#define PRAGMA_ENABLE_OPTIMIZATION_ACTUAL
#endif

// Disable optimization of a specific function
#ifndef DISABLE_FUNCTION_OPTIMIZATION
	#define DISABLE_FUNCTION_OPTIMIZATION
#endif

#ifndef FORCEINLINE_DEBUGGABLE_ACTUAL
	#define FORCEINLINE_DEBUGGABLE_ACTUAL inline
#endif

#ifndef DECLARE_UINT64						
	#define DECLARE_UINT64(x) x##ULL	/* Define a 64 bit immediate int **/
#endif

// Backwater of the spec. All compilers support this except microsoft, and they will soon
#ifndef TYPENAME_OUTSIDE_TEMPLATE
	#define TYPENAME_OUTSIDE_TEMPLATE	typename
#endif

// Legacy method modifier macros.  You shouldn't use these macros in modern code.  Use the built-in keyword directly.
#ifndef OVERRIDE
	#define OVERRIDE \
		EMIT_DEPRECATED_WARNING_MESSAGE("OVERRIDE macro is deprecated. Please use override keyword instead.") \
		override
#endif
#ifndef FINAL						
	#define FINAL \
		EMIT_DEPRECATED_WARNING_MESSAGE("FINAL macro is deprecated. Please use final keyword instead.") \
		final
#endif

// Method modifiers
#ifndef ABSTRACT						
	#define ABSTRACT
#endif
#ifndef CONSTEXPR
	#define CONSTEXPR constexpr
#endif

// String constants
#ifndef LINE_TERMINATOR						
	#define LINE_TERMINATOR TEXT("\n")
#endif
#ifndef LINE_TERMINATOR_ANSI
	#define LINE_TERMINATOR_ANSI "\n"
#endif

// Alignment.
#ifndef GCC_PACK						
	#define GCC_PACK(n)
#endif
#ifndef GCC_ALIGN						
	#define GCC_ALIGN(n)
#endif
#ifndef MS_ALIGN						
	#define MS_ALIGN(n)
#endif

// MSVC pragmas - used so other platforms can remove them easily (by not defining this)
#ifndef MSVC_PRAGMA
	#define MSVC_PRAGMA(...)
#endif


// Inlining
#ifndef PRAGMA_DISABLE_INLINING
	#define PRAGMA_DISABLE_INLINING	
	#define PRAGMA_ENABLE_INLINING	
#endif

// Cache control
#ifndef FLUSH_CACHE_LINE
	#define FLUSH_CACHE_LINE(x)
#endif

// Prefetch
#ifndef CACHE_LINE_SIZE
	#define CACHE_LINE_SIZE	128
#endif

// These have to be forced inline on some OSes so the dynamic loader will not 
// resolve to our allocators for the system libraries.
#ifndef OPERATOR_NEW_INLINE
	#define OPERATOR_NEW_INLINE FORCEINLINE
#endif

#ifndef OPERATOR_NEW_THROW_SPEC
	#define OPERATOR_NEW_THROW_SPEC
#endif
#ifndef OPERATOR_DELETE_THROW_SPEC
	#define OPERATOR_DELETE_THROW_SPEC
#endif
#ifndef OPERATOR_NEW_NOTHROW_SPEC
	#define OPERATOR_NEW_NOTHROW_SPEC throw()
#endif
#ifndef OPERATOR_DELETE_NOTHROW_SPEC
	#define OPERATOR_DELETE_NOTHROW_SPEC throw()
#endif

#ifndef checkAtCompileTime
	#define checkAtCompileTime(expr, msg) \
		EMIT_DEPRECATED_WARNING_MESSAGE("checkAtCompileTime is deprecated. Please use static_assert instead.") \
		static_assert(expr, #msg)
#endif

// DLL export and import definitions
#ifndef DLLEXPORT
	#define DLLEXPORT
	#define DLLIMPORT
#endif


#ifndef DEPRECATED_FORGAME
	#define DEPRECATED_FORGAME(...)
#endif

// This is a temporary macro, will be removed when TSubobjectPtr can be safely removed
#ifndef private_subobject
#define private_subobject public
#endif

// explicit bool support
namespace FHasOperatorImpl
{
	struct FNotSpecified {};

	template <typename T>
	struct FReturnValueCheck
	{
		static char (&Func())[2];
	};

	template <>
	struct FReturnValueCheck<FNotSpecified>
	{
		static char (&Func())[1];
	};

	template <typename T>
	FNotSpecified operator==(const T&, const T&);

	template <typename T>
	FNotSpecified operator!=(const T&, const T&);

	template <typename T>
	const T& Make();

	template <typename T>
	struct Equals
	{
		enum { Value = sizeof(FReturnValueCheck<decltype(Make<T>() == Make<T>())>::Func()) == sizeof(char[2]) };
	};

	template <typename T>
	struct NotEquals
	{
		enum { Value = sizeof(FReturnValueCheck<decltype(Make<T>() != Make<T>())>::Func()) == sizeof(char[2]) };
	};
}

template <typename T>
struct THasOperatorEquals
{
	enum { Value = FHasOperatorImpl::Equals<T>::Value };
};

template <typename T>
struct THasOperatorNotEquals
{
	enum { Value = FHasOperatorImpl::NotEquals<T>::Value };
};

#if PLATFORM_COMPILER_HAS_EXPLICIT_OPERATORS
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL FORCEINLINE explicit operator bool
	#define SAFE_BOOL_OPERATORS(...)		// not needed when compiler supports explicit operator bool()
#elif PLATFORM_COMPILER_COMMON_LANGUAGE_RUNTIME_COMPILATION
	// unsafe version
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL FORCEINLINE operator bool
	#define SAFE_BOOL_OPERATORS(...)
#else
	#define FORCEINLINE_EXPLICIT_OPERATOR_BOOL \
					private: \
						template <typename TheClassType> \
						static TheClassType UE_ClassTypeHelper(const TheClassType&); \
						struct UE_PrivateBoolType \
						{ \
							int x; \
						}; \
					public: \
						FORCEINLINE operator int UE_PrivateBoolType::*() const \
						{ \
							static_assert(THasOperatorEquals   <decltype(UE_ClassTypeHelper(*this))>::Value, "Class needs operator==()."); \
							static_assert(THasOperatorNotEquals<decltype(UE_ClassTypeHelper(*this))>::Value, "Class needs operator!=()."); \
							return UE_OperatorBool() ? &UE_PrivateBoolType::x : 0; \
						} \
						FORCEINLINE bool UE_OperatorBool

	// these are left unimplemented to cause link errors if used
	#define SAFE_BOOL_OPERATORS(...)	\
			void operator ==( const __VA_ARGS__ & RHS ) const; \
			void operator !=( const __VA_ARGS__ & RHS ) const;
#endif


// Console ANSICHAR/TCHAR command line handling
#if PLATFORM_COMPILER_HAS_TCHAR_WMAIN
#define INT32_MAIN_INT32_ARGC_TCHAR_ARGV() int32 wmain(int32 ArgC, TCHAR* ArgV[])
#else
#define INT32_MAIN_INT32_ARGC_TCHAR_ARGV() \
int32 tchar_main(int32 ArgC, TCHAR* ArgV[]); \
int32 main(int32 ArgC, ANSICHAR* Utf8ArgV[]) \
{ \
	TCHAR** ArgV = new TCHAR*[ArgC]; \
	for (int32 a = 0; a < ArgC; a++) \
	{ \
		FUTF8ToTCHAR ConvertFromUtf8(Utf8ArgV[a]); \
		ArgV[a] = new TCHAR[ConvertFromUtf8.Length() + 1]; \
		FCString::Strcpy(ArgV[a], ConvertFromUtf8.Length(), ConvertFromUtf8.Get()); \
	} \
	int32 Result = tchar_main(ArgC, ArgV); \
	for (int32 a = 0; a < ArgC; a++) \
	{ \
		delete[] ArgV[a]; \
	} \
	delete[] ArgV; \
	return Result; \
} \
int32 tchar_main(int32 ArgC, TCHAR* ArgV[])
#endif

//--------------------------------------------------------------------------------------------
// POD types refactor for porting old code:
//
// Replace 'FLOAT' with 'float'
// Replace 'DOUBLE' with 'double'
// Replace 'INT' with 'int32' (make sure not to replace INT text in localization strings!)
// Replace 'UINT' with 'uint32'
// Replace 'DWORD' with 'uint32' (DWORD is still used in Windows platform source files)
// Replace 'WORD' with 'uint16'
// Replace 'USHORT' with 'uint16'
// Replace 'SWORD' with 'int16'
// Replace 'QWORD' with 'uint64'
// Replace 'SQWORD' with 'int64'
// Replace 'BYTE' with 'uint8'
// Replace 'SBYTE' with 'int8'
// Replace 'BITFIELD' with 'uint32'
// Replace 'UBOOL' with 'bool'
// Replace 'FALSE' with 'false' and 'TRUE' with true.
// Make sure any platform API uses its own bool types. 
// For example WinAPI uses BOOL, FALSE and TRUE otherwise sometimes it doesn't work properly
// or it doesn't compile.
// Replace 'ExtractUBOOLFromBitfield' with 'ExtractBoolFromBitfield'
// Replace 'SetUBOOLInBitField' with 'SetBoolInBitField'
// Replace 'ParseUBOOL' with 'FParse::Bool'
// Replace 'MINBYTE', with 'MIN_uint8'
// Replace 'MINWORD', with 'MIN_uint16'
// Replace 'MINDWORD', with 'MIN_uint32'
// Replace 'MINSBYTE', with 'MIN_int8'
// Replace 'MINSWORD', with 'MIN_int16'
// Replace 'MAXINT', with 'MAX_int32'
// Replace 'MAXBYTE', with 'MAX_uint8'
// Replace 'MAXWORD', with 'MAX_uint16'
// Replace 'MAXDWORD', with 'MAX_uint32'
// Replace 'MAXSBYTE', with 'MAX_int8'
// Replace 'MAXSWORD', with 'MAX_int16'
// Replace 'MAXINT', with 'MAX_int32'
//--------------------------------------------------------------------------------------------

//------------------------------------------------------------------
// Transfer the platform types to global types
//------------------------------------------------------------------

// Unsigned base types.
typedef FPlatformTypes::uint8		uint8;		///< An 8-bit unsigned integer.
typedef FPlatformTypes::uint16		uint16;		///< A 16-bit unsigned integer.
typedef FPlatformTypes::uint32		uint32;		///< A 32-bit unsigned integer.
typedef FPlatformTypes::uint64		uint64;		///< A 64-bit unsigned integer.

// Signed base types.
typedef	FPlatformTypes::int8		int8;		///< An 8-bit signed integer.
typedef FPlatformTypes::int16		int16;		///< A 16-bit signed integer.
typedef FPlatformTypes::int32		int32;		///< A 32-bit signed integer.
typedef FPlatformTypes::int64		int64;		///< A 64-bit signed integer.

// Character types.
typedef FPlatformTypes::ANSICHAR	ANSICHAR;	///< An ANSI character. Normally a signed type.
typedef FPlatformTypes::WIDECHAR	WIDECHAR;	///< A wide character. Normally a signed type.
typedef FPlatformTypes::TCHAR		TCHAR;		///< Either ANSICHAR or WIDECHAR, depending on whether the platform supports wide characters or the requirements of the licensee.
typedef FPlatformTypes::CHAR8		UTF8CHAR;	///< An 8-bit character containing a UTF8 (Unicode, 8-bit, variable-width) code unit.
typedef FPlatformTypes::CHAR16		UCS2CHAR;	///< A 16-bit character containing a UCS2 (Unicode, 16-bit, fixed-width) code unit, used for compatibility with 'Windows TCHAR' across multiple platforms.
typedef FPlatformTypes::CHAR16		UTF16CHAR;	///< A 16-bit character containing a UTF16 (Unicode, 16-bit, variable-width) code unit.
typedef FPlatformTypes::CHAR32		UTF32CHAR;	///< A 32-bit character containing a UTF32 (Unicode, 32-bit, fixed-width) code unit.

typedef FPlatformTypes::UPTRINT UPTRINT;		///< An unsigned integer the same size as a pointer
typedef FPlatformTypes::PTRINT PTRINT;			///< A signed integer the same size as a pointer
typedef FPlatformTypes::SIZE_T SIZE_T;			///< An unsigned integer the same size as a pointer, the same as UPTRINT
typedef FPlatformTypes::SSIZE_T SSIZE_T;		///< An integer the same size as a pointer, the same as PTRINT

typedef FPlatformTypes::TYPE_OF_NULL	TYPE_OF_NULL;		///< The type of the NULL constant.
typedef FPlatformTypes::TYPE_OF_NULLPTR	TYPE_OF_NULLPTR;	///< The type of the C++ nullptr keyword.

//------------------------------------------------------------------
// Test the global types
//------------------------------------------------------------------
namespace TypeTests
{
	template <typename A, typename B>
	struct TAreTypesEqual
	{
		enum { Value = false };
	};

	template <typename T>
	struct TAreTypesEqual<T, T>
	{
		enum { Value = true };
	};

	static_assert(!PLATFORM_TCHAR_IS_4_BYTES || sizeof(TCHAR) == 4, "TCHAR size must be 4 bytes.");
	static_assert(PLATFORM_TCHAR_IS_4_BYTES || sizeof(TCHAR) == 2, "TCHAR size must be 2 bytes.");

	static_assert(PLATFORM_32BITS || PLATFORM_64BITS, "Type tests pointer size failed.");
	static_assert(PLATFORM_32BITS != PLATFORM_64BITS, "Type tests pointer exclusive failed.");
	static_assert(!PLATFORM_64BITS || sizeof(void*) == 8, "Pointer size is 64bit, but pointers are short.");
	static_assert(PLATFORM_64BITS || sizeof(void*) == 4, "Pointer size is 32bit, but pointers are long.");

	static_assert(char(-1) < char(0), "Unsigned char type test failed.");

	static_assert((!TAreTypesEqual<ANSICHAR, WIDECHAR>::Value), "ANSICHAR and WIDECHAR should be different types.");
	static_assert((!TAreTypesEqual<ANSICHAR, UCS2CHAR>::Value), "ANSICHAR and CHAR16 should be different types.");
	static_assert((!TAreTypesEqual<WIDECHAR, UCS2CHAR>::Value), "WIDECHAR and CHAR16 should be different types.");
	static_assert((TAreTypesEqual<TCHAR, ANSICHAR>::Value || TAreTypesEqual<TCHAR, WIDECHAR>::Value), "TCHAR should either be ANSICHAR or WIDECHAR.");

	static_assert(sizeof(uint8) == 1, "BYTE type size test failed.");
	static_assert(int32(uint8(-1)) == 0xFF, "BYTE type sign test failed.");

	static_assert(sizeof(uint16) == 2, "WORD type size test failed.");
	static_assert(int32(uint16(-1)) == 0xFFFF, "WORD type sign test failed.");

	static_assert(sizeof(uint32) == 4, "DWORD type size test failed.");
	static_assert(int64(uint32(-1)) == int64(0xFFFFFFFF), "DWORD type sign test failed.");

	static_assert(sizeof(uint64) == 8, "QWORD type size test failed.");
	static_assert(uint64(-1) > uint64(0), "QWORD type sign test failed.");


	static_assert(sizeof(int8) == 1, "SBYTE type size test failed.");
	static_assert(int32(int8(-1)) == -1, "SBYTE type sign test failed.");

	static_assert(sizeof(int16) == 2, "SWORD type size test failed.");
	static_assert(int32(int16(-1)) == -1, "SWORD type sign test failed.");

	static_assert(sizeof(int32) == 4, "INT type size test failed.");
	static_assert(int64(int32(-1)) == int64(-1), "INT type sign test failed.");

	static_assert(sizeof(int64) == 8, "SQWORD type size test failed.");
	static_assert(int64(-1) < int64(0), "SQWORD type sign test failed.");

	static_assert(sizeof(ANSICHAR) == 1, "ANSICHAR type size test failed.");
	static_assert(int32(ANSICHAR(-1)) == -1, "ANSICHAR type sign test failed.");

	static_assert(sizeof(WIDECHAR) == 2 || sizeof(WIDECHAR) == 4, "WIDECHAR type size test failed.");

	static_assert(sizeof(UCS2CHAR) == 2, "UCS2CHAR type size test failed.");

	static_assert(sizeof(uint32) == 4, "BITFIELD type size test failed.");
	static_assert(int64(uint32(-1)) == int64(0xFFFFFFFF), "BITFIELD type sign test failed.");

	static_assert(sizeof(PTRINT) == sizeof(void *), "PTRINT type size test failed.");
	static_assert(PTRINT(-1) < PTRINT(0), "PTRINT type sign test failed.");

	static_assert(sizeof(UPTRINT) == sizeof(void *), "UPTRINT type size test failed.");
	static_assert(UPTRINT(-1) > UPTRINT(0), "UPTRINT type sign test failed.");

	static_assert(sizeof(SIZE_T) == sizeof(void *), "SIZE_T type size test failed.");
	static_assert(SIZE_T(-1) > SIZE_T(0), "SIZE_T type sign test failed.");
}
