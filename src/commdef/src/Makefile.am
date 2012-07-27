SUBDIRS=docmessage resultformat

AM_LDFLAGS = -static-libtool-libs

lib_LTLIBRARIES=libcommdef.la
libcommdef_la_SOURCES=docmessage/DocIndexMessage.pb.cc \
					 docmessage/DocMessage.pb.cc \
					 SourceFilter.cpp \
					 resultformat/KSResult.pb.cc \
					 ${NULL}

.PHONY: clean
clean:
	make -C docmessage clean
	rm -rf *.pb.* *.o *.lo *.la .libs/
