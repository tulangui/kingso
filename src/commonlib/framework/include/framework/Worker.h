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
 * $Id: Worker.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: define Worker object which used in worker thread$
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_WORKER_H_
#define _FRAMEWORK_WORKER_H_
#include "util/namespace.h"
#include "util/Thread.h"
#include "framework/namespace.h"

FRAMEWORK_BEGIN;

class Session;
class Server;

/**
 *@brief 开始服务调度
 *@return 0,成功 非0,失败
 */
class Worker : public UTIL::Runnable {
    public:
        Worker(Session & session) : _session(session) { }
        virtual ~Worker() { }
    protected:
        Session &_session;
};

class WorkerFactory {
    public:
        WorkerFactory() : _server(NULL) { }
        virtual ~WorkerFactory() { }
    public:
        /**
         *@brief 初始化工厂类
         *@param path 配置文件
         *@return 0,成功 非0,失败
         */
        virtual int32_t initilize(const char *path) { return 0; }
        /**
         *@brief 根据会话对象，生成Worker对象
         *@param session 会话对象
         *@return Worker对象
         */
        virtual Worker * makeWorker(Session &session) = 0;
        /**
         *@brief 设置属主Server
         */
        void setOwner(Server *server) {
            _server = server;
        }
    protected:
        Server *_server;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_WORKER_H_
