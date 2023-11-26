/* ----------------------------------------------------------------------------
 *
 * Dual 2-BSD/MIT license. Either or both licenses can be used.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2021 Ruslan Nikolaev.  All Rights Reserved.
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
 * Copyright (c) 2021 Ruslan Nikolaev
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

#ifndef __WFRING_H
#define __WFRING_H	1

#include <stdbool.h>
#include <inttypes.h>
#include <sys/types.h>
#include <stdatomic.h>
#include <stdio.h>

#include "lf/lf.h"

#if LFATOMIC_WIDTH == 32
# define WFRING_MIN	(LF_CACHE_SHIFT - 3)
#elif LFATOMIC_WIDTH == 64
# define WFRING_MIN	(LF_CACHE_SHIFT - 4)
#elif LFATOMIC_WIDTH == 128
# define WFRING_MIN	(LF_CACHE_SHIFT - 5)
#else
# error "Unsupported LFATOMIC_WIDTH."
#endif

#define WFRING_PATIENCE_ENQ 16
#define WFRING_PATIENCE_DEQ 64
#define WFRING_DELAY		16

#define WFRING_ALIGN	(_Alignof(struct __wfring))
#define WFRING_SIZE(o)	\
	(offsetof(struct __wfring, array) + (sizeof(lfatomic_big_t) << ((o) + 1)))

#define WFRING_EMPTY    (~(size_t) 0U)

#define __wfring_cmp(x, op, y)	((lfsatomic_t) ((x) - (y)) op 0)

#if WFRING_MIN != 0
static inline size_t __wfring_raw_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (((idx & (n - 1)) >> (order - WFRING_MIN)) |
			((idx << WFRING_MIN) & (n - 1)));
}
#else
static inline size_t __wfring_raw_map(lfatomic_t idx, size_t order, size_t n)
{
	return (size_t) (idx & (n - 1));
}
#endif

static inline size_t __wfring_map(lfatomic_t idx, size_t order, size_t n)
{
	return __wfring_raw_map(idx, order + 1, n);
}

#define __wfring_threshold3(half, n) ((long) ((half) + (n) - 1))

#if defined(__LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN)
# define __wfring_pair_addon(x)	((_Atomic(lfatomic_t) *) (x))
# define __wfring_pair_entry(x)	((_Atomic(lfatomic_t) *) (x) + 1)
#else
# define __wfring_pair_addon(x)	((_Atomic(lfatomic_t) *) (x) + 1)
# define __wfring_pair_entry(x)	((_Atomic(lfatomic_t) *) (x))
#endif

#define __wfring_entry(x)	((lfatomic_t) (((x) & __lfaba_mask) >>	\
								__lfaba_shift))
#define __wfring_addon(x)	((lfatomic_t) (((x) & ~__lfaba_mask) >>	\
								__lfaptr_shift))
#define __wfring_pair(e,c)	(((lfatomic_big_t) (e) << __lfaba_shift) |	\
					((lfatomic_big_t) ((lfatomic_t) (c)) << __lfaptr_shift))

#define __WFRING_FIN 0x1
#define __WFRING_INC 0x2

#define __WFRING_EIDX_TERM 0
#define __WFRING_EIDX_DEQ  1

static inline size_t wfring_pow2(size_t order)
{
	return (size_t) 1U << order;
}

struct __wfring {
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_big_t) head;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfsatomic_t) threshold;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_big_t) tail;
	__attribute__ ((aligned(LF_CACHE_BYTES))) _Atomic(lfatomic_big_t) array[1];
};

struct wfring;

struct wfring_phase2 {
	_Atomic(lfatomic_t) seq1;
	_Atomic(lfatomic_t) *local;
	lfatomic_t cnt;
	_Atomic(lfatomic_t) seq2;
};

struct wfring_state {
	__attribute__ ((aligned(LF_CACHE_BYTES)))
		_Atomic(struct wfring_state *) next;
	size_t nextCheck;
	struct wfring_state * currThread;

	struct wfring_phase2 phase2;

	_Atomic(lfatomic_t) seq1;
	_Atomic(lfatomic_t) tail;
	lfatomic_t initTail;
	_Atomic(lfatomic_t) head;
	lfatomic_t initHead;
	_Atomic(size_t) eidx;
	_Atomic(lfatomic_t) seq2;
};

static inline void wfring_init_empty(struct wfring * ring, size_t order)
{
	struct __wfring * q = (struct __wfring *) ring;
	size_t i, n = wfring_pow2(order + 1);

	for (i = 0; i != n; i++)
		__lfaba_init(&q->array[i], __wfring_pair((lfsatomic_t) -1, (lfsatomic_t) (-n - 1)));

	__lfaba_init(&q->head, 0);
	atomic_init(&q->threshold, -1);
	__lfaba_init(&q->tail, 0);
}

static inline void wfring_init_full(struct wfring * ring, size_t order)
{
	struct __wfring * q = (struct __wfring *) ring;
	size_t i, half = wfring_pow2(order), n = half * 2;

	for (i = 0; i != half; i++)
		__lfaba_init(&q->array[__wfring_map(i, order, n)], __wfring_pair(2 * n + n + __wfring_raw_map(i, order, half), (lfsatomic_t) -1));
	for (; i != n; i++)
		__lfaba_init(&q->array[__wfring_map(i, order, n)], __wfring_pair((lfsatomic_t) -1, (lfsatomic_t) (-n - 1)));

	__lfaba_init(&q->head, 0);
	atomic_init(&q->threshold, __wfring_threshold3(half, n));
	__lfaba_init(&q->tail, __wfring_pair(half << 2, 0));
}

static inline void wfring_init_fill(struct wfring * ring,
		size_t s, size_t e, size_t order)
{
	struct __wfring * q = (struct __wfring *) ring;
	size_t i, half = wfring_pow2(order), n = half * 2;

	for (i = 0; i != s; i++)
		__lfaba_init(&q->array[__wfring_map(i, order, n)], __wfring_pair(4 * n - 1, (lfsatomic_t) -1));
	for (; i != e; i++)
		__lfaba_init(&q->array[__wfring_map(i, order, n)], __wfring_pair(2 * n + n + i, (lfsatomic_t) -1));
	for (; i != n; i++)
		__lfaba_init(&q->array[__wfring_map(i, order, n)],  __wfring_pair((lfsatomic_t) -1, (lfsatomic_t) (-n - 1)));

	__lfaba_init(&q->head, __wfring_pair(s << 2, 0));
	atomic_init(&q->threshold, __wfring_threshold3(half, n));
	__lfaba_init(&q->tail, __wfring_pair(e << 2, 0));
}

static inline void wfring_init_state(struct wfring * ring,
	struct wfring_state * state)
{
	atomic_init(&state->next, NULL);

	state->currThread = state;
	state->nextCheck = WFRING_DELAY;

	atomic_init(&state->seq1, 1);
	atomic_init(&state->eidx, __WFRING_EIDX_TERM);
	atomic_init(&state->seq2, 0);

	atomic_init(&state->phase2.seq1, 1);
	atomic_init(&state->phase2.seq2, 0);
}

static inline lfatomic_t __wfring_load_global_help_phase2(
	_Atomic(lfatomic_big_t) * global, lfatomic_big_t gp)
{
	struct wfring_phase2 * phase2;
	_Atomic(lfatomic_t) * local;
	lfatomic_t seq, cnt;

	do {
		phase2 = (struct wfring_phase2 *) __wfring_addon(gp);
		if (phase2 == NULL) break;
		seq = atomic_load(&phase2->seq2);
		local = phase2->local;
		cnt = phase2->cnt;
		if (atomic_load(&phase2->seq1) == seq) {
			lfatomic_t cnt_inc = cnt + __WFRING_INC;
			atomic_compare_exchange_strong(local, &cnt_inc, cnt);
		}
	} while (!__lfaba_cmpxchg_strong(global, &gp,
				__wfring_pair(__wfring_entry(gp), 0),
				memory_order_acq_rel, memory_order_acquire));
	return __wfring_entry(gp);
}

static inline bool __wfring_slow_inc(_Atomic(lfatomic_big_t) * global,
	_Atomic(lfatomic_t) * local, lfatomic_t * prev,
	_Atomic(lfsatomic_t) * threshold, struct wfring_phase2 * phase2)
{
	lfatomic_t seq, cnt, cnt_inc;
	lfatomic_big_t gp = __lfaba_load_atomic(global, memory_order_acquire);

	do {
		if (atomic_load(local) & __WFRING_FIN)
			return false;
		cnt = __wfring_load_global_help_phase2(global, gp);
		if (!atomic_compare_exchange_strong(local, prev, cnt + __WFRING_INC)) {
			if (*prev & __WFRING_FIN) return false;
			if (!(*prev & __WFRING_INC)) return true;
			cnt = *prev - __WFRING_INC;
		} else {
			*prev = cnt + __WFRING_INC;
		}
		seq = atomic_load(&phase2->seq1) + 1;
		atomic_store(&phase2->seq1, seq);
		phase2->local = local;
		phase2->cnt = cnt;
		atomic_store(&phase2->seq2, seq);
		gp = __wfring_pair(cnt, 0);
	} while (!__lfaba_cmpxchg_strong(global, &gp,
		__wfring_pair(cnt + 1, phase2),
		memory_order_acq_rel, memory_order_acquire));

	if (threshold != NULL)
		atomic_fetch_sub_explicit(threshold, 1, memory_order_acq_rel);

	cnt_inc = cnt + __WFRING_INC;
	atomic_compare_exchange_strong(local, &cnt_inc, cnt);
	gp = __wfring_pair(cnt + 1, phase2);
	__lfaba_cmpxchg_strong(global, &gp, __wfring_pair(cnt + 1, 0),
		memory_order_acq_rel, memory_order_acquire);
	*prev = cnt;

	return true;
}

static inline void __wfring_do_enqueue_slow(struct __wfring * q, size_t order,
	size_t eidx, lfatomic_t seq, lfatomic_t tail, bool nonempty,
	struct wfring_state * state)
{
	size_t tidx, half = wfring_pow2(order), n = half * 2;
	lfatomic_t entry, note, ecycle, tcycle;
	lfatomic_big_t pair;

	while (__wfring_slow_inc(&q->tail, &state->tail, &tail, NULL,
			&state->phase2)) {
		if (atomic_load(&state->seq1) != seq)
			break;
		tcycle = tail | (4 * n - 1);
		tidx = __wfring_map(tail >> 2, order, n);
		pair = __lfaba_load(&q->array[tidx], memory_order_acquire);
retry:
		entry = __wfring_entry(pair);
		note = __wfring_addon(pair);
		ecycle = entry | (4 * n - 1);
		if (__wfring_cmp(ecycle, <, tcycle) && __wfring_cmp(note, <, tcycle)) {
			if ((((entry | 0x1) == ecycle) ||
				(((entry | 0x1) == (ecycle ^ n)) &&
				 __wfring_cmp(
					atomic_load_explicit(__wfring_pair_entry(&q->head),
						memory_order_acquire), <=, tail)))) {

				if (!__lfaba_cmpxchg_weak(&q->array[tidx],
						&pair, __wfring_pair(tcycle ^ eidx ^ n, note),
						memory_order_acq_rel, memory_order_acquire))
					goto retry;

				entry = tcycle ^ eidx;

				if (atomic_compare_exchange_strong_explicit(&state->tail, &tail,
						tail + 0x1,
						memory_order_acq_rel, memory_order_acquire)) {

					/* Finalize the entry. */
					atomic_compare_exchange_strong_explicit(
							__wfring_pair_entry(&q->array[tidx]),
							&entry, entry ^ n, memory_order_acq_rel,
							memory_order_acquire);
				}

				if (!nonempty && (atomic_load(&q->threshold) != __wfring_threshold3(half, n)))
					atomic_store(&q->threshold, __wfring_threshold3(half, n));
				return;
			} else if ((entry | (2 * n + n)) == tcycle) {
				/* Already produced. */
				return;
			} else {
				/* Skip this entry. */
				if (!__lfaba_cmpxchg_weak(&q->array[tidx],
						&pair, __wfring_pair(entry, tcycle),
						memory_order_acq_rel, memory_order_acquire))
					goto retry;
			}
		}
	}
}

__attribute__((noinline))  static void __wfring_enqueue_slow(
	struct __wfring * q, size_t order, size_t eidx,
	lfatomic_t tail, bool nonempty, struct wfring_state * state)
{
	lfatomic_t seq = atomic_load(&state->seq1);

	/* Initiate a helping request. */
	atomic_store(&state->tail, tail);
	state->initTail = tail;
	atomic_store(&state->eidx, eidx);
	atomic_store(&state->seq2, seq);

	__wfring_do_enqueue_slow(q, order, eidx, seq, tail, nonempty, state);

	/* Terminate the helping request. */
	atomic_store(&state->seq1, seq + 1);
	atomic_store(&state->eidx, __WFRING_EIDX_TERM);
}

__attribute__((noinline)) static void __wfring_enqueue_help_thread(
	struct __wfring * q, size_t order, bool nonempty,
	struct wfring_state * state)
{
	lfatomic_t seq = atomic_load(&state->seq2);
	size_t eidx = atomic_load(&state->eidx);
	lfatomic_t tail = state->initTail;
	if (eidx <= __WFRING_EIDX_DEQ || atomic_load(&state->seq1) != seq)
		return;

	__wfring_do_enqueue_slow(q, order, eidx, seq, tail, nonempty, state);
}

static inline void __wfring_catchup(struct __wfring * q,
	lfatomic_t tail, lfatomic_t head)
{
	while (!atomic_compare_exchange_weak_explicit(__wfring_pair_entry(&q->tail),
			&tail, head, memory_order_acq_rel, memory_order_acquire)) {
		head = atomic_load(__wfring_pair_entry(&q->head));
		tail = atomic_load(__wfring_pair_entry(&q->tail));
		if (__wfring_cmp(tail, >=, head))
			break;
	}
}

static inline void __wfring_lookup(struct wfring_state * state,
	lfatomic_t tail, size_t n)
{
	struct wfring_state * curr = atomic_load(&state->next);
	while (curr != state) {
		if ((atomic_load(&curr->tail) & ~((lfatomic_t) 0x3)) == tail) {
			atomic_compare_exchange_strong(&curr->tail, &tail, tail ^ 0x1);
			return;
		}
		curr = atomic_load(&curr->next);
	}
	return;
}

static inline void __wfring_do_dequeue_slow(struct __wfring * q, size_t order,
	lfatomic_t seq, lfatomic_t head, bool nonempty, struct wfring_state * state)
{
	size_t hidx, n = wfring_pow2(order + 1);
	lfatomic_t entry, note, entry_new, ecycle, hcycle, tail;
	lfatomic_big_t pair;
	_Atomic(lfsatomic_t) * threshold = nonempty ? NULL : &q->threshold;

	while (__wfring_slow_inc(&q->head, &state->head, &head, threshold,
			&state->phase2)) {
		hcycle = head | (4 * n - 1);
		hidx = __wfring_map(head >> 2, order, n);
		pair = __lfaba_load(&q->array[hidx], memory_order_acquire);
retry:
		do {
			entry = __wfring_entry(pair);
			note = __wfring_addon(pair);
			ecycle = entry | (4 * n - 1);
			if (ecycle == hcycle && (entry & (n - 1)) != (n - 2)) {
				lfatomic_t _h = head;
				atomic_compare_exchange_strong(&state->head, &_h, head ^ 0x1);
				return;
			}

			if ((entry | (2 * n) | 0x1) != ecycle) {
				if (__wfring_cmp(ecycle, <, hcycle) &&
						__wfring_cmp(note, <, hcycle)) {
					/* Do not enqueue in this entry. */
					if (!__lfaba_cmpxchg_weak(&q->array[hidx], &pair,
							__wfring_pair(entry, hcycle),
							memory_order_acq_rel, memory_order_acquire))
						goto retry;
				}
				entry_new = entry & ~(lfatomic_t) (2 * n);
				if (entry == entry_new)
					break;
			} else {
				entry_new = hcycle ^ ((~entry) & (2 * n)) ^ 0x1;
			}
		} while (__wfring_cmp(ecycle, <, hcycle) &&
					!__lfaba_cmpxchg_weak(&q->array[hidx], &pair,
						__wfring_pair(entry_new, note),
						memory_order_acq_rel, memory_order_acquire));

		if (!nonempty) {
			tail = atomic_load_explicit(__wfring_pair_entry(&q->tail),
				memory_order_acquire);
			if (__wfring_cmp(tail, <=, head + 4)) {
				__wfring_catchup(q, tail, head + 4);
			}
			if (atomic_load(&q->threshold) < 0) {
				lfatomic_t _h = head;
				atomic_compare_exchange_strong(&state->head, &_h,
					head + __WFRING_FIN);
			}
		}
	}
}

__attribute__((noinline))  static size_t __wfring_dequeue_slow(struct __wfring * q, size_t order,
	lfatomic_t head, bool nonempty, struct wfring_state * state)
{
	size_t hidx, n = wfring_pow2(order + 1);
	lfatomic_t entry, hcycle;
	lfatomic_t seq = atomic_load(&state->seq1);

	/* Initiate a helping request. */
	atomic_store(&state->head, head);
	state->initHead = head;
	atomic_store(&state->eidx, __WFRING_EIDX_DEQ);
	atomic_store(&state->seq2, seq);

	__wfring_do_dequeue_slow(q, order, seq, head, nonempty, state);

	/* Terminate the helping request. */
	atomic_store(&state->seq1, seq + 1);
	atomic_store(&state->eidx, __WFRING_EIDX_TERM);

	/* Consume an element. */
	head = atomic_load(&state->head);
	hcycle = head | (4 * n - 1);
	hidx = __wfring_map(head >> 2, order, n);
	entry = atomic_load_explicit(__wfring_pair_entry(&q->array[hidx]), memory_order_acquire);
	if (nonempty || ((entry | (2 * n + n)) == hcycle)) {
		if (!(entry & n))
			__wfring_lookup(state, head, n);
		atomic_fetch_or_explicit(__wfring_pair_entry(&q->array[hidx]),
			(2 * n - 1), memory_order_acq_rel);
		return (size_t) (entry & (n - 1));
	}

	return WFRING_EMPTY;
}

__attribute__((noinline)) static void __wfring_dequeue_help_thread(struct __wfring * q,
	size_t order, bool nonempty, struct wfring_state * state)
{
	lfatomic_t seq = atomic_load(&state->seq2);
	lfatomic_t eidx = atomic_load(&state->eidx);
	lfatomic_t head = atomic_load(&state->initHead);
	if (eidx != __WFRING_EIDX_DEQ || atomic_load(&state->seq1) != seq)
		return;

	__wfring_do_dequeue_slow(q, order, seq, head, nonempty, state);
}

__attribute__((noinline)) static void __wfring_help(struct __wfring * q, size_t order,
	bool nonempty, struct wfring_state * state)
{
	struct wfring_state * curr = state->currThread;
	if (curr != state) {
		size_t eidx = atomic_load(&curr->eidx);
		if (eidx != __WFRING_EIDX_TERM) {
			if (eidx != __WFRING_EIDX_DEQ)
				__wfring_enqueue_help_thread(q, order, nonempty, curr);
			else
				__wfring_dequeue_help_thread(q, order, nonempty, curr);
		}
		curr = atomic_load(&curr->next);
	}
	state->currThread = atomic_load(&curr->next);
	state->nextCheck = WFRING_DELAY;
}

static inline void wfring_enqueue(struct wfring * ring,
	size_t order, size_t eidx, bool nonempty, struct wfring_state * state)
{
	struct __wfring * q = (struct __wfring *) ring;
	size_t tidx, half = wfring_pow2(order), n = half * 2;
	lfatomic_t tail, entry, ecycle, tcycle;
	size_t patience = WFRING_PATIENCE_ENQ;

	eidx ^= (n - 1);
	if (--state->nextCheck == 0)
		__wfring_help(q, order, nonempty, state);

	do {
		tail = atomic_fetch_add_explicit(__wfring_pair_entry(&q->tail), 4, memory_order_acq_rel);
		tcycle = tail | (4 * n - 1);
		tidx = __wfring_map(tail >> 2, order, n);
		entry = atomic_load_explicit(__wfring_pair_entry(&q->array[tidx]), memory_order_acquire);
retry:
		ecycle = entry | (4 * n - 1);
		if (__wfring_cmp(ecycle, <, tcycle) && (((entry | 0x1) == ecycle) ||
				(((entry | 0x1) == (ecycle ^ (2 * n))) &&
					__wfring_cmp(atomic_load_explicit(
						__wfring_pair_entry(&q->head),
							memory_order_acquire), <=, tail)))) {

			if (!atomic_compare_exchange_weak_explicit(
					__wfring_pair_entry(&q->array[tidx]),
					&entry, tcycle ^ eidx,
					memory_order_acq_rel, memory_order_acquire))
				goto retry;

			if (!nonempty && (atomic_load(&q->threshold) != __wfring_threshold3(half, n)))
				atomic_store(&q->threshold, __wfring_threshold3(half, n));
			return;
		}
	} while (--patience != 0);

	__wfring_enqueue_slow(q, order, eidx, tail, nonempty, state);
}

static inline size_t wfring_dequeue(struct wfring * ring, size_t order,
		bool nonempty, struct wfring_state * state)
{
	struct __wfring * q = (struct __wfring *) ring;
	size_t hidx, n = wfring_pow2(order + 1);
	lfatomic_t head, entry, entry_new, ecycle, hcycle, tail;
//	size_t attempt;
	size_t patience = WFRING_PATIENCE_DEQ;

	if (!nonempty && atomic_load(&q->threshold) < 0) {
		return WFRING_EMPTY;
	}

	if (--state->nextCheck == 0)
		__wfring_help(q, order, nonempty, state);

	do {
		head = atomic_fetch_add_explicit(__wfring_pair_entry(&q->head), 4, memory_order_acq_rel);
		hcycle = head | (4 * n - 1);
		hidx = __wfring_map(head >> 2, order, n);
		//attempt = 0;
//again:
		entry = atomic_load_explicit(__wfring_pair_entry(&q->array[hidx]), memory_order_acquire);

		do {
			ecycle = entry | (4 * n - 1);
			if (ecycle == hcycle) {
				/* Need to help finalizing the entry. */
				if (!(entry & n))
					__wfring_lookup(state, head, n);
				atomic_fetch_or_explicit(__wfring_pair_entry(&q->array[hidx]),
						(2 * n - 1), memory_order_acq_rel);
				return (size_t) (entry & (n - 1));
			}

			if ((entry | (2 * n) | 0x1) != ecycle) {
				entry_new = entry & ~(lfatomic_t) (2 * n);
				if (entry == entry_new)
					break;
			} else {
	//			if (++attempt <= 10000)
	//				goto again;
				entry_new = hcycle ^ ((~entry) & (2 * n)) ^ 0x1;
			}
		} while (__wfring_cmp(ecycle, <, hcycle) &&
					!atomic_compare_exchange_weak_explicit(
					__wfring_pair_entry(&q->array[hidx]), &entry, entry_new,
					memory_order_acq_rel, memory_order_acquire));

		if (!nonempty) {
			tail = atomic_load_explicit(__wfring_pair_entry(&q->tail),
				memory_order_acquire);
			if (__wfring_cmp(tail, <=, head + 4)) {
				__wfring_catchup(q, tail, head + 4);
				atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel);
				return WFRING_EMPTY;
			}

			if (atomic_fetch_sub_explicit(&q->threshold, 1,
					memory_order_acq_rel) <= 0)
				return WFRING_EMPTY;
		}
	} while (--patience != 0);

	return __wfring_dequeue_slow(q, order, head, nonempty, state);
}

#endif	/* !__WFRING_H */

/* vi: set tabstop=4: */
