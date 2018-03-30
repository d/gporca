//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2008 Greenplum, Inc.
//
//	@filename:
//		types.h
//
//	@doc:
//		Type definitions for gpos to avoid using native types directly;
//
//		TODO: 03/15/2008; the seletion of basic types which then
//		get mapped to internal types should be done by autoconf or other
//		external tools; for the time being these are hard-coded; cpl asserts
//		are used to make sure things are failing if compiled with a different
//		compiler.
//---------------------------------------------------------------------------
#ifndef GPOS_types_H
#define GPOS_types_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include <sys/types.h>

#include <iostream>

#include "gpos/assert.h"

#define GPOS_SIZEOF(x)		((gpos::ULONG) sizeof(x))
#define GPOS_ARRAY_SIZE(x)	(GPOS_SIZEOF(x) / GPOS_SIZEOF(x[0]))
#define GPOS_OFFSET(T, M)	((gpos::ULONG) (SIZE_T)&(((T*)0x1)->M)-1)

/* wide character string literate */
#define GPOS_WSZ_LIT(x)		L##x

// failpoint simulation is enabled on debug build
#ifdef GPOS_DEBUG
#define GPOS_FPSIMULATOR 1
#endif // GPOS_DEBUG

namespace gpos
{

	// Basic types to be used instead of built-ins
	// Add types as needed;
	
	typedef unsigned char BYTE;
	typedef char CHAR;
	// ignore signed char for the moment

	// wide character type
	typedef wchar_t WCHAR;
		
	typedef bool BOOL;

	// numeric types
	
	typedef size_t SIZE_T;
	typedef ssize_t SSIZE_T;
	typedef mode_t MODE_T;

	// define ULONG,ULLONG as types which implement standard's
	// requirements for GPOS_ULONG_MAX and GPOS_ULLONG_MAX; eliminate standard's slack
	// by fixed sizes rather than min requirements

	typedef uint32_t ULONG;
	GPOS_CPL_ASSERT(4 == sizeof(ULONG));

#ifdef GPOS_ULONG_MAX
#error "GPOS_ULONG_MAX already defined. It's supposed to be defined in gpos/types.h"
#endif  // GPOS_ULONG_MAX
#define GPOS_ULONG_MAX		((::gpos::ULONG)-1)

	typedef uint64_t ULLONG;
	GPOS_CPL_ASSERT(8 == sizeof(ULLONG));
#ifdef GPOS_ULLONG_MAX
#error "GPOS_ULLONG_MAX already defined. It's supposed to be defined in gpos/types.h"
#endif  // GPOS_ULLONG_MAX
#define GPOS_ULLONG_MAX		((::gpos::ULLONG)-1)

	typedef uintptr_t	ULONG_PTR;
#ifdef GPOS_32BIT
#define ULONG_PTR_MAX (GPOS_ULONG_MAX)
#else
#define ULONG_PTR_MAX (GPOS_ULLONG_MAX)
#endif

	typedef uint16_t USINT;
	typedef int16_t SINT;
	typedef int32_t INT;
	typedef int64_t LINT;
	typedef intptr_t INT_PTR;

	GPOS_CPL_ASSERT(2 == sizeof(USINT));
	GPOS_CPL_ASSERT(2 == sizeof(SINT));
	GPOS_CPL_ASSERT(4 == sizeof(INT));
	GPOS_CPL_ASSERT(8 == sizeof(LINT));

#ifdef GPOS_INT_MAX
#error "GPOS_INT_MAX already defined. It's supposed to be defined in gpos/types.h"
#endif  // GPOS_INT_MAX
#define GPOS_INT_MAX ((::gpos::INT)(GPOS_ULONG_MAX >> 1))

#ifdef GPOS_INT_MIN
#error "GPOS_INT_MIN already defined. It's supposed to be defined in gpos/types.h"
#endif  // GPOS_INT_MIN
#define GPOS_INT_MIN (-GPOS_INT_MAX - 1)

#ifdef LINT_MAX
#undef LINT_MAX
#endif // LINT_MAX
#define LINT_MAX ((::gpos::LINT)(GPOS_ULLONG_MAX >> 1))

#ifdef LINT_MIN
#undef LINT_MIN
#endif // LINT_MIN
#define LINT_MIN		(-LINT_MAX - 1)

#ifdef USINT_MAX
#undef USINT_MAX
#endif // USINT_MAX
#define USINT_MAX ((::gpos::USINT) -1)

#ifdef SINT_MAX
#undef SINT_MAX
#endif // SINT_MAX
#define SINT_MAX ((::gpos::SINT)(USINT_MAX >> 1))

#ifdef SINT_MIN
#undef SINT_MIN
#endif // SINT_MIN
#define SINT_MIN		(-SINT_MAX - 1)

	typedef double DOUBLE;

	typedef void * VOID_PTR;

	// holds for all platforms
	GPOS_CPL_ASSERT(sizeof(ULONG_PTR) == sizeof(void*));

	// variadic parameter list type
	typedef va_list VA_LIST;

	// wide char ostream
 	typedef std::basic_ostream<WCHAR, std::char_traits<WCHAR> >  WOSTREAM;
 	typedef std::ios_base IOS_BASE;

 	// bad allocation exception
 	typedef std::bad_alloc BAD_ALLOC;

 	// no throw type
 	typedef std::nothrow_t NO_THROW;

	// enum for results on OS level (instead of using a global error variable)
	enum GPOS_RESULT
	{
		GPOS_OK = 0,
		
		GPOS_FAILED = 1,
		GPOS_OOM = 2,
		GPOS_NOT_FOUND = 3,
		GPOS_TIMEOUT = 4
	};


	// enum for locale encoding
	enum ELocale
	{
		ELocInvalid,	// invalid key for hashtable iteration
		ElocEnUS_Utf8,
		ElocGeDE_Utf8,
		
		ElocSentinel
	};

}

#endif // !GPOS_types_H

// EOF

