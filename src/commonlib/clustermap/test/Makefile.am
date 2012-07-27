AM_LDFLAGS=-static-libtool-libs

check_PROGRAMS=dotest
dotest_SOURCES=dotest.cpp \
               ClustermapServerTest.cpp \
			   $(NULL)

dotest_LDADD=../src/libclustermap.la -lkslib -lpthread -lz -lcppunit -ldl

TESTS_ENVIRONMENT=LD_PRELOAD=/usr/lib/libtcmalloc.so HEAPCHECK=normal
TESTS=$(check_PROGRAMS)
