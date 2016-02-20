/*
 * Copyright (c) 2011-2014 zhangjianlin �ż���
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
//���¼�
#define EV_TYPE_READ                        1
//д�¼�
#define EV_TYPE_WRITE                       2


#define MAX_SOCK_BUF_SIZE           64000
#define IOP_ERR_SYS  -1
#define IOP_ERR_TIMEOUT  -2


//һ������¼����͵ĺ�
#define EV_IS_SET_R(e) ((e)&EV_TYPE_READ)
#define EV_IS_SET_W(e) ((e)&EV_TYPE_WRITE)

#define EV_SET_R(e) ((e) | EV_TYPE_READ)
#define EV_CLR_R(e) ((e) & ~EV_TYPE_READ)
#define EV_SET_W(e) ((e) | EV_TYPE_WRITE)
#define EV_CLR_W(e) ((e) & ~EV_TYPE_WRITE)

#define EV_SET_RW(e) ((e)|EV_TYPE_READ|EV_TYPE_WRITE)
#define EV_TYPE_RW (EV_TYPE_READ|EV_TYPE_WRITE)

//ϵͳ֧�ֵ��¼�ģ��

//NULL������ϵͳ�Զ�ѡ��
#define IOP_MODEL_NULL                      0
//event portsģ����δʵ��
#define IOP_MODEL_EVENT_PORTS       1
//kqueueģ����δʵ��
#define IOP_MODEL_KQUEUE                  2
//linux epollģ���Ѿ�֧��
#define IOP_MODEL_EPOLL                     3
//dev pollģ����δʵ��
#define IOP_MODEL_DEV_POLL              4
//pollģ���Ѿ�֧��
#define IOP_MODEL_POLL                       5
//select ģ���Ѿ�֧��. windows & linux
#define IOP_MODEL_SELECT                   6
//windows iocpģ����δʵ��
#define IOP_MODEL_IOCP                       7
//ֻ֧�ֶ�ʱ��
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


    /*�¼��ص��������Ͷ���
    *iop->id:�¼�id
    *iop->handle:�¼������ľ��
    *iop->revent:������ʲô�¼�
    *iop->cb_arg:�¼�����������
    */
    typedef void (*iop_cb) (iop_t *iop);
    typedef void (*iop_free_cb) (void *arg);




    struct tag_iop_op
    {
        const char *name;                               //ģ������
        void (*base_free) (iop_base_t *);       //��Դ�ͷŵĽӿ�
        int (*base_dispatch) (iop_base_t *, int); //ģ�͵��Ƚӿ�
        //�����¼�
        int (*base_add) (iop_base_t *, int, io_handle_t, unsigned int);
        //ɾ���¼�
        int (*base_del) (iop_base_t *, int, io_handle_t);
        //�޸��¼�
        int (*base_mod) (iop_base_t *, int, io_handle_t, unsigned int);
    };

//��ʱ��,ֻ֧���뼶��ȷ��.
    struct tag_iop_timer
    {
        int id;                 //�¼�id
        int timeout;        //��ʱ�����ʱ��(��λ:��)
        int trigger_time;   //��ʱ����һ�ε��ڵ�ʱ��(��λ:��)
        int is_persist;         //��ʱ���Ƿ��ǳ־õ�.

        iop_cb timer_cb; //�ص�����
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


//iop��������
    struct tag_iop_base
    {
        time_t cur_time;            //��ǰʱ��.���0.5������
        time_t last_time;             //��һ�δ����¼�ѭ����ʱ��
        volatile int exit_loop;     //�Ƿ�Ҫ�˳�
        iop_op_t *op_imp;           //�¼�ģ�͵��ڲ�ʵ��
        void *model_data;         //�¼�ģ���ض�������
        array_list_t iop_list;                  //����ע�ᵽ������¼�

        int ev_timer_head;          //��һ��event��ʱ����iop_list�е�����
        time_t last_ev_time;            //�ϴμ��event��ʱ����ʱ��
        int check_ev_timer_interval;    //���event��ʱ�����


        iop_as_queue_t *asq;
        as_queue_item_proc aq_proc;
        volatile int dispatch_interval;

        int cur_id;                 //��ǰ���ڴ������¼�
        void *ext_info;             //�û��Զ��������
    };



    struct tag_iop_cb_arg
    {
        iop_free_cb free_cb;
        void *arg;
    };

//iop�¼�����
    struct tag_iop
    {
        time_t ctime;               //this object create time.
        iop_base_t *base;            //pointer to iop_base.
        int id;                      //iop id
        unsigned int revent;         //current event. ��ǰ�������¼�
        int status;                  //��ǰ״̬
        io_handle_t handle;         //INVALID_HANDLE����û�й����ľ��
        //�����ľ����(socket, file description, pipe,etc...).
        time_t create_time;            //�¼�����ʱ��
        unsigned int event;            //��ע���¼�.   �¼��������
        struct tag_iop_cb_arg cb_arg;          //�¼�����������
        iop_cb cb;                             //�¼�����ʱ�Ĵ�������
        //int pri;                             //���ȼ����ݲ�֧��
        iop_timer_t *ev_timer;                 //��ʱ��

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
        * @param  data:��ǰ�յ�������  data_len:��ǰ�յ������ݳ���
        * @return value: -1 error, 0 need more data, >0 ok.
        * ����ֵ-1����Э���������
        *        0������Ҫ��������ݣ���ǰ���ݲ����Խ�����һ����������Ϣ
        *        >0������������һ����������Ϣ�� ����ֵΪ��Ϣ�ĳ���
        **/
        iop_protocol_parse_t protocol_parse;

        /**
        * int iop_protocol_proc_t(const void *data,unsigned int data_len,iop_t *iop);
        * process request.
        * @param: data:��Ϣ����
        *         data_len:��Ϣ����
        *         iop:the iop object
        * @return: 0 ok, other error.
        *          ����ֵ0���������ɹ�����������ʧ��,��ܲ��������������ֵ
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