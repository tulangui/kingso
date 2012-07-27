#include "util/ThreadPool.h"
#include <new>
#include <stdio.h>

UTIL_BEGIN;

ThreadPool::ThreadPool(uint32_t capability, uint32_t stack_size) 
	: _freelist(0), _freecnt(0),
	_busylist(0), _busycnt(0),
	_termlist(0), _termcnt(0),
	_size(0), _capability(capability),
	_stack_size(stack_size),
	_stack_guard_size(0),
	_terminated(false),
	_conds(3)
{ 
	if(_stack_size > 0 && _stack_size < 1024L * 1024L) {
		_stack_size = 0; //set as default.
	}
	if(_stack_guard_size > 0 && _stack_guard_size < 1024L * 1024L) {
		_stack_guard_size = 0;
	}
}

ThreadPool::~ThreadPool() { 
	Thread * thr;
	terminate();
	_conds.lock();
	while( _termcnt < _size ) _conds.wait(2);
	while( _freelist ) {
		thr = _freelist->next;
		delete _freelist;
		_freelist = thr;
	}
	_freecnt = 0;
	while( _busylist ) {
		thr = _busylist->next;
		delete _busylist;
		_busylist = thr;
	}
	_busycnt = 0;
	while( _termlist ) {
		thr = _termlist->next;
		delete _termlist;
		_termlist = thr;
	}
	_conds.unlock();
}

int32_t ThreadPool::newThread(Thread ** thrp, Runnable * runner, bool block) {
	int32_t ret;
	Thread * thr;
	if( unlikely(!runner) ) return EINVAL;
	_conds.lock();
	if( unlikely(_terminated) ) {
		_conds.unlock();
		return EFAILED;
	}
	if( _freecnt == 0 ) {
		if( _capability > 0 && _size >= _capability ) {
			if( !block ) {
				_conds.unlock();
				return EFAILED;
			}
			_conds.wait(0);
			if( unlikely(_freecnt == 0) ) {
				if( _terminated ) {
					_conds.unlock();
					return EFAILED;
				}
				_conds.unlock();
				return EFAILED;
			}
		} else {
			thr = new (std::nothrow) Thread(this, _stack_size, _stack_guard_size);
			if( unlikely(!thr) ) {
				_conds.unlock();
				return ENOMEM;
			}
			ret = thr->start();
			if( unlikely(ret != SUCCESS) ) {
				_conds.unlock();
				delete thr;
				return ret;
			}
			thr->next = _freelist;
			thr->prev = NULL;
			_freelist = thr;
			_freecnt++;
			_size++;
		}
	}
	thr = _freelist;
	_freelist = thr->next;
	if( _freelist ) _freelist->prev = NULL;
	_freecnt--;
	thr->prev = NULL;
	thr->next = _busylist;
	if( _busylist ) _busylist->prev = thr;
	_busylist = thr;
	_busycnt++;
	_conds.unlock();
	ret = thr->dispatch(runner);
	if( unlikely(ret != SUCCESS) ) {
		freeThread(thr);
		return ret;
	}
	if( thrp ) *thrp = thr;
	return SUCCESS;
}

int32_t ThreadPool::freeThread(Thread * thr) {
	if( thr ) {
		_conds.lock();
		if( thr->next ) thr->next->prev = thr->prev;
		if( _busylist == thr ) _busylist = thr->next;
		else thr->prev->next = thr->next;
		_busycnt--;
		thr->prev = NULL;
		if( _freelist ) _freelist->prev = thr;
		thr->next = _freelist;
		_freelist = thr;
		_freecnt++;
		if( _capability > 0 && _freecnt == 1 ) {
			_conds.signal(0);
		}
		if( _busycnt == 0 ) {
			_conds.broadcast(1);
		}
		_conds.unlock();
	}
	return SUCCESS;
}

int32_t ThreadPool::terminateThread(Thread * thr) {
	Thread * p;
	if( thr ) {
		_conds.lock();
		for(p = _freelist; p && p != thr; p = p->next);
		if( p ) {
			if( thr->next ) thr->next->prev = thr->prev;
			if( _freelist == thr ) _freelist = thr->next;
			else thr->prev->next = thr->next;
			_freecnt--;
		} else {
			if( thr->next ) thr->next->prev = thr->prev;
			if( _busylist == thr ) _busylist = thr->next;
			else thr->prev->next = thr->next;
			_busycnt--;
		}
		thr->prev = NULL;
		if( _termlist ) _termlist->prev = thr;
		thr->next = _termlist;
		_termlist = thr;
		_termcnt++;
		if( _termcnt == _size ) {
			_conds.broadcast(2);
		}
		_conds.unlock();
	}
	return SUCCESS;
}

void ThreadPool::join() {
	_conds.lock();
	while( _busycnt > 0 ) {
		_conds.wait(1);
	}
	_conds.unlock();
}

void ThreadPool::terminate() {
	Thread * thr;
	_conds.lock();
	_terminated = true;
	for(thr = _freelist; thr; thr = thr->next) thr->terminate();
	for(thr = _busylist; thr; thr = thr->next) thr->terminate();
	_conds.broadcast(0);
	_conds.unlock();
}

void ThreadPool::interrupt() {
	_conds.lock();
	_conds.broadcast(0);
	_conds.unlock();
}

UTIL_END;

