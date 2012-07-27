AM_LDFLAGS = -static-libtool-libs
SUBDIRS=index profile

lib_LTLIBRARIES=libindex_lib.la
libindex_lib_la_SOURCES=DocIdManager.cpp \
			IncManager.cpp \
			IndexLib.cpp \
			ProvCityManager.cpp \
			RealPostFeeManager.cpp \
			LatLngManager.cpp \
			IndexConfigParams.cpp \
			$(NULL)
libindex_lib_la_LIBADD=$(top_srcdir)/src/index/libindex.la \
			$(top_srcdir)/src/profile/libprofile.la \
			$(NULL)
libindex_lib_la_LDFLAGS=-static
libindex_libincludedir=$(includedir)/index_lib
libindex_libinclude_HEADERS=$(top_srcdir)/include/index_lib/IndexLib.h \
				$(top_srcdir)/include/index_lib/IndexBuilder.h \
				$(top_srcdir)/include/index_lib/IndexReader.h \
				$(top_srcdir)/include/index_lib/IndexIncManager.h \
				$(top_srcdir)/include/index_lib/IndexInfoManager.h \
				$(top_srcdir)/include/index_lib/IndexTerm.h \
				$(top_srcdir)/include/index_lib/DocIdManager.h \
				$(top_srcdir)/include/index_lib/IncManager.h \
				$(top_srcdir)/include/index_lib/ProfileManager.h \
				$(top_srcdir)/include/index_lib/ProfileStruct.h \
				$(top_srcdir)/include/index_lib/ProfileDocAccessor.h \
				$(top_srcdir)/include/index_lib/ProfileMMapFile.h \
				$(top_srcdir)/include/index_lib/ProfileEncodeFile.h \
				$(top_srcdir)/include/index_lib/ProfileHashTable.h \
				$(top_srcdir)/include/index_lib/ProfileSyncManager.h \
				$(top_srcdir)/include/index_lib/ProfileVarintWrapper.h \
				$(top_srcdir)/include/index_lib/ProfileMultiValueIterator.h \
				$(top_srcdir)/include/index_lib/varint.h \
				$(top_srcdir)/include/index_lib/ProvCityManager.h \
				$(top_srcdir)/include/index_lib/RealPostFeeManager.h \
				$(top_srcdir)/include/index_lib/RealPostFeeProcessor.h \
				$(top_srcdir)/include/index_lib/LatLngManager.h \
				$(top_srcdir)/include/index_lib/LatLngProcessor.h \
				$(top_srcdir)/include/index_lib/IndexConfigParams.h \
				$(NULL)
