#ifndef _ENTERWORKER_H_
#define _ENTERWORKER_H_

#include "common/Config.h"
#include "common/netdef.h"
#include "common/FileQueue.h"
#include "common/XmlReader.h"
#include "util/ThreadLock.h"
#include "commdef/DocMessage.pb.h"
#include "WorkerFactory.h"

namespace update {

class EnterWorker : public Worker
{ 
public:
    EnterWorker(FileQueue& que);
    ~EnterWorker();
   
    int init(Config* cfg, ClusterMessage* info);
    int process(net_head_t* head, char* message);
    
private:
    int getDispatcherId(DocMessage* msg, uint64_t& id);

private:
    Config* _cfg;
    XmlReader* _reader;

    int _dispFieldAdd;       // add 消息分发字段下标
    int _dispFieldDel;       // del 消息分发字段下标

    util::Mutex _lock;       // 分配更新序号
    FileQueue& _enterQueue;  // 存储更新消息

    int _fdSeq;              // 记录更新序号
};

}

#endif //PLUGIN_ENTERWORKER_H_
