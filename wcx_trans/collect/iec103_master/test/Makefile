#ARCH=ARM

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
CROSS=arm-linux-
endif

CFLAG:=-g -rdynamic 
#CFLAG+=-ldl
CFLAG+=-fno-stack-protector

ifeq ($(ARCH),ARM)
IPATH+=-I../
LPATH+=-L../
else
IPATH+=-I../
LPATH+=-L../
endif

LIBS:=-lpthread
ifeq ($(ARCH),ARM)
LIBS+=-liec103_master_arm
else
LIBS+=-liec103_master_ubuntu
endif

ifeq ($(ARCH),ARM)
TARGET:=iec103_master_test_arm.bin
else
TARGET:=iec103_master_test_ubuntu.bin
endif

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(IPATH) $(LPATH) $(LIBS) 

clean:
	rm -f  *.bin  *.dis  *.elf  *.o

