/* ----------------------------------------------------------------------------
 *
 * Dual 2-BSD/MIT license. Either or both licenses can be used.
 *
 * ----------------------------------------------------------------------------
 *
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
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2018 Ruslan Nikolaev
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

#ifndef __BITS_LF_GCC_X86_H
#define __BITS_LF_GCC_X86_H 1

#include <stdatomic.h>
#include <stdint.h>

#define LFATOMIC(x)				_Atomic(x)
#define LFATOMIC_VAR_INIT(x)	ATOMIC_VAR_INIT(x)

static inline void __lfbig_init(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t val)
{
	*((volatile lfatomic_big_t *) ((uintptr_t) obj)) = val;
}

static inline lfatomic_big_t __lfbig_load(_Atomic(lfatomic_big_t) * obj,
		memory_order order)
{
	return *((volatile lfatomic_big_t *) ((uintptr_t) obj));
}

static inline bool __lfbig_cmpxchg_strong(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t * expected, lfatomic_big_t desired,
		memory_order succ, memory_order fail)
{
	lfatomic_t low = (lfatomic_t) desired;
	lfatomic_t high = (lfatomic_t) (desired >> (sizeof(lfatomic_t) * 8));
	bool result;

#if defined(__x86_64__)
# define __LFX86_CMPXCHG "cmpxchg16b"
#elif defined(__i386__)
# define __LFX86_CMPXCHG "cmpxchg8b"
#endif
	__asm__ __volatile__ ("lock " __LFX86_CMPXCHG " %0"
						  : "+m" (*obj), "=@ccz" (result), "+A" (*expected)
						  : "b" (low), "c" (high)
	);
#undef __LFX86_CMPXCHG

	return result;
}

static inline lfatomic_big_t __lfbig_load_atomic(_Atomic(lfatomic_big_t) * obj,
		memory_order order)
{
	lfatomic_big_t value = 0;
	__lfbig_cmpxchg_strong(obj, &value, 0, order, order);
	return value;
}

static inline bool __lfbig_cmpxchg_weak(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t * expected, lfatomic_big_t desired,
		memory_order succ, memory_order fail)
{
	return __lfbig_cmpxchg_strong(obj, expected, desired, succ, fail);
}

static inline lfatomic_big_t __lfbig_fetch_and(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t arg, memory_order order)
{
	lfatomic_big_t new_val, old_val = __lfbig_load(obj, order);
	do {
		new_val = old_val & arg;
	} while (!__lfbig_cmpxchg_weak(obj, &old_val, new_val, order, order));
	__LF_ASSUME(new_val == (old_val & arg));
	return old_val;
}

#define __lfaba_init			__lfbig_init
#define __lfaba_load			__lfbig_load
#define __lfaba_load_atomic		__lfbig_load_atomic
#define __lfaba_cmpxchg_weak	__lfbig_cmpxchg_weak
#define __lfaba_cmpxchg_strong	__lfbig_cmpxchg_strong
#define __lfaba_fetch_and		__lfbig_fetch_and

#endif /* !__BITS_LF_GGC_X86_H */

/* vi: set tabstop=4: */
