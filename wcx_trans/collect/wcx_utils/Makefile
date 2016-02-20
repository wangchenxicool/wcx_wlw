ARCH=ARM
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
IPATH:=-I/usr/local/src/socket-lib/libevent-2.0.12-stable/output/arm/include
LPATH:=-L/usr/local/src/socket-lib/libevent-2.0.12-stable/output/arm/lib
else
IPATH:=-I/usr/local/src/socket-lib/libevent-2.0.12-stable/output/ubuntu/include
LPATH:=-L/usr/local/src/socket-lib/libevent-2.0.12-stable/output/ubuntu/lib
endif

#LIBS:=-liconv

ifeq ($(ARCH),ARM)
TARGET:=libwcx_utils_arm.so
else
TARGET:=libwcx_utils_ubuntu.so
endif

$(TARGET) : $(OBJS)
	$(CROSS)g++ $(CFLAG) -o $(TARGET) $^ $(IPATH) $(LPATH) $(LIBS) 
	$(CROSS)strip $(TARGET)
	rm *.o
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/
	
%.o : %.c
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LIBPATH) $(LIBS) 

%.o : %.cpp
	$(CROSS)g++ -c -o $@ $^ $(IPATH) $(LIBPATH) $(LIBS) 

install:
	@$(INSTALL) -m 755 ./$(TARGET) /usr/lib/

clean:
	rm -f  *.bin  *.dis  *.elf  *.o *.so

