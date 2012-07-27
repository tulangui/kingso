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
 * $Id: ThreadMap.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _THREAD_MAP_H_
#define _THREAD_MAP_H_
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include "util/namespace.h"
#include "util/ThreadLock.h"

UTIL_BEGIN;

class CThreadMap {
	public:
		static const uint32_t DEFAULT_MAX_THREADS_COUNT = 100;
	public:
		CThreadMap(uint32_t size = DEFAULT_MAX_THREADS_COUNT);
		~CThreadMap();
	public:
		uint16_t getThreadId(pthread_t id);
		uint16_t getThreadId() { return getThreadId(pthread_self()); }
	public:
		uint32_t getSize() const { return _size; }
	private:
		uint16_t * _val;
		pthread_t * _key;
		uint32_t _size;
		uint32_t _counter;
		CThreadMutex _lock;
};

UTIL_END;

#endif //_THREAD_MAP_H_
