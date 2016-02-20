/*
 * Copyright (c) 2011-2014 zhangjianlin 张荐林
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

#ifndef _IOP_SERVICE_H_
#define _IOP_SERVICE_H_

#include "iop_def.h"
#include "iop_list.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
*因编码水平或某些设计不太良好的
*第三方API调用，
*可能导致工作线程被长时间阻塞，
*而这对一个高负载的服务器来说
*是不能接受的情况。
*故框架在设计应该保证
*即使有部分线程挂死仍能正常运行
**/
struct tag_iop_task
{
    int id;
    iop_thread_t tid;
    volatile int status;
    volatile time_t last_upd_time;
    void *arg;
    iop_base_t *base;
    void *ext_info;
};

typedef struct tag_iop_task iop_task_t;
typedef int (*iop_task_cb)(iop_task_t *);

struct tag_iop_task_pool
{
    volatile int has_new_job;        //是否有新的任务
    iop_lock_t lock;
    int max_concurrent;             //最大并发
    int task_num;                       //线程数
    iop_task_t **task_pool;    
    iop_list_head_t job_list;      //等待处理的工作列表
    int new_job_num;

    iop_task_cb on_task_start;
    iop_task_cb on_task_exit;
    
    iop_protocol_t proto;
    iop_base_t *base;
    void *ext_info;
};
typedef struct tag_iop_task_pool iop_task_pool_t;

iop_task_pool_t *iop_task_pool_create(int task_num);
void iop_task_start_cb(iop_task_pool_t *tp, iop_task_cb start_cb);
void iop_task_exit_cb(iop_task_pool_t * tp, iop_task_cb exit_cb);
void iop_task_pool_ext(iop_task_pool_t *tp, void *ext);

int iop_task_pool_destroy(iop_task_pool_t *tp);
int iop_task_pool_start(iop_task_pool_t *tp, const char *ip, unsigned short port, iop_protocol_t *proto);

#ifdef __cplusplus
}
#endif

#endif

