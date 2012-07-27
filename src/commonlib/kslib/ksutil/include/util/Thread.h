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
 * $Id: Thread.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _THREAD_H_
#define _THREAD_H_
#include "util/common.h"
#include "util/ThreadLock.h"

UTIL_BEGIN

enum _thread_stat_t {
	thr_unknown,
	thr_inited,
	thr_started,
	thr_running,
	thr_waiting,
	thr_terminated
};
typedef enum _thread_stat_t thread_stat_t;

class Thread;
class ThreadPool;

class Runnable {
	public:
		Runnable() { }
		virtual ~Runnable() { }
	public:
		virtual int32_t run() = 0;
		virtual void free();
	public:
		void setOwner(Thread * thr) { _owner = thr; }
		Thread * getOwner() const { return _owner; }
	protected:
		Thread * _owner;
};

class Thread : public Runnable {
	public:
		Thread(ThreadPool * p = NULL, 
				uint32_t stack_size = 0, uint32_t stack_guard_size = 0);
		~Thread();
	public:
		int32_t start();
		int32_t dispatch(Runnable * runner);
		void interrupt();
		void terminate();
		void join();
		int32_t setPriority(int priority, int policy);
		thread_stat_t getStat() const { return _stat; }
		int32_t getLastReturnValue() const { return _lastret; }
	public:
		virtual int32_t run();
	protected:
		pthread_t _tid;
		uint32_t _stack_size;
		uint32_t _stack_guard_size;
		int32_t _lastret;
		thread_stat_t _stat;
		Runnable * _runner;
		Conditions _conds;
	public:
		ThreadPool * pool;
		Thread * next;
		Thread * prev;
};

typedef Runnable CRunnable;
typedef Thread CThread;

UTIL_END

#endif //_THREAD_H_
