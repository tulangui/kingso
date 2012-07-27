/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $LastChangedDate: 2011-09-24 09:50:55 +0800 (SAT, 09 SEP 2011) $
 *
 * $Brief: 内存队列 $
 ********************************************************************/

#ifndef _MEMQUEUE_H_
#define _MEMQUEUE_H_

#include "Queue.h"
#include <pthread.h>

namespace update {

class MemQueue : public Queue
{ 
    public:
        MemQueue();
        ~MemQueue();
    private:
        /*
         * 禁止拷贝构造函数和赋值操作
         */
        MemQueue(const MemQueue&);
        MemQueue& operator=(const MemQueue&);
    public:
        /*
         * 打开一个队列
         *
         * path   : 待加载资源的路径
         *
         * return : 0表示成功，其他表示失败
         */
        int open(const char* path=0);
        /*
         * 在队列结尾写入一条消息
         *
         * msg : 待写入消息
         *
         * return : 1表示成功，0表示队列已满，-1表示失败
         */
        int push(StorableMessage& msg);
        /*
         * 从队列头读出一条消息
         *
         * msg    : 消息缓存区
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        int pop(StorableMessage& msg);
        /*  
         * 查看队列头一条消息
         *
         * msg    : 消息缓存区
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        int touch(StorableMessage& msg);
        /*  
         * 翻过队列头一条消息
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        int next();
        /*
         * 重设队列头位置
         *
         * seq : 消息编号
         *
         * return : 大于0表示成功，0表示失败
         */
        uint64_t reset(const uint64_t seq);
        /*
         * 获取现在写到的位置
         *
         * return : 现在写到的位置
         */
        const uint64_t tail() const;
        /**
         * 终止队列读写
         *
         * return : 0表示成功，其他表示失败.
         */
        int close();
        /*
         * 获取资源路径
         *
         * return : 资源路径
         */
        const char* path() const;
        /*  
         * 获取队列中的数据数量
         *
         * return : 数据数量
         */
        const uint64_t count() const;
    private:
        static const int MESSAGE_ARRAY_LEN = 4096;      // 4096个缓存节点

        StorableMessage _message[MESSAGE_ARRAY_LEN];    // 缓存数组
        int _tail;                                      // 队尾位置
        int _head;                                      // 队首位置
        int _count;                                     // 队列中节点数量
        pthread_spinlock_t _lock;                       // 队列锁

        bool _opened;                                   // 是否被打开
};

}

#endif // _UTIL_MEMQUEUE_H_
