#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "plugin/WorkerFactory.h"

using namespace update;

int main(int argc, char** argv)
{
    alog::Configurator::configureRootLogger();

    Config cfg;
    WorkerFactory fac;

    if(cfg.init(argv[1], DETAIL) < 0) {
        return -1;
    }

    if(fac.init(&cfg) < 0) {
        return -1;
    }

    ClusterMessage info;
    info.port = 6611;
    strcpy(info.ip, "10.232.42.17");
    info.colNum = 1;     // 总列数
    info.colNo = 0;      // 当前列号，暂时分析配置文件获取
    info.role = DETAIL;           // 角色
    info.seq = 4225603;

    Worker* worker = fac.make(&info, 0);

    int ret = 0;
    int num = 0;

    net_head_t head;
    char message[1<<20];

    while((ret = worker->process(&head, message)) != RET_NO) {
        num++;
        if(ret == RET_FAILE) {
            printf("fail num=%d\n", num);
        }
        worker->nextWork();
    }

    return 0;
}

