#include "MemQueue.h"
#include <string.h>
#include <malloc.h>
#include "util/Log.h"

namespace update {

MemQueue::MemQueue()
{
    _tail = 0;
    _head = 0;
    _count = 0;
    pthread_spin_init(&_lock, PTHREAD_PROCESS_PRIVATE);
   _opened = false;
}

MemQueue::~MemQueue()
{
    close();
    pthread_spin_destroy(&_lock);
}

/* 打开一个队列 */
int MemQueue::open(const char* path)
{
    _opened = true;
    return 0;
}

/* 终止队列读写 */
int MemQueue::close()
{
    _tail = 0;
    _head = 0;
    _count = 0;
    _opened = false;
    return 0;
}

/* 在队列结尾写入一条消息 */
int MemQueue::push(StorableMessage& msg)
{
    if(_opened==false){
        TWARN("MemQueue not opened, return!");
        return -1;
    }
    pthread_spin_lock(&_lock);
    if(_count==MESSAGE_ARRAY_LEN){
        pthread_spin_unlock(&_lock);
        return 0;
    }
    if(_message[_tail].max<msg.size){
        char* mem = (char*)malloc(msg.size);
        if(!mem){
            TWARN("malloc message buffer error, reutrn![size=%d]", msg.size);
            pthread_spin_unlock(&_lock);
            return -1;
        }
        if(_message[_tail].freeFn){
            _message[_tail].freeFn(_message[_tail].ptr);
        }
        _message[_tail].ptr = mem;
        _message[_tail].freeFn = free;
        _message[_tail].max = msg.size;
    }
    _message[_tail].seq = msg.seq;
    if(msg.size>0){
        memcpy(_message[_tail].ptr, msg.ptr, msg.size);
    }
    _message[_tail].size = msg.size;
    if(++_tail==MESSAGE_ARRAY_LEN){
        _tail = 0;
    }
    _count ++;
    pthread_spin_unlock(&_lock);
    return 1;
}

/* 从队列头读出一条消息 */
int MemQueue::pop(StorableMessage& msg)
{
    if(_opened==false){
        TWARN("MemQueue not opened, return!");
        return -1;
    }
    pthread_spin_lock(&_lock);
    if(_count==0){
        pthread_spin_unlock(&_lock);
        return 0;
    }
    if(msg.max<_message[_head].size){
        char* mem = (char*)malloc(_message[_head].size);
        if(!mem){
            TWARN("malloc message buffer error, reutrn![size=%d]", _message[_head].size);
            pthread_spin_unlock(&_lock);
            return -1;
        }
        if(msg.freeFn){
            msg.freeFn(msg.ptr);
        }
        msg.ptr = mem;
        msg.freeFn = free;
        msg.max = _message[_head].size;
    }
    msg.seq = _message[_head].seq;
    if(_message[_head].size>0){
        memcpy(msg.ptr, _message[_head].ptr, _message[_head].size);
    }
    msg.size = _message[_head].size;
    if(++_head==MESSAGE_ARRAY_LEN){
        _head = 0;
    }
    _count --;
    pthread_spin_unlock(&_lock);
    return 1;
}

/* 查看队列头一条消息 */
int MemQueue::touch(StorableMessage& msg)
{
    if(_opened==false){
        TWARN("MemQueue not opened, return!");
        return -1;
    }
    pthread_spin_lock(&_lock);
    if(_count==0){
        pthread_spin_unlock(&_lock);
        return 0;
    }
    if(msg.max<_message[_head].size){
        char* mem = (char*)malloc(_message[_head].size);
        if(!mem){
            TWARN("malloc message buffer error, reutrn![size=%d]", _message[_head].size);
            pthread_spin_unlock(&_lock);
            return -1;
        }
        if(msg.freeFn){
            msg.freeFn(msg.ptr);
        }
        msg.ptr = mem;
        msg.freeFn = free;
        msg.max = _message[_head].size;
    }
    msg.seq = _message[_head].seq;
    if(_message[_head].size>0){
        memcpy(msg.ptr, _message[_head].ptr, _message[_head].size);
    }
    msg.size = _message[_head].size;
    pthread_spin_unlock(&_lock);
    return 1;
}

/* 翻过队列头一条消息 */
int MemQueue::next()
{
    if(_opened==false){
        TWARN("MemQueue not opened, return!");
        return -1;
    }
    pthread_spin_lock(&_lock);
    if(_count==0){
        pthread_spin_unlock(&_lock);
        return 0;
    }
    if(++_head==MESSAGE_ARRAY_LEN){
        _head = 0;
    }
    _count --;
    pthread_spin_unlock(&_lock);
    return 1;
}

/* 重设队列头位置 */
uint64_t MemQueue::reset(const uint64_t seq)
{
    return 0;
}

/* 获取现在写到的位置 */
const uint64_t MemQueue::tail() const
{
    return 0;
}

/* 获取资源路径 */
const char* MemQueue::path() const
{
    return 0;
}

/* 获取队列中的数据数量 */
const uint64_t MemQueue::count() const
{
    return _count;
}

}
