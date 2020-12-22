#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scq2.h"

void queue_init(queue_t * q, int nprocs)
{
  lfring_ptr_init_empty((struct lfring_ptr *) q->ring, SCQ2_ORDER);
}


void queue_register(queue_t * q, handle_t * th, int id)
{
  lfring_ptr_init_lhead(&th->lhead, SCQ2_ORDER);
}

void enqueue(queue_t * q, handle_t * th, void * val)
{
  lfring_ptr_enqueue((struct lfring_ptr *) q->ring, SCQ2_ORDER, val + 1,
    false, false, &th->lhead);
}

void * dequeue(queue_t * q, handle_t * th)
{
  void *ptr;
  if (!lfring_ptr_dequeue((struct lfring_ptr *) q->ring, SCQ2_ORDER,
      &ptr, false))
    return EMPTY;
  ptr--;
  return ptr;
}

void queue_free(queue_t * q, handle_t * h)
{
}
