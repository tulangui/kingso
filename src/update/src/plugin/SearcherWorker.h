#ifndef _SEARCHER_WORKER_H_ 
#define _SEARCHER_WORKER_H_ 

#include <arpa/inet.h>
#include "WorkerFactory.h"
#include "QueueCache.h"
#include "api/UpdateProcessor.h"
#include "commdef/DocIndexMessage.pb.h"

namespace update {

class SearcherWorker : public Worker 
{ 
public:
    SearcherWorker(QueueCache& cache, WorkerMap& detail);
    ~SearcherWorker();

    int init(Config* cfg, ClusterMessage* info);
    int process(net_head_t* head, char* message);

private:
    int serial(net_head_t* head, char* message, char zip);

private:
    QueueCache& _cache;             // 处理结果cache
    WorkerMap& _detail;         // 依赖的detail

    DocIndexMessage _message;       //
};

}

#endif // _SEARCHER_WORKER_H_
