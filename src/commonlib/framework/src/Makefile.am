AM_LDFLAGS = -static-libtool-libs
CURRENTPATH=$(shell /bin/pwd)
INCLUDES=-I$(CURRENTPATH)/../.
export INCLUDES

lib_LTLIBRARIES=libframework.la
libframework_la_SOURCES=Dispatcher.cpp \
									Worker.cpp \
									TaskQueue.cpp \
									ParallelCounter.cpp \
									Service.cpp \
									Server.cpp \
									Command.cpp \
									Request.cpp \
									ConnectionPool.cpp \
									Query.cpp \
                                    Session.cpp \
									WebProbe.cpp \
									MemPoolFactory.cpp \
									Compressor.cpp \
									Cache.cpp \
									$(NULL)

