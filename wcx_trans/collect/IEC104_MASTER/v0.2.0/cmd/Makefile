#ARCH=ARM
SRC:=$(shell ls *.c)
SRC+=$(shell ls *.cpp)

ifeq ($(ARCH),ARM)
	CROSS=arm-linux-
endif

CFLAG:=-g -O0 -rdynamic -fstack-protector-all
#CFLAG+=-DMEMWATCH -DMW_STDIO
#CFLAG+=-DDMALLOC -DDMALLOC_FUNC_CHECK

ifeq ($(ARCH),ARM)
	#IPATH:=-I./iec104/
	#IPATH:=-I/usr/local/src/sqlite-3.7.3/output/arm/include
	#IPATH+=-I/usr/local/src/dmalloc-5.5.2/output/arm/include
	#LIBPATH:=-L./iec104/
	#LIBPATH+=-L/usr/local/src/sqlite-3.7.3/output/arm/lib
	#LIBPATH+=-L/usr/local/src/dmalloc-5.5.2/output/arm/lib
else
	#IPATH:=-I./iec104/
	#IPATH:=-I/usr/local/src/sqlite-3.7.3/output/ubuntu/include
	#IPATH+=-I/usr/local/src/dmalloc-5.5.2/output/ubuntu/include
	#LIBPATH:=-L./iec104/
	#LIBPATH+=-L/usr/local/src/sqlite-3.7.3/output/ubuntu/lib
	#LIBPATH+=-L/usr/local/src/dmalloc-5.5.2/output/ubuntu/lib
endif

LIBS:=-lpthread 
#LIBS+=-lsqlite3
#LIBS+=-ldmalloc
#LIBS+=-ldmallocth
#LIBS+=-liec104

TARGET:=cmd.bin

$(TARGET) : $(SRC)
	$(CROSS)g++ $(CFLAG) -o  $(TARGET)  $^ $(LIBPATH) $(IPATH) $(LIBS)

clean:
	rm -f  *.bin  *.dis  *.elf  *.o
