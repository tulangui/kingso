#include "util/ThreadMap.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

UTIL_BEGIN;

static bool is_prime(uint32_t n) {
	if( n == 0 || n == 1 ) return false;
	uint32_t max = (uint32_t)::sqrt(n);
	uint32_t i;
	for( i = 2; i <= max; i++ ) {
		if( n % i == 0 ) return false;
	}
	return true;
}

static uint32_t next_prime(uint32_t n) {
	n = n * 4 / 3;
	for(; !is_prime(n); n++);
	return n;
}

CThreadMap::CThreadMap(uint32_t size)
	: _val(NULL), 
	_key(NULL),
	_size(size),
	_counter(1)
{
	if( _size == 0 ) {
		_size = DEFAULT_MAX_THREADS_COUNT;
	}
	_size = next_prime(_size * 4 / 3);
	char * ptr = (char *)malloc((sizeof(pthread_t) + sizeof(uint16_t)) * _size);
	if( ptr == NULL ) {
		return;
	}
	memset(ptr, 0, (sizeof(pthread_t) + sizeof(uint16_t)) * _size);
	_val = (uint16_t *)ptr;
	_key = (pthread_t *)(ptr + sizeof(uint16_t) * _size);
}

CThreadMap::~CThreadMap() {
	if( _val != NULL ) {
		::free((char*)_val);
		_val = NULL;
	}
}

uint16_t CThreadMap::getThreadId(pthread_t id) {
	uint32_t pos;
	uint32_t i;
	uint16_t ret;
	if( (uint32_t)id == 0 ) return 0;
	_lock.lock();
	if( _size == 0 || _val == NULL || _key == NULL ) {
		_lock.unlock();
		return 0;
	}
	ret = 0;
	pos = (uint32_t)id % _size;
	for( i = 0; i < _size; i++ ) {
		if( (uint32_t)_key[pos] == 0 ) {
			ret = _counter++;
			_val[pos] = ret;
			_key[pos] = id;
			break;
		}
		if( pthread_equal(id, _key[pos]) != 0 ) {
			ret = _val[pos];
			break;
		} 
		pos++;
		if( pos == _size ) pos = 0;
	}
	_lock.unlock();
	return ret;
}

UTIL_END;
