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

#ifndef __LFRING_H
#define __LFRING_H	1

#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <stdio.h>

#include "lf/lf.h"

#if LFATOMIC_WIDTH == 32
# define LFRING_PTR_MIN	(LF_CACHE_SHIFT - 3)
#elif LFATOMIC_WIDTH == 64
# define LFRING_PTR_MIN	(LF_CACHE_SHIFT - 4)
#elif LFATOMIC_WIDTH == 128
# define LFRING_PTR_MIN	(LF_CACHE_SHIFT - 5)
#else
# error "Unsupported LFATOMIC_WIDTH."
#endif

#define LFRING_PTR_ALIGN	(_Alignof(struct __lfring_ptr))
#define LFRING_PTR_SIZE(o)	\
	(offsetof(struct __lfring_ptr, array) + (sizeof(lfatomic_big_t) << ((o) + 1)))

#define __lfring_cmp(x, op, y)	((lfsatomic_t) ((x) - (y)) op 0)

#if LFRING_PTR_MIN != 0
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (((idx & (n - 1)) >> (order + 1 - LFRING_PTR_MIN)) |
			((idx << LFRING_PTR_MIN) & (n - 1)));
}
#else
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (idx & (n - 1));
}
#endif

#define __lfring_threshold4(n) ((long) (2 * (n) - 1))

static inline size_t lfring_pow2(size_t order)
{
	return (size_t) 1U << order;
}

struct __lfring_ptr {
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) head;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfsatomic_t) threshold;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) tail;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_big_t) array[1];
};

struct lfring_ptr;

#if defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN)
# define __lfring_array_pointer(x)	((_Atomic(lfatomic_t) *) (x))
# define __lfring_array_entry(x)	((_Atomic(lfatomic_t) *) (x) + 1)
#else
# define __lfring_array_pointer(x)	((_Atomic(lfatomic_t) *) (x) + 1)
# define __lfring_array_entry(x)	((_Atomic(lfatomic_t) *) (x))
#endif

#define __lfring_entry(x)	((lfatomic_t) (((x) & __lfaba_mask) >>	\
								__lfaba_shift))
#define __lfring_pointer(x)	((lfatomic_t) (((x) & ~__lfaba_mask) >>	\
								__lfaptr_shift))
#define __lfring_pair(e,p)	(((lfatomic_big_t) (e) << __lfaba_shift) |	\
								((lfatomic_big_t) (p) << __lfaptr_shift))

static inline void lfring_ptr_init_empty(struct lfring_ptr * ring, size_t order)
{
	struct __lfring_ptr * q = (struct __lfring_ptr *) ring;
	size_t i, n = lfring_pow2(order + 1);

	for (i = 0; i != n; i++)
		__lfaba_init(&q->array[i], 0);

	atomic_init(&q->head, n);
	atomic_init(&q->threshold, -1);
	atomic_init(&q->tail, n);
}

static inline void lfring_ptr_init_lhead(lfatomic_t *lhead, size_t order)
{
	*lhead = lfring_pow2(order + 1);
}

static inline bool lfring_ptr_enqueue(struct lfring_ptr * ring, size_t order,
		void * ptr, bool nonempty, bool nonfull, lfatomic_t *lhead)
{
	struct __lfring_ptr * q = (struct __lfring_ptr *) ring;
	size_t tidx, n = lfring_pow2(order + 1);
	lfatomic_t tail, entry, ecycle, tcycle;
	lfatomic_big_t pair;

	if (!nonfull) {
		tail = atomic_load(&q->tail);
		if (tail >= *lhead + n) {
			*lhead = atomic_load(&q->head);
			if (tail >= *lhead + n)
				return false;
		}
	}

	while (1) {
		tail = atomic_fetch_add_explicit(&q->tail, 1, memory_order_acq_rel);
		tcycle = tail & ~(lfatomic_t) (n - 1);
		tidx = __lfring_map(tail, order, n);
		pair = __lfaba_load(&q->array[tidx], memory_order_acquire);
retry:
		entry = __lfring_entry(pair);
		ecycle = entry & ~(lfatomic_t) (n - 1);
		if (__lfring_cmp(ecycle, <, tcycle) && (entry == ecycle ||
				(entry == (ecycle | 0x2) && atomic_load_explicit(&q->head,
				 memory_order_acquire) <= tail))) {

			if (!__lfaba_cmpxchg_weak(&q->array[tidx],
					&pair, __lfring_pair(tcycle | 0x1, (lfatomic_t) ptr),
					memory_order_acq_rel, memory_order_acquire))
				goto retry;

			if (!nonempty && atomic_load(&q->threshold) != __lfring_threshold4(n))
				atomic_store(&q->threshold, __lfring_threshold4(n));

			return true;
		}

		if (!nonfull) {
			if (tail + 1 >= *lhead + n) {
				*lhead = atomic_load(&q->head);
				if (tail + 1 >= *lhead + n)
					return false;
			}
		}
	}
}

static inline void __lfring_ptr_catchup(struct lfring_ptr * ring,
	lfatomic_t tail, lfatomic_t head)
{
	struct __lfring_ptr * q = (struct __lfring_ptr *) ring;

	while (!atomic_compare_exchange_weak_explicit(&q->tail, &tail, head,
			memory_order_acq_rel, memory_order_acquire)) {
		head = atomic_load_explicit(&q->head, memory_order_acquire);
		tail = atomic_load_explicit(&q->tail, memory_order_acquire);
		if (__lfring_cmp(tail, >=, head))
			break;
	}
}

static inline bool lfring_ptr_dequeue(struct lfring_ptr * ring, size_t order,
		void ** ptr, bool nonempty)
{
	struct __lfring_ptr * q = (struct __lfring_ptr *) ring;
	size_t hidx, n = lfring_pow2(order + 1);
	lfatomic_t head, entry, entry_new, ecycle, hcycle, tail;
	lfatomic_big_t pair;

	if (!nonempty && atomic_load(&q->threshold) < 0) {
		return false;
	}

	while (1) {
		head = atomic_fetch_add_explicit(&q->head, 1, memory_order_acq_rel);
		hcycle = head & ~(lfatomic_t) (n - 1);
		hidx = __lfring_map(head, order, n);
		entry = atomic_load_explicit(__lfring_array_entry(&q->array[hidx]),
					memory_order_acquire);
		do {
			ecycle = entry & ~(lfatomic_t) (n - 1);
			if (ecycle == hcycle) {
				pair = __lfaba_fetch_and(&q->array[hidx],
					__lfring_pair(~(lfatomic_t) 0x1, 0), memory_order_acq_rel);
				*ptr = (void *) __lfring_pointer(pair);
				return true;
			}
			if ((entry & (~(lfatomic_t) 0x2)) != ecycle) {
				entry_new = entry | 0x2;
				if (entry == entry_new)
					break;
			} else {
				entry_new = hcycle | (entry & 0x2);
			}
		} while (__lfring_cmp(ecycle, <, hcycle) &&
				!atomic_compare_exchange_weak_explicit(
				__lfring_array_entry(&q->array[hidx]),
				&entry, entry_new,
				memory_order_acq_rel, memory_order_acquire));

		if (!nonempty) {
			tail = atomic_load_explicit(&q->tail, memory_order_acquire);
			if (__lfring_cmp(tail, <=, head + 1)) {
				__lfring_ptr_catchup(ring, tail, head + 1);
				atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel);
				return false;
			}
			if (atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel) <= 0)
				return false;
		}
	}
}

#endif	/* !__LFRING_H */

/* vi: set tabstop=4: */
