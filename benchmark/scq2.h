#ifndef SCQ2_H
#define SCQ2_H

#ifdef SCQ2

#include <stddef.h>
#include "../lfring_cas2.h"
#include "align.h"

#define SCQ2_ORDER 15
#define EMPTY (void *) -1

typedef struct _queue_t {
  char ring[LFRING_PTR_SIZE(SCQ2_ORDER)];
} queue_t DOUBLE_CACHE_ALIGNED;

typedef struct _handle_t {
  lfatomic_t lhead;
} handle_t DOUBLE_CACHE_ALIGNED;

#endif

#endif /* end of include guard: SCQ_H */
