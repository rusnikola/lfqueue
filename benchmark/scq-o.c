/* ----------------------------------------------------------------------------
 *
 * Dual 2-BSD/MIT license. Either or both licenses can be used.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2026, Xi Wang, Ruslan Nikolaev
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ----------------------------------------------------------------------------
 *
 * Copyright (c) 2026, Xi Wang, Ruslan Nikolaev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "scq-o.h"

void queue_init(queue_t *q, int nprocs)
{
  /* initialize allocated queue */
  lfring_init_empty((struct lfring *)q->aq, SCQ_O_ORDER);

  /* initialize free queue */
  lfring_init_full((struct lfring *)q->fq1, SCQ_O_ORDER - SCQ_O_CACHE_ORDER);
  lfring_init_empty((struct lfring *)q->fq2, SCQ_O_ORDER - SCQ_O_CACHE_ORDER);

  /* initialize index blocks and local cache */
  for (size_t i = 0; i < (1u << SCQ_O_ORDER); i++)
  {
    q->idx[i] = i;
  }
}

void queue_register(queue_t *q, handle_t *th, int id)
{
  th->local_size = 0;
}

void enqueue(queue_t *q, handle_t *th, void *val)
{
  size_t idx, bidx;
  if (th->local_size > 0)
  {
    /* get an index from local cache */
    idx = th->owned_cache[--th->local_size];
  }
  else
  {
    /* get an index block pointer by dequeue from block buffer */
    bidx = lfring_dequeue((struct lfring *)q->fq1, SCQ_O_ORDER - SCQ_O_CACHE_ORDER, false);
    if (bidx == LFRING_EMPTY)
      return;

		memcpy(th->owned_cache, &q->idx[bidx << SCQ_O_CACHE_ORDER], sizeof(size_t) * SCQ_O_CACHE_SIZE);

    /* indicate block allocated */
    lfring_enqueue((struct lfring *)q->fq2, SCQ_O_ORDER - SCQ_O_CACHE_ORDER, bidx, false);
    th->local_size = SCQ_O_CACHE_SIZE;
    idx = th->owned_cache[--th->local_size];
  }

  q->val[idx] = val;

  lfring_enqueue((struct lfring *)q->aq, SCQ_O_ORDER, idx, false);
}

void *dequeue(queue_t *q, handle_t *th)
{
  size_t idx, bidx;
  void *val = NULL;

  idx = lfring_dequeue((struct lfring *)q->aq, SCQ_O_ORDER, false);
  if (idx == LFRING_EMPTY)
    return EMPTY;

  val = q->val[idx];

  th->owned_cache[th->local_size++] = idx;
  if (th->local_size == SCQ_O_CACHE_THRESHOLD)
  {
    /* return index block */
    bidx = lfring_dequeue((struct lfring *)q->fq2, SCQ_O_ORDER - SCQ_O_CACHE_ORDER, false);
    if (bidx == LFRING_EMPTY)
      return EMPTY;

    memcpy(&q->idx[bidx << SCQ_O_CACHE_ORDER], th->owned_cache + SCQ_O_CACHE_SIZE, sizeof(size_t) * SCQ_O_CACHE_SIZE);
    th->local_size = SCQ_O_CACHE_SIZE;

    lfring_enqueue((struct lfring *)q->fq1, SCQ_O_ORDER - SCQ_O_CACHE_ORDER, bidx, false);
  }

  return val;
}

void queue_free(queue_t *q, handle_t *h)
{

}
