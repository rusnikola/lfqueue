#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "ncq.h"

void queue_init(queue_t * q, int nprocs)
{
  lfring_init_empty((struct lfring *) q->ring, NCQ_ORDER);
}


void queue_register(queue_t * q, handle_t * th, int id)
{
}

void enqueue(queue_t * q, handle_t * th, void * val)
{
  size_t eidx = (size_t) val;
  lfring_enqueue((struct lfring *) q->ring, NCQ_ORDER, eidx, false);
}

void * dequeue(queue_t * q, handle_t * th)
{
  return (void *) lfring_dequeue((struct lfring *) q->ring, NCQ_ORDER, false);
}

void queue_free(queue_t * q, handle_t * h)
{
}
