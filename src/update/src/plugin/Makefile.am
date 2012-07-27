AM_LDFLAGS = -static-libtool-libs


lib_LTLIBRARIES=libplugin.la 
libplugin_la_SOURCES=EnterWorker.cpp \
					 DetailWorker.cpp \
					 SearcherWorker.cpp \
					 SignDict.cpp \
					 QueueCache.cpp \
					 WorkerFactory.cpp \
					 $(NULL)
libplugin_la_LDFLAGS=../common/libupdater_common.la -lprotobuf

