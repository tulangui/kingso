## -- Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/src
lib_LTLIBRARIES = libpool.la

libpool_la_SOURCES = backend/_MemPool.cpp MemMonitor.cpp

libpoolincludedir=$(includedir)/pool

libpoolinclude_HEADERS=\
	    Allocators.h\
            MonotoneChunkPolicy.h\
            ExpIncChunkPolicy.h\
            AdaptiveStatPolicy.h\
            ObjectPool.h\
            PooledSTLAllocator.h\
            PoolDefine.h\
            Pool.h\
            SimpleSTLAllocator.h\
            SimpleSegregatedPool.h\
            SynchronizedSSPool.h\
            Singleton.h\
            SingletonSegregatedPool.h\
            Guard.h\
            Mutex.h\
            backend/_MemPool.h\
            MemMonitor.h\
            TSS.h
