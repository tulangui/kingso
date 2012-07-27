#ifndef _CLUSTERWORKERFACTORY_H_
#define _CLUSTERWORKERFACTORY_H_

#include <arpa/inet.h>
#include "util/ThreadLock.h"
#include "util/HashMap.hpp"
#include "ks_build/ks_build.h"
#include "common/Config.h"
#include "common/FileQueue.h"
#include "common/TaskQueue.h"
#include "commdef/DocMessage.pb.h"
#include "api/UpdateProcessor.h"
#include "QueueCache.h"

#define MAX_COL_NUM 512         // 最大列数 
#define MAX_WORKER_NUM 10240    // 最大worker数量

namespace update {

enum {
    RET_FAILE = -1, // 处理失败，调用者跳过
    RET_NO    = 0,  // 没有需要处理的数据
    RET_DOING = 1,  // 其它线程正在处理相同的任务，已经挂载到一起 
    RET_SUCCESS = 2 // 处理成功，需要发送
};

class Worker : public Session
{ 
public:
    Worker();
    ~Worker();

    virtual int init(Config* cfg, ClusterMessage* info);
    virtual int process(net_head_t* head, char* message);

    int setSocket(int fd) { _fd = fd; return 0; }
    int getSocket() { return _fd; }

    /* 与之连接的客户端信息 */
    int port() { return _info.port; }
    uint32_t colNum() { return _info.colNum; }
    uint32_t colNo()  { return _info.colNo;  }
    uint32_t Role()  { return _info.role;  }
    const char* ip() { return _info.ip; }

    int checkFlux(); 
    int statFlux(int size);
    
    Worker* next() { return _nextWorker; }
    int nextWork();

    void setValid(bool valid) { _valid = valid; }
    int isValid() { return _valid; }

    // 角色
    const char* role();
    // 操作
    const char* action();
    // nid
    uint64_t nid();
    // 序列号
    uint64_t seq();                 // 当前seq
    uint64_t fseq() {return _seq;}  // 已经完成的seq
    
    Worker* _nextWorker;    // 相同任务的worker
    Worker* _nextRow;       // 同一列的下一行
protected:
    int _fd;        // socket
    bool _valid;    // 是否有效

    ksb_conf_s *_pKsbConf;       //libbuild的配置文件
    ksb_auction_s *_pKsbAuction; //libbuild中间结果
    ksb_result_s *_pKsbResult;   //libbuild输出

    bool _msgInit;
    StorableMessage _msg;        // 原始msg
    DocMessage _docMessage;      // 

    FileQueue _que;              //
    ClusterMessage _info;
    FileQueueSelect _select;     // 选择

    char* _unCompMem;           // 解压缩内存
    uint32_t _unCompSize;       // 解压内存大小

    time_t _beginTime;          // 开始时间
    uint64_t _flux;             // 流量，字节数
    uint64_t _fluxLimit;        // 限流

    char* _roleName[ROLE_LAST+1];      // 角色名
    char* _actionName[ACTION_LAST+1];  // 操作名
};

class WorkerMap {
    public:
        WorkerMap();
        ~WorkerMap();

        /* 检测该序号的detail是否处理完成 */
        bool checkPass(StorableMessage* msg);

        /* 增加、删除一个worker */
        bool addWorker(Worker* worker);
        bool delWorker(Worker* worker);

    private:
        uint32_t _colNum;
        util::RWLock _lock;
        Worker* _map[MAX_COL_NUM]; 
};

class WorkerFactory
{ 
public:
    WorkerFactory();
    ~WorkerFactory();

    int init(Config* cfg);

    /*
     * 根据cluster信息，生成对应的worker
     */
    Worker* make(ClusterMessage* info, int fd);
   
    /*
     * 因为网络等异常，需要保护好序列号等信息
     */
    void free(Worker* worker);

private:
    Config* _cfg;
    util::Mutex _lock;
    WorkerMap _workerMap;                           // 拓扑结构
    std::map<std::string, Worker*> _workerTable;    // ip+port 到worker的映射
     
    FileQueue _writeQue;                // 写队列
    QueueCache _searchCache;            // search worker cache
    QueueCache _detailCache;            // detail worker cache

    int _workerNum;                     // 创建的worker数量
    int _workerStat[MAX_WORKER_NUM];    // worker的状态
};

}

#endif //_CLUSTERWORKERFACTORY_H_
