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
 * $Id: Dispatcher.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: dispatch task from queue to worker thread$
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_DISPATCHER_H_
#define _FRAMEWORK_DISPATCHER_H_
#include "util/namespace.h"
#include "framework/namespace.h"
#include "util/Thread.h"

UTIL_BEGIN;
class ThreadPool;
UTIL_END;

FRAMEWORK_BEGIN;

class TaskQueue;
class WorkerFactory;

class Dispatcher : public UTIL::Thread {
    public:
        /**
         *@brief 设置超时时间
         *@param tp 线程池
         *@param tq 任务队列
         *@param wf Worker工厂类
         */
        Dispatcher(UTIL::ThreadPool &tp, 
                TaskQueue &tq, 
                WorkerFactory &wf);
        ~Dispatcher();
    public:
        /**
         *@brief 开始服务调度
         *@return 0,成功 非0,失败
         */
        int32_t run();
        /**
         *@brief 设置超时时间
         *@param n 超时时间(单位:ms)
         */
        void setTimeoutLimit(uint64_t n) { _timeout = n; }
    private:
        //任务队列
        TaskQueue &_taskQueue;
        //线程池
        UTIL::ThreadPool &_thrPool;
        //worker工厂类
        WorkerFactory &_workerFactory;
        //超时时间
        uint64_t _timeout;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_DISPATCHER_H_

