#ifndef _IOP_QUEUE_H
#define _IOP_QUEUE_H

#include <vector>
#include <queue>
#include <iostream>
#include <stdint.h>
#include <getopt.h>
#include "wcx_log.h"
#include "iop_obj_pool.h"
#include "iop.h"
#include "iop_service.h"
#include "iop_log_service.h"

typedef struct
{
    iop_t *iop;
    int sn;

} client_tp;

class iop_queue
{
public:
    std::vector<client_tp> client;
    iop_queue ();
    ~iop_queue ();
    void push (client_tp);
    int erase(client_tp);
    int get_size ();

private:
    struct log_tp error_log;
    pthread_mutex_t mutex;
};

#endif
