#$(shell protoc --cpp_out=./ -I=./ -I=./ ./kingso_cm.proto)

AM_LDFLAGS = -static-libtool-libs
lib_LTLIBRARIES=libclustermap.la
libclustermap_la_SOURCES=CMClient.cpp \
                         CMTree.cpp \
                         CMClientInterface.cpp \
                         CMClientLocal.cpp \
                         CMClientRemote.cpp \
                         DGramCommon.cpp \
                         DGramClient.cpp \
                         DGramServer.cpp \
                         CMTCPClient.cpp \
                         CMServer.cpp \
                         CMWatchPoint.cpp \
                         CMNode.cpp \
                         $(NULL)

libclustermapincludedir=$(includedir)/clustermap
libclustermapinclude_HEADERS=CMClient.h \
                             CMClientInterface.h \
                             CMNode.h \
                             CMTree.h \
                             CMWatchPoint.h \
                             DGramCommon.h \
                             DGramClient.h \
                             DGramServer.h \
                             $(NULL)

bin_PROGRAMS=nodeinfo clustermap webserver sync

nodeinfo_SOURCES=CMClientNodeInfoMain.cpp \
				 kingso_cm.pb.cc \
				 $(NULL)
nodeinfo_LDADD= libclustermap.la -lkslib -lpthread -lz -lprotobuf

clustermap_SOURCES=ClusterServerMain.cpp \
				   CMTCPServer.cpp \
				   kingso_cm.pb.cc \
				   $(NULL)
clustermap_LDADD= libclustermap.la -lkslib -lpthread -lz -lprotobuf

webserver_SOURCES=CMWebServerMain.cpp
webserver_LDADD=libclustermap.la -lkslib -lpthread -lz -lprotobuf

sync_SOURCES=sync.cpp

noinst_PROGRAMS=webclient

webclient_SOURCES=WebClient.cpp
webclient_LDADD=libclustermap.la -lkslib -lpthread -lz -lprotobuf

