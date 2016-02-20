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
#ifndef _IOP_H_
#define _IOP_H_
#include <time.h>
#include "iop_config.h"
#include "iop_util.h"
#include "array_list.h"
#include "iop_def.h"
#include "iop_buf.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
*iop对定时器只支持精确到秒级
*可以满足绝大多数业务需求，
*并尽可能减少定时器带来的性能损耗
*/

struct tag_iop_setting
{
    int maxio;
    iop_base_t *default_base;
};
typedef struct tag_iop_setting iop_setting_t;


struct tag_iop_tcp_conn_buf
{
    dbuf_t rbuf;                    //socket 读缓存
    dbuf_t wbuf;                    //socket 写缓存
    iop_protocol_t *proto;      //协议处理
    iop_t *iop;
    void *ptr;                      //用户可以自定义的数据
};
typedef struct tag_iop_tcp_conn_buf iop_tcp_conn_buf_t;


iop_setting_t *iop_sys_setting();
int iop_sys_init(int maxio);
int iop_set_service_timeout(int timeout);
int iop_sys_destroy();

iop_base_t *iop_sys_base();
#define IOP_SYS_BASE iop_sys_base()

void iop_init_data(iop_t *iop, iop_base_t *base,int id, io_handle_t sock, unsigned int event, iop_cb cb, iop_cb_arg_t cb_arg);

//由系统自动选择最优的模型
iop_base_t * iop_new(int maxio);

//is full? 1:yes 0:no
int iop_is_full(iop_base_t *base);
int iop_is_empty(iop_base_t *base);

//添加事件
int iop_add(iop_base_t *base, io_handle_t handle,
    unsigned int events, iop_cb cb, iop_cb_arg_t arg);
//进入事件调度
int iop_dispatch(iop_base_t * base);
int iop_dispatch_ex(iop_base_t * base, void (*func)(void *), void *arg);

//取消事件，关闭句柄。
int iop_del(iop_base_t * base, int id);

//关闭事件
int iop_close(iop_base_t *base, int id);

//挂起
int iop_suspend(iop_base_t *base, int id);
//清除所有事件和参数,保留句柄
int iop_clear(iop_base_t * base, int id);

//选择系统支持的特定模型
void iop_exit(iop_base_t *base);
//释放资源
void iop_free(iop_base_t *base);

//修改事件
int iop_mod_ev(iop_base_t * base,int id, unsigned int event);
int iop_set_ev(iop_base_t * base,int id, unsigned int event);
int iop_clr_ev(iop_base_t * base,int id, unsigned int event);

//修改事件，回调，参数
int iop_mod(iop_base_t *base, int id,
                            unsigned int event, iop_cb cb, iop_cb_arg_t arg);

//根据事件id获取事件数据
iop_t *iop_get(iop_base_t *base, int id);
//取事件模型名称
const char *get_iop_name(iop_base_t *base);
//忽略事件回调
void iop_ignore_cb(iop_t *iop);
//指定系统支持的网络模型
iop_base_t * iop_new_special(int maxio, int iop_model);
//用户自定义的网络模型
iop_base_t * iop_new_user_def(int maxio, iop_op_t *op_imp, void *model_data);
//退出事件循环，在当前事件循环处理完成后生效


//分配定时器对象
iop_timer_t *iop_new_timer(iop_base_t *base, int id, int timeout, iop_cb cb, int ispersist);
//释放定时器对象
void iop_free_timer(iop_base_t *base, iop_timer_t *timer);
//为已有事件设置一个定时器
int iop_set_timer(iop_base_t *base, int id, iop_cb cb,int timeout, int ispersist);
int iop_clear_timer(iop_base_t *base, int id);


//新增一个定时器事件
int iop_add_timer(iop_base_t *base, iop_cb cb, iop_cb_arg_t arg,int timeout, int ispersist);
//设置事件定时器检查间隔。默认是1秒
void iop_check_ev_timer_interval(iop_base_t *base,int i);

int iop_add_listen(iop_base_t *base, const char *ip, unsigned short port, iop_cb cb,  iop_cb_arg_t cb_arg);


struct tag_iop_asyn_conn_arg
{
    int flag;
    iop_sockaddr_in addr;
    iop_cb connect_ok_cb;
    iop_cb connect_err_cb;
    iop_cb timeout_cb;
    int timeout;
    void *arg;
    void (*free_arg)(void *);
};
typedef struct tag_iop_asyn_conn_arg iop_asyn_conn_arg_t;

int iop_asyn_connect(iop_base_t *base, iop_asyn_conn_arg_t arg);

void iop_tcp_conn_cb(iop_t *iop);
void iop_listen_cb(iop_t *iop);
int iop_add_tcp_server(iop_base_t *base,const char *ip, unsigned short port,iop_protocol_t *proto);
int iop_buf_send(iop_t *iop,const void *buf,unsigned int len);
int iop_buf_send_ex(iop_base_t *base, time_t ctime,int id,const void *buf,unsigned int len);
int iop_queue_send(iop_base_t *base, int id,unsigned int len, void *buf);
int iop_set_asq(iop_base_t *base, as_queue_item_proc aq_proc);


//网络服务器在处理一个业务时，通常需要多个步骤才能完成。
//如果在一个同步的服务模型中，这不会有什么问题
//但是在需要处理超大并发请求的业务中，通常不能使用同步模型
//那么异步模型的中间过程的数据需要保存起来，即业务的上下文与状态机

#ifdef __cplusplus
}
#endif

#endif

