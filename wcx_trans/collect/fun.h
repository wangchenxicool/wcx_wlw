#ifndef _FUN_H
#define _FUN_H

/*
#define FC_READ_COIL_STATUS          0x01
#define FC_READ_INPUT_STATUS         0x02
#define FC_READ_HOLDING_REGISTERS    0x03
#define FC_READ_INPUT_REGISTERS      0x04
#define FC_FORCE_SINGLE_COIL         0x05
#define FC_PRESET_SINGLE_REGISTER    0x06
#define FC_READ_EXCEPTION_STATUS     0x07
#define FC_FORCE_MULTIPLE_COILS      0x0F
#define FC_PRESET_MULTIPLE_REGISTERS 0x10
#define FC_REPORT_SLAVE_ID           0x11
*/

#define  READ_HOLDING_REGISTERS          0x00 /* (0x03)�����ּĴ��� */ 
#define  READ_INPUT_REGISTERS            0x01 /* (0x04)������Ĵ��� */ 
#define  PRESET_SINGLE_REGISTER          0x02 /* (0x06)д�����Ĵ��� */ 
#define  PRESET_MULTIPLE_REGISTERS       0x03 /* (0x10)д����Ĵ��� */ 
#define  READ_COIL_STATUS                0x04 /* (0x01)����Ȧ */ 
#define  READ_INPUT_STATUS               0x05 /* (0x02)����ɢ������ */ 
#define  FORCE_SINGLE_COIL               0x06 /* (0x05)д������Ȧ */ 
#define  FORCE_MULTIPLE_COILS            0x07 /* (0x0F)д�����Ȧ */ 

#endif

