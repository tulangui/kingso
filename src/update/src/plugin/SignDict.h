#ifndef DISPATCHER_SIGNDICT_H_
#define DISPATCHER_SIGNDICT_H_

#include "util/HashMap.hpp"
#include "util/ThreadLock.h"
#include "update/update_def.h"

#define HASH_DICT_NUM 16    // 便于多线程更新操作

UPDATE_BEGIN

struct Finger {
    uint64_t high;
    uint64_t low;
};

struct SignInfo {
    int64_t nid;
    Finger finger;
};

class SignDict
{ 
public:
    SignDict();
    ~SignDict();

    static SignDict* getInstance() {return &_signDictInstance;}
    /*
     *
     */
    int init(const char* dictPath);

    /*
     * 检测nid是否有更新，如果有更新，则更新指纹
     * ret 无更新 0, 更新 非0
     */
    int check(int64_t nid, unsigned char sign[16]);  

private:
    // 读取签名信息
    int loadSign(const char* filename);

private:
    int _updateNum;     // 更新总数量
    int _fullNum;       // 全量指纹信息数量
    util::Mutex _lock;  // 计数lock

    int _fd[HASH_DICT_NUM];        // 记录更新后的指纹
    util::Mutex _hashLock[HASH_DICT_NUM];
    util::HashMap<int64_t, Finger>* _hashTable[HASH_DICT_NUM]; 

    static SignDict _signDictInstance;
};

UPDATE_END
 
#endif //DISPATCHER_SIGNDICT_H_
