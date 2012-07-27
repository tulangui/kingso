#ifndef _QUEUECACHE_H_
#define _QUEUECACHE_H_

#include <map>
#include <queue>
#include <stdint.h>
#include "util/ThreadLock.h"
#include "update/update_def.h"

UPDATE_BEGIN

#define MAX_LOCK_NUM 16

class Worker;
struct CacheValue {
    union {
        Worker* worker; // map 中表示处理状态
        uint64_t key;   // que 中表示key
    };
    char* value;
    int32_t len;
};

struct CacheMem {
    char* begin;
    char* end;
    char* blockBegin;
    char* blockEnd;
};

class QueueCache
{ 
public:
    QueueCache();
    ~QueueCache();

    int init(uint64_t cacheSize);

    /*
     * 存入实际数据，并移除worker信息
    */
    int put(uint64_t key, const char* message, int len); 

    /*
     * 如果正在处理中，将当前worker加入到处理链表中
     * ret 失败: -1  处理:0 成功: > 0
     */
    int get(uint64_t key, Worker* worker, char* value);

private:
    /*
     * 分配一块儿内存，如果不够，释放后再分配
     */
    char* alloc(uint64_t key, int no, int len); 
    
    /*
     * 记录分配的内存空间
     */
    int record(std::queue<CacheValue>& que, uint64_t key, char* mem, int len);

private:
    util::Mutex _lock[MAX_LOCK_NUM];                   // 每个队列的锁 
    std::queue<CacheValue> _queue[MAX_LOCK_NUM];       // 记录进入队列的先后顺序              
    std::map<uint64_t, CacheValue> _map[MAX_LOCK_NUM]; // 用于查询
    CacheMem _mem[MAX_LOCK_NUM];                       // value使用的内存空间
};

UPDATE_END
 
#endif //_QUEUECACHE_H_
