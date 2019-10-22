#ifndef NCQ_H
#define NCQ_H

#ifdef NCQ

#include <stddef.h>
#include "../lfring_naive.h"
#include "align.h"

#define NCQ_ORDER 16
#define EMPTY (void *) LFRING_EMPTY

typedef struct _queue_t {
  char ring[LFRING_SIZE(NCQ_ORDER)];
} queue_t DOUBLE_CACHE_ALIGNED;

typedef struct _handle_t {
  int pad;
} handle_t DOUBLE_CACHE_ALIGNED;

#endif

#endif /* end of include guard: NCQ_H */
