#include "iop_queue.h"
#include "wcx_utils.h"
#include "wcx_log.h"

iop_queue::iop_queue ()
{
    pthread_mutex_init (&mutex, NULL);
    log_init (&error_log, "/etc/iop_queue.log", "iop_queue", "v0.1.0", "wang_chen_xi", "err log");
}

iop_queue::~iop_queue ()
{
}

void iop_queue::push (client_tp p_client)
{
    client.push_back (p_client);
}

int iop_queue::erase (client_tp p_client)
{
    for (size_t i = 0; i < client.size(); i++)
    {
        if (client[i].iop == p_client.iop && client[i].sn == p_client.sn)
        {
            client.erase (client.begin() + i);
            return 0;
        }
    }
    return -1;
}

int iop_queue::get_size ()
{
    return client.size ();
}
