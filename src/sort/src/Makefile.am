LIBS=""
AM_LDFLAGS = -static-libtool-libs

SUBDIR=Util

lib_LTLIBRARIES=libsort.la
libsort_la_SOURCES= SortResult.cpp \
			SearchSortFacade.cpp \
			MergerSortFacade.cpp \
			SortConfig.cpp \
			SortQuery.cpp \
			AppManage.cpp \
			Util/MemPoolAlloc.cpp \
			Util/SDM.cpp \
			Util/SortCompare.cpp \
			Util/Reader/SDMReader.cpp \
			Util/Reader/ProfileReader.cpp \
			Util/HashMap.cpp \
			$(NULL)

#bin_PROGRAMS=sorttest
#sorttest_SOURCES=sorttest.cpp
#sorttest_LDFLAGS=libsort.la -lalog -lmxml -ldl -lpool -lksutil -lindex_lib -lqueryparser -lssl  -lprotobuf
