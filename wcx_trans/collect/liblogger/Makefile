#ARCH=ARM
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
#IPATH+=-I../wcx_utils/
else
#IPATH+=-I../wcx_utils
endif

ifeq ($(ARCH),ARM)
#LPATH+=-L../wcx_utils/
else
#LPATH+=-L../wcx_utils
endif

ifeq ($(ARCH),ARM)
#LIBS:=-lwcx_utils_arm
else
#LIBS:=-lwcx_utils_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=liblogger_arm.so
else
TARGET:=liblogger_ubuntu.so
endif

$(TARGET) : $(OBJS)
	$(CROSS)gcc $(CFLAG) -o $(TARGET) $^ $(IPATH) $(LPATH) $(LIBS) 
	$(CROSS)strip $(TARGET)
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/
	rm *.o
	
%.o : %.c
	$(CROSS)gcc -c -o $@ $^ $(IPATH) $(LPATH) $(LIBS) 

%.o : %.cpp
	$(CROSS)gcc -c -o $@ $^ $(IPATH) $(LPATH) $(LIBS) 

install:
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/

clean:
	rm -f  *.bin  *.dis  *.elf  *.o *.so

