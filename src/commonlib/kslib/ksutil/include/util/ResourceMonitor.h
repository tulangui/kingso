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
 * $Id: ResourceMonitor.h 5 2011-03-15 06:53:52Z zhuangyuan $
 *
 *******************************************************************
 */

#ifndef _RESOURCE_MONITOR_H_
#define _RESOURCE_MONITOR_H_
#include "util/UtilNamespace.h"
#include "util/ThreadLock.h"
#include "util/ScopedLock.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <new>

UTIL_BEGIN;

struct Input {
};

struct Output {
};

class ResourceInterface {
	public:
		ResourceInterface() { }
		virtual ~ResourceInterface() { }
	public:
		virtual bool reload() = 0;
};

template<typename T>
class ResourceProxy : public ResourceInterface {
	public:
		ResourceProxy() : _res(NULL) { _path[0] = '\0'; }
		virtual ~ResourceProxy() {
			WR_LOCK(_lock);
			if(_res) delete _res;
			_res = NULL;
			WR_UNLOCK(_lock);
		}
	public:
		inline T * get() const { return _res; }
		T * release() {
			T * res;
			WR_LOCK(_lock);
			res = _res;
			_res = NULL;
			WR_UNLOCK(_lock);
			return res;
		}
		bool load(const char * path) {
			strcpy(_path, (path ? (strlen(path) > PATH_MAX ? "" : path): ""));
			if(_res) delete _res;
			_res = new (std::nothrow) T();
			if(!_res || !_res->load(_path)) {
				delete _res;
				_res = NULL;
				return false;
			}
			return true;
		}
		virtual bool reload() {
			T * res;
			res = new (std::nothrow) T();
			if(!res) return false;
			if(!res->load(_path)) {
				delete res;
				return false;
			}
			WR_LOCK(_lock);
			if(_res) delete _res;
			_res = res;
			WR_UNLOCK(_lock);
			return true;
		}
		bool process(const Input & input, Output & out) {
			RD_LOCK(_lock);
			if(!_res) return false;
			return _res->process(input, out);
			RD_UNLOCK(_lock);
		}
	private:
		char _path[PATH_MAX + 1];
		T * _res;
		UTIL::RWLock _lock;
};

class ResourceManager {
	public:
		ResourceManager();
		~ResourceManager();
	public:
		bool registerProxy(char c, ResourceInterface * r);
		ResourceInterface * get(char c) const { return _proxy[c]; }
		ResourceInterface * cancel(char c);
	private:
		ResourceInterface * _proxy[256];
};

class ResourceMonitor {
	public:
		ResourceMonitor(const ResourceManager & mgr);
		~ResourceMonitor();
	public:
		bool handleEvent(char c);
		bool start(const char * path);
		bool monitor();
		void wait();
		bool terminate();
	protected:
		const ResourceManager & _mgr;
		bool _running;
		char _pipe_path[PATH_MAX + 1];
		pthread_t _tid;
};

UTIL_END;

#endif //_RESOURCE_MONITOR_H_
