#ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAG:=-g -rdynamic 

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/include
IPATH+=-I../
IPATH+=-I../../../wcx_utils/
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/lib
LPATH+=-L../
LPATH+=-L../../../wcx_utils
else
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/include
IPATH+=-I../
IPATH+=-I../../../wcx_utils
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/lib
LPATH+=-L../
LPATH+=-L../../../wcx_utils
endif

LIBS:=-lpthread
LIBS+=-levent
LIBS+=-lsqlite3
ifeq ($(ARCH),ARM)
LIBS+=-lwcx_utils_arm
LIBS+=-liec104_master_arm
else
LIBS+=-lwcx_utils_ubuntu
LIBS+=-liec104_master_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=iec104_master_test.bin
else
TARGET:=iec104_master_test.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(IPATH) $(LPATH) $(LIBS) 

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
