#include "StatisticInterface.h"
#include "StatisticFactory.h"
#include "StatisticDef.h"
#include "StatisticParser.h"

#include "statistic/StatisticManager.h"
#include "queryparser/Param.h"
#include "commdef/commdef.h"
#include "index_lib/ProfileManager.h"
#include "util/PriorityQueue.h"
#include "util/StringUtility.h"

#include <string.h>

namespace statistic {
 
    typedef UTIL::PriorityQueue<StatisticItem*, ItemLess> StatHeap;

    StatisticManager::StatisticManager()
       : _pStatConf(NULL), _pProfileAccessor(NULL)
    { 
    }

    StatisticManager::~StatisticManager()
    { 
        if(_pStatConf)
        {
            delete _pStatConf;
            _pStatConf = NULL;
        }
        _pProfileAccessor = NULL;
    }

    int32_t StatisticManager::init(mxml_node_t *pRoot)
    {
        if(!pRoot)
        {
            return ENOENT;
        }
        mxml_node_t *pModulesNode = NULL;
        mxml_node_t *pStatNode = NULL;

        const char *szAttr = NULL;
        int32_t nStatisticNum = -1; // nStatisticNum初始化为-1，因为0代表精确统计
        int32_t nSampleStatisticNum = 0;
        int32_t nSampleStatisticMaxCount = 0;
        int32_t nSampleStatisticRate = 0;

        pModulesNode = mxmlFindElement(pRoot, pRoot, KS_XML_ROOT_NODE_NAME,
                                       NULL, NULL, MXML_DESCEND);
        pStatNode = mxmlFindElement(pModulesNode, pRoot, KS_MODULE_NAME_STATISTIC,
                                    NULL, NULL, MXML_DESCEND);
        szAttr = mxmlElementGetAttr(pStatNode, "statistic_limit");
        if(szAttr != NULL && szAttr[0] != '\0')
        {
            nStatisticNum = atoi(szAttr);
            if(nStatisticNum < 0)
            {
                nStatisticNum = 0;
            }
        }
        szAttr = mxmlElementGetAttr(pStatNode, "sample_statistic_limit");
        if(szAttr != NULL && szAttr[0] != '\0')
        {
            nSampleStatisticNum = atoi(szAttr);
            if(nSampleStatisticNum < 0)
            {
                nSampleStatisticNum = 0;
            }
        }

        if(nSampleStatisticNum > 0)
        {
            szAttr = mxmlElementGetAttr(pStatNode, "sample_statistic_max_count");
            if(szAttr != NULL && szAttr[0] != '\0')
            {
                nSampleStatisticMaxCount = atoi(szAttr);
                if(nSampleStatisticMaxCount < 0)
                {
                    nSampleStatisticMaxCount = 0;
                }
            }
            szAttr = mxmlElementGetAttr(pStatNode, "sample_statistic_rate");
            if(szAttr != NULL && szAttr[0] != '\0')
            {
                nSampleStatisticRate = atoi(szAttr);
                if(nSampleStatisticRate < 0)
                {
                    nSampleStatisticRate = 0;
                }
            }
            if(nSampleStatisticMaxCount > 0 && nSampleStatisticRate > 0)
            {
                TERR("不能同时配置sample_statistic_max_count和sample_statistic_rate");
                return KS_EINVAL;
            }
        }   

        _pStatConf = new StatisticConfig();
        if(_pStatConf == NULL)
        {
           TERR("为统计配置信息申请内存空间失败");
           return KS_ENOMEM;
        }
        _pStatConf->nStatisticNum = nStatisticNum;
        _pStatConf->nSampleStatisticNum = nSampleStatisticNum;
        _pStatConf->nSampleStatisticRate = nSampleStatisticRate;
        _pStatConf->nSampleStatisticMaxCount = nSampleStatisticMaxCount;
        
        _pProfileAccessor = index_lib::ProfileManager::getDocAccessor();
        if(_pProfileAccessor == NULL)
        {
            TERR("获取Profile读取器单例失败");
            return KS_EINVAL;
        }

        return KS_SUCCESS;
    }

    int32_t StatisticManager::doStatisticOnSearcher(const FRAMEWORK::Context *pContext,
                                                    const queryparser::Param &param,
                                                    SearchResult *pSearchResult,
                                                    StatisticResult *pStatisticResult) const
    {
        // 统计结果对象需要调用端分配内存空间
        assert(pStatisticResult != NULL);
        const char *szStatisticClause = param.getParam("statistic");
        // query里面没有统计参数或者检索集合为空，直接返回
        if(szStatisticClause == NULL
           || (pSearchResult == NULL || pSearchResult->nDocFound == 0)) 
        {
            pStatisticResult->nCount = 0;
            pStatisticResult->ppStatResItems = NULL;
            return KS_SUCCESS;
        }

        int32_t ret;
        MemPool *pMemPool = pContext->getMemPool();
        char *szStatClause = UTIL::replicate(szStatisticClause, pMemPool,
                                             strlen(szStatisticClause));
        StatisticParser *pStatParser = (StatisticParser *)
                                        NEW(pMemPool, StatisticParser)(_pProfileAccessor);
        if(unlikely(pStatParser == NULL))
        {
            TWARN("构建统计解析器实例失败");
            return KS_ENOMEM;
        }
        ret = pStatParser->doParse(pMemPool, szStatClause);
        if(unlikely(ret < 0))
        {
            TWARN("统计参数解析失败，请按错误提示信息检查"); 
            pStatisticResult->nCount = 0;
            pStatisticResult->ppStatResItems = NULL;
            return KS_EINVAL;
        }
        int32_t nStatNum = pStatParser->getStatNum();
        pStatisticResult->nCount = nStatNum;
        pStatisticResult->ppStatResItems = NEW_VEC(pMemPool,
                                                   StatisticResultItem*, nStatNum);
        if(unlikely(pStatisticResult->ppStatResItems == NULL))
        {
            TWARN("构建统计参数子句结果对象实例失败"); 
            pStatisticResult->nCount = 0;
            return KS_ENOMEM;
        }
        StatisticInterface *pStatistic = NULL;
        StatisticResultItem *pStatResItem = NULL;
        StatisticHeader *pStatHeader = NULL;
        StatisticParameter *pStatParam = NULL;

        for(int32_t i = 0; i < nStatNum; i++)
        {
            if(unlikely((pStatParam = pStatParser->getStatParam(i)) == NULL))
            {
                TWARN("获取统计参数子句信息失败");
                continue;
            }
            pStatistic = StatisticFactory::create(pContext, _pProfileAccessor,
                                                  _pStatConf, pStatParam, param);
            if(pStatistic)
            {
                pStatResItem = NEW(pMemPool, StatisticResultItem);
                if(unlikely(pStatResItem == NULL))
                {
                    TWARN("构建统计参数子句结果对象实例失败");
                    return KS_ENOMEM;
                }
                pStatResItem->nCount = 0;
                
                pStatHeader = NEW(pMemPool, StatisticHeader);
                if(unlikely(pStatHeader == NULL))
                {
                    TWARN("构建统计参数子句结果头信息对象失败");
                    return KS_ENOMEM;
                }
                pStatHeader->nCount = pStatParam->nCount;
                pStatHeader->szFieldName = pStatParam->szFieldName;
                pStatHeader->szSumFieldName = pStatParam->szSumFieldName;
                pStatResItem->pStatHeader = pStatHeader;

                ret = pStatistic->doStatistic(pSearchResult, pStatResItem);
                if(unlikely(ret != KS_SUCCESS))
                {
                    pStatResItem->nCount = 0;
                }
                pStatisticResult->ppStatResItems[i] = pStatResItem;
            }
        }

        return KS_SUCCESS;
    }

    int32_t StatisticManager::doStatisticOnMerger(const FRAMEWORK::Context *pContext,
                                                  commdef::ClusterResult **ppClusterResults,
                                                  int32_t nReqSize,
                                                  StatisticResult *pMergeStatResult) const
    {   
        // 统计结果对象需要调用端分配内存空间
        assert(pMergeStatResult != NULL);
 
        // searcher上的检索集合为空，直接返回
        if(unlikely(ppClusterResults == NULL))
        {
            pMergeStatResult->nCount = 0;
            pMergeStatResult->ppStatResItems = NULL;
            return KS_SUCCESS;
        }

        MemPool *pMemPool = pContext->getMemPool();
        int32_t nStatNum = 0;
        StatisticResult *pSearchStatResult = NULL;
        StatisticHeader *pStatHeader = NULL;
        StatisticResultItem *pStatResItem = NULL, *pMergeStatResItem = NULL;
        StatisticItem *pStatItem = NULL, *pMergeStatItem = NULL;
        int32_t nItemNum = 0;   // 统计项个数
        int32_t nTopN = 0;      // 要求返回的结果数

        for(int32_t i = 0; i < nReqSize; ++i)
        {
            if(ppClusterResults[i] != NULL)
            {
                pSearchStatResult = ppClusterResults[i]->_pStatisticResult;
                if(pSearchStatResult && nStatNum < pSearchStatResult->nCount)
                {
                    nStatNum = pSearchStatResult->nCount;
                }
            }
        }

        // query里面没有统计参数，或者searcher上的检索集合为空，直接返回
        if(unlikely(nStatNum == 0))
        {
            pMergeStatResult->nCount = 0;
            pMergeStatResult->ppStatResItems = NULL;
            return KS_SUCCESS;
        }
        
        pMergeStatResult->nCount = nStatNum;
        pMergeStatResult->ppStatResItems = NEW_VEC(pMemPool,
                                                   StatisticResultItem*, nStatNum);
        if(unlikely(pMergeStatResult->ppStatResItems == NULL))
        {
            TWARN("构建统计参数子句结果对象实例失败"); 
            pMergeStatResult->nCount = 0;
            return KS_ENOMEM;
        }

        for(int32_t i = 0; i < nStatNum; ++i)
        {
            StatMap statMergeMap;
            StatIterator statItor;

            for(int32_t j = 0; j < nReqSize; ++j)
            {
                if(unlikely(ppClusterResults[j] == NULL))
                {
                    continue;
                }
                pSearchStatResult = ppClusterResults[j]->_pStatisticResult;
                if(unlikely(pSearchStatResult == NULL || pSearchStatResult->nCount == 0))
                {
                    continue;
                }
                pStatResItem = pSearchStatResult->ppStatResItems[i];
                nItemNum = pStatResItem->nCount;

                for(int32_t k = 0; k < nItemNum; ++k)
                {
                    pStatItem = pStatResItem->ppStatItems[k];
                    if(unlikely(pStatItem == NULL))
                    {
                        continue;
                    }
                    if((statItor=statMergeMap.find(pStatItem->statKey)) != statMergeMap.end())
                    {
                        statItor->value->nCount += pStatItem->nCount;
               //       statItor->value->sumCount += pStatItem->sumCount;
                    }
                    else
                    {
                        statMergeMap.insert(pStatItem->statKey, pStatItem);
                    }
                }
            }

            StatHeap statHeap;
            StatisticItem *pMinItem = NULL;
            StatisticItem *pTopItem = NULL;

            // 将最后一列searcher的统计头信息中的nCount作为merger后的TopN
            pStatHeader = pStatResItem->pStatHeader;
            nTopN = pStatHeader->nCount;
            if(nTopN > statMergeMap.size() || nTopN <= 0)
            {
                nTopN = statMergeMap.size();
            }
            statHeap.initialize(nTopN);

            for(statItor = statMergeMap.begin(); statItor != statMergeMap.end(); statItor++)
            {
                if(statHeap.size() < nTopN || statItor->value->nCount == pMinItem->nCount)
                {
                    statHeap.insert(statItor->value);
                    pMinItem = statHeap.top();
                }
                else if(statItor->value->nCount > pMinItem->nCount)
                {
                    pTopItem = statHeap.pop();
                    statHeap.insert(statItor->value);
                    pMinItem = statHeap.top();
                }
            }

            pMergeStatResItem = NEW(pMemPool, StatisticResultItem);
            if(unlikely(pMergeStatResItem == NULL))
            {
                TWARN("构建统计参数子句结果对象实例失败"); 
                pMergeStatResult->ppStatResItems[i] = NULL;
                continue;
            }
            pMergeStatResItem->nCount = statHeap.size();
            // 将最后一列searcher的统计子结果头信息作为merger后的统计子结果头信息
            pMergeStatResItem->pStatHeader = pStatHeader;

            pMergeStatResItem->ppStatItems = NEW_VEC(pMemPool, StatisticItem*,
                                                     pMergeStatResItem->nCount);
            if(unlikely(pMergeStatResItem->ppStatItems == NULL))
            {
                TWARN("构建统计参数子句结果中的统计项实例失败");
                pMergeStatResItem->nCount = 0;
                continue;
            }
            for(int32_t n = statHeap.size(); n > 0; --n)
            {
                pMergeStatItem = statHeap.pop();
                pMergeStatResItem->ppStatItems[n-1] = pMergeStatItem;
            }
            pMergeStatResult->ppStatResItems[i] = pMergeStatResItem;
        }

        return KS_SUCCESS;
    }
    
}
