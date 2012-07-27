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
 * $Id: ThreadPool.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_
#include "util/common.h"
#include "util/Thread.h"

UTIL_BEGIN

class ThreadPool {
	public:
		ThreadPool(uint32_t capability = 0, uint32_t stack_size = 0);
		~ThreadPool();
	public:
		/**
		 * @brief assign or create a thread to run from pool
		 * @param thrp the thread handle
		 * @param runner the object which the thread will run
		 * @param block block or unblock the caller
		 * @return errno 0,success other,failed
		 */
		int32_t newThread(Thread ** thrp, Runnable * runner, bool block = true);
		/**
		 * @brief free the thread which is running stat to pool
		 * @param thr the thread handle
		 * @return errno 0,success other,failed
		 */
		int32_t freeThread(Thread * thr);
		/**
		 * @brief free the thread which is running stat to pool
		 * @param thr the thread handle
		 * @return errno 0,success other,failed
		 */
		int32_t terminateThread(Thread * thr);
		/**
		 * @brief wait all running threads end and gather infomation
		 * @return errno 0,success other,failed
		 */
		void join();
		/**
		 * @brief awake all blocked thread
		 * @return
		 */
		void interrupt();
		/**
		 * @brief terminate all thread which in the pool
		 * @return
		 */
		void terminate();
	public:
		/**
		 * @brief get free thread count in the pool
		 * @return free thread count
		 */
		uint32_t getFreeCount() const { return _freecnt; }
		/**
		 * @brief get busy thread count in the pool
		 * @return busy thread count
		 */
		uint32_t getBusyCount() const { return _busycnt; }
		/**
		 * @brief get terminated thread count in the pool
		 * @return terminated  thread count
		 */
		uint32_t getTerminatedCount() const { return _termcnt; }
		/**
		 * @brief get total thread count in the pool
		 * @return total thread count
		 */
		uint32_t size() const { return _size; }
		/**
		 * @brief get the capability of the pool
		 * @return the capability of the pool
		 */
		uint32_t capability() const { return _capability; }
		/**
		 * @brief is under terminated stat
		 * @return true,terminated stat false,not terminated
		 */
		bool isTerminated() const { return _terminated; }
	private:
		Thread * _freelist;
		uint32_t _freecnt;
		Thread * _busylist;
		uint32_t _busycnt;
		Thread * _termlist;
		uint32_t _termcnt;
		uint32_t _size;
		uint32_t _capability;
		uint32_t _stack_size;
		uint32_t _stack_guard_size;
		bool _terminated;
		Conditions _conds;
};

typedef ThreadPool CThreadPool;

UTIL_END;

#endif //_THREADPOOL_H_
