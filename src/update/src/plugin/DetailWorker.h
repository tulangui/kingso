#ifndef _DETAIL_WORKER_H_ 
#define _DETAIL_WORKER_H_ 

#include <arpa/inet.h>
#include "api/UpdateProcessor.h"
#include "QueueCache.h"
#include "WorkerFactory.h"

namespace update {

class DetailWorker : public Worker
{ 
public:
    DetailWorker(QueueCache& cache);
    ~DetailWorker();

    int init(Config* cfg, ClusterMessage* info);
    int process(net_head_t* head, char* message);

private:
    QueueCache& _cache;
    DocMessage _message;

    int serial(net_head_t* head, char* message, char zip);
};

}

#endif // _DETAIL_WORKER_H_
