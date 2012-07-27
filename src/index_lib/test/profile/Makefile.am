AM_LDFLAGS=-static-libtool-libs
DEFS+=$(MACRO)

#LIBS=""
#CPPFLAGS="-I$(top_srcdir)/src/index"
#LDFLAGS="-D_FILE_OFFSET_BITS=64 -DSHMEM -I. $LDFLAGS -I`pwd`/include -I`pwd`/src -I`pwd`"

check_PROGRAMS=dotest 
#noinst_PROGRAMS=dotest
dotest_SOURCES=DoProfileTest.cpp \
		ProfileHashTableTest.cpp \
		ProfileMMapFileTest.cpp \
		ProfileEncodeFileTest.cpp \
		ProfileDocAccessorTest.cpp \
		ProfileManagerTest.cpp
dotest_LDADD=$(top_srcdir)/src/libindex_lib.la -lkslib -lssl -lcppunit

remove:
	-rm -rf *.encode*
	-rm -rf *.seg*
	-rm -rf profile.desc
	-rm -rf *.idx

#TESTS_ENVIRONMENT=
#TESTS=$(check_PROGRAMS)

