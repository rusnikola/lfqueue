/*
  Copyright (c) 2018, Ruslan Nikolaev
  All rights reserved.
*/

#ifndef __BITS_LFCONFIG_H
#define __BITS_LFCONFIG_H	1

#include <inttypes.h>

/* For the following architectures, it is cheaper to use split (word-atomic)
   loads whenever possible. */
#if defined(__i386__) || defined(__x86_64__) || defined(__arm__) ||	\
	defined(__aarch64__)
# define __LFLOAD_SPLIT(dtype_width)	(dtype_width > LFATOMIC_WIDTH)
#else
# define __LFLOAD_SPLIT(dtype_width)	0
#endif

/* IA-64 provides a 128-bit single-compare/double-swap instruction, so
   LFCMPXCHG_SPLIT is true for 128-bit types. */
#if defined(__ia64__)
# define __LFCMPXCHG_SPLIT(dtype_width)	(dtype_width > LFATOMIC_WIDTH)
#else
# define __LFCMPXCHG_SPLIT(dtype_width)	0
#endif

#if defined(__x86_64__) || defined (__aarch64__) || defined(__powerpc64__)	\
	|| (defined(__mips__) && _MIPS_SIM == _MIPS_SIM_ABI64)
typedef uint64_t lfepoch_t;
typedef int64_t lfepoch_signed_t;
typedef int64_t lfsatomic_t;
typedef uint64_t lfatomic_t;
typedef __uint128_t lfatomic_big_t;
# define LFATOMIC_LOG2			3
# define LFATOMIC_WIDTH			64
# define LFATOMIC_BIG_WIDTH		128
#elif defined(__i386__) || defined(__arm__) || defined(__powerpc__)			\
	|| (defined(__mips__) &&												\
		(_MIPS_SIM == _MIPS_SIM_ABI32 || _MIPS_SIM == _MIPS_SIM_NABI32))
# if defined(__i386__)
typedef uint64_t lfepoch_t; /* Still makes sense to use 64-bit numbers. */
typedef int64_t lfepoch_signed_t;
# else
typedef uint32_t lfepoch_t;
typedef int32_t lfepoch_signed_t;
# endif
typedef int32_t lfsatomic_t;
typedef uint32_t lfatomic_t;
typedef uint64_t lfatomic_big_t;
# define LFATOMIC_LOG2			2
# define LFATOMIC_WIDTH			32
# define LFATOMIC_BIG_WIDTH		64
#else
typedef uintptr_t lfepoch_t;
typedef intptr_t lfsatomic_t;
typedef uintptr_t lfatomic_t;
typedef uintptr_t lfatomic_big_t;
# if UINTPTR_MAX == UINT32_C(0xFFFFFFFF)
#  define LFATOMIC_LOG2			2
#  define LFATOMIC_WIDTH		32
#  define LFATOMIC_BIG_WIDTH	32
# elif UINTPTR_MAX == UINT64_C(0xFFFFFFFFFFFFFFFF)
#  define LFATOMIC_LOG2			3
#  define LFATOMIC_WIDTH		64
#  define LFATOMIC_BIG_WIDTH	64
# endif
#endif

/* XXX: True for x86/x86-64 but needs to be properly defined for other CPUs. */
#define LF_CACHE_SHIFT		7U
#define LF_CACHE_BYTES		(1U << LF_CACHE_SHIFT)

/* Allow to use LEA for x86/x86-64. */
#if defined(__i386__) || defined(__x86_64__)
# define __LFMERGE(x,y)	((x) + (y))
#else
# define __LFMERGE(x,y)	((x) | (y))
#endif

#endif /* !__BITS_LFCONFIG_H */

/* vi: set tabstop=4: */
