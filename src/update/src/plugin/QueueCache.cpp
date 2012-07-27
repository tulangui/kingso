#include "QueueCache.h"
#include "WorkerFactory.h"

UPDATE_BEGIN

QueueCache::QueueCache()
{
    memset(_mem, 0, sizeof(CacheMem) * MAX_LOCK_NUM);
}
 
QueueCache::~QueueCache()
{
    for(int i = 0; i < MAX_LOCK_NUM; i++) {
        if(NULL == _mem[i].blockBegin) {
            continue;
        }
        delete[] _mem[i].blockBegin;
        _mem[i].blockBegin = NULL;
    }
}

int QueueCache::init(uint64_t cacheSize)
{
    uint64_t size = cacheSize / MAX_LOCK_NUM;
    size = ((size + (1<<20))>>20)<<20;
    
    for(int i = 0; i < MAX_LOCK_NUM; i++) {
        _mem[i].blockBegin = new(std::nothrow) char[size];
        if(NULL == _mem[i].blockBegin) {
            return -1;
        }
        _mem[i].blockEnd = _mem[i].blockBegin + size;
        _mem[i].begin    = _mem[i].blockBegin;
        _mem[i].end      = _mem[i].blockEnd;
    }

    return 0;
}

int QueueCache::put(uint64_t key, const char* message, int len)
{
    char* mem = NULL;
    int no = key % MAX_LOCK_NUM;

    util::Mutex& lock = _lock[no];
    std::map<uint64_t, CacheValue>& map = _map[no];
    std::map<uint64_t, CacheValue>::iterator it;

    lock.lock();
    it = map.find(key);
    if(it != map.end()) {
        it->second.worker = NULL;
        it->second.len = len;
        mem = it->second.value = alloc(key, no, len);
    } else {
        CacheValue value;
        value.worker = NULL;
        value.len = len;
        mem = value.value = alloc(key, no, len);
        map.insert(std::make_pair<uint64_t,CacheValue>(key, value));
    }
    if(NULL == mem) {
        map.erase(key);
        lock.unlock();
        return 0;
    }
    memcpy(mem, message, len);
    lock.unlock();

    return 0;
}

int QueueCache::get(uint64_t key, Worker* worker, char* value)
{
    int ret = -1;
    int no = key % MAX_LOCK_NUM;

    util::Mutex& lock = _lock[no];
    std::map<uint64_t, CacheValue>& map = _map[no];
    std::map<uint64_t, CacheValue>::iterator it;

    lock.lock();
    it = map.find(key);
    if(it != map.end()) {
        if(it->second.worker) { // 正在处理中，挂到后面
            ret = 0;
            worker->_nextWorker = it->second.worker->_nextWorker;
            it->second.worker->_nextWorker = worker;
        } else {
            ret = it->second.len;
            memcpy(value, it->second.value, it->second.len);
        }
    } else {
        CacheValue value;
        value.worker = worker;
        value.len = 0;
        value.value = NULL;
        map.insert(std::make_pair<uint64_t,CacheValue>(key, value));
    }
    lock.unlock();

    return ret;
}
    
char* QueueCache::alloc(uint64_t key, int no, int len)
{
    char* ret = NULL;
    CacheValue value;
    CacheMem& mem = _mem[no];
    std::queue<CacheValue>& que = _queue[no];

    if(mem.begin + len < mem.end) {
        ret = mem.begin;
        if(record(que, key, ret, len) < 0) {
            return NULL;
        }
        mem.begin += len;
        return ret;
    }
    
    // 释放以前的内存
    while(!que.empty()) {
        value = que.front();
        if(value.key == key) {
            return NULL;
        }
        _map[no].erase(value.key);         
        if(value.value != mem.end) { // 走到头了
            mem.end = mem.blockEnd;
            if(mem.begin + len < mem.end) {
                ret = mem.begin;
                if(record(que, key, ret, len) < 0) {
                    return NULL;
                }
                mem.begin += len;
                return ret;
            } else {
                mem.begin = value.value;
                mem.end = value.value + value.len;
            }
        } else {
            mem.end = value.value + value.len;
        }
        que.pop();
        if(mem.begin + len < mem.end) {
            ret = mem.begin;
            if(record(que, key, ret, len) < 0) {
                return NULL;
            }
            mem.begin += len;
            return ret;
        }
    }

    return NULL;
}

int QueueCache::record(std::queue<CacheValue>& que, uint64_t key, char* mem, int len)
{
    CacheValue value;

    value.key = key;
    value.value = mem;
    value.len = len;
    que.push(value);

    return 0;
}

UPDATE_END
 
