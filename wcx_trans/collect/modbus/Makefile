ARCH=ARM
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

ifeq ($(ARCH),ARM)
IPATH+=-I../wcx_utils/
IPATH+=-I../liblogger/
else
IPATH+=-I../wcx_utils
IPATH+=-I../liblogger
endif

ifeq ($(ARCH),ARM)
LPATH+=-L../wcx_utils/
LPATH+=-L../liblogger/
else
LPATH+=-L../wcx_utils
LPATH+=-L../liblogger
endif

ifeq ($(ARCH),ARM)
LIBS:=-lwcx_utils_arm
#LIBS+=-llogger_arm
else
LIBS:=-lwcx_utils_ubuntu
#LIBS+=-llogger_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=libmodbus_arm.so
else
TARGET:=libmodbus_ubuntu.so
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

