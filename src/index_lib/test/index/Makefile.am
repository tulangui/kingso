AM_LDFLAGS=-static-libtool-libs

INCLUDES=-I$(top_srcdir)/src/index
export INCLUDES

check_PROGRAMS=dotest testNext

dotest_SOURCES=dotest.cpp \
			   test.cpp \
			   template.cpp \
			   testIdxDict.cpp \
			   testIndexTerm.cpp \
			   testIndexBuilder.cpp \
			   testIndexFieldBuilder.cpp \
			   testIndexFieldReader.cpp \
			   testIndexReader.cpp \
			   testIndexIncManager.cpp \
			   testIndexTermParse.cpp 
dotest_LDADD=$(top_srcdir)/src/libindex_lib.la -lkslib -lssl -lcppunit -lcommdef -lprotobuf 


testNext_SOURCES=testBitmapNext.cpp \
				 $(top_srcdir)/src/index/IndexTerm.cpp \
				 $(NULL)
testNext_LDADD=$(top_srcdir)/src/libindex_lib.la -lkslib -lssl -lcommdef -lprotobuf 
testNext_LDFLAGS=$(CXXFLAGS) 

TESTS_ENVIRONMENT=
#valgrind --leak-check=full --track-fds=yes  
TESTS=$(check_PROGRAMS)

DEFS+=$(MACRO)
