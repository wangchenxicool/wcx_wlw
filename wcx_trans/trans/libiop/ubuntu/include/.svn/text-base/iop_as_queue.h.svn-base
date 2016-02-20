/*
 * Copyright (c) 2011-2014 zhangjianlin ’≈ºˆ¡÷
 * eamil:305080833@qq.com
 * addr: china
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 #ifndef _IOP_AS_QUEUE_H_
#define _IOP_AS_QUEUE_H_

#include "iop_config.h"
#include "iop_obj_pool.h"
#include "iop_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

struct tag_iop_as_queue_node_t
{
    time_t ctime;
    int id;
    unsigned int len;
    void *item;
    struct tag_iop_as_queue_node_t *next;
};
typedef struct tag_iop_as_queue_node_t iop_as_queue_node_t;

struct tag_iop_as_queue_t
{
    iop_lock_t *plock;
    io_handle_t rfd;
    io_handle_t wfd;

    iop_as_queue_node_t *head;
    iop_as_queue_node_t *tail;
    int que_size;
    int max_que_size;
};

typedef struct tag_iop_as_queue_t iop_as_queue_t;

int iop_as_queue_init(iop_as_queue_t *q);
int iop_as_queue_destroy(iop_as_queue_t *q);

int iop_as_queue_size(iop_as_queue_t *q);
int iop_as_queue_empty(iop_as_queue_t *q);
int iop_as_queue_push_back(iop_as_queue_t *q, time_t ctime, int id, unsigned int len, const void *item);
int iop_as_queue_pop_front(iop_as_queue_t *q, time_t *ctime, int *id, unsigned int *len, void **item);
iop_as_queue_node_t *iop_as_queue_pop_all(iop_as_queue_t *q);

#ifdef __cplusplus
}
#endif

#endif

