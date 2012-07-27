#compiler
GCC = g++
AR = ar

#directory definition
WORKPATH   = ./

#macro definition
DEFINES = -DLINUX -D_REENTERANT -D_FILE_OFFSET_BITS=64

#include directory
INCLUDE = -I.

#lib directory
LIB = -L./lib -lmempool

#compile flags
CPPFLAGS = -g -finline-functions -Wall -W -Winline -pipe $(DEFINES)

#lib name definition
TARGET_LIB = ./lib/libmempool.a
EXECUTABLE1 = cleanAllMem
EXECUTABLE2 = dumpMemData

#objects definition
LIB_OBJS = DelayFreeQueue.o  Freelist.o  LogicMemSpace.o  MetaDataManager.o  ShareMemPool.o  ShareMemSegment.o
EXEC_OBJS1 = cleanAllMem.o
EXEC_OBJS2 = dumpMemData.o


#targets definition
all	: $(TARGET_LIB) $(EXECUTABLE1) $(EXECUTABLE2)
	rm -f *.o

$(EXECUTABLE1) : $(EXEC_OBJS1)
	$(GCC) -o $@ $^ $(LIB)

$(EXECUTABLE2) : $(EXEC_OBJS2)
	$(GCC) -o $@ $^ $(LIB)

$(TARGET_LIB) : $(LIB_OBJS)
	$(AR) cr $@ $^

%.o     : %.cpp
	$(GCC) -c $< -o $@ $(CPPFLAGS) $(INCLUDE)

clean	:
	-rm -f *.o
	-rm $(TARGET_LIB)
	-rm $(EXECUTABLE1)
	-rm $(EXECUTABLE2)
