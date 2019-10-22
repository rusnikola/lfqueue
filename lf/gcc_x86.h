/*
  Copyright (c) 2018, Ruslan Nikolaev
  All rights reserved.
*/

#ifndef __LF_GCC_X86_H
#define __LF_GCC_X86_H 1

#include <stdatomic.h>

#define LFATOMIC(x)				_Atomic(x)
#define LFATOMIC_VAR_INIT(x)	ATOMIC_VAR_INIT(x)

static inline void __lfbig_init(_Atomic(lfatomic_big_t) * obj,
		lfatomic_big_t val)
{
	*((volatile lfatomic_big_t *) obj) = val;
}

static inline lfatomic_big_t __lfbig_load(_Atomic(lfatomic_big_t) * obj,
		memory_order order)
{
	return *((volatile lfatomic_big_t *) obj);
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
#define __lfaba_cmpxchg_weak	__lfbig_cmpxchg_weak
#define __lfaba_cmpxchg_strong	__lfbig_cmpxchg_strong
#define __lfaba_fetch_and		__lfbig_fetch_and

#endif /* !__LF_GGC_X86_H */

/* vi: set tabstop=4: */
