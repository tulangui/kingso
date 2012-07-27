#ifndef _MS_MERGER_MEMPOOL_FACTORY_H_
#define _MS_MERGER_MEMPOOL_FACTORY_H_
#include "merger.h"
#include "util/ThreadLock.h"
#include "util/namespace.h"
#include <stack>

class MemPool;

MERGER_BEGIN;

class MemPoolFactory {
	public:
		MemPoolFactory();
		~MemPoolFactory();
	public:
		void setMSize(uint32_t size) { _msize = size; }
		MemPool * make();
		uint32_t recycle(MemPool * p);
		uint32_t getNewCount(){ return _newcount; }
	private:
        uint32_t _msize;
	    uint32_t _newcount;

		std::stack<MemPool *> _pool_stack;
		UTIL::CThreadMutex _lock;
};

MERGER_END;

#endif //_MS_MERGER_MEMPOOL_FACTORY_H_
