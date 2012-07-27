#ifndef _MS_MERGER_SERVER_H_
#define _MS_MERGER_SERVER_H_

#include "util/common.h"
#include "merger.h"
#include "MemPoolFactory.h"
#include "framework/Server.h"
#include "framework/Worker.h"
#include "framework/Session.h"
#include "framework/TaskQueue.h"
#include "framework/Command.h"
#include "framework/ConnectionPool.h"
#include "framework/Request.h"
#include "FlowControl.h"

using namespace std;

MERGER_BEGIN;

class MergerWorker : public FRAMEWORK::Worker
{
    public:
        MergerWorker(
                // 会话对象类
                FRAMEWORK::Session& session,
                // 流程控制插件管理类
                FlowControl& flowControl,
                // 内存池工
                MemPoolFactory& mpFactory,
                // 任务队列
                FRAMEWORK::TaskQueue& taskQueue);
        ~MergerWorker();
    public:
        // 主处理流程
        virtual int32_t run();
    private:
        // 处理超时
        void handleTimeout();
        // 处理blender请求
        int32_t handleRequest();
        // 处理searcher/detail返回结果
        int32_t handleResponse();
        // 生成最终结果
        void makeResponse();
    private:    
        // 流程控制插件管理类
        FlowControl& _flowControl;
        // 内存池工厂
        MemPoolFactory& _mpFactory;
        // 任务队列
        FRAMEWORK::TaskQueue& _taskQueue;
};

class MergerWorkerFactory : public FRAMEWORK::WorkerFactory
{
    public:
        MergerWorkerFactory();
        ~MergerWorkerFactory();
    public:
        // Worker工厂资源初始化
        virtual int32_t initilize(const char* path);
        // 生成Worker对象
        virtual FRAMEWORK::Worker* makeWorker(FRAMEWORK::Session& session);
    private:
        // 服务是否已经准备就绪
        bool _ready;
        // 流程控制插件管理类
        FlowControl _flowControl;
        // 内存池工厂
        MemPoolFactory _mpFactory;
};

class MergerServer : public FRAMEWORK::Server {
    public:
        MergerServer() { }
        ~MergerServer() { }
    protected:
        virtual FRAMEWORK::WorkerFactory * makeWorkerFactory();
};

class MergerCommand : public FRAMEWORK::CommandLine {
    public:
        MergerCommand() : CommandLine("ks_merge_server") {}
        ~MergerCommand() {}
    protected:
        virtual FRAMEWORK::Server * makeServer();
};

MERGER_END;

#endif //_MS_MERGER_SERVER_H_

