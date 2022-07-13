#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "wcq.h"

void queue_init(queue_t * q, int nprocs)
{
  wfring_init_empty((struct wfring *) q->ring, WCQ_ORDER);
}

static _Atomic(struct wfring_state *) handle_tail = ATOMIC_VAR_INIT(NULL);

void queue_register(queue_t * q, handle_t * th, int id)
{
  wfring_init_state((struct wfring *) q->ring, &th->state);

  struct wfring_state *tail = atomic_load(&handle_tail);
  if (tail == NULL) {
    th->state.next = &th->state;
    if (atomic_compare_exchange_strong(&handle_tail, &tail, &th->state))
      return;
  }

  struct wfring_state *next = atomic_load(&tail->next);
  do {
    th->state.next = next;
  } while (!atomic_compare_exchange_weak(&tail->next, &next, &th->state));
}

void enqueue(queue_t * q, handle_t * th, void * val)
{
  size_t eidx = (size_t) val;
  wfring_enqueue((struct wfring *) q->ring, WCQ_ORDER, eidx, false, &th->state);
}

void * dequeue(queue_t * q, handle_t * th)
{
  return (void *) wfring_dequeue((struct wfring *) q->ring, WCQ_ORDER, false, &th->state);
}

void queue_free(queue_t * q, handle_t * h)
{
}
