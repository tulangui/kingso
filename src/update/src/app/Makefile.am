AM_LDFLAGS=-static-libtool-libs

LIBS=-lbuild -lkslib -lanalysis -lscws -lcommdef -lindex_lib -lprotobuf -lssl  

bin_PROGRAMS=ks_dispatcher ks_notify_pusher 

ks_dispatcher_SOURCES=ks_updater_server.cpp
ks_dispatcher_LDADD=$(top_srcdir)/src/libupdater.la

ks_notify_pusher_SOURCES=notify.cpp
ks_notify_pusher_LDADD=$(top_srcdir)/src/libupdater.la $(top_srcdir)/src/api/libupdate_api.la

noinst_PROGRAMS=test_que

test_que_SOURCES=testFileQue.cpp
test_que_LDADD=$(top_srcdir)/src/libupdater.la

