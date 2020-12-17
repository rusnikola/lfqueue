/*-
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
 */

#ifndef __LFRING_H
#define __LFRING_H	1

#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>

#include "lfconfig.h"

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
	__attribute__ ((aligned(LF_CACHE_BYTES))) lfatomic_t head;
	__attribute__ ((aligned(LF_CACHE_BYTES))) lfatomic_t tail;
	__attribute__ ((aligned(LF_CACHE_BYTES))) lfatomic_t array[1];
};

struct lfring;
typedef bool (*lfring_process_t) (struct lfring *, size_t, void *);

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

/* An equivalent to lfring_init_empty but avoids the reserved index 0. */
static inline void lfring_init_mark(struct lfring * ring, size_t order)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t i, n = lfring_pow2(order);

	for (i = 0; i != n; i++) {
		q->array[i] = n - 1;
	}

	q->head = n;
	q->tail = n;
}

static inline lfatomic_t lfring_head(struct lfring * ring)
{
	struct __lfring * q = (struct __lfring *) ring;
	return __atomic_load_n(&q->head, __ATOMIC_ACQUIRE);
}

static inline size_t lfring_enqueue(struct lfring * ring,
		size_t order, size_t eidx, bool nonempty)
{
	struct __lfring * q = (struct __lfring *) ring;
	size_t n = lfring_pow2(order);
	lfatomic_t tail;

start_over:
	tail = __atomic_load_n(&q->tail, __ATOMIC_ACQUIRE);

	while (1) {
		lfatomic_t tcycle = tail & ~(n - 1);
		size_t tidx = __lfring_map(tail, order, n);
		lfatomic_t entry = __atomic_load_n(&q->array[tidx], __ATOMIC_ACQUIRE);

		while (1) {
			lfatomic_t ecycle = entry & ~(n - 1);

			if (ecycle == tcycle) {
				/* Advance the tail pointer. */
				if (__atomic_compare_exchange_n(&q->tail, &tail,
						tail + 1, 0, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
					tail++;
				}
				break;
			}

			/* Wrapping around. */
			if ((lfatomic_t) (ecycle + n) != tcycle) {
				goto start_over;
			}

			/* An empty entry. */
			if (__atomic_compare_exchange_n(&q->array[tidx],
					&entry, __LFMERGE(tcycle, eidx), 0,
					__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
				/* Try to advance the tail pointer. */
				__atomic_compare_exchange_n(&q->tail, &tail, tail + 1, 1,
								__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
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
	head = __atomic_load_n(&q->head, __ATOMIC_ACQUIRE);

	do {
		lfatomic_t ecycle, hcycle = head & ~(n - 1);
		size_t hidx = __lfring_map(head, order, n);
		entry = __atomic_load_n(&q->array[hidx], __ATOMIC_ACQUIRE);
		ecycle = entry & ~(n - 1);
		if (ecycle != hcycle) {
			/* Wrapping around. */
			if (!nonempty && (lfatomic_t) (ecycle + n) == hcycle) {
				return LFRING_EMPTY;
			}
			goto start_over;
		}
	} while (!__atomic_compare_exchange_n(&q->head, &head, head + 1, 1,
							__ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE));

	return (size_t) (entry & (n - 1));
}

#endif	/* !__LFRING_H */

/* vi: set tabstop=4: */
