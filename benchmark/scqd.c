#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scqd.h"

void queue_init(queue_t * q, int nprocs)
{
  lfring_init_empty((struct lfring *) q->aq, SCQD_ORDER);
  lfring_init_full((struct lfring *) q->fq, SCQD_ORDER);
}


void queue_register(queue_t * q, handle_t * th, int id)
{
}

void enqueue(queue_t * q, handle_t * th, void * val)
{
  size_t eidx;
  eidx = lfring_dequeue((struct lfring *) q->fq, SCQD_ORDER, true);
  if (eidx == LFRING_EMPTY) return;
  q->val[eidx] = val;
  lfring_enqueue((struct lfring *) q->aq, SCQD_ORDER, eidx, false);
}

void * dequeue(queue_t * q, handle_t * th)
{
  size_t eidx;
  void *val;
  eidx = lfring_dequeue((struct lfring *) q->aq, SCQD_ORDER, false);
  if (eidx == LFRING_EMPTY) return EMPTY;
  val = q->val[eidx];
  lfring_enqueue((struct lfring *) q->fq, SCQD_ORDER, eidx, true);
  return val;
}

void queue_free(queue_t * q, handle_t * h)
{
}
