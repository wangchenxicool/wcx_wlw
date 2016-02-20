#ARCH=ARM

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAG:=-g -rdynamic 
CFLAG+=-DMEMWATCH -DMW_STDIO

ifeq ($(ARCH),ARM)
IPATH:=-I/usr/local/src/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I../../modbus/
IPATH+=-I../../wcx_utils/
IPATH+=-I../
LPATH:=-L/usr/local/src/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L../../modbus/
LPATH+=-L../../wcx_utils/
LPATH+=-L../
else
IPATH:=-I/usr/local/src/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I../../modbus/
IPATH+=-I../../wcx_utils/
IPATH+=-I../
LPATH:=-L/usr/local/src/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L../../modbus/
LPATH+=-L../../wcx_utils/
LPATH+=-L../
endif

LIBS:=-lpthread
LIBS+=-levent
ifeq ($(ARCH),ARM)
LIBS+=-lwcx_utils_arm
LIBS+=-liec104_slave_arm
LIBS+=-lmodbus_arm
else
LIBS+=-lwcx_utils_ubuntu
LIBS+=-liec104_slave_ubuntu
LIBS+=-lmodbus_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=slave_arm.bin
else
TARGET:=slave_ubuntu.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(IPATH) $(LPATH) $(LIBS) 

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
