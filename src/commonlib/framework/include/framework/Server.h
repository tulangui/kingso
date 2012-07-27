/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 191 $
 *
 * $LastChangedDate: 2011-03-25 15:45:28 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: Server.h 191 2011-03-25 07:45:28Z taiyi.zjh $
 *
 * $Brief: Server object which define the framework of server procedure $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_SERVER_H_
#define _FRAMEWORK_SERVER_H_
#include "framework/namespace.h"
#include "util/ThreadLock.h"

namespace anet {
    class Transport;
}

namespace clustermap {
    class CMClient;
}

UTIL_BEGIN;
class ThreadPool;
UTIL_END;

FRAMEWORK_BEGIN;

class Service;
class TaskQueue;
class Dispatcher;
class WorkerFactory;
class ParallelCounter;
class CacheHandle;
class WebProbe;

//服务器状态
typedef enum {
    srv_unknown,
    //服务启动中
    srv_startting,
    //服务已启动
    srv_startted,
    //服务停止中
    srv_stopping,
    //服务已停止
    srv_stopped
} server_status_t;

//服务的角色
typedef enum {
    srv_type_unknown,
    //searcher server
    srv_type_searcher,
    //merger server
    srv_type_merger,
    //blender server
    srv_type_blender
} server_type_t;

/**
 *@class Server
 *@brief 服务框架类，定义了整个服务流程
 */
class Server {
    public:
        Server();
        virtual ~Server();
        /**
         *@brief 启动服务
         *@param str 服务名称
         *@return 0,成功 非0,失败
         */
        int32_t start(const char *str);
        /**
         *@brief 停止服务
         *@return 0,成功 非0,失败
         */
        int32_t stop();
        /**
         *@brief 等待其他线程退出
         *@return 0,成功 非0,失败
         */
        int32_t wait();
        /**
         *@brief 获取任务队列
         *@return 任务队列
         */
        TaskQueue * getTaskQueue() { return _taskQueue; }
        /**
         *@brief 获取anet通信对象
         *@return anet通信对象
         */
        anet::Transport * getANETTransport() { return _transport; }
        /**
         *@brief 获取clustermap对象
         *@return clustermap对象
         */
        clustermap::CMClient * getCMClient() { return _cmClient; }
        /**
         *@brief 获取服务对象链表
         *@return 服务对象链表
         */
        Service * getServiceList() { return _services; }
        /**
         *@brief 获取超时时间
         *@return 超时时间
         */
        uint64_t getTimeoutLimit() const { return _timeout; }
        /**
         *@brief 获取内存限制
         *@return 内存限制
         */
        uint64_t getMemLimit() const { return _memLimit; }

    protected:
        /**
         *@brief 创建worker工厂类
         */
        virtual WorkerFactory * makeWorkerFactory() = 0;

    private:
        /**
         *@brief 添加新的服务到框架中
         *@param protcol 使用的协议，如http、tcp
         *@param server 服务名称 
         *@param port 端口号
         *@return 0,成功 非0,失败
         */
        int32_t addService(const char *protcol, 
                const char *server, uint16_t port);
        /**
         *@brief 启动线程池
         *@param maxThrCount 最大线程数
         *@param stackSize 线程的堆栈大小
         *@return 0,成功 非0,失败
         */
        int32_t startThreads(uint32_t maxThrCount, uint32_t stackSize);
        //设置异常管理
        int32_t initParallelLimitter(uint32_t maxParallel, 
                uint64_t sessionTimeout, uint32_t seriateTimeoutCntLimit,
                uint32_t seriateErrCntLimit);
        /**
         *@brief 根据配置文件，初始化worker工厂类
         *@param path 配置文件路径 
         *@return 0,成功 非0,失败
         */
        int32_t initWorkerFactory(const char *path);
        /**
         *@brief 根据配置文件，初始化clustermap client类
         */
        int32_t initCMClient(const char *localCfg, const char *servers);
        /**
         *@brief 在clustermap server上，注册本机
         */
        int32_t regCMClient();
        /**
         *@brief 在clustermap server上，订阅消息
         */
        int32_t subCMClient();
        /**
         *@brief 释放资源
         */
        void release();
    public:
        server_type_t _type;
    private:
        //超时时间
        uint64_t _timeout;
        //内存限制
        uint64_t _memLimit;
        //服务列表
        Service *_services;
        //任务队列
        TaskQueue *_taskQueue;
        //线程池
        UTIL::ThreadPool *_thrPool;
        //调度器
        Dispatcher *_disp;
        //worker工厂类
        WorkerFactory *_workerFactory;
        //anet通信类
        anet::Transport *_transport;
        //并发控制及异常回复
        ParallelCounter *_counter;
        CacheHandle *_cache;
        clustermap::CMClient *_cmClient;
        server_status_t _status;
        //线程条件变量
        UTIL::Condition _cond;
        //
        WebProbe *_webProbe;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_SERVER_H_

