EXTRA_DIST =  searcher.h \
			  ResultSerializer.h \
			  SearcherServer.h \
			  ResultSerializer.cpp \
			  SearcherServer.cpp \
			  ks_searcher_server.cpp \
			  test/QueryParameter.h \
			  Document.h \
			  ResultFormat.h \
   			  ResultFormat.cpp \
			  FormatProcessor.h \
			  FormatProcessor.cpp 

AM_LDFLAGS = -static-libtool-libs

#SUBDIRS=cache

INCLUDES = "-I ./test"

noinst_LTLIBRARIES=libsearcher.la
libsearcher_la_SOURCES= SearcherServer.cpp \
				ResultSerializer.cpp \
				ResultFormat.cpp \
				FormatProcessor.cpp \
			 $(NULL)

bin_PROGRAMS=ks_searcher_server

ks_searcher_server_SOURCES=ks_searcher_server.cpp
#ks_searcher_server_LDADD=libsearcher.la $(top_srcdir)/src/cache/libsearchercache.la -ldl -lssl
ks_searcher_server_LDADD=libsearcher.la -ldl -lssl  -lscws
