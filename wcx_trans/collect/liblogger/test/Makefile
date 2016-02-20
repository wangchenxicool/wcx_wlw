#ARCH=ARM

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

#CFLAG:=-g -rdynamic -fno-stack-protector 
CFLAG:=-g -O0 -rdynamic -fstack-protector-all
CFLAG+=-DMEMWATCH -DMW_STDIO
CFLAG+=-DDMALLOC -DDMALLOC_FUNC_CHECK
#CFLAG+=-fPIC

ifeq ($(ARCH),ARM)
IPATH:=-I../
else
IPATH:=-I../
endif

ifeq ($(ARCH),ARM)
LPATH:=-L../
else
LPATH:=-L../
endif

ifeq ($(ARCH),ARM)
LIBS+=-llogger_arm
else
LIBS+=-llogger_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=log_arm.bin
else
TARGET:=log_ubuntu.bin
endif


$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o

