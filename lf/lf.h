/*
  Copyright (c) 2019, Ruslan Nikolaev
  All rights reserved.
*/

#ifndef __BITS_LF_H
#define __BITS_LF_H	1

#include <inttypes.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stddef.h>

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

typedef bool (*lf_check_t) (void * data, void * addr, size_t size);

static inline size_t lf_pow2(size_t order)
{
	return (size_t) 1U << order;
}

static inline bool LF_DONTCHECK(void * head, void * addr, size_t size)
{
	return true;
}

#define lf_container_of(addr, type, field)									\
	(type *) ((char *) (addr) - offsetof(type, field))

#ifdef __cplusplus
# define LF_ERROR	UINTPTR_MAX
#else
# define LF_ERROR	((void *) UINTPTR_MAX)
#endif

/* Available on all 64-bit and CAS2 32-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 64
__LFABA_IMPL(32, uint32_t)
#endif

/* Available on CAS2 64-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 128
__LFABA_IMPL(64, uint64_t)
#endif

/* Available on CAS2 32/64-bit architectures. */
#if LFATOMIC_BIG_WIDTH >= 2 * __LFPTR_WIDTH
__LFABA_IMPL(, uintptr_t)
#endif

#endif	/* !__BITS_LF_H */

/* vi: set tabstop=4: */
