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

OBJS:=$(patsubst %.S, %.o, $(SRC))
OBJS+=$(patsubst %.c, %.o, $(SRC))
OBJS+=$(patsubst %.cpp, %.o, $(SRC))
OBJS:=$(filter %.o, $(OBJS))

IPATH:=-I../wcx_utils/
IPATH+=-I../modbus/
IPATH+=-I../liblogger/

ifeq ($(ARCH),ARM)
TARGET:=libmodbus_RPC3FxC_arm.so
else
TARGET:=libmodbus_RPC3FxC_ubuntu.so
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
