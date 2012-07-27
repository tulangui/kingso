AM_LDFLAGS = -static-libtool-libs

$(shell protoc --cpp_out=./ ./KSResult.proto)

EXTRA_DIST =  merger.h \
			  MergerServer.h \
			  MergerServer.cc \
			  MemPoolFactory.h \
			  MemPoolFactory.cc \
			  ks_merger_server.cc \
			  RowLocalizer.h \
			  RowLocalizer.cc \
			  FlowControl.h \
			  FlowControl.cc \
			  ResultFormat.h \
			  ResultFormat.cc \
			  QueryParameter.h \
			  Document.h \
			  Document.cc \
			  DebugInfo.h \
			  DebugInfo.cc \
			  KSResult.pb.h \
			  KSResult.pb.cc

noinst_LTLIBRARIES=libmerger.la
libmerger_la_SOURCES=MergerServer.cc \
					 MemPoolFactory.cc \
					 FlowControl.cc \
					 RowLocalizer.cc \
					 $(NULL)

bin_PROGRAMS=ks_merger_server

ks_merger_server_SOURCES=ks_merger_server.cc
ks_merger_server_LDADD=libmerger.la -ldl -lssl  -lscws
