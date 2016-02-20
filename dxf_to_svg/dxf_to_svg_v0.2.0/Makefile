#ARCH=ARM
OTHERSLIBPWD=/home/project/usr/local/src

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAGS:=-Wall -D_REENTRANT -D_GNU_SOURCE -g -fPIC 

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
else
IPATH:=-I$(OTHERSLIBPWD)/dxflib-2.2.0.0-1.src/output/ubuntu/include/dxflib/
IPATH+=-I$(OTHERSLIBPWD)/mxml-2.7/output/ubuntu/include/
LPATH:=-L$(OTHERSLIBPWD)/dxflib-2.2.0.0-1.src/output/ubuntu/lib/
LPATH+=-L$(OTHERSLIBPWD)/mxml-2.7/output/ubuntu/lib/
LPATH+=-L/usr/X11R6/lib
endif

#LIBS:=-lpthread -ldxflib -lX11
LIBS:=-lpthread -ldxflib
LIBS+=-lmxml
LIBS+=-lm

ifeq ($(ARCH),ARM)
TARGET:=to_svg.bin
else
TARGET:=to_svg.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o $(TARGET) $^ $(IPATH) $(LPATH) $(LIBS) 

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
