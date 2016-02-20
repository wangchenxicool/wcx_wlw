#ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

CFLAGS=-c -W -O2
CFLAGS+=-pipe -fomit-frame-pointer -fpic -D_MP_MODE 

ifeq ($(ARCH),ARM)
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/include
else
IPATH:=-I$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/include
IPATH+=-I$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/include
IPATH+=-I./libiop/ubuntu/include/
endif

ifeq ($(ARCH),ARM)
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/arm/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/arm/lib
else
LPATH:=-L$(OTHERSLIBPWD)/sqlite-3.7.3/output/ubuntu/lib
LPATH+=-L$(OTHERSLIBPWD)/dmalloc-5.5.2/output/ubuntu/lib
LPATH+=-L./libiop/ubuntu/lib/
endif

ifeq ($(ARCH),ARM)
else
endif
LIBS:=-lpthread
LIBS+=-lsqlite3
LIBS+=-liop

ifeq ($(ARCH),ARM)
TARGET:=trans_arm.bin
else
TARGET:=trans_ubuntu.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
