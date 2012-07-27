#ifndef _UPDATERSERVER_H_
#define _UPDATERSERVER_H_

#include <arpa/inet.h>

#include "util/ThreadLock.h"
#include "common/Config.h"
#include "common/FileQueue.h"
#include "plugin/EnterWorker.h"
#include "plugin/WorkerFactory.h"

#define ACCEPT_FD_MAX 1024

namespace update {

struct RecvArgv {
    int id;                     // 记录线程信息
    int port;                   // 客户端口
    char ip[INET_ADDRSTRLEN];   // 客户端ip
    void* arg;
    void* worker;
};

enum ThreadState {
    THREAD_IDLE = 0,    // 空闲，可直接用
    THREAD_BUSY = 1,    // 繁忙中，需要等待
    THREAD_EXIT = 2     // 已经退出，可以回收
};

class UpdaterServer
{ 
public:
    UpdaterServer();
    ~UpdaterServer();

    /*
     * 
     */
    int init(const char* cfg, const char* log);
    
    /*
     * 启动服务
     */
    int start();

    /*
     * 停止服务
     */
    int stop();

private:
    int allocThreadId();

    // flag : 线程存在: true, 线程不存在: false 
    void freeThreadId(int id, bool flag);

private:
    Config _cfg;
    bool _start_simon;
    int  _start_flag;       // stop 0: start: 1

    int _enter_listen;      // 监听dump中心
    int _cluster_listen;    // 监听search、detail

    util::Mutex _lock;      // 记录线程id
    pthread_t _thread[ACCEPT_FD_MAX]; 
    char _threadUsing[ACCEPT_FD_MAX];
    RecvArgv _recvArg[ACCEPT_FD_MAX];

    TaskQueue _taskQueue;
    WorkerFactory _factory;

public:
    /* 
     * 接收dump中心发起的新的连接
     */
    static void* enter_accept(void* para);
    
    /* 
     * 接收dump中心的数据,并完成存储
     */
    static void* enter_worker(void* para);
   
    /*
     * 接收search、detail发起的新连接
     */
    static void* cluster_accept(void* para); 

    /*
     * 数据处理并发送，负载较重
     */
    static void* cluster_worker(void* para);  
};

}
 
#endif //FRAME_UPDATERSERVER_H_
