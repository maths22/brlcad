/*                        C O M M O N . H
 * BRL-CAD
 *
 * Copyright (c) 2004-2010 United States Government as represented by
 * the U.S. Army Research Laboratory.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this file; see the file named COPYING for more
 * information.
 */
/** @addtogroup fixme */
/** @{ */
/** @file common.h
 *
 * @brief
 *  Header file for the BRL-CAD common definitions.
 *
 *  This header wraps the system-specific encapsulation of
 *  brlcad_config.h and removes need to conditionally include
 *  brlcad_config.h everywhere based on HAVE_CONFIG_H.  The common
 *  definitions are symbols common to the platform being built that
 *  are either detected via configure or hand crafted, as is the case
 *  for the win32 platform.
 *
 */

#ifndef __COMMON_H__
#define __COMMON_H__

/* include the venerable config.h file.  use a pregenerated one for
 * windows when we cannot autogenerate it easily. do not include
 * config.h if this file has been installed.  (public header files
 * should not use config defines)
 */
#if defined(BRLCADBUILD) && defined(HAVE_CONFIG_H)
#  if defined(_WIN32) && !defined(__CYGWIN__)
#    include "config_win.h"
#  else
#    include "brlcad_config.h"
#  endif  /* _WIN32 */

/* Simulates drand48() functionality using rand() which is assumed to
 * exist everywhere. The range is [0, 1).
 */
#  ifndef HAVE_DRAND48
#    define drand48() ((double)rand() / (double)(RAND_MAX + 1))
#    define HAVE_DRAND48 1
#  endif

#endif  /* BRLCADBUILD & HAVE_CONFIG_H */

/* provide declaration markers for header externals */
#ifdef __cplusplus
#  define __BEGIN_DECLS   extern "C" {
#  define __END_DECLS     }
#else
#  define __BEGIN_DECLS
#  define __END_DECLS
#endif

/* Functions local to one file should be declared HIDDEN.  This is
 * sometimes helpful to debuggers.
 */
#if !defined(HIDDEN)
#  if defined(NDEBUG)
#    define HIDDEN	static
#  else
#    define HIDDEN	/***/
#  endif
#endif

/* ansi c89 does not allow the 'inline' keyword */
#ifdef __STRICT_ANSI__
#  ifndef inline
#    define inline /***/
#  endif
#endif

#ifndef FMAX
#  define FMAX(a, b)	(((a)>(b))?(a):(b))
#endif
#ifndef FMIN
#  define FMIN(a, b)	(((a)<(b))?(a):(b))
#endif

/* C99 does not provide a ssize_t even though it is provided by SUS97.
 * regardless, we use it so make sure it's declared by using the
 * similar POSIX ptrdiff_t type.
 */
#ifndef HAVE_SSIZE_T
#  ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#  endif
#  include <limits.h>
#  include <stddef.h>
#  ifndef SSIZE_MAX
typedef ptrdiff_t ssize_t;
#    define HAVE_SSIZE_T 1
#  endif
#endif

/* C99 says uintptr_t is an optional type, so make sure something is
 * provided.  make sure all of the C99 stdint types are provided.
 *
 * make sure we get the limit macros for C++ as well even though
 * they're not in the standard via __STDC_LIMIT_MACROS.
 */
 */
#if !defined(HAVE_UINTPTR_T) && !defined(uintptr_t)
#  ifndef __STDC_LIMIT_MACROS
#    define __STDC_LIMIT_MACROS 1
#  endif
#  ifdef HAVE_STDINT_H
#    include <stdint.h>
#  endif
#  ifdef HAVE_INTTYPES_H
#    include <inttypes.h>
#  endif
#  if !defined(UINTPTR_MAX)
#    if defined(_I64_MIN)
typedef __int8 int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;
typedef __int64 intmax_t;
typedef unsigned __int8 uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#      if !defined(_UINTPTR_T_DEFINED)
typedef uint64_t uintptr_t;
#      endif
#    else
typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef int64_t intmax_t;
typedef unsigned char uint8_t;
typedef unsigned short  uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
#      if !defined(_UINTPTR_T_DEFINED)
typedef unsigned long long uintptr_t;
#      endif
#    endif
#    define HAVE_INT8_T 1
#    define HAVE_INT16_T 1
#    define HAVE_INT32_T 1
#    define HAVE_INT64_T 1
#    define HAVE_UINT8_T 1
#    define HAVE_UINT16_T 1
#    define HAVE_UINT32_T 1
#    define HAVE_UINT64_T 1
#    define HAVE_INTMAX_T 1
#    define HAVE_UINTPTR_T 1
#  endif
#endif

#endif  /* __COMMON_H__ */
/** @} */
/*
 * Local Variables:
 * mode: C
 * tab-width: 8
 * indent-tabs-mode: t
 * c-file-style: "stroustrup"
 * End:
 * ex: shiftwidth=4 tabstop=8
 */
