#include "CustomStatistic.h"
#include "string.h"

namespace statistic {

    CustomStatistic::CustomStatistic(const FRAMEWORK::Context *pContext,
                                     index_lib::ProfileDocAccessor *pProfileAccessor,
                                     StatisticConfig *pStatConf, 
                                     StatisticParameter *pStatParam)
        : CountStatistic(pContext, pProfileAccessor, pStatConf, pStatParam, NULL)
    { }

    CustomStatistic::~CustomStatistic()
    {
    //   _statMap.clear(); 
    }


    int32_t CustomStatistic::doStatistic(SearchResult *pSearchResult,
                                         StatisticResultItem *pStatResItem)
    {
        char * szCusValue = _pStatParam->szCusValue;
        int32_t nCusValue = 0;
        
        if((nCusValue = processCusValue(szCusValue)) <= 0)
        {
           TWARN("处理统计指定值失败：未指定CusValue或者格式不正确");
           return KS_EINVAL;
        }

        MemPool *pMemPool = _pContext->getMemPool();
        uint32_t nDocFound = pSearchResult->nDocFound;  // nDocIds数组中有效docid个数
        uint32_t nEstDocFound = pSearchResult->nEstimateDocFound;  // 索引截断后预估全部docFound数 
        uint32_t nReadCount = 0; // 进行统计的doc数量(抽样即返回抽样个数)
        int32_t nRet = readAttributeInfo(pSearchResult);
        
        if(unlikely(nRet <= 0 || (nReadCount = static_cast<uint32_t>(nRet)) > nDocFound))
        {
            return KS_EINVAL;
        }

        assert(pStatResItem != NULL);
        pStatResItem->nCount = _statMap.size();
        StatisticHeader *pStatHeader = pStatResItem->pStatHeader;
        assert(pStatHeader != NULL);
        // Custom统计一般不带count参数，因此将count参数改写成CusValue的个数，便于后续合并
        pStatHeader->nCount = _pStatParam->nCount > 0 ? _pStatParam->nCount : nCusValue;

        if(pStatResItem->nCount)
        {
            pStatResItem->ppStatItems = NEW_VEC(pMemPool, StatisticItem*, pStatResItem->nCount);
            if(unlikely(pStatResItem->ppStatItems == NULL))
            {
                TWARN("构建统计参数子句结果中的统计项实例失败");
                pStatResItem->nCount = 0;
                return KS_ENOMEM;
            }
            // 按抽样比例还原count值
            double dSampleRate = (nReadCount < nDocFound) ? (double)nDocFound / nReadCount : 1.0;
            // 按索引截断比例还原count值
            double dCutOffRate = (nDocFound < nEstDocFound) ? (double)nEstDocFound / nDocFound : 1.0;
            int32_t i = 0;
            StatIterator statItor;
            for(statItor = _statMap.begin(); statItor != _statMap.end(); statItor++)
            {
                statItor->value->nCount = (int32_t)(statItor->value->nCount * dSampleRate);
                if(unlikely(static_cast<uint32_t>(statItor->value->nCount) > nDocFound))
                {
                    statItor->value->nCount = static_cast<int32_t>(nDocFound);
                }

                statItor->value->nCount = (int32_t)(statItor->value->nCount * dCutOffRate);
                if(unlikely(static_cast<uint32_t>(statItor->value->nCount) > nEstDocFound))
                {
                    statItor->value->nCount = static_cast<int32_t>(nEstDocFound);
                }
                pStatResItem->ppStatItems[i] = statItor->value;
                ++i;
            }
        }

        return KS_SUCCESS;
    }

    int32_t CustomStatistic::process(uint32_t nDocId,
                                     StatisticResultItem *pStatResItem)
    {
        return 0;
    }

    /**
     * szCusValue内部以'|'分隔，
     * 形式为：cusvalue=50038469|50044400|50042824|50040635
     **/
    int32_t CustomStatistic::processCusValue(char *szCusValue)
    {
        int32_t nCusValue = 0;
        int32_t nCusItemLen = 0;
        int32_t nLen = strlen(szCusValue);
        char szCusItem[MAX_CUSVALUE_LENGTH];

        const char *szFieldName = _pStatParam->szFieldName;
        const index_lib::ProfileField *pProfileField = _pProfileAccessor->getProfileField(szFieldName);
        assert(pProfileField != NULL); // 参数解析时已经做过判断
        PF_DATA_TYPE eType = _pProfileAccessor->getFieldDataType(pProfileField);

        for(int32_t i = 0; i < nLen; ++i)
        {
            if(szCusValue[i] == '|')
            {
                if(nCusItemLen > 0)
                {
                    szCusItem[nCusItemLen] = 0;
                    if(insertValue(szCusItem, eType) == 0)
                    {
                        nCusItemLen = 0;
                        nCusValue++;
                    }
                } 
            }
            else
            {
                szCusItem[nCusItemLen] = szCusValue[i];
                nCusItemLen++; 
            }
        }
        if(nCusItemLen > 0)
        {
            szCusItem[nCusItemLen] = 0;
            if(insertValue(szCusItem, eType) == 0)
            {
                nCusItemLen = 0;
                nCusValue++;
            }
        }
        return nCusValue;
    }

    int32_t CustomStatistic::insertValue(char * szCusItem, PF_DATA_TYPE eType)
    {
        if(unlikely(szCusItem == NULL || szCusItem[0] == '\0'))
        {
            return -1;
        }

        MemPool *pMemPool = _pContext->getMemPool();
        StatisticItem *pStatItem= NEW(pMemPool, StatisticItem);
        if(unlikely(pStatItem == NULL))
        {
            TWARN("构建统计参数子句结果中的统计项实例失败");
            return KS_EINVAL; 
        }

        StatisticKey statKey;
        statKey.eFieldType = eType;
        switch(eType)
        {
            case DT_INT8:
            case DT_INT16:
            case DT_INT32:
            case DT_INT64:
            case DT_KV32:
                statKey.uFieldValue.lvalue = atoll(szCusItem);
                break;
            case DT_UINT8:
            case DT_UINT16:
            case DT_UINT32:
            case DT_UINT64:
                statKey.uFieldValue.ulvalue = (uint64_t)atoll(szCusItem);
                break;
            case DT_FLOAT:
                statKey.uFieldValue.fvalue = atof(szCusItem);
                break;
            case DT_DOUBLE:
                statKey.uFieldValue.dvalue = atof(szCusItem);
                break;
            case DT_STRING:
                statKey.uFieldValue.svalue = UTIL::replicate(szCusItem, pMemPool, strlen(szCusItem));
                break;
            default:
                break;
        }

        pStatItem->statKey = statKey;
        pStatItem->nCount = 0;
        _statMap.insert(pStatItem->statKey, pStatItem);

        return KS_SUCCESS;
    }

    int32_t CustomStatistic::insertStatMap(StatisticKey &statKey)
    {
        StatIterator statItor;
        if((statItor = _statMap.find(statKey)) != _statMap.end())
        {
            statItor->value->nCount++;
        }

        return KS_SUCCESS;
    }

}
