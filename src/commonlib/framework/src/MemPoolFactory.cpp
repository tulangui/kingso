#include "framework/MemPoolFactory.h"
#include <new>

FRAMEWORK_BEGIN;

MemPoolFactory::MemPoolFactory() { }

MemPoolFactory::~MemPoolFactory() 
{
    std::map<uint64_t, MemPool*>::iterator it;
    _lock.wrlock();
    it  = _mp.begin();
    for (; it != _mp.end(); it++) {
        if (it->second) {
            delete it->second;
        }
    }
    _lock.unlock();
}

MemPool *MemPoolFactory::make(uint64_t key) 
{
    MemPool *p = NULL;
    std::map<uint64_t, MemPool *>::iterator it;
    _lock.rdlock();
    it = _mp.find(key);
    if (it == _mp.end()) {
        _lock.unlock();
        p = new (std::nothrow) MemPool();
        if (unlikely(!p)) {
            return NULL;
        }
        _lock.wrlock();
        it = _mp.find(key);
        if (it == _mp.end()) {
            _mp.insert(std::make_pair(key, p));
        } 
        else {
            delete p;
            p = it->second;
        }
        _lock.unlock();
    } 
    else {
        p = it->second;
        _lock.unlock();
    }

    return p;
}

FRAMEWORK_END;

