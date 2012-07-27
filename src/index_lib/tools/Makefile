ROOTPATH=/home/yewang

LDPATH=\
	-L. \
	-L/usr/lib64/ \
	-L$(ROOTPATH)/install_commonlib/lib/


INCLUDE=\
	-I. \
	-I/usr/include/ \
	-I$(ROOTPATH)/kingso/commonlib/libutil/trunk/include/ \
	-I../ \
	-I../include \
	-I$(ROOTPATH)/install_commonlib/include \
	-I$(ROOTPATH)/kingso/project/commdef/trunk/include 

CFLAG = -c -ggdb -Wall -DDEBUG

GCC = g++
CC = gcc
.SUFFIXES: .o .cpp .c

.cpp.o:
	$(GCC) $(CFLAG)  $(INCLUDE) $(UNIT_TEST) -o $@ $<



all: DocIdDictCreater DocIdDictChecker



DocIdDictCreater: DocIdDictCreater.o ../src/DocIdManager.o
	$(GCC) $(LDPATH) -o $@  $^  -lalog -lksutil


DocIdDictChecker: DocIdDictChecker.o ../src/DocIdManager.o
	$(GCC) $(LDPATH) -o $@  $^  -lalog -lksutil



MlrEncoder: MlrEncoder.o ../src/varint.o
	$(GCC) $(LDPATH) -o $@  $^ 


DocIdDictCreater.o: DocIdDictCreater.cpp
DocIdDictChecker.o: DocIdDictChecker.cpp
MlrEncoder.o: MlrEncoder.cpp


clean:
	rm -f *.o
	rm -f DocIdDictCreater
	rm -f DocIdDictChecker