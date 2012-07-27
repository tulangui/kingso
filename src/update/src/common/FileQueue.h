/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $LastChangedDate: 2011-09-29 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Brief: 文件队列实现 $
 ********************************************************************/

#ifndef _FILEQUEUE_H_
#define _FILEQUEUE_H_

#include "Queue.h"
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

namespace update {

class FileQueueChunk;

typedef struct
{
    uint64_t index_count;
    uint64_t write_seq;
}
FileQueueInfo;

typedef struct
{
    uint64_t date : 27;
    uint64_t fseq : 64;
    uint64_t lseq : 64;
}
FileQueueIndex;

typedef struct
{
    uint32_t colNum; // 列数
    uint32_t colNo;  // 列号
    uint64_t seq;    // 当前seq
} FileQueueSelect;

/**
 * 一个写入进程，一个读进程.
 */
class FileQueue : public Queue
{ 
    public:
        FileQueue();
        ~FileQueue();
    private:
        /*
         * 禁止拷贝构造函数和赋值操作
         */
        FileQueue(const FileQueue&);
        FileQueue& operator=(const FileQueue&);
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
         * msg : 消息缓存区
         *
         * return : 1表示成功，0表示队列已空，-1表示失败
         */
        int pop(StorableMessage& msg);
        int pop(StorableMessage& msg, FileQueueSelect& sel); 

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
         * no : 消息编号
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
        int switchChunk();
    private:
        FileQueueInfo*  _info;      // 文件队列的公共信息
        FileQueueIndex* _index;     // 文件队列的chunk索引
        int _iffd;                  // 文件队列的公共信息文件描述符
        int _idfd;                  // 文件队列的chunk索引文件描述符

        uint32_t _inow;
        uint32_t _iyes;
        FileQueueChunk* _todchunk;  // 今天的数据
        FileQueueChunk* _yeschunk;  // 昨天的数据
        FileQueueIndex* _todindex;
        FileQueueIndex* _yesindex;

        uint64_t _read_seq;          // 读取序列号

        char _path[PATH_MAX+1];     // 文件队列的存储位置
        bool _opened;               // 是否被打开

        pthread_spinlock_t _lock;
        struct flock _info_lock;
        struct flock _info_unlock;
        static pthread_mutex_t _switch_lock;
};

}

#endif // _UTIL_FILEQUEUE_H_
