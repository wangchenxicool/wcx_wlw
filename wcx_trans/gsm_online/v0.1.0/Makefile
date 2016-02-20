CROSS=arm-linux-

CFLAG:=-g -rdynamic 

SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

TARGET:=gsm_online.bin

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LIBPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
