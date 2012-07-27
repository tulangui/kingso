/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 167 $
 *
 * $LastChangedDate: 2011-03-25 11:13:57 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: TaskQueue.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: queue of task $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_TASKQUEUE_H_
#define _FRAMEWORK_TASKQUEUE_H_
#include "util/namespace.h"
#include "framework/Session.h"
#include "util/ThreadLock.h"
#include <stdint.h>

FRAMEWORK_BEGIN;

class TaskQueue {
    public:
        TaskQueue();
        ~TaskQueue();
    public:
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
        bool _terminated;
        Session *_head;
        Session *_tail;
        uint32_t _size;
        UTIL::Condition _lock;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_TASKQUEUE_H_

