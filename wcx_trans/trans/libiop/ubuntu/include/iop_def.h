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
#ifndef _IOP_DEF_H_
#define _IOP_DEF_H_
#include <time.h>
#include "iop_config.h"
#include "iop_util.h"
#include "array_list.h"
#include "iop_as_queue.h"

#ifdef __cplusplus
extern "C" {
#endif
//读事件
#define EV_TYPE_READ                        1
//写事件
#define EV_TYPE_WRITE                       2


#define MAX_SOCK_BUF_SIZE           64000
#define IOP_ERR_SYS  -1
#define IOP_ERR_TIMEOUT  -2


//一组操作事件类型的宏
#define EV_IS_SET_R(e) ((e)&EV_TYPE_READ)
#define EV_IS_SET_W(e) ((e)&EV_TYPE_WRITE)

#define EV_SET_R(e) ((e) | EV_TYPE_READ)
#define EV_CLR_R(e) ((e) & ~EV_TYPE_READ)
#define EV_SET_W(e) ((e) | EV_TYPE_WRITE)
#define EV_CLR_W(e) ((e) & ~EV_TYPE_WRITE)

#define EV_SET_RW(e) ((e)|EV_TYPE_READ|EV_TYPE_WRITE)
#define EV_TYPE_RW (EV_TYPE_READ|EV_TYPE_WRITE)

//系统支持的事件模型

//NULL代表由系统自动选择
#define IOP_MODEL_NULL                      0
//event ports模型暂未实现
#define IOP_MODEL_EVENT_PORTS       1
//kqueue模型暂未实现
#define IOP_MODEL_KQUEUE                  2
//linux epoll模型已经支持
#define IOP_MODEL_EPOLL                     3
//dev poll模型暂未实现
#define IOP_MODEL_DEV_POLL              4
//poll模型已经支持
#define IOP_MODEL_POLL                       5
//select 模型已经支持. windows & linux
#define IOP_MODEL_SELECT                   6
//windows iocp模型暂未实现
#define IOP_MODEL_IOCP                       7
//只支持定时器
#define IOP_MODEL_TIMER                     8




#define IOP_STATUS_SUSPEND              0
#define IOP_STATUS_NORMAL               1
#define IOP_STATUS_EXIT                     2


#define IOP_ADD_TIMER(base,cb,arg,timeout) iop_add(base,INVALID_HANDLE,0,arg,timeout)
#define IOP_ADD_PTIMER(base,cb,arg,timeout) iop_add(base,INVALID_HANDLE,EV_TYPE_PERSIST_TIMER,arg,timeout)

#define SYS_IOP_ADD_TIMER(cb,arg,timeout) sys_iop_add(INVALID_HANDLE,0,arg,timeout)
#define SYS_IOP_ADD_PTIMER(cb,arg,timeout) sys_iop_add(INVALID_HANDLE,EV_TYPE_PERSIST_TIMER,arg,timeout)


#define IOP_CB_ARG_INIT(cb_arg, readcb,writecb,timercb,destroycb,keepalivecb,the_arg) do{ \
                                        (cb_arg)->read_cb = readcb;  \
                                        (cb_arg)->write_cb = writecb;    \
                                        (cb_arg)->timer_cb = timercb;    \
                                        (cb_arg)->destroy_cb = destroycb;    \
                                        (cb_arg)->keepalive_cb = keepalivecb;    \
                                        (cb_arg)->arg = the_arg;}while(0)



#define IOP_CB_DISPATCH(iop, base, id, event, read_cb, write_cb)    \
                                            do{   \
                                               if(EV_IS_SET_R(event))   {   \
                                                    (read_cb)(iop);    \
                                                    if(EV_IS_SET_W(event) && (iop->status != IOP_STATUS_EXIT)){  \
                                                        (write_cb)(iop);}   \
                                                    break;} \
                                                if(EV_IS_SET_W(event)){ \
                                                    (write_cb)(iop);    \
                                                    break;}    \
                                            }while(0);


#define IOP_GET(base,id,iop) array_list_get(&(base->iop_list),id,(void **)(&iop))


    struct tag_iop_op;
    typedef struct tag_iop_op iop_op_t;
    struct tag_iop_base;
    typedef struct tag_iop_base iop_base_t;
    struct tag_iop;
    typedef struct tag_iop iop_t;
    struct tag_iop_cb_arg;
    typedef struct tag_iop_cb_arg iop_cb_arg_t;
    struct tag_iop_timer;
    typedef struct tag_iop_timer iop_timer_t;


    /*事件回调函数类型定义
    *iop->id:事件id
    *iop->handle:事件关联的句柄
    *iop->revent:发生了什么事件
    *iop->cb_arg:事件关联的数据
    */
    typedef void (*iop_cb) (iop_t *iop);
    typedef void (*iop_free_cb) (void *arg);




    struct tag_iop_op
    {
        const char *name;                               //模型名称
        void (*base_free) (iop_base_t *);       //资源释放的接口
        int (*base_dispatch) (iop_base_t *, int); //模型调度接口
        //添加事件
        int (*base_add) (iop_base_t *, int, io_handle_t, unsigned int);
        //删除事件
        int (*base_del) (iop_base_t *, int, io_handle_t);
        //修改事件
        int (*base_mod) (iop_base_t *, int, io_handle_t, unsigned int);
    };

//定时器,只支持秒级精确度.
    struct tag_iop_timer
    {
        int id;                 //事件id
        int timeout;        //定时器间隔时间(单位:秒)
        int trigger_time;   //定时器下一次到期的时间(单位:秒)
        int is_persist;         //定时器是否是持久的.

        iop_cb timer_cb; //回调函数
        int prev;
        int next;
    };



    struct tag_iop_ka_timer
    {
        int id;
        int ka_timer_id;
        int timeout;
        iop_cb cb;
    };
    typedef struct tag_iop_ka_timer iop_ka_timer_t;

    typedef  int (*as_queue_item_proc) (iop_base_t *, time_t, int, const void *, unsigned int);


//iop基础数据
    struct tag_iop_base
    {
        time_t cur_time;            //当前时间.误差0.5秒左右
        time_t last_time;             //上一次处理事件循环的时间
        volatile int exit_loop;     //是否要退出
        iop_op_t *op_imp;           //事件模型的内部实现
        void *model_data;         //事件模型特定的数据
        array_list_t iop_list;                  //所有注册到这里的事件

        int ev_timer_head;          //第一个event定时器在iop_list中的索引
        time_t last_ev_time;            //上次检查event定时器的时间
        int check_ev_timer_interval;    //检查event定时器间隔


        iop_as_queue_t *asq;
        as_queue_item_proc aq_proc;
        volatile int dispatch_interval;

        int cur_id;                 //当前正在处理的事件
        void *ext_info;             //用户自定义的数据
    };



    struct tag_iop_cb_arg
    {
        iop_free_cb free_cb;
        void *arg;
    };

//iop事件数据
    struct tag_iop
    {
        time_t ctime;               //this object create time.
        iop_base_t *base;            //pointer to iop_base.
        int id;                      //iop id
        unsigned int revent;         //current event. 当前发生的事件
        int status;                  //当前状态
        io_handle_t handle;         //INVALID_HANDLE代表没有关联的句柄
        //关联的句柄。(socket, file description, pipe,etc...).
        time_t create_time;            //事件创建时间
        unsigned int event;            //关注的事件.   事件可以组合
        struct tag_iop_cb_arg cb_arg;          //事件关联的数据
        iop_cb cb;                             //事件发生时的处理函数
        //int pri;                             //优先级，暂不支持
        iop_timer_t *ev_timer;                 //定时器

    };


//ret=0: incomplete   ret<0: error      ret>0:ok
//const void *buf, unsigned int len.
    typedef int (*iop_protocol_parse_t) (const void *, unsigned int);

//void *buf, unsigned int len, iop_t *iop
    typedef int (*iop_protocol_proc_t) (const void *, unsigned int, iop_t *);


    struct tag_iop_protocol
    {
        /**
        *int iop_protocol_parse_t(const void *data,unsigned int data_len);
        * parse the input data.
        * @param  data:当前收到的数据  data_len:当前收到的数据长度
        * @return value: -1 error, 0 need more data, >0 ok.
        * 返回值-1代表协议解析出错
        *        0代表需要更多的数据，当前数据不足以解析出一个完整的消息
        *        >0代表解析出了一个完整的消息， 返回值为消息的长度
        **/
        iop_protocol_parse_t protocol_parse;

        /**
        * int iop_protocol_proc_t(const void *data,unsigned int data_len,iop_t *iop);
        * process request.
        * @param: data:消息数据
        *         data_len:消息长度
        *         iop:the iop object
        * @return: 0 ok, other error.
        *          返回值0代表处理成功，其它代表失败,框架并不关心这个返回值
        **/
        iop_protocol_proc_t process;

        /**
        * when a connection create, on_create will be callback.
        **/
        iop_cb on_create;

        /**
        * when a connection close, on_destroy will be callback.
        **/
        iop_cb on_destroy;
    };
    typedef struct tag_iop_protocol iop_protocol_t;

    extern iop_protocol_t IOP_PROTOCOL_INIT;
    extern iop_cb_arg_t IOP_CB_ARG_NULL;



#ifdef __cplusplus
}
#endif

#endif
