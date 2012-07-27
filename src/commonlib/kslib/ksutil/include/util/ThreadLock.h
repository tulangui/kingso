/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 5 $
 *
 * $LastChangedDate: 2011-03-15 14:53:52 +0800 (Tue, 15 Mar 2011) $
 *
 * $Id: ThreadLock.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _THREAD_LOCK_H_
#define _THREAD_LOCK_H_
#include "util/common.h"
#include "util/namespace.h"
#include <pthread.h>
#include <stdint.h>
#include <errno.h>

#ifndef __USE_PTHREAD_RWLOCK
#  if defined __USE_UNIX98 || defined __USE_XOPEN2K
#    define __USE_PTHREAD_RWLOCK
#  endif
#endif //__USE_PTHREAD_RWLOCK

UTIL_BEGIN;

class Mutex {
	public:
		Mutex();
		~Mutex();
	private:
		COPY_CONSTRUCTOR(Mutex);
	public:
		int32_t lock();
		int32_t trylock();
		int32_t unlock();
	private:
		pthread_mutex_t _inst;
};

class RWLock {
	public:
		RWLock();
		~RWLock();
	private:
		COPY_CONSTRUCTOR(RWLock);
	public:
		int32_t rdlock();
		int32_t wrlock();
		int32_t tryrdlock();
		int32_t trywrlock();
		int32_t unlock();
	private:
#ifdef __USE_PTHREAD_RWLOCK
		pthread_rwlock_t _inst;
#else //__USE_PTHREAD_RWLOCK
		pthread_mutex_t _inst;
#endif //__USE_PTHREAD_RWLOCK
};

class Condition {
	public:
		Condition();
		~Condition();
	private:
		COPY_CONSTRUCTOR(Condition);
	public:
		int32_t lock();
		int32_t unlock();
		int32_t wait();
		int32_t timedwait(uint32_t ms);
		int32_t signal();
		int32_t broadcast();
	private:
		pthread_mutex_t _lock;
		pthread_cond_t _cond;
};

class Conditions {
	public:
		Conditions(uint32_t n);
		~Conditions();
	private:
		COPY_CONSTRUCTOR(Conditions);
	public:
		int32_t lock();
		int32_t unlock();
		int32_t wait(uint32_t idx);
		int32_t timedwait(uint32_t idx, uint32_t ms);
		int32_t signal(uint32_t idx);
		int32_t broadcast(uint32_t idx);
	private:
		uint32_t _unCount;
		pthread_cond_t * _conds;
		pthread_mutex_t _lock;
};

class BoolCondition {
	public:
		BoolCondition();
		~BoolCondition();
	private:
		COPY_CONSTRUCTOR(BoolCondition);
	public:
		int32_t setBusy();
		int32_t waitIdle();
		bool isBusy();
		int32_t clearBusy();
		int32_t clearBusyBroadcast();
	private:
		bool _bBusy;
		Condition _cond;
};

typedef Mutex CThreadMutex;
typedef RWLock CThreadRWLock;
typedef Condition CThreadCond;
typedef Conditions CThreadConds;

UTIL_END;

#endif //_THREAD_LOCK_H_
