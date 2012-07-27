#include "MemPoolFactory.h"
#include "util/MemPool.h"
#include "util/ScopedLock.h"
#include <new>
#include <stdio.h>

MERGER_BEGIN;

MemPoolFactory::MemPoolFactory() : _msize(0),_newcount(0) {
}

MemPoolFactory::~MemPoolFactory() {
	MemPool * p;
	MUTEX_LOCK(_lock);
	while(_pool_stack.size() > 0) {
		p = _pool_stack.top();
		_pool_stack.pop();
		if(p) {
			p->reset();
			delete p;
		}
	}
	MUTEX_UNLOCK(_lock);
}

MemPool * MemPoolFactory::make() {
	MemPool * p;
	MUTEX_LOCK(_lock);
	while(_pool_stack.size() > 0) {
		p = _pool_stack.top();
		_pool_stack.pop();
		if(p) return p;
	}
	_newcount++;
	MUTEX_UNLOCK(_lock);
	p = new (std::nothrow) MemPool(_msize);
	return p;
}

uint32_t MemPoolFactory::recycle(MemPool * p) {
	uint32_t mpsize = 0;
	if(!p) return mpsize;
	mpsize = p->reset();
	MUTEX_LOCK(_lock);
	_pool_stack.push(p);
	MUTEX_UNLOCK(_lock);
	return mpsize;
}

MERGER_END;

