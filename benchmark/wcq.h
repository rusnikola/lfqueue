#ifndef WCQ_H
#define WCQ_H

#ifdef WCQ

#include <stddef.h>
#include "../wfring_cas2.h"
#include "align.h"

#define WCQ_ORDER 15
#define EMPTY (void *) WFRING_EMPTY

typedef struct _queue_t {
  char ring[WFRING_SIZE(WCQ_ORDER)];
} queue_t DOUBLE_CACHE_ALIGNED;

typedef struct _handle_t {
  struct wfring_state state;
} handle_t DOUBLE_CACHE_ALIGNED;

#endif

#endif /* end of include guard: WCQ_H */
