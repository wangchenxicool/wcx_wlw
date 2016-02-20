#ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src
INSTALL=install -o $(USER) -g $(USER)

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAGS=-c -W -O2
CFLAGS+=-pipe -fomit-frame-pointer -fpic -D_MP_MODE 

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/include
IPATH+=-I../../wcx_utils/
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/lib
LPATH+=-L../../wcx_utils/
else
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/include
IPATH+=-I../../wcx_utils/
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/lib
LPATH+=-L../../wcx_utils/
endif

ifeq ($(ARCH),ARM)
LIBS:=-lwcx_utils_arm
else
LIBS:=-lwcx_utils_ubuntu
endif
LIBS+=-lpthread
LIBS+=-lsqlite3
LIBS+=-levent

ifeq ($(ARCH),ARM)
TARGET:=wcx_client.bin
else
TARGET:=wcx_client.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
