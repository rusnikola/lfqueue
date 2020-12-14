/*
  Copyright (c) 2019, Ruslan Nikolaev
  All rights reserved.
*/

#ifndef __LF_C11_H
#define __LF_C11_H 1

#include <stdatomic.h>

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
	_Atomic(lfatomic_t) * hobj = (_Atomic(lfatomic_t) *) obj;
	lfatomic_t * hres = (lfatomic_t *) &res;

	hres[0] = atomic_load_explicit(hobj, order);
	hres[1] = atomic_load_explicit(hobj + 1, order);
	return res;
#elif __LFLOAD_SPLIT(LFATOMIC_BIG_WIDTH) == 0
	return atomic_load_explicit(obj, order);
#endif
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

#endif /* !__LF_C11_H */

/* vi: set tabstop=4: */
