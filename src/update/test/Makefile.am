AM_LDFLAGS=-static-libtool-libs

#INCLUDES=-I$(top_srcdir)/src/index
#export INCLUDES

check_PROGRAMS=dotest 

dotest_SOURCES=dotest.cpp \
			   testTaskQueue.cpp \
			   $(NULL)
dotest_LDADD=$(top_srcdir)/src/libupdater.la -lkslib -lssl -lcppunit -lcommdef

TESTS=$(check_PROGRAMS)

#bin_PROGRAMS=FileQueue_test frame_talker simulator

#FileQueue_test_SOURCES=FileQueue_test.cc
#FileQueue_test_LDADD=-lupdater_commen -L../src/commen/

#frame_talker_SOURCES=frame_talker.cc
#frame_talker_LDADD=-lupdater_commen -L../src/commen/ -lupdate_api -L../src/api

#simulator_SOURCES=simulator.cc
#simulator_LDADD=-ldl -lssl -lupdater -L../src/api

#lib_LTLIBRARIES=libframe_plugin_test.la

#libframe_plugin_test_la_SOURCES=frame_plugin_test.cc \
								$(NULL)
#libframe_plugin_test_la_LDFLAGS=-module -shared -lupdater_commen -L../src/commen

DEFS+=$(MACRO)
