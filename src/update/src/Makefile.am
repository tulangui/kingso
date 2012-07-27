SUBDIRS=common plugin api 

AM_LDFLAGS = -static-libtool-libs
	    
lib_LTLIBRARIES=libupdater.la
libupdater_la_SOURCES=UpdaterServer.cpp \
					  $(NULL)
libupdater_la_LIBADD=plugin/libplugin.la \
					 common/libupdater_common.la \
					 $(NULL)
libupdater_la_LDFLAGS=-static

