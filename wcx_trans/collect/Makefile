ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

CFLAG:=-g -rdynamic -fno-stack-protector 
#CFLAG:=-g -O0 -rdynamic -fstack-protector-all
CFLAG+=-DMEMWATCH -DMW_STDIO
CFLAG+=-DDMALLOC -DDMALLOC_FUNC_CHECK
#CFLAG+=-fPIC

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/include
IPATH+=-I./modbus/
IPATH+=-I./modbus_RPC3FxC/
IPATH+=-I./modbus_LD_B10_220/
IPATH+=-I./IEC104_MASTER/v0.2.0/
IPATH+=-I./IEC104_SLAVE/
IPATH+=-I./wcx_utils
else
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/include
IPATH+=-I./modbus/
IPATH+=-I./modbus_RPC3FxC/
IPATH+=-I./modbus_LD_B10_220/
IPATH+=-I./IEC104_MASTER/v0.2.0/
IPATH+=-I./IEC104_SLAVE/
IPATH+=-I./wcx_utils
endif

ifeq ($(ARCH),ARM)
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/lib
LPATH+=-L./modbus/
LPATH+=-L./modbus_RPC3FxC/
LPATH+=-L./modbus_LD_B10_220/
LPATH+=-L./IEC104_SLAVE/
LPATH+=-L./IEC104_MASTER/v0.2.0/
LPATH+=-L./wcx_utils
else
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/lib
LPATH+=-L./modbus/
LPATH+=-L./modbus_RPC3FxC/
LPATH+=-L./modbus_LD_B10_220/
LPATH+=-L./IEC104_SLAVE/
LPATH+=-L./IEC104_MASTER/v0.2.0/
LPATH+=-L./wcx_utils
endif

ifeq ($(ARCH),ARM)
LIBS:=-lwcx_utils_arm
LIBS+=-ldl
LIBS+=-lmodbus_arm
LIBS+=-lmodbus_RPC3FxC_arm
LIBS+=-lmodbus_LD_B10_220_arm
LIBS+=-liec104_master_arm
else
LIBS:=-lwcx_utils_ubuntu
LIBS+=-ldl
LIBS+=-lmodbus_ubuntu
LIBS+=-lmodbus_RPC3FxC_ubuntu
LIBS+=-lmodbus_LD_B10_220_ubuntu
LIBS+=-liec104_master_ubuntu
endif
LIBS+=-lpthread
LIBS+=-ldmallocth
#LIBS+=-ldmalloc
LIBS+=-lsqlite3
LIBS+=-levent

ifeq ($(ARCH),ARM)
TARGET:=wcx_collect.bin
else
TARGET:=wcx_collect_ubuntu.bin
endif


$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
