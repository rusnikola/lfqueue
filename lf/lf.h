/* ----------------------------------------------------------------------------
 *
 * Dual 2-BSD/MIT license. Either or both licenses can be used.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Ruslan Nikolaev.  All Rights Reserved.
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
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2019 Ruslan Nikolaev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#ifndef __BITS_LF_H
#define __BITS_LF_H	1

#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config.h"

#ifdef __GNUC__
# define __LF_ASSUME(c) do { if (!(c)) __builtin_unreachable(); } while (0)
#else
# define __LF_ASSUME(c)
#endif

/* GCC does not have a sane implementation of wide atomics for x86-64
   in recent versions, so use inline assembly workarounds whenever possible.
   No aarch64 support in GCC for right now. */
#if (defined(__i386__) || defined(__x86_64__)) && defined(__GNUC__) &&	\
	!defined(__llvm__) && defined(__GCC_ASM_FLAG_OUTPUTS__)
# include "gcc_x86.h"
#else
# include "c11.h"
#endif

/* ABA tagging with split (word-atomic) load/cmpxchg operation. */
#if __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 1 ||								\
		__LFCMPXCHG_SPLIT(LFATOMIC_BIG_WIDTH) == 1
# define __LFABA_IMPL(w, type_t)											\
static const size_t __lfaba_shift##w = sizeof(lfatomic_big_t) * 4;			\
static const size_t __lfaptr_shift##w = 0;									\
static const lfatomic_big_t __lfaba_mask##w =								\
				~(lfatomic_big_t) 0U << (sizeof(lfatomic_big_t) * 4);		\
static const lfatomic_big_t __lfaba_step##w =								\
				(lfatomic_big_t) 1U << (sizeof(lfatomic_big_t) * 4);
#endif

/* ABA tagging when load/cmpxchg is not split. Note that unlike previous
   case, __lfaptr_shift is required to be 0. */
#if __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 0 &&								\
		__LFCMPXCHG_SPLIT(LFATOMIC_BIG_WIDTH) == 0
# define __LFABA_IMPL(w, type_t)											\
static const size_t __lfaba_shift##w = sizeof(type_t) * 8;					\
static const size_t __lfaptr_shift##w = 0;									\
static const lfatomic_big_t __lfaba_mask##w =								\
				~(lfatomic_big_t) 0U << (sizeof(type_t) * 8);				\
static const lfatomic_big_t __lfaba_step##w =								\
				(lfatomic_big_t) 1U << (sizeof(type_t) * 8);
#endif

/* Available on CAS2 32/64-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 2 * __LFPTR_WIDTH
__LFABA_IMPL(, uintptr_t)
#endif

#endif	/* !__BITS_LF_H */

/* vi: set tabstop=4: */
