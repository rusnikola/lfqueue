#ifndef SCQ_H
#define SCQ_H

#ifdef SCQ

#include <stddef.h>
#include "../lfring_cas1.h"
#include "align.h"

#define SCQ_ORDER 15
#define EMPTY (void *) LFRING_EMPTY

typedef struct _queue_t {
  char ring[LFRING_SIZE(SCQ_ORDER)];
} queue_t DOUBLE_CACHE_ALIGNED;

typedef struct _handle_t {
  int pad;
} handle_t DOUBLE_CACHE_ALIGNED;

#endif

#endif /* end of include guard: SCQ_H */
