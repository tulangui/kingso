AM_LDFLAGS = -static-libtool-libs
LIBS=-lqueryparser -lsort -lstatistic -lindex_lib -lclustermap -lkslib -lprotobuf -lanalysis
bindir = $(prefix)/lib

bin_PROGRAMS=libmainse_plugin.so
libmainse_plugin_so_SOURCES= \
							mainse_plugin.cc \
							DocumentRewrite.cpp \
							../frame/MemPoolFactory.cc \
							../frame/ResultFormat.cc \
							../frame/RowLocalizer.cc \
							../frame/Document.cc \
							../frame/KSResult.pb.cc \
                            $(NULL)
libmainse_plugin_so_CPPFLAGS=-fPIC
libmainse_plugin_so_LDFLAGS=-module -shared

DEFS+=$(MACRO)
