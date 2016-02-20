ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

CFLAG:=-g -rdynamic -fno-stack-protector 
CFLAG:=-g -O0 -rdynamic -fstack-protector-all
CFLAG+=-DMEMWATCH -DMW_STDIO
CFLAG+=-DDMALLOC -DDMALLOC_FUNC_CHECK
#CFLAG+=-fPIC

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/include
IPATH+=-I../
IPATH+=-I../../modbus/
IPATH+=-I../../wcx_utils
else
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/include
IPATH+=-I../
IPATH+=-I../../modbus/
IPATH+=-I../../wcx_utils
endif

ifeq ($(ARCH),ARM)
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/lib
LPATH+=-L../
LPATH+=-L../../modbus/
LPATH+=-L../../wcx_utils
else
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/lib
LPATH+=-L../
LPATH+=-L../../modbus/
LPATH+=-L../../wcx_utils
endif

ifeq ($(ARCH),ARM)
LIBS:=-lwcx_utils_arm
LIBS+=-lmodbus_arm
LIBS+=-lmodbus_RPC3FxC_arm
else
LIBS:=-lwcx_utils_ubuntu
LIBS+=-lmodbus_ubuntu
LIBS+=-lmodbus_RPC3FxC_ubuntu
endif
LIBS+=-ldmallocth
#LIBS+=-ldmalloc
LIBS+=-lsqlite3
LIBS+=-levent
LIBS+=-lpthread

ifeq ($(ARCH),ARM)
TARGET:=read_coil_status_RPC3FxC_arm.bin
else
TARGET:=read_coil_status_RPC3FxC_ubuntu.bin
endif


$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o

