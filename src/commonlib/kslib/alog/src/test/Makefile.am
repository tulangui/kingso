check_PROGRAMS = ThreadTest LoggerTest AppenderTest ConfiguratorTest LayoutTest
ThreadTest_SOURCES = ThreadTest.cpp
ThreadTest_LDADD=$(top_srcdir)/src/cpp/libalog.la
LoggerTest_SOURCES = LoggerTest.cpp
LoggerTest_LDADD=$(top_srcdir)/src/cpp/libalog.la
AppenderTest_SOURCES = AppenderTest.cpp
AppenderTest_LDADD=$(top_srcdir)/src/cpp/libalog.la
ConfiguratorTest_SOURCES = ConfiguratorTest.cpp
ConfiguratorTest_LDADD=$(top_srcdir)/src/cpp/libalog.la
LayoutTest_SOURCES = LayoutTest.cpp
LayoutTest_LDADD=$(top_srcdir)/src/cpp/libalog.la


INCLUDES=-I$(top_srcdir)/src/include

##add by zhangli
EXTRA_DIST=tut.hpp tut_reporter.hpp tut_restartable.hpp test.h.in t1.conf.in t2.conf.in t3.conf.in t4.conf.in t5.conf.in t6.conf.in t7.conf.in t8.conf.in t9.conf.in t10.conf.in t11.conf.in t12.conf.in
TESTS=$(check_PROGRAMS)
