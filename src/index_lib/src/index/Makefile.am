AM_LDFLAGS = -static-libtool-libs


noinst_LTLIBRARIES=libindex.la
libindex_la_SOURCES=IndexBuilder.cpp \
	IndexFieldBuilder.cpp \
	IndexReader.cpp \
	IndexFieldReader.cpp \
	IndexIncManager.cpp \
	IndexInfoManager.cpp \
	IndexTerm.cpp \
	idx_dict.cpp \
	BaseFile.cpp \
	IdxMMapFile.cpp \
	IndexFieldInc.cpp \
	IndexFieldSyncManager.cpp \
	IndexTermParse.cpp \
	IndexIncMemManager.cpp \
	IndexTermFactory.cpp \
	IndexInfoManager.cpp \
	IndexCompression.cpp \
	mempool/Freelist.cpp \
	mempool/DelayFreeQueue.cpp \
	mempool/LogicMemSpace.cpp \
	mempool/MetaDataManager.cpp \
	mempool/ShareMemPool.cpp \
	mempool/ShareMemSegment.cpp \
	$(NULL)

bin_PROGRAMS=cleanAllMem dumpMemData
cleanAllMem_SOURCES=mempool/cleanAllMem.cpp
cleanAllMem_LDADD=libindex.la
dumpMemData_SOURCES=mempool/dumpMemData.cpp
dumpMemData_LDADD=libindex.la

DEFS+=$(MACRO)
