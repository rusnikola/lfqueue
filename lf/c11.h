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

#ifndef __BITS_LF_C11_H
#define __BITS_LF_C11_H 1

#include <stdatomic.h>
#include <stdint.h>

#define LFATOMIC(x)				_Atomic(x)
#define LFATOMIC_VAR_INIT(x)	ATOMIC_VAR_INIT(x)

static inline void __lfaba_init(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t val)
{
	atomic_init(obj, val);
}

static inline lfatomic_big_t __lfaba_load(_Atomic(lfatomic_big_t) * obj,
		memory_order order)
{
#if __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 1
	lfatomic_big_t res;
	_Atomic(lfatomic_t) * hobj = (_Atomic(lfatomic_t) *) ((uintptr_t) obj);
	lfatomic_t * hres = (lfatomic_t *) &res;

	hres[0] = atomic_load_explicit(hobj, order);
	hres[1] = atomic_load_explicit(hobj + 1, order);
	return res;
#elif __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 0
	return atomic_load_explicit(obj, order);
#endif
}

static inline lfatomic_big_t __lfaba_load_atomic(_Atomic(lfatomic_big_t) * obj,
		memory_order order)
{
	return atomic_load_explicit(obj, order);
}

static inline bool __lfaba_cmpxchg_weak(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t * expected, lfatomic_big_t desired,
		memory_order succ, memory_order fail)
{
	return atomic_compare_exchange_weak_explicit(obj, expected, desired,
					succ, fail);
}

static inline bool __lfaba_cmpxchg_strong(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t * expected, lfatomic_big_t desired,
		memory_order succ, memory_order fail)
{
	return atomic_compare_exchange_strong_explicit(obj, expected, desired,
					succ, fail);
}

static inline lfatomic_big_t __lfaba_fetch_and(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t arg, memory_order order)
{
	return atomic_fetch_and_explicit(obj, arg, order);
}

#endif /* !__BITS_LF_C11_H */

/* vi: set tabstop=4: */
