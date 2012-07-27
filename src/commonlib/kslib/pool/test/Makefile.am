INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/test
check_PROGRAMS = perftest unittest 

perftest_SOURCES = PerfTest.cpp

unittest_SOURCES = UnitTest.cpp \
	UnitTestFramework.h\
	UnitTestFramework.cpp\
	PoolTest.h

EXTRA_DIST = MemMonitorTest.h\
	ObjectPoolTest.h\
	PoolTest.h\
	PooledSTLAllocatorTest.h\
	SimpleSTLAllocatorTest.h\
	UnitTestFramework.h

perftest_LDADD = $(top_builddir)/src/libpool.la
perftest_LDFLAGS = $(LIBADD_DL)

unittest_LDADD = $(top_builddir)/src/libpool.la
unittest_LDFLAGS = $(LIBADD_DL)

TESTS=unittest
