/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $LastChangedDate: 2011-09-24 09:50:55 +0800 (SAT, 09 SEP 2011) $
 *
 * $Brief: 队列基类 $
 ********************************************************************/

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdint.h>
#include "StorableMessage.h"

namespace update {

class Queue
{ 
    public:
        Queue() {}
        virtual ~Queue() {}
    private:
        /*
         * 禁止拷贝构造函数和赋值操作
         */
        Queue(const Queue&);
        Queue& operator=(const Queue&);
    public:
        /*
         * 打开一个队列
         *
         * path   : 待加载资源的路径
         *
         * return : 0表示成功，其他表示失败
         */
        virtual int open(const char* path=0) = 0;
        /*
         * 在队列结尾写入一条消息
         *
         * msg : 待写入消息
         *
         * return : 1表示成功，0表示队列已满，-1表示失败
         */
        virtual int push(StorableMessage& msg) = 0;
        /*
         * 从队列头读出一条消息
         *
         * msg    : 消息缓存区
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        virtual int pop(StorableMessage& msg) = 0;
        /*
         * 查看队列头一条消息
         *
         * msg    : 消息缓存区
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        virtual int touch(StorableMessage& msg) = 0;
        /*
         * 翻过队列头一条消息
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        virtual int next() = 0;
        /*
         * 重设队列头位置
         *
         * seq : 消息编号
         *
         * return : 大于0表示成功，0表示失败
         */
        virtual uint64_t reset(const uint64_t seq) = 0;
        /*
         * 获取现在写到的位置
         *
         * return : 现在写到的位置
         */
        virtual const uint64_t tail() const = 0;
        /**
         * 终止队列读写
         *
         * return : 0表示成功，其他表示失败.
         */
        virtual int close() = 0;
        /*
         * 获取资源路径
         *
         * return : 资源路径
         */
        virtual const char* path() const = 0;
        /*
         * 获取队列中的数据数量
         *
         * return : 数据数量
         */
        virtual const uint64_t count() const = 0;
};

}

#endif // _UTIL_MEMQUEUE_H_
