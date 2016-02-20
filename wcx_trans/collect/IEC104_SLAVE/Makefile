#ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src
INSTALL=install -o $(USER) -g $(USER)

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAG:=-g -rdynamic 
CFLAG+=-shared -fPCI 
#CFLAG+=-DDMALLOC -DDMALLOC_FUNC_CHECK
CFLAG+=-DMEMWATCH -DMW_STDIO

OBJS:=$(patsubst %.S, %.o, $(SRC))
OBJS+=$(patsubst %.c, %.o, $(SRC))
OBJS+=$(patsubst %.cpp, %.o, $(SRC))
OBJS:=$(filter %.o, $(OBJS))

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/include
IPATH+=-I../wcx_utils/
IPATH+=-I../modbus
IPATH+=-I../
else
IPATH:=-I$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
IPATH+=-I../wcx_utils
IPATH+=-I../modbus
IPATH+=-I../
endif

ifeq ($(ARCH),ARM)
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/arm/lib
LPATH+=-L../wcx_utils/
LPATH+=-L../modbus
else
LPATH:=-L$(OTHERSLIBPWD)/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
LPATH+=-L../wcx_utils
LPATH+=-L../modbus
endif

#ifeq ($(ARCH),ARM)
#LIBS:=-lwcx_utils_arm
#LIBS+=-lmodbus_arm
#else
#LIBS:=-lwcx_utils_ubuntu
#LIBS+=-lmodbus_ubuntu
#endif
#LIBS+=-levent
#LIBS+=-liconv

ifeq ($(ARCH),ARM)
TARGET:=libiec104_slave_arm.so
else
TARGET:=libiec104_slave_ubuntu.so
endif

$(TARGET) : $(OBJS)
	$(CROSS)g++ $(CFLAG) -o $(TARGET) $^ $(IPATH) $(LPATH) $(LIBS) 
	$(CROSS)strip $(TARGET)
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/
	rm *.o
	
%.o : %.c
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LPATH) $(LIBS) 

%.o : %.cpp
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LPATH) $(LIBS) 

install:
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/

clean:
	rm -f  *.bin  *.dis  *.elf  *.o *.so
