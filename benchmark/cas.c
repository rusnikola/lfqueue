#include "queue.h"
#include "primitives.h"

void queue_init(queue_t * q, int nprocs) {}
void queue_register(queue_t * q, handle_t * hd, int id)
{
  *hd = id + 1;
}

void enqueue(queue_t * q, handle_t * th, void * val)
{
  long p = q->P;
  while (!CAS(&q->P, &p, p + 1))
    ;
}

void * dequeue(queue_t * q, handle_t * th)
{
  long c = q->C;
  while (!CAS(&q->C, &c, c + 1))
    ;
  return (void *) (long) *th;
}

void queue_free(queue_t * q, handle_t * h) {}

