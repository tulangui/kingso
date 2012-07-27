#ifndef _FRAMEWORK_CACHE_H_
#define _FRAMEWORK_CACHE_H_
//#include "common/common.h"
#include "util/ThreadLock.h"
#include "libmemcached/memcached.h"
#include "framework/namespace.h"
#include <vector>
#include <string>
#include <stack>
#include <sys/uio.h>
#include "util/MD5.h"

FRAMEWORK_BEGIN

class CacheHandle {
	public:
		CacheHandle();
		virtual ~CacheHandle() { }
	public:
		virtual int32_t initialize(const char * path) = 0;
		bool get(struct iovec & key, struct iovec & val);
		bool put(struct iovec & key, struct iovec & val);
	public:
		void setKeyLimit(uint32_t max_key_len) { _max_key_len = max_key_len; }
		void setValLimit(uint32_t max_val_len) { _max_val_len = max_val_len; }
		void setConnTimeout(uint32_t conn_timeout) { _conn_timeout = conn_timeout;}
                void setSendTimeout(uint32_t send_timeout) { _send_timeout = send_timeout;}
                void setRecvTimeout(uint32_t recv_timeout) { _recv_timeout = recv_timeout;}

	public:
		int getMinDocsFound() const { return _nMinDocsFound; }
		uint32_t getPutCount() const { return _put; }
		uint32_t getPutFailCount() const { return _putfail; }
		uint32_t getPutSucceedCount() const { return _put - _putfail; }
		uint32_t getLookupCount() const { return _lookup; }
		uint32_t getHitCount() const { return _hit; }
		double getHitRate() const { return (double)_hit / (double)_lookup; }
		uint32_t getConnTimeout() { return _conn_timeout; }
                uint32_t getSendTimeout() { return _send_timeout; }
                uint32_t getRecvTimeout() { return _recv_timeout; }
	protected:
		virtual bool doGet(const struct iovec & key, 
				struct iovec & val) = 0;
		virtual bool doPut(const struct iovec & key, 
				const struct iovec & val) = 0;
	private:
		uint32_t _max_key_len;
		uint32_t _max_val_len;
		uint32_t _put;
		uint32_t _putfail;
		uint32_t _lookup;
		uint32_t _hit;
		uint32_t _conn_timeout;
		uint32_t _send_timeout;
		uint32_t _recv_timeout; 
	protected:
		uint32_t _expire_time;
		int _nMinDocsFound;
};

class MemcacheClient : public CacheHandle {
	public:
		MemcacheClient() : _handle(NULL),_key_compress(NULL), _val_compress(NULL) { }
		~MemcacheClient();
	public:
		virtual int32_t initialize(const char * path);
                void setKeyCompr(const char *key_compress)
                {
			if (!key_compress || !key_compress[0])
				return;
                        if(_key_compress)
                                free(_key_compress);
                        _key_compress = strdup(key_compress);
                }
                void setValCompr(const char *val_compress)
                {
			if (!val_compress || !val_compress[0])
				return;
                        if(_val_compress)
                                free(_val_compress);
                        _val_compress = strdup(val_compress);
                }
                void setIgnore(const char *ignore_list);
	protected:
		virtual bool doGet(const struct iovec & key, 
				struct iovec & val);
		virtual bool doPut(const struct iovec & key, 
				const struct iovec & val);
		void processUrl(char *url);
	private:
		void * _handle;
		std::stack<void *> _handles;
		UTIL::Mutex _lock;
                char *_key_compress;
                char *_val_compress;
                std::vector<std::string> _ignore_vec;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_CACHE_H_

