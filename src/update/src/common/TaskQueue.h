#ifndef _UPDATE_TASKQUEUE_H_
#define _UPDATE_TASKQUEUE_H_

#include <stdint.h>
#include "util/ThreadLock.h"

namespace update {

class Session {
public:
    Session() {
        _level= 0;
        _seq  = 0;
    }
    virtual ~Session() {
    }

    bool less(Session* session) {
        if(_seq < session->_seq) {
            return false;
        }
        if(_seq > session->_seq) {
            return true;
        }
        if(_level < session->_level) {
            return false;
        }
        if(_level > session->_level) {
            return true;
        }
        return true;
    }
    void show(char* mess) {
        sprintf(mess, "%d %d", _level, _seq);
    }

protected:
    uint32_t _level;  // 级别
    uint64_t _seq;    // 消息号
};

class TaskQueue {
    public:
        TaskQueue();
        ~TaskQueue();
    public:
	
        /* 初始化一个优先队列 */
        int init( int maxSize );

        /**
         *@brief 入队操作
         *@param task 会话对像
         *@return 0,成功 非0,失败
         */
        int32_t enqueue(Session *task);

        /**
         *@brief 出队操作
         *@param taskp 会话对像
         *@return 0,成功 非0,失败
         */
        int32_t dequeue(Session **taskp);

        /**
         *@brief 激活block状态的队列
         */
        void interrupt();

        /**
         *@brief 终止task queue当前工作
         */
        void terminate();

        /**
         *@brief 获取队列中元素个数
         */
        uint32_t size() {return _size;}

    private:
        /* 加一个元素到优先队列,如果队列已满,否则返回false */
        bool insert( Session* task );
        
        /* 从优先队列里移去最小的元素 */
        Session* pop();
        
        /*
         * 加一个元素到优先队列
         */
        void push( Session* task );
	
        /* 向上调整优先队列 */
        inline void upHeap();
        
        /* 向下调整优先队列 */
        inline void downHeap();

    private:

        bool _terminated;
        Session** _heap;
        uint32_t  _size;
        uint32_t  _max;
        UTIL::Condition _lock;
};

}

#endif //_FRAMEWORK_TASKQUEUE_H_

