#include <stdlib.h>
#include <string.h>

#include "VisitStat.h"
#include "VisitStatArgs.h"
#include "VisitStatSync.h"

int main(int argc, char** argv)
{
    alog::Configurator::configureRootLogger();
    alog::Logger::MAX_MESSAGE_LENGTH = 10240;

    VisitStatArgs args;
    if(args.parseArgs(argc, argv) < 0) {
        return -1;
    }

    VisitStat st;
    st.setStatMaxNum(args.getMaxStatNum());
    st.setSegTime(args.getSegBegin(), args.getSegEnd());
    if(st.init(args.getServerCfg(), args.getLogCfg())) {
        TERR("init failed");
        return -1;
    }

    // 多列结果统计
    VisitResult mulResult(args.getColNum());
    VisitStatAccpet accpet(mulResult, args.getListenPort());
    if(accpet.start() < 0) {
        TERR("trans start failed");
        return -1;
    }

    void *retval = NULL;
    int threadNum = args.getThreadNum();
    pthread_t threadId[threadNum];

    // 日志处理
    time_t begin = time(NULL);
    for(int i = 0; i < threadNum; i++) {
        pthread_create(&threadId[i], NULL, st.deal_query, &st);
    }
    int fail = 0, allNum, trueNum;
    for (int i = 0; i < threadNum; i++) {
        pthread_join(threadId[i], &retval);
        if(retval) fail++;
    }
    int ret = st.getQueryNum(allNum, trueNum);
    if(fail > 0 || ret < 0 || trueNum <= 0) {
        TERR("query deal fail %d", fail);
        return -1;
    }
    TLOG("log deal use %d, allNum=%d, trueNum=%d", time(NULL) - begin, allNum, trueNum);

    // 统计处理
    time_t stBegin = time(NULL);
    for(int i = 0; i < threadNum; i++) {
        pthread_create(&threadId[i], NULL, st.stat_visit, &st);
    }
    fail = 0;
    for (int i = 0; i < threadNum; i++) {
        pthread_join(threadId[i], &retval);
        if(retval) fail++;
    }
    time_t stEnd = time(NULL);
    if(fail > 0) {
        TERR("visit stat fail %d", fail);
        return -1;
    }
    int32_t use = stEnd - stBegin;
    if(use <= 0) use = 1;
    TLOG("success stat, use time = %d, qps=%d", stEnd - stBegin, trueNum / use);

    VisitResult result(args.getColNum());
    if(st.output(result) < 0) {
        TERR("get result error");
        return -1;
    }

    // 发送统计结果
    VisitStatClient client(result, args);
    if(client.sendResult() < 0) {
        TERR("send Result error");
        return -1;
    }
    
    // 合并多列统计结果
    accpet.stop();
    if(mulResult.output(args.getOutPath(), args.getTop()) < 0) {
        TERR("output error");
        return -1;
    }
    TLOG("success get top, use time = %d", time(NULL) - begin);

    return 0;
}

