EXTRA_DIST =  DetailServer.h

AM_LDFLAGS=-static-libtool-libs

noinst_LTLIBRARIES=libdetailserver.la
libdetailserver_la_SOURCES=DetailServer.cc \
                           $(NULL)

bin_PROGRAMS=ks_detail_server

ks_detail_server_SOURCES=ks_detail_server.cc
ks_detail_server_LDADD=libdetailserver.la -ldl -lssl
