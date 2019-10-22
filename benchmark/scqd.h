#ifndef SCQD_H
#define SCQD_H

#ifdef SCQD

#include <stddef.h>
#include "../lfring_cas1.h"
#include "align.h"

#define SCQD_ORDER 16
#define EMPTY (void *) LFRING_EMPTY

typedef struct _queue_t {
  char aq[LFRING_SIZE(SCQD_ORDER)];
  char fq[LFRING_SIZE(SCQD_ORDER)];
  void  *val[(1U << SCQD_ORDER)];
} queue_t DOUBLE_CACHE_ALIGNED;

typedef struct _handle_t {
  int pad;
} handle_t DOUBLE_CACHE_ALIGNED;

#endif

#endif /* end of include guard: SCQD_H */
