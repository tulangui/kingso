#include "util/Thread.h"
#include "util/ThreadPool.h"
#include <stdio.h>
#include <stdlib.h>

UTIL_BEGIN;

void Runnable::free() {
	delete this;
}

Thread::Thread(ThreadPool * p, 
		uint32_t stack_size, uint32_t stack_guard_size) 
: _stack_size(stack_size), _stack_guard_size(stack_guard_size),
	_lastret(0), _stat(thr_inited), 
	_runner(NULL), _conds(2),
	pool(p), next(NULL), prev(NULL)
{ 
}

Thread::~Thread() { 
	_conds.lock();
	if(_runner) {
		_runner->free();
		_runner = NULL;
	}
	_conds.unlock();
}

int32_t Thread::run() {
	Runnable * runner = NULL;
	_conds.lock();
	while( likely(_stat != thr_terminated) ) {
		if( !_runner ) {
			if(_stat != thr_terminated) _stat = thr_waiting;
			_conds.wait(0);
			if( unlikely(!_runner) ) {
				continue;
			}
		}
		if(_stat != thr_terminated) _stat = thr_running;
		runner = _runner;
		_runner = NULL;
		_conds.signal(1);
		if( !pool ) break;
		_conds.unlock();
		if( runner ) {
			_lastret = runner->run();
			runner->free();
			runner = NULL;
		}
		pool->freeThread(this);
		_conds.lock();
	}
	_conds.unlock();
	if( runner ) {
		_lastret = runner->run();
		runner->free();
	}
	if( pool ) pool->terminateThread(this);
	_stat = thr_terminated;
	return 0;
}

static void * thread_hook(void * arg) {
	  Thread * thr = (Thread*)arg;
		return (void *)thr->run();
}

int32_t Thread::start() {
	pthread_attr_t * attr = NULL;
	int ret;
	if( unlikely(_stat != thr_inited) ) return EFAILED;
	if( _stack_size > 0 || _stack_guard_size > 0 ) {
		attr = (pthread_attr_t*)malloc(sizeof(pthread_attr_t));
		if(unlikely(!attr)) return ENOMEM;
		if((ret=pthread_attr_init(attr)) != 0) {
			::free(attr);
			return ret;
		}
#ifdef PTHREAD_SCOPE_SYSTEM
# if !defined(__FreeBSD__)
		if((ret=pthread_attr_setscope(attr, PTHREAD_SCOPE_SYSTEM)) != 0) {
			pthread_attr_destroy(attr);
			::free(attr);
			return ret;
		}
# endif
#endif
		if(_stack_size > 0) {
			if((ret=pthread_attr_setstacksize(attr, _stack_size)) != 0) {
				pthread_attr_destroy(attr);
				::free(attr);
				return ret;
			}
		}
		if(_stack_guard_size > 0) {
			if((ret=pthread_attr_setguardsize(attr, _stack_guard_size)) != 0) {
				pthread_attr_destroy(attr);
				::free(attr);
				return ret;
			}
		}
		if((ret=pthread_attr_setdetachstate(attr, PTHREAD_CREATE_JOINABLE)) != 0) {
			pthread_attr_destroy(attr);
			::free(attr);
			return ret;
		}
	}
	if( unlikely((ret=pthread_create(&_tid, attr, thread_hook, this)) != 0) ) {
		if(attr) {
			pthread_attr_destroy(attr);
			::free(attr);
		}
		return ret;
	}
	if(attr) {
		pthread_attr_destroy(attr);
		::free(attr);
	}
	_stat = thr_started;
	return 0;
}

int32_t Thread::dispatch(Runnable * runner) {
	_conds.lock();
	if( unlikely(_stat == thr_terminated) ) {
		_conds.unlock();
		return EFAILED;
	}
	if( _runner ) {
		_conds.wait(1);
		if( unlikely(_runner) ) {
			if( _stat == thr_terminated ) {
				_conds.unlock();
				return EFAILED;
			}
			_conds.unlock();
			return EFAILED;
		}
	}
	_runner = runner;
	_runner->setOwner(this);
	_conds.signal(0);
	_conds.unlock();
	return 0;
}

void Thread::interrupt() {
	_conds.lock();
	_conds.broadcast(0);
	_conds.broadcast(1);
	_conds.unlock();
}

void Thread::terminate() {
	_conds.lock();
	_stat = thr_terminated;
	_conds.broadcast(0);
	_conds.broadcast(1);
	_conds.unlock();
}

void Thread::join() {
	if( _stat != thr_inited && _stat != thr_unknown ) {
		pthread_join(_tid, NULL);
	}
}

int32_t Thread::setPriority(int priority, int policy) {
	if( _stat == thr_inited || _stat == thr_unknown ) {
		struct sched_param schedparam;
		schedparam.sched_priority = priority;
		return pthread_setschedparam(_tid, policy, &schedparam);
	}
	return 0;
}

UTIL_END;
