ROOTPATH=/home/yewang

LDPATH=\
	-L. \
	-L/usr/lib64/ \
	-L${ROOTPATH}/install_commonlib/lib/ \

INCLUDE=\
	-I. \
	-I/usr/include/ \
	-I$(ROOTPATH)/kingso/commonlib/libutil/trunk/include/ \
	-I../ \
	-I../../include \
	-I$(ROOTPATH)/install_commonlib/include \
	-I$(ROOTPATH)/kingso/project/commdef/trunk/include 

CFLAG = -c -ggdb -Wall -DDEBUG

GCC = g++
CC = gcc
.SUFFIXES: .o .cpp .c

.cpp.o:
	$(GCC) $(CFLAG)  $(INCLUDE) $(UNIT_TEST) -o $@ $<



all: DocIdManagerTest ProvCityManagerTest VarintTest GroupVarintTest IndexLibTest


IndexLibTest: IndexLibTest.o ../../src/.libs/libindex_lib.a 
	$(GCC) $(LDPATH) -o $@  $^  -lpthread  -lalog -lprotobuf -lmxml -lksutil -lssl -lcommdef
	
GroupVarintTest: GroupVarintTest.o  ../../src/varint.o	
	$(GCC) $(LDPATH) -o $@  $^

VarintTest: VarintTest.o ../../src/varint.o	
	$(GCC) $(LDPATH) -o $@  $^  -lgtest -lpthread

DocIdManagerTest: DocIdManagerTest.o ../../src/DocIdManager.o
	$(GCC) $(LDPATH) -o $@  $^  -lgtest -lpthread -lalog -lksutil

ProvCityManagerTest: ProvCityManagerTest.o ../../src/ProvCityManager.o
	$(GCC) $(LDPATH) -o $@  $^ -lksutil -lgtest -lpthread -lalog -lssl

InlineTest: InlineTest.o ClassA.o
	$(GCC) $(LDPATH) -o $@  $^

DocIdManagerTest.o: DocIdManagerTest.cpp 
ProvCityManagerTest.o: ProvCityManagerTest.cpp
VarintTest.o: VarintTest.cpp
GroupVarintTest.o: GroupVarintTest.cpp
IndexLibTest.o: IndexLibTest.cpp
ClassA.o: ClassA.cpp
InlineTest.o: InlineTest.cpp

clean:
	rm -f *.o
	rm -f *Test
