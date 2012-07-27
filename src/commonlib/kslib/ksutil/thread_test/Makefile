.SUFFIXES: .c .o

CFLAG = -Wall -O2
CFFFLAG = -Wall -O2
INC_PATH = -I/usr/local/include -I../include/util
LIB_PATH = -L/usr/local/lib -lpthread ../src/.libs/libksutil.a 

SHARED_OBJS = task_queue.o

ALL_OBJS = $(SHARED_OBJS)

ALL_PRGS = m2 M1ThreadPool

all: $(ALL_OBJS) $(ALL_PRGS)
.o:
	$(CC) $(CFLAG) -o $@ $<  $(SHARED_OBJS) $(LIB_PATH) $(INC_PATH)
.c:
	$(CC) $(CFLAG) -o $@ $<  $(ALL_OBJS) $(LIB_PATH) $(INC_PATH)
.cpp:
	$(CXX) $(CFFFLAG) -o $@ $<  $(ALL_OBJS) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(CC) $(CFLAG) -c -o $@ $<  $(INC_PATH)
#install:
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS)
