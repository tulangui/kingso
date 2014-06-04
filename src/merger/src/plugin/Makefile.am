AM_LDFLAGS = -static-libtool-libs
LIBS=-lqueryparser -lsort -lstatistic -lindex_lib -lclustermap -lkslib -lprotobuf -lanalysis
bindir = $(prefix)/lib

lib_LTLIBRARIES = libmainse_plugin.la

libmainse_plugin_la_SOURCES = mainse_plugin.cc \
							DocumentRewrite.cpp \
							../frame/MemPoolFactory.cc \
							../frame/ResultFormat.cc \
							../frame/RowLocalizer.cc \
							../frame/Document.cc \
							../frame/KSResult.pb.cc \
                            $(NULL)
libmainse_plugin_la_LDFLAGS = -fPIC -module -shared

DEFS+=$(MACRO)
