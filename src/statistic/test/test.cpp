#include "statistic/StatisticManager.h"
#include "commdef/commdef.h"
#include "commdef/SearchResult.h"
#include "framework/Context.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"
#include "mxml.h"
#include "util/MemPool.h"
#include "pool/MemMonitor.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <sys/time.h>

#define MAX_FILE_PATH    1024
#define MAX_VALUE_LENGTH 512000000   //512M
#define MAX_STATISTIC_CLAUSE_COUNT 20000
#define STATISTIC_CLAUSE_KEY "statistic="

static char szBuffer[MAX_VALUE_LENGTH];

static char szConfigFileName[MAX_FILE_PATH];
static char szDocIdsFileName[MAX_FILE_PATH];
static char szQueryFileName[MAX_FILE_PATH];
static char szProfileFileName[MAX_FILE_PATH];


void printResult(statistic::StatisticResult *pStatRes);
void processArgs(int argc, char * argv[]);
void usage(const char *szProgramName);
int64_t nowTime();

using namespace statistic;

int main(int argc, char * argv[]) 
{
    processArgs(argc, argv);

    FILE *fp;
    int i;
    int ret = 0;
    
    // 加载统计配置文件
    mxml_node_t *pXMLTree = NULL;
    if((fp = fopen(szConfigFileName, "r")) == NULL)
    {
        fprintf(stderr, "统计配置文件打开出错, 文件可能不存在.\n");
        return -1;
    }
    pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    if(pXMLTree == NULL) 
    {
        fprintf(stderr, "统计配置文件格式有错, 请根据错误提示修正您的配置文件.\n");
        return -1;
    }
    fclose(fp);

    // 读取统计参数集合
    char *ppStatClauses[MAX_STATISTIC_CLAUSE_COUNT] = {NULL};
    if((fp = fopen(szQueryFileName, "r")) == NULL)
    {
        fprintf(stderr, "统计参数集合文件打开出错, 文件可能不存在.\n");
        return -1;
    }
    i = 0;
    while(fgets(szBuffer, MAX_VALUE_LENGTH, fp))
    {
        size_t len = strlen(szBuffer);
        size_t stat_len = 0;
        if(szBuffer && szBuffer[len-1] == '\n')
        {
            szBuffer[len-1] = '\0';
            len--;
        }
        char * pBegin = strstr(szBuffer, STATISTIC_CLAUSE_KEY);
        if(pBegin != NULL && strlen(pBegin) > strlen(STATISTIC_CLAUSE_KEY))
        {
            pBegin += strlen(STATISTIC_CLAUSE_KEY);
            char * pEnd = strchr(pBegin, '&');

            if(pEnd != NULL)
            {
                stat_len = pEnd - pBegin;
            }
            else
                stat_len = strlen(pBegin);

            char *szStatClause = new char[stat_len+1];
            strncpy(szStatClause, pBegin, stat_len);
            szStatClause[stat_len] = '\0';
            
 //           printf("%s\n", szStatClause);
            ppStatClauses[i] = szStatClause;

        }
        ++i;
        if(i >= MAX_STATISTIC_CLAUSE_COUNT)
            break;

    }
    int nStatClause = i;
    fclose(fp);

    fprintf(stderr, "读取统计参数集合完毕\n");


    // 读取search&filter结果集合，即docid集合 
    SearchResult *ppSearchResults[MAX_STATISTIC_CLAUSE_COUNT] = {NULL};
    SearchResult *pSearchResult = NULL;
    if((fp = fopen(szDocIdsFileName, "r")) == NULL)
    {
        fprintf(stderr, "检索过滤集合文件打开出错, 文件可能不存在.\n");
        return -1;
    }
    i = 0;
    while(fgets(szBuffer, MAX_VALUE_LENGTH, fp))
    {
        size_t len = strlen(szBuffer);
        if(szBuffer && szBuffer[len-1] == '\n')
        {
            szBuffer[len-1] = '\0';
            len--;
        }
        if(len == MAX_VALUE_LENGTH)
        {
            fprintf(stderr, "检索过滤集合文件中的第 %d 行可能存在过多的docid.\n", i+1);
        }
        char *szDocids = szBuffer;
        if(szDocids == NULL || szDocids[0] == '\0')
        {
            fprintf(stderr, "检索过滤集合文件中的第 %d 行为空行.\n", i+1);
            continue;
        }

        
        uint32_t k = 0;
        int nDocSize = 0;
        bool isDocFound = true;
        bool bFinished = false;
        char *ptr = szDocids;

        do {
            switch(*ptr)
            {
                case ' ':
                    *ptr = '\0';
                    if(isDocFound)
                    {
                        nDocSize = atoi(szDocids);
                        if(nDocSize <= 0)
                        {
                            break;
                        }
                        pSearchResult = new SearchResult;
                        if(pSearchResult == NULL)
                        {
                            fprintf(stderr, "SearchResult申请内存出错.\n"); 
                            exit(-1);
                        }
                        pSearchResult->nDocSize = nDocSize;
                        pSearchResult->nDocFound = pSearchResult->nDocSize;
                        pSearchResult->nEstimateDocFound = pSearchResult->nDocFound;
                        pSearchResult->nDocIds = new uint32_t [pSearchResult->nDocSize];
                        isDocFound = false;
                    }
                    else 
                    {
                        pSearchResult->nDocIds[k] = atoi(szDocids);
                        ++k;
                    }
                    szDocids = ptr+1;
                    ptr++;
                    break;
                case '\0':
                    if(pSearchResult)
                    {
                        pSearchResult->nDocIds[k] = atoi(szDocids);
                        ++k;
                    }
                    bFinished = true;
                    break;
                default:
                    ptr++;
            }
        } while(!bFinished);
        if(pSearchResult != NULL && k != pSearchResult->nDocSize)
        {
            fprintf(stderr, "切分得到的docid个数和预先写入的不一致. %d!=%d\n", k, pSearchResult->nDocSize); 
        }
        ppSearchResults[i] = pSearchResult;
        pSearchResult = NULL;
        ++i;
        if(i >= MAX_STATISTIC_CLAUSE_COUNT)
            break;
    }
    int nDocIds = i;
    fclose(fp);

    if(nDocIds != nStatClause)
    {
        fprintf(stderr, "统计参数和docid集合务必是一一对应.\n");
        return -1;
    }

    fprintf(stderr, "读取检索过滤集合完毕\n");

    // 加载Profile文件
    index_lib::ProfileManager *pProfileManager = index_lib::ProfileManager::getInstance();
    if(pProfileManager)
    {
        pProfileManager->setProfilePath(szProfileFileName);
        if((ret = pProfileManager->load()) != 0) 
        {
            fprintf(stderr, "load profile failed, exit!\n");
            return -1;
        }
    }

    fprintf(stderr, "前期准备工作完毕，开始进行统计...\n");

    // 初始化完毕，准备开始统计
    FRAMEWORK::Context context;
    MemPool memPool;
    MemMonitor memMonitor(&memPool, 1024*1024*200);
    memPool.setMonitor(&memMonitor);
    memMonitor.enableException();
    context.setMemPool(&memPool);
    int64_t beginTime, endTime;

    // 统计管理类初始化
    statistic::StatisticManager statMgr;
    ret = statMgr.init(pXMLTree);
    if(ret < 0)
    {
        fprintf(stderr, "statistic manager init failed, exit!\n");
        return -1;
    }

    beginTime = nowTime();
    for(i = 0; i < nStatClause; ++i)
    {
        statistic::StatisticResult statRes;
        //const char *szStatisticClause = ppStatClauses[i];
        pSearchResult = ppSearchResults[i];

//        ret = statMgr.doStatisticOnSearcher(&context, szStatisticClause, pSearchResult, &statRes);
        if(unlikely(ret < 0))
        {
            fprintf(stderr, "doStatisticOnSearcher failed, exit!\n");
            return -1;
        }

        if(statRes.nCount > 0)
///*
        {
        //    printResult(&statRes);
            int nSerialSize = statRes.getSerialSize();
            if(nSerialSize == 0)
            {
                fprintf(stderr, "Statistic Result On Searcher is NULL, exit!\n");
                return -1;
            }
            char *buffer = NEW_VEC(context.getMemPool(), char, nSerialSize+1); 
            ret = statRes.serialize(buffer, nSerialSize);

            if(ret != nSerialSize)
            {
                fprintf(stderr, "Statistic Result On Searcher serialize failed, exit!\n");
                return -1;
            }

            statistic::StatisticResult deStatRes;
            deStatRes.deserialize(buffer, nSerialSize, context.getMemPool());
            
   //         printf("\n\nafter deserialize:\n");
   //         printResult(&deStatRes);
            
            int nReqSize = 1;
            commdef::ClusterResult **ppClusterResults =   
                (commdef::ClusterResult**)NEW_VEC(context.getMemPool(), commdef::ClusterResult*, nReqSize);   

            for(int j = 0; j < nReqSize; ++j)   
            {   
                ppClusterResults[j] = (commdef::ClusterResult*)NEW(context.getMemPool(), commdef::ClusterResult);   
                ppClusterResults[j]->_pStatisticResult = &deStatRes;   
            }   

            statistic::StatisticResult statMergeRes;   
            ret = statMgr.doStatisticOnMerger(&context, ppClusterResults, nReqSize, &statMergeRes);   
            if(ret < 0)   
            {   
                fprintf(stderr, "doStatisticOnMerger failed, exit!\n");   
                return -1;   
            }   

    //        printf("\nafter merge and sort:\n\n");   
    //        if(statMergeRes.nCount > 0)   
      //          printResult(&statMergeRes);  
        }
        //*/

        context.getMemPool()->reset();
    }
    endTime = nowTime();

    fprintf(stderr, "excuted query: %d, time cost: %f ms\n", nStatClause, (double)(endTime-beginTime)/1000);

    for(int i = 0; i < nStatClause; ++i)
    {
        if(ppSearchResults[i])
        {
            delete [] ppSearchResults[i]->nDocIds;
            delete ppSearchResults[i];
        }
        if(ppStatClauses[i])
        {
            delete ppStatClauses[i];
        }
    }

    index_lib::ProfileManager::freeInstance();
    printf("It's over!\n");
    return 0;
}

void usage(const char *szProgramName)
{
    fprintf(stderr, "用法: %s -c config_file -d docids_file -q query_file -p profile_file.\n", szProgramName);
    exit(1);
}

void processArgs(int argc, char * argv[])
{
    if(argc < 8)
        usage(argv[0]);

    int ch; 
    while((ch = getopt(argc, argv, "c:d:q:p:")) != -1)
    {
        switch(ch)
        {
            case 'c':
                strncpy(szConfigFileName, optarg, strlen(optarg));
                szConfigFileName[strlen(optarg)] = 0;
                break;
            case 'd':
                strncpy(szDocIdsFileName, optarg, strlen(optarg));
                szDocIdsFileName[strlen(optarg)] = 0;
                break;
            case 'q':
                strncpy(szQueryFileName, optarg, strlen(optarg));
                szQueryFileName[strlen(optarg)] = 0;
                break;
            case 'p':
                strncpy(szProfileFileName, optarg, strlen(optarg));
                szProfileFileName[strlen(optarg)] = 0;
                break;
            case '?':
            default:
                usage(argv[0]);
                break;
        }
    }
       
    if(strlen(szConfigFileName) <= 1)
    {
        usage(argv[0]);
    }

    return;
}

/* 得到当前时间, 以微秒为单位 */
int64_t nowTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000LL + t.tv_usec);
}

void printResult(statistic::StatisticResult *pStatRes)
{
    StatisticResultItem *pStatResItem = NULL;
    StatisticHeader *pStatHeader = NULL;
    StatisticItem *pStatItem = NULL;
    PF_DATA_TYPE eType;
    int nCount = pStatRes->nCount;
    StatisticResultItem ** ppStatResItems  = pStatRes->ppStatResItems;
    if(nCount > 0)
    {
        printf("statset count=%d\n", nCount);

        for(int i = 0; i < nCount; ++i)
        {
            pStatResItem = ppStatResItems[i];

            if(pStatResItem)
            {
                pStatHeader = pStatResItem->pStatHeader;
                int nItemCount = pStatResItem->nCount;

                printf("statele count=%d, key=%s\n", nItemCount, pStatHeader->szFieldName);

                if(nItemCount > 0)
                {
                    for(int j = 0; j < nItemCount; ++j)
                    {
                        pStatItem = pStatResItem->ppStatItems[j];
                        if(pStatItem)
                        {
                            eType = pStatItem->statKey.eFieldType;
                            if(eType == DT_INT8 || eType == DT_INT16
                                    || eType == DT_INT32 || eType == DT_INT64)
                            {
                                printf("statitem count=%d, statid=%lld\n",
                                        pStatItem->nCount, static_cast<long long int>(pStatItem->statKey.uFieldValue.lvalue));
                            }
                            else if(eType == DT_UINT8 || eType == DT_UINT16
                                    || eType == DT_UINT32 || eType == DT_UINT64)
                            {
                                printf("statitem count=%d, statid=%llu\n",
                                        pStatItem->nCount, static_cast<long long unsigned int>(pStatItem->statKey.uFieldValue.ulvalue));
                            }
                            else if(eType == DT_FLOAT)
                            {
                                printf("statitem count=%d, statid=%f\n",
                                        pStatItem->nCount, pStatItem->statKey.uFieldValue.fvalue);
                            }
                            else if(eType == DT_DOUBLE)
                            {
                                printf("statitem count=%d, statid=%f\n",
                                        pStatItem->nCount, pStatItem->statKey.uFieldValue.dvalue);
                            }
                            else // DT_STRING
                            {
                                printf("statitem count=%d, statid=%s\n",
                                        pStatItem->nCount, pStatItem->statKey.uFieldValue.svalue);
                            }
                        }
                    }
                }
            }
            printf("\n");
        }
    }

}

