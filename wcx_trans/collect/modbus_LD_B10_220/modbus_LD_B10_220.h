#ifndef _MODBUS_LD_B10_220_H
#define _MODBUS_LD_B10_220_H

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#if defined(__FreeBSD__ ) && __FreeBSD__ < 5
#include <netinet/in_systm.h>
#endif
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <termios.h>

#include "wcx_utils.h"
#include "modbus.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int read_holding_registers_LD_B10_220 (modbus_param_t *mb_param, int start_addr, int nb,
                                       uint32_t *data_dest, int select_time);
int read_coil_status_LD_B10_220 (modbus_param_t *mb_param, uint8_t *data_dest, int select_time);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _MODBUS_H_ */

