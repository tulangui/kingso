#include "framework/Dispatcher.h"
#include "util/ThreadPool.h"
#include "framework/Worker.h"
#include "framework/TaskQueue.h"
#include "framework/Session.h"
#include <new>
#include <stdio.h>

FRAMEWORK_BEGIN;

Dispatcher::Dispatcher(UTIL::ThreadPool &tp, 
        TaskQueue &tq, 
        WorkerFactory &wf)
    : _taskQueue(tq),
    _thrPool(tp), 
    _workerFactory(wf) { }

Dispatcher::~Dispatcher() { }

int32_t Dispatcher::run() 
{
    Session *session = NULL;
    int32_t ret = 0;
    Worker *worker = NULL;
    while(likely(_stat != UTIL::thr_terminated)) {
        ret = _taskQueue.dequeue(&session);
        if (unlikely(ret == EAGAIN)) {
            continue;
        }
        if (unlikely(ret == EOF)) {
            break;
        }
        if (unlikely(ret != KS_SUCCESS)) {
            continue;
        }
        if (unlikely(!session)) {
            continue;
        }
        //判断是否超时
        if (_timeout > 0 && 
                Session::getCurrentTime() > 
                (session->getStartTime()+_timeout)) 
        {
            //session->setStatus(ss_error);
            //session->response();
            //continue;
            switch (session->getStatus()) {
                case ss_phase1_send:
                case ss_phase1_return:
                    session->setStatus(ss_phase1_timeout);
                    break;
                case ss_phase2_send:
                case ss_phase2_return:
                case ss_uniqinfo_send:
                    session->setStatus(ss_phase2_timeout);
                    break;
                case ss_phase3_send:
                case ss_phase3_return:
                    session->setStatus(ss_phase3_timeout);
                    break;
                default:
                    session->setStatus(ss_timeout);
                    break;
            };
        }
        //创建worker
        worker = _workerFactory.makeWorker(*session);
        if (unlikely(!worker)) {
            return ENOMEM;
        }
        //分配工作线程
        ret = _thrPool.newThread(NULL, worker);
        if (ret != KS_SUCCESS) {
            session->setStatus(ss_error);
            session->response();
        }
    }
    return KS_SUCCESS;
}

FRAMEWORK_END;

