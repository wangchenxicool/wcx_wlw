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
CFLAG+=-DMEMWATCH -DMW_STDIO

OBJS:=$(patsubst %.S, %.o, $(SRC))
OBJS+=$(patsubst %.c, %.o, $(SRC))
OBJS+=$(patsubst %.cpp, %.o, $(SRC))
OBJS:=$(filter %.o, $(OBJS))

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
LIBS+=-levent
LIBS+=-lsqlite3

ifeq ($(ARCH),ARM)
TARGET:=libiec104_master_arm.so
else
TARGET:=libiec104_master_ubuntu.so
endif

$(TARGET) : $(OBJS)
	$(CROSS)g++ $(CFLAG) -o $(TARGET) $^ $(IPATH) $(LPATH) $(LIBS) 
	$(CROSS)strip $(TARGET)
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/
	rm *.o
	
%.o : %.c
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LIBPATH) $(LIBS) 

%.o : %.cpp
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LIBPATH) $(LIBS) 

install:
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/

clean:
	rm -f  *.bin  *.dis  *.elf  *.o *.so
