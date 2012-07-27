#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>

#include "util/strfunc.h"
#include "util/Timer.h"
#include "util/Log.h"
#include "util/Vector.h"
#include "util/ThreadLock.h"
#include "update/update_api.h"
#include "update/update_def.h"
#include "commdef/DocIndexMessage.pb.h"
#include "commdef/DocMessage.pb.h"

#define MACHINE_MAX 64
#define MAX_SPEC_SIZE 2048  // dispatcher 最大数量

using namespace update;

typedef UTIL::Vector<char *> TaskQueue;

struct ThreadPara {
    update_api_t* api;
    util::Mutex* lock;
    TaskQueue* pQueue;
    UpdateAction nAction;
    pthread_t nThrId;
};

int buildMessage(DocMessage &docMessage, char **pBuff, int nBuffSize)
{
    int nLen = docMessage.ByteSize();
    if (nBuffSize < nLen) {
        if ((*pBuff = new char[nLen + 1]) == NULL) {
            TWARN("new buffer fail, size=%d", nLen + 1);
            return 0;
        }
    }
    if (!docMessage.SerializeToArray(*pBuff, nLen)) {
        TWARN("DocMessage SerializeToString fail");
        return 0;
    }
    return nLen;
}

int push(update_api_t* api, DocMessage &docMessage)
{
    char pBuff[DEFAULT_MESSAGE_SIZE];
    char *pMessage = pBuff;
    int nMessageSize = buildMessage(docMessage, &pMessage, DEFAULT_MESSAGE_SIZE);
    if (nMessageSize > 0) {
        if(update_api_send(api, docMessage.nid(), pMessage, nMessageSize, 'N', docMessage.action())){
            TWARN("update_api_send error.");
            return -1;
        }
        if(pMessage!=pBuff){
            delete[] pMessage;
        }
    }
    return 0;
}

int pushDelMessage(update_api_t* api, const char *pFileName)
{
    FILE *fp = fopen(pFileName, "r");
    if (unlikely(fp == NULL)) {
        TWARN("open file: %s failed!", pFileName);
        return -1;
    }

    uint32_t nCnt = 0, failed = 0;
    DocMessage docMessage;
    posix_fadvise(fileno(fp), 0, 0, POSIX_FADV_SEQUENTIAL);

    const static char* header = "auction?";
    const static int headerLen = strlen(header);

    uint64_t nBeginTime = UTIL::Timer::getMicroTime();
    char line[DEFAULT_MESSAGE_SIZE];
    while (fgets(line, DEFAULT_MESSAGE_SIZE, fp) != NULL) {
        if (line[0] == '\n') continue;
        char* p = line;
        // auction?nid=10419037265&user_id=XXXX
        while(*p && isspace(*p)) p++;
        if(memcmp(p, "auction?", headerLen)) {
            TWARN("delMessage format error, ignore: %s", line);
            continue;
        }
        p += headerLen;
        char* end = p + strlen(p) - 1;
        while(p <= end && isspace(*end)) end--;
        *(++end) = 0;

        docMessage.Clear();
        docMessage.set_action(ACTION_DELETE);
        docMessage.set_seq(0);

        bool success = false;
        while(*p) {
            char* name = p;
            while(*p && *p != '&') p++;
            char ch = *p; *p = 0;

            char* value = name;
            while(value < p && *value != '=') value++;
            if(value >= p) {
                TWARN("delMessage format error, ignore: %s", line);
                break;
            }
            *value++ = 0;
        
            if(memcmp(name, "nid", 3) == 0) {
                long long nid = strtoul(value, NULL, 10);
                docMessage.set_nid(nid);
                success = true;
            }
            
            DocMessage_Field *pField = docMessage.add_fields();
            pField->set_field_name(name);
            pField->set_field_value(value);

            *(value-1) = '=';
            *p = ch;
            if(*p) p++;
        }
        if(success == false) {
            failed++;
            continue;
        } else {
            nCnt++;
            push(api, docMessage);
        }
    }
    fclose(fp);

    uint64_t nEndTime = UTIL::Timer::getMicroTime();
    TLOG("file=%s, cnt=%u, fail=%u, cost=%lu(ms)", pFileName, nCnt, failed, (nEndTime - nBeginTime) /1000);
    return 0;
}

int pushAddMessage(update_api_t* api, const char *pFileName)
{               
    FILE *fp = fopen(pFileName, "r");
    if (fp == NULL) {
        TWARN("open file: %s failed!", pFileName);
        return -1;
    }
    posix_fadvise(fileno(fp), 0, 0, POSIX_FADV_SEQUENTIAL);
    bool bBegin = false;
    bool bHasNid = false;
    uint32_t nCnt = 0;
    DocMessage docMessage;

    uint64_t nBeginTime = UTIL::Timer::getMicroTime();
    
    int lineNo = 0;
    char line[DEFAULT_MESSAGE_SIZE];

    while (fgets(line, DEFAULT_MESSAGE_SIZE, fp) != NULL) {
        lineNo++;
        if(line[0] == '\n'){
            continue;
        }
        if (strstr(line, "<doc>") == line) {
            if(bBegin){
                TWARN("one doc no </doc> flag.");
            }
            docMessage.Clear();
            bBegin = true;
            bHasNid = false;
        }
        else if (bBegin && strstr(line, "</doc>") == line) {
            bBegin = false;
            if (bHasNid) {
                docMessage.set_action(ACTION_ADD);
                docMessage.set_seq(0);
                push(api, docMessage);
                nCnt++;
            } else {
                TWARN("find </doc>, but not find nid, line=%d,file=%s", lineNo, pFileName);
            }
        }
        else if (bBegin) {
            char *p = strstr(line, "=");
            if (p == NULL) {
                TWARN("not find '=', line=%d,file=%s", lineNo, pFileName);
                continue;
            }
            char *szValue = p + 1;
            *p = '\0';
            int len = strlen(szValue);
            if (len > 0) {
                szValue[len - 1] = 0;
                if (len > 1 && szValue[len - 2] == '\01') {
                    szValue[len - 2] = '\0';
                }
            }
            if (strcmp(line, "nid") == 0 && strlen(szValue) > 0) {
                long nid = strtoul(szValue, NULL, 10);
                docMessage.set_nid(nid);
                bHasNid = true;
            }
            DocMessage_Field *pField = docMessage.add_fields();
            pField->set_field_name(line);
            pField->set_field_value(szValue);
        }
    }
    if(bBegin){
        TWARN("one doc no </doc> flag.");
    }
    uint64_t nEndTime = UTIL::Timer::getMicroTime();
    TLOG("file=%s, cnt=%u, cost=%lu(ms)", pFileName, nCnt, (nEndTime - nBeginTime) /1000);
    fclose(fp);
    return 0;
}

void* pushMessage(void* para) 
{
    ThreadPara* threadPara = (ThreadPara*)para;
    TaskQueue* pQueue = threadPara->pQueue;
    UpdateAction nAction = threadPara->nAction;

    if (pQueue == NULL) {
        return NULL;
    }

    while (1) {
        const char *pFile = NULL;

        threadPara->lock->lock();
        if (pQueue->size() == 0) {
            threadPara->lock->unlock();
            break;
        }
        pFile = pQueue->back();
        pQueue->popBack();
        threadPara->lock->unlock();

        if (pFile == NULL) {
            break;
        }
        if (ACTION_ADD == nAction) {
            TNOTE("pushing add file %s", pFile);
            pushAddMessage(threadPara->api, pFile);
        }
        else {
            TNOTE("pushing del file %s", pFile);
            pushDelMessage(threadPara->api, pFile);
        }
        delete[] pFile;
        pFile = NULL;
    }

    return NULL;
}

int loadQueue(const char *pPath, TaskQueue &queue)
{
    DIR *pDir = opendir(pPath);
    if (!pDir){
        if (errno == ENOTDIR && access(pPath, F_OK) == 0) {
            char *pFile = new char[strlen(pPath) + 1];
            strcpy(pFile, pPath);
            queue.pushBack(pFile);
            return 0;
        }
        return -1;
    }

    struct dirent *de = NULL;
    while ((de=readdir(pDir))!=NULL) {
        if (de->d_type != DT_REG && de->d_type != DT_LNK) {
            continue;
        }
        char *pFile = new char[strlen(pPath) + strlen(de->d_name) + 2];
        snprintf(pFile, strlen(pPath) + strlen(de->d_name) + 2, "%s/%s", pPath, de->d_name);
        queue.pushBack(pFile);
        TNOTE("find file %s", pFile);
    }
    closedir(pDir);
    return 0;
}

int init(char *pSpec, const char *pLogConf, ThreadPara* threadPara, int nThreadCnt)
{
    //初始化alog
    if (pLogConf != NULL && strlen(pLogConf) > 0) {
        try {
            alog::Configurator::configureLogger(pLogConf);
        } catch(std::exception &e) {
            fprintf(stderr, "config from '%s' fail, using default root.\n",
                    pLogConf);
            alog::Configurator::configureRootLogger();
        }
    } else {
        alog::Configurator::configureRootLogger();
    }

    if (pSpec == NULL) {
        TERR("get host fail, spec = null");
        return -1;
    }

    char* addr_strs[MACHINE_MAX+1];
    up_addr_t addrs[MACHINE_MAX];
    int ret = str_split_ex(pSpec, ';', addr_strs, MACHINE_MAX+1);
    if(ret<=0 || ret>MACHINE_MAX){
        TERR("too muth address input, count=%d.", ret);
        return -1;
    }
    int addr_count = ret;
    int j = 0;
    for(int i=0; i<addr_count; i++){
        addr_strs[i] = str_trim(addr_strs[i]);
        int len = strlen(addr_strs[i]);
        if(len<=0){
            continue;
        }
        char *p = strchr(addr_strs[i], ':');
        if(p){
            if(p==addr_strs[i] || p==addr_strs[i]+len-1){
                continue;
            }
            *p = '\0'; 
            char *p2 = p + 1;
            strcpy(addrs[j].ip, addr_strs[i]);
            addrs[j].port = atoi(p2);
            j ++;
        }
    }
    addr_count = j;

    for(int i = 0; i < nThreadCnt; i++) {
        threadPara[i].api = update_api_create(addrs, addr_count);
        if(!threadPara[i].api) {
            TERR("update_api_create %d error.", i);
            return -1;
        }
    }

    return 0;
}

void usage(const char *pService)
{               
    fprintf(stderr, "Usage: %s -t ip:port -l pusher_log.cfg -k {add|del} -i input\n", pService);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -t ip:port        : dispatcherip地址和端口\n");
    fprintf(stderr, "  -l pusher_log.cfg : alog配置文件\n");
    fprintf(stderr, "  -k add|del        : 操作命令\n");
    fprintf(stderr, "  -n thread_count   : 发送线程数，默认是6\n");
    fprintf(stderr, "  -i input          : 输入的文件目录，也可以是单个文件，要求utf-8格式\n");
}

int main(int argc, char **argv)
{
    const static int MAX_ACTION_LEN = 10;
    int c = 0;
    int nThreadCnt = 6;
    char pSpec[MAX_SPEC_SIZE] = {0};
    char pAction[MAX_ACTION_LEN] = {0};
    char pLog[PATH_MAX] = {0};
    char pInput[PATH_MAX] = {0};

    while ((c = getopt(argc, argv, "t:l:k:i:n:h")) != -1) {
        switch(c) {
            case 't': 
                snprintf(pSpec, MAX_SPEC_SIZE, "%s", optarg);
                break;
            case 'k':
                snprintf(pAction, MAX_ACTION_LEN, "%s", optarg);
                break;
            case 'l':
                snprintf(pLog, PATH_MAX, "%s", optarg);
                break;
            case 'i':
                snprintf(pInput, PATH_MAX, "%s", optarg);
                break;
            case 'n':
                nThreadCnt = atoi(optarg);
                break;
            case 'h':
            default:
                break;
        }
    }

    if (pSpec[0] == 0 || pAction[0] == 0 || pLog[0] == 0 || pInput[0] == 0 || nThreadCnt <= 0) {
        usage(argv[0]);
        return -1;
    }

    ThreadPara threadPara[nThreadCnt];
    if (init(pSpec, pLog, threadPara, nThreadCnt) < 0) {
        return -1;
    }
   
    UpdateAction nAction;
    if (strcmp(pAction, "add") == 0) {
        nAction = ACTION_ADD;
    }
    else if (strcmp(pAction, "del") == 0) {
        nAction = ACTION_DELETE;
    } 
    else {
        usage(argv[0]);
        return -1;
    }

    TaskQueue queue;
    util::Mutex lock;

    if (loadQueue(pInput, queue) != 0 || queue.size() == 0) {
        TERR("load doc fail, path=%s", pInput);
        for(int i = 0; i < nThreadCnt; i++) {
            update_api_destroy(threadPara[i].api);
        }
        alog::Logger::shutdown();
        return -1;
    }

    for (int i = 0; i < nThreadCnt; i++) {
        threadPara[i].pQueue = &queue;
        threadPara[i].lock = &lock;
        threadPara[i].nAction = nAction;
        if (0 != pthread_create(&threadPara[i].nThrId, NULL, pushMessage, &threadPara[i])) {
            TERR("thread_create() failed. id=%d", i);
            for(int i = 0; i < nThreadCnt; i++) {
                update_api_destroy(threadPara[i].api);
            }
            alog::Logger::shutdown();
            return -1;
        }
    }
    for (int i = 0; i < nThreadCnt; i++) {
        if (0 != pthread_join(threadPara[i].nThrId, NULL)) {
            TERR("thread_join() failed. id=%d", i);
        }
        update_api_destroy(threadPara[i].api);
    }

    alog::Logger::shutdown();

    return 0;
}

