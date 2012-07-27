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
 * $Id: ScopedLock.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _SCOPED_LOCK_H_
#define _SCOPED_LOCK_H_
#include "util/common.h"
#include "util/ThreadLock.h"

#define MUTEX_LOCK(x) { UTIL::ScopedMutex __scoped_mutex_##x(x);
#define MUTEX_UNLOCK(x) }

#define RD_LOCK(x) { UTIL::ScopedRdLock __scoped_rd_##x(x);
#define RD_UNLOCK(x) }

#define WR_LOCK(x) { UTIL::ScopedWrLock __scoped_wr_##x(x);
#define WR_UNLOCK(x) }

#define COND_LOCK(x) { UTIL::ScopedCondition __scoped_cond_##x(x);
#define COND_UNLOCK(x) }

UTIL_BEGIN;

class ScopedMutex {
	public:
		inline ScopedMutex(Mutex & lock) : _lock(lock) {
			_lock.lock();
		}
		inline ~ScopedMutex() {
			_lock.unlock();
		}
	private:
		Mutex & _lock;
};

class ScopedRdLock {
	public:
		inline ScopedRdLock(RWLock & lock) : _lock(lock) {
			_lock.rdlock();
		}
		inline ~ScopedRdLock() {
			_lock.unlock();
		}
	private:
		RWLock & _lock;
};

class ScopedWrLock {
	public:
		inline ScopedWrLock(RWLock & lock) : _lock(lock) {
			_lock.wrlock();
		}
		inline ~ScopedWrLock() {
			_lock.unlock();
		}
	private:
		RWLock & _lock;
};

class ScopedCondition {
	public:
		inline ScopedCondition(Condition & lock) : _lock(lock) {
			_lock.lock();
		}
		inline ~ScopedCondition() {
			_lock.unlock();
		}
	private:
		Condition & _lock;
};

UTIL_END;

#endif //_SCOPED_LOCK_H_
