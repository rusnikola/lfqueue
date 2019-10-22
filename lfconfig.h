/*-
 * Copyright (c) 2018 Ruslan Nikolaev.  All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef __BITS_LFCONFIG_H
#define __BITS_LFCONFIG_H	1

/* For the following architectures, it is cheaper to use split (word-atomic)
   loads whenever possible. */
#if defined(__i386__) || defined(__x86_64__) ||	defined (__arm__) ||	\
	defined (__aarch64__)
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

#if defined(__x86_64__) || defined (__aarch64__) || defined(__powerpc64__)
typedef uint64_t lfatomic_t;
typedef int64_t lfsatomic_t;
typedef __uint128_t lfatomic_big_t;
# define LFATOMIC_LOG2			3
# define LFATOMIC_WIDTH			64
# define LFATOMIC_BIG_WIDTH		128
#elif defined(__i386__) || defined(__arm__) || defined(__powerpc__)
typedef uint32_t lfatomic_t;
typedef int32_t lfsatomic_t;
typedef uint64_t lfatomic_big_t;
# define LFATOMIC_LOG2			2
# define LFATOMIC_WIDTH			32
# define LFATOMIC_BIG_WIDTH		64
#else
typedef uintptr_t lfatomic_t;
typedef intptr_t lfsatomic_t;
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

/* XXX: True for x86/x86-64 but needs to be defined for other CPUs. */
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
