#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "util/Log.h"
#include "util/errcode.h"
#include "update/update_def.h"
#include "common/netfunc.h"
#include "UpdaterServer.h"
#include "api/UpdateProcessor.h"

namespace update {

UpdaterServer::UpdaterServer()
{
    _start_simon = false;
    _start_flag  = 0;
    _enter_listen = 0;
    _cluster_listen = 0;
    for(int i = 0; i < ACCEPT_FD_MAX; i++) { 
        _threadUsing[i] = THREAD_IDLE;
    }
}

UpdaterServer::~UpdaterServer()
{
    if(_enter_listen > 0) close(_enter_listen);
    if(_cluster_listen > 0) close(_cluster_listen);

    _enter_listen = 0;
    _cluster_listen = 0;
}

int UpdaterServer::init(const char* cfg, const char* log)
{
    if(log && log[0]) {
        try {
            alog::Configurator::configureLogger(log);
        } catch(std::exception &e) {
            fprintf(stderr, "open alog error, path : %s\n", log);
            return -1;
        }
    } else{
        alog::Configurator::configureRootLogger();
    }
    
    if(_cfg.init(cfg, UPDATE_SERVER) < 0) {
        TERR("init config %s failed", cfg);
        return -1;
    }
    
    if(_factory.init(&_cfg) < 0) {
        TERR("worker factory init failed");
        return -1;
    }
    if(_taskQueue.init(MAX_WORKER_NUM) < 0) {
        TERR("init task queue failed");
        return -1;
    }
    return 0;
}

int UpdaterServer::start()
{
    // 监听dump中心
    _enter_listen = listening(_cfg._enter_port);
    if(_enter_listen <= 0) {
        TERR("create listen port %d failed", _cfg._enter_port);
        return -1;
    }
    // 监听cluster 
    _cluster_listen = listening(_cfg._cluster_port);
    if(_cluster_listen <= 0) {
        TERR("create listen port %d failed", _cfg._cluster_port);
        return -1;
    }
   
    _start_flag = 1;
    RecvArgv* para = NULL;
    for(int i = 0; i < _cfg._cluster_thread_num; i++) {
        int id = allocThreadId();
        if(id < 0) {
            TERR("alloc Thread failed");
            return -1;
        }
        para = &_recvArg[id];
        para->id = id;
        para->arg = this;
        if(pthread_create(&_thread[id], NULL, cluster_worker, para)) {
            TERR("creat thread cluster_worker failed");
            return -1;
        }
    }
    
    int id = allocThreadId();
    if(id <= 0) {
        TERR("alloc Thread failed");
        return -1;
    }
    para = &_recvArg[id];
    para->id = id;
    para->arg = this;
    if(pthread_create(&_thread[id], NULL, enter_accept, para)) {
        TERR("creat thread enter_accept failed");
        return -1;
    }

    id = allocThreadId();
    if(id <= 0) {
        TERR("alloc Thread failed");
        return -1;
    }
    para = &_recvArg[id];
    para->id = id;
    para->arg = this;
    if(pthread_create(&_thread[id], NULL, cluster_accept, para)) {
        TERR("creat thread cluster_accept failed");
        return -1;
    }
    TLOG("UpdaterServer: start success");

    return 0;
}

int UpdaterServer::stop()
{
    _start_flag = 0;
    _taskQueue.terminate();

    for(int i = 0; i < ACCEPT_FD_MAX; i++) {
        if(_threadUsing[i] == THREAD_IDLE) continue;
        while(_threadUsing[i] == THREAD_BUSY) usleep(1);
        pthread_join(_thread[i], NULL);
    }

    if(_enter_listen > 0) close(_enter_listen);
    if(_cluster_listen > 0) close(_cluster_listen);
    _enter_listen = 0;
    _cluster_listen = 0;

    TLOG("UpdaterServer: stop success");

    alog::Logger::shutdown();
    _start_simon = false;

    return 0;
}
    
int UpdaterServer::allocThreadId()
{
    if(_start_flag == 0) {
        return -1;
    }

    _lock.lock();
    for(int i = 0; i < ACCEPT_FD_MAX; i++) {
        if(_threadUsing[i] == THREAD_BUSY) {
            continue;
        }
        
        if(_threadUsing[i] == THREAD_EXIT) { // 已经释放，回收下线程
            pthread_join(_thread[i], NULL);
        }
        _threadUsing[i] = THREAD_BUSY;
        _lock.unlock();
        return i;
    }
    _lock.unlock();
    return -1;
}

void UpdaterServer::freeThreadId(int id, bool flag)
{
    _lock.lock();
    if(flag) {// 线程存在，需要回收线程资源
        _threadUsing[id] = THREAD_EXIT;
    } else { // 直接可用
        _threadUsing[id] = THREAD_IDLE;
    }
    _lock.unlock();
}

void* UpdaterServer::enter_accept(void* para)
{
    RecvArgv* argv = (RecvArgv*)para;
    UpdaterServer* server = (UpdaterServer*)argv->arg;

    struct sockaddr addr;
    socklen_t addr_len;
    timeval send_timeout;
    
    int keepalive = 1;
    send_timeout.tv_usec = 0;
    send_timeout.tv_sec = server->_cfg._enter_send_timeout;
    struct sockaddr_in* addr_p = (struct sockaddr_in*)(&addr);
    net_head_t head = {0ll, UPCMD_ACK_SUCCESS, 0, 0};

    ClusterMessage info;
    
    info.seq = 0;
    info.colNum = 0;
    info.colNo = 0;
    info.role = UPDATE_SERVER;

    while(server->_start_flag) {
        int fd = accept_wait(server->_enter_listen, &addr, &addr_len, &server->_start_flag);
        if(fd <= 0) {
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(timeval));

        int id = server->allocThreadId();
        if(id < 0) {
            TWARN("socket limit arrival, continue!");
            close(fd);
            continue;
        }

        RecvArgv* arg = &server->_recvArg[id];
        inet_ntop(AF_INET, &(addr_p->sin_addr), arg->ip, INET_ADDRSTRLEN);
        arg->port = ntohs(addr_p->sin_port);
        arg->id = id;
        arg->arg = argv->arg;
        
        info.port = arg->port;
        strcpy(info.ip, arg->ip);
        
        Worker* worker = server->_factory.make(&info, fd);
        if(worker == NULL) {
            TWARN("create worker failed, continue");
            close(fd);
            server->freeThreadId(id, false);
            continue;
        }
        arg->worker = worker; 
        
        if(write_check(fd, &head, sizeof(net_head_t))!=sizeof(net_head_t)){
            TWARN("write first ack error, return!");
            server->_factory.free(worker);
            server->freeThreadId(id, false);
            continue;
        }

        if(pthread_create(&server->_thread[id], NULL, enter_worker, arg)) {
            TWARN("create receiving thread error, continue!");
            server->_factory.free(worker);
            server->freeThreadId(id, false);
        } else {
            TLOG("enter get %s:%d, using id=%d", arg->ip, arg->port, arg->id);
        }
    }

    TLOG("enter accept thread ending...");
    server->freeThreadId(argv->id, true);

    return NULL;
}
    
void* UpdaterServer::enter_worker(void* para)
{
    RecvArgv* arg = (RecvArgv*)para;
    UpdaterServer* server = (UpdaterServer*)arg->arg; 
    EnterWorker* worker = (EnterWorker*)arg->worker;
    
    int fd = worker->getSocket();
    net_head_t head = {0ll, UPCMD_ACK_SUCCESS, 0, 0};
    char recvBuf[DEFAULT_MESSAGE_SIZE];

    while(server->_start_flag) {
        if(read_wait(fd, &head, sizeof(net_head_t), &server->_start_flag) != sizeof(net_head_t)) {
            break;
        }
        
        if(head.size > (uint32_t)DEFAULT_MESSAGE_SIZE) {
            TWARN("message seq=%lu, cmd=%c, size overlimit(%d/%d), abandon", head.seq, head.cmd, head.size, DEFAULT_MESSAGE_SIZE);
            uint32_t size = 0;
            do {
                uint32_t need = head.size - size > (uint32_t)DEFAULT_MESSAGE_SIZE ? DEFAULT_MESSAGE_SIZE : head.size - size; 
                if(read_wait(fd, recvBuf, need, &server->_start_flag) != need) {
                    break;
                }
                size += need;
            } while(size < head.size);
            if(size < head.size) {
                TWARN("read client data error, return!");
                break;
            }
            // 返回异常错误
            head.cmd = UPCMD_ACK_EUNKOWN;
            if(write_check(fd, &head, sizeof(net_head_t)) != sizeof(net_head_t)) {
                TWARN("write ack error, return!");
                break;
            }
        } else {
            if(read_wait(fd, recvBuf, head.size, &server->_start_flag) != head.size) {
                TWARN("read client data error, return!");
                break;
            }
            if(worker->process(&head, recvBuf) < 0) {
                head.cmd = UPCMD_ACK_EPROCESS;
            } else {
                head.cmd = UPCMD_ACK_SUCCESS;
            }
            if(write_check(fd, &head, sizeof(net_head_t)) != sizeof(net_head_t)) {
                TWARN("write ack error, return!");
                break;
            }
            if(head.seq % 10000 == 0) {
                TACCESS("%s:%d(%s) %s nid=%lu,seq=%lu,head.seq=%lu", worker->ip(), worker->port(),
                        worker->role(), worker->action(), worker->nid(), worker->seq(), head.seq);
            }
        }
    }

    server->_factory.free(worker);
    server->freeThreadId(arg->id, true);

    return NULL;
}
   
void* UpdaterServer::cluster_accept(void* para)
{
    RecvArgv* arg = (RecvArgv*)para;
    UpdaterServer* server = (UpdaterServer*)arg->arg;

    int listen = server->_cluster_listen;
    TaskQueue& que = server->_taskQueue;
    WorkerFactory& factory = server->_factory;

    struct sockaddr addr;
    socklen_t addr_len;
    timeval send_timeout;
    
    int keepalive = 1;
    send_timeout.tv_usec = 0;
    send_timeout.tv_sec = server->_cfg._cluster_send_timeout;

    ClusterMessage info;

    while(server->_start_flag) {
        int fd = accept_wait(listen, &addr, &addr_len, &server->_start_flag);
        if(fd <= 0) {
            continue;
        }

        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(timeval));

        if(read_wait(fd, &info, sizeof(info), &server->_start_flag) != sizeof(info)) {
            TWARN("read cluster info error, continue!");
            close(fd);
            continue;
        }
        
        Worker* worker = factory.make(&info, fd);
        if(worker == NULL) {
            close(fd);
            TWARN("create worker failed, continue");
            continue;
        }
        if(que.enqueue(worker) != KS_SUCCESS) {
            TWARN("enqueue worker failed, continue");
            factory.free(worker);
        }
    }
    
    TLOG("cluster thread ending...");
    server->freeThreadId(arg->id, true);

    return NULL;
}

void* UpdaterServer::cluster_worker(void* para)
{
    RecvArgv* arg = (RecvArgv*)para;
    UpdaterServer* server = (UpdaterServer*)arg->arg;

    TaskQueue& que = server->_taskQueue;
    WorkerFactory& factory = server->_factory;
    
    Session* session = NULL;
    char message[DEFAULT_MESSAGE_SIZE];
    net_head_t head = {0ll, UPCMD_ACK_SUCCESS, 0, 0};

    while(server->_start_flag) {
        if(que.dequeue(&session) != KS_SUCCESS) {
            continue;
        }

        Worker* worker = (Worker*)session;
        if(worker->isValid() == false) { // 无效了
            TLOG("worker %s:%d unvalid, free!", worker->ip(), worker->port());
            factory.free(worker);
            continue;
        }
        Worker* bak = NULL;

        int ret = worker->process(&head, message);
        if(ret != RET_SUCCESS) { // 失败,或者正在处理
            switch (ret) {
                case RET_FAILE:
                    TWARN("worker process %lu failed, continue", worker->nid());
                    while(worker) {
                        bak = worker->next();
                        worker->_nextWorker = NULL;
                        worker->nextWork();
                        if(que.enqueue(worker) != KS_SUCCESS) {
                            if(server->_start_flag) TERR("enqueue worker failed, return");
                            server->freeThreadId(arg->id, true);
                            return NULL;
                        }
                        worker = bak;
                    }
                    break;
                case RET_NO:
                    usleep(1); // 全部处理完成，等待新的任务
                    if(worker->_nextWorker) {
                        TERR("can't valid nextworker");
                    }
                    if(que.enqueue(worker) != KS_SUCCESS) {
                        if(server->_start_flag) TERR("enqueue worker failed, return");
                        server->freeThreadId(arg->id, true);
                        return NULL;
                    }
                    break;
                case RET_DOING:
                    break;
                default:
                    factory.free(worker);
                    TERR("unknow process ret value");
                    server->freeThreadId(arg->id, true);
                    return NULL;
            }
            continue;
        } 
        
        // 发送数据并接收响应
        while(worker) {
            bak = worker->next();
            worker->_nextWorker = NULL;

            // 处理完成，发送
            if((ret = worker->checkFlux()) > 0) { // 被流控了
                if(que.enqueue(worker) != KS_SUCCESS) {
                    if(server->_start_flag) TERR("enqueue worker failed, return");
                    server->freeThreadId(arg->id, true);
                    return NULL;
                }
                worker = bak;
                continue;
            }
            
            do {
                int fd = worker->getSocket();
                if(write_check(fd, message, head.size) != head.size) {
                    TWARN("send to %s:%d error, return!", worker->ip(), worker->port());
                    factory.free(worker);
                    break;
                }
                net_head_t res;
                if(read_wait(fd, &res, sizeof(res), &server->_start_flag) != sizeof(res)) {
                    TWARN("send to %s:%d error, return!", worker->ip(), worker->port());
                    factory.free(worker);
                    break;
                }
                // 检查响应
                if(head.seq != res.seq) {
                    TWARN("send to %s:%d receive ack seq %lu, not match local seq %lu, return!",
                            worker->ip(), worker->port(), res.seq, head.seq);
                    factory.free(worker);
                    break;
                }
                if(res.cmd != UPCMD_ACK_SUCCESS) {
                    TWARN("send to %s:%d return error %d, continue!",  worker->ip(), worker->port(), res.cmd);
                }
                if(head.seq % 10000 == 0) {
                    TACCESS("%s:%d(%s) %s nid=%lu,seq=%lu,head.seq=%lu", worker->ip(), worker->port(),
                            worker->role(), worker->action(), worker->nid(), worker->seq(), head.seq);
                }
                worker->statFlux(head.size);
                worker->nextWork();
                if(que.enqueue(worker) != KS_SUCCESS) {
                    if(server->_start_flag) TERR("enqueue worker failed, return");
                    server->freeThreadId(arg->id, true);
                    return NULL;
                }
            }while(0);
            worker = bak;    
        }
    }
    
    server->freeThreadId(arg->id, true);
    return NULL;
}

}
 
