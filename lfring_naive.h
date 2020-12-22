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
#include <stdatomic.h>
#include <inttypes.h>
#include <sys/types.h>

#include "lf/lf.h"

#if LFATOMIC_WIDTH == 32
# define LFRING_MIN	(LF_CACHE_SHIFT - 2)
#elif LFATOMIC_WIDTH == 64
# define LFRING_MIN	(LF_CACHE_SHIFT - 3)
#elif LFATOMIC_WIDTH == 128
# define LFRING_MIN	(LF_CACHE_SHIFT - 4)
#else
# error "Unsupported LFATOMIC_WIDTH."
#endif

#define LFRING_ALIGN	(_Alignof(struct __lfring))
#define LFRING_SIZE(o)	\
	(offsetof(struct __lfring, array) + (sizeof(lfatomic_t) << (o)))

#define LFRING_EMPTY	(~(size_t) 0U)

#if LFRING_MIN != 0
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (((idx & (n - 1)) >> (order - LFRING_MIN)) |
			((idx << LFRING_MIN) & (n - 1)));
}
#else
static inline size_t __lfring_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (idx & (n - 1));
}
#endif

static inline size_t lfring_pow2(size_t order)
{
	return (size_t) 1U << order;
}

struct __lfring {
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) head;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) tail;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_t) array[1];
};

struct lfring;

static inline void lfring_init_empty(struct lfring * ring, size_t order)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t i, n = lfring_pow2(order);

	for (i = 0; i != n; i++) {
		q->array[i] = 0;
	}

	q->head = n;
	q->tail = n;
}

static inline void lfring_init_full(struct lfring * ring, size_t order)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t i, n = lfring_pow2(order);

	for (i = 0; i != n; i++) {
		q->array[i] = i;
	}

	q->head = 0;
	q->tail = n;
}

static inline void lfring_init_fill(struct lfring * ring,
		size_t s, size_t e, size_t order)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t i, n = lfring_pow2(order);

	for (i = 0; i != s; i++) {
		q->array[__lfring_map(i, order, n)] = 0;
	}
	for (; i != e; i++) {
		q->array[__lfring_map(i, order, n)] = i;
	}
	for (; i != n; i++) {
		q->array[__lfring_map(i, order, n)] = (lfatomic_t) -n;
	}
	q->head = s;
	q->tail = e;
}

static inline size_t lfring_enqueue(struct lfring * ring,
		size_t order, size_t eidx, bool nonempty)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t n = lfring_pow2(order);
	lfatomic_t tail;

start_over:
	tail = atomic_load_explicit(&q->tail, memory_order_acquire);

	while (1) {
		lfatomic_t tcycle = tail & ~(n - 1);
		size_t tidx = __lfring_map(tail, order, n);
		lfatomic_t entry = atomic_load_explicit(&q->array[tidx], memory_order_acquire);

		while (1) {
			lfatomic_t ecycle = entry & ~(n - 1);

			if (ecycle == tcycle) {
				/* Advance the tail pointer. */
				if (atomic_compare_exchange_strong_explicit(&q->tail, &tail,
						tail + 1, memory_order_acq_rel, memory_order_acquire)) {
					tail++;
				}
				break;
			}

			/* Wrapping around. */
			if ((lfatomic_t) (ecycle + n) != tcycle) {
				goto start_over;
			}

			/* An empty entry. */
			if (atomic_compare_exchange_strong_explicit(&q->array[tidx],
					&entry, __LFMERGE(tcycle, eidx),
					memory_order_acq_rel, memory_order_acquire)) {
				/* Try to advance the tail pointer. */
				atomic_compare_exchange_weak_explicit(&q->tail, &tail, tail + 1,
					memory_order_acq_rel, memory_order_acquire);
				return entry & (n - 1);
			}
		}
	}
}

static inline size_t lfring_dequeue(struct lfring * ring, size_t order,
		bool nonempty)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t n = lfring_pow2(order);
	lfatomic_t head, entry;

start_over:
	head = atomic_load_explicit(&q->head, memory_order_acquire);

	do {
		lfatomic_t ecycle, hcycle = head & ~(n - 1);
		size_t hidx = __lfring_map(head, order, n);
		entry = atomic_load_explicit(&q->array[hidx], memory_order_acquire);
		ecycle = entry & ~(n - 1);
		if (ecycle != hcycle) {
			/* Wrapping around. */
			if (!nonempty && (lfatomic_t) (ecycle + n) == hcycle) {
				return LFRING_EMPTY;
			}
			goto start_over;
		}
	} while (!atomic_compare_exchange_weak_explicit(&q->head, &head, head + 1,
				memory_order_acq_rel, memory_order_acquire));

	return (size_t) (entry & (n - 1));
}

#endif	/* !__LFRING_H */

/* vi: set tabstop=4: */
