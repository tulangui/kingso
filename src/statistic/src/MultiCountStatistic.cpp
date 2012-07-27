#include "util/StringUtility.h"
#include "MultiCountStatistic.h"
#include "string.h"

namespace statistic {

    MultiCountStatistic::MultiCountStatistic(const FRAMEWORK::Context *pContext,
                                     index_lib::ProfileDocAccessor *pProfileAccessor,
                                     StatisticConfig *pStatConf, 
                                     StatisticParameter *pStatParam)
        : CountStatistic(pContext, pProfileAccessor, pStatConf, pStatParam, NULL)
    { }

    MultiCountStatistic::~MultiCountStatistic()
    {
    //   _statMap.clear(); 
    }


    int32_t MultiCountStatistic::doStatistic(SearchResult *pSearchResult,
                                             StatisticResultItem *pStatResItem)
    {
        char *szFirstName = _pStatParam->szFirstFieldName;
        char *szFirstValue = _pStatParam->szFirstValue;
        char *szSecondName = _pStatParam->szSecondFieldName;
        char *szSecondValue = _pStatParam->szSecondValue;

        RestrictFieldInfo firstInfo, secondInfo;

        if(unlikely(szFirstValue == NULL || szFirstValue[0] == '\0'))
        {
           TWARN("多维统计中未指定限定字段field2或取值");
           return KS_EINVAL;
        }
        if((prepareRestrictInfo(szFirstName, szFirstValue, firstInfo)) != KS_SUCCESS)
        {
            TWARN("处理多维统计第一限定字段值失败: 未指定或者格式不正确");
            return KS_EINVAL;
        }
        _restrictInfos.pushBack(&firstInfo);
        if(unlikely(szSecondValue != NULL && szSecondValue[0] != '\0'))
        {
            if(prepareRestrictInfo(szSecondName, szSecondValue, secondInfo) != KS_SUCCESS)
            {
                TWARN("处理多维统计第二限定字段值失败: 未指定或者格式不正确");
                return KS_EINVAL;
            }
            _restrictInfos.pushBack(&secondInfo);
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
        // 如果统计子句中未指定count值，则返回全部结果，兼容isearch
        pStatHeader->nCount = _pStatParam->nCount > 0 ? _pStatParam->nCount
                                                      : pStatResItem->nCount;

        if(pStatResItem->nCount)
        {
            pStatResItem->ppStatItems = NEW_VEC(pMemPool, StatisticItem*,
                                                pStatResItem->nCount);
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

    int32_t MultiCountStatistic::process(uint32_t nDocId,
                                         StatisticResultItem *pStatResItem)
    {
        return 0;
    }

    int32_t MultiCountStatistic::prepareRestrictInfo(char* szFieldName,
                                                     char *szFieldValue,
                                                     RestrictFieldInfo &restrictInfo)
    {
        MemPool *pMemPool = _pContext->getMemPool();

        const index_lib::ProfileField *pProfileField = _pProfileAccessor->getProfileField(szFieldName);
        assert(pProfileField != NULL); // 参数解析时已经做过判断
        PF_DATA_TYPE eType = _pProfileAccessor->getFieldDataType(pProfileField);
        int32_t nMultiValNum = _pProfileAccessor->getFieldMultiValNum(pProfileField);

        StatMap *pRestrictMap = (StatMap *)NEW(pMemPool, StatMap);
        if(unlikely(pRestrictMap == NULL))
        {
            TWARN("构建保存多维统计限定字段值的HashMap对象失败");
            return KS_ENOMEM;
        }
        if(unlikely(processMultiValue(szFieldValue, eType, pRestrictMap) <= 0))
        {
            TWARN("处理多维统计指定限定字段值 %s 失败", szFieldValue);
            return KS_EINVAL;
        }

        restrictInfo.szFieldName = szFieldName;
        restrictInfo.szRestrictValue = szFieldValue;
        restrictInfo.eType = eType;
        restrictInfo.pProfileField = pProfileField;
        restrictInfo.nMultiValNum = nMultiValNum;
        restrictInfo.pRestrictMap= pRestrictMap;

        return KS_SUCCESS;
    }

    bool MultiCountStatistic::readRestrictAttrInfo(uint32_t nDocId,
                                                   RestrictFieldInfo *pRestrictFieldInfo,
                                                   RestrictValue *pRestrictValue)
    {
        int32_t ret;
        bool bFound = false;
        bool bVariableLen = false;
        const index_lib::ProfileField *pProfileField = pRestrictFieldInfo->pProfileField;
        PF_DATA_TYPE eType = pRestrictFieldInfo->eType;
        StatMap* pRestrictMap = pRestrictFieldInfo->pRestrictMap;
        int32_t nMultiValNum = pRestrictFieldInfo->nMultiValNum;
        if(nMultiValNum == 0)   // 变长profile字段
        {
            bVariableLen = true;
        }
        int32_t nValueCount = nMultiValNum; // 定长多值profile字段
        index_lib::ProfileMultiValueIterator profileItor; // 多值profile字段读取迭代器
        int32_t nMaxKeyCount = pRestrictMap->size();
        
        MemPool *pMemPool = _pContext->getMemPool();
        StatisticKey *pStatKeys = (StatisticKey *)NEW_VEC(pMemPool,
                                                          StatisticKey, nMaxKeyCount);
        if(unlikely(pStatKeys == NULL))
        {
            TWARN("构建多维统计限定字段值信息对象失败");
            return false;

        }
        StatisticKey statKey;
        int32_t nKeyCount = 0;

        if(nMultiValNum == 1)   // 单值字段
        {
            ret = getSingleAttrValue(nDocId, pProfileField, eType, statKey);
            if(ret == 0 && pRestrictMap->find(statKey) != pRestrictMap->end())
            {
                pStatKeys[nKeyCount] = statKey;
                nKeyCount++;
                bFound = true;
            }
        }
        else
        {
            _pProfileAccessor->getRepeatedValue(nDocId, pProfileField, profileItor);
            if(bVariableLen)
            {
                nValueCount = profileItor.getValueNum(); 
            }
            for(int32_t i = 0; i < nValueCount; ++i)
            {
                ret = getMultiAttrValue(profileItor, eType, statKey);
                if(ret == 0 && pRestrictMap->find(statKey) != pRestrictMap->end())
                {
                    pStatKeys[nKeyCount] = statKey;
                    nKeyCount++;
                    bFound = true;
                    if(nKeyCount >= nMaxKeyCount)
                    {
                        break;
                    }
                }
            }
        }
        pRestrictValue->pStatKeys = pStatKeys;
        pRestrictValue->nCount = nKeyCount;

        return bFound;
    }

    void MultiCountStatistic::transStatKey(StatisticKey &statKey)
    {
        MemPool *pMemPool = _pContext->getMemPool();
        PF_DATA_TYPE eType = statKey.eFieldType;
        char *buffer = NULL;

        switch(eType)
        {
            case DT_INT8:
            case DT_INT16:
            case DT_INT32:
            case DT_INT64:
            case DT_KV32:
                buffer = (char *) NEW_VEC(pMemPool, char, sizeof(int64_t) * 8 + 1);
                sprintf(buffer, "%lld", static_cast<long long int>(statKey.uFieldValue.lvalue));
                statKey.uFieldValue.svalue = buffer;
                break;
            case DT_UINT8:
            case DT_UINT16:
            case DT_UINT32:
            case DT_UINT64:
                buffer = (char *) NEW_VEC(pMemPool, char, sizeof(uint64_t) * 8 + 1);
                sprintf(buffer, "%llu", static_cast<long long unsigned int>(statKey.uFieldValue.ulvalue));
                statKey.uFieldValue.svalue = buffer;
                break;
            case DT_FLOAT:
                buffer = (char *) NEW_VEC(pMemPool, char, sizeof(float) * 8 + 1);
                sprintf(buffer, "%f", statKey.uFieldValue.fvalue);
                statKey.uFieldValue.svalue = buffer;
                break;
            case DT_DOUBLE:
                buffer = (char *) NEW_VEC(pMemPool, char, sizeof(double) * 8 + 1);
                sprintf(buffer, "%f", statKey.uFieldValue.dvalue);
                statKey.uFieldValue.svalue = buffer;
                break;
            case DT_STRING:
                break;
            default:
                break;
        }

        return;
    }

    int32_t MultiCountStatistic::readAttributeInfo(SearchResult *pSearchResult)
    {
        int32_t ret = 0;
        uint32_t nDocSize = pSearchResult->nDocSize;
        uint32_t *pDocIds = pSearchResult->nDocIds;
        int32_t nStatMaxNum = _pStatConf->nStatisticNum;
        int32_t nStep = getSampleStep(nDocSize);
        if(unlikely(_pStatParam->nExact == 1))
        {
            nStatMaxNum = 0;
            nStep = 1;
        }

        MemPool *pMemPool = _pContext->getMemPool();
        const char *szFieldName = _pStatParam->szFieldName;
        const index_lib::ProfileField *pProfileField = _pProfileAccessor->getProfileField(szFieldName);
        if(unlikely(pProfileField == NULL))
        {
            TWARN("统计参数中指定的字段 %s 无效，请检查", szFieldName);
            return KS_EINVAL;
        }
        PF_DATA_TYPE eType = _pProfileAccessor->getFieldDataType(pProfileField);
        bool bVariableLen = false;
        int32_t nMultiValNum = _pProfileAccessor->getFieldMultiValNum(pProfileField);
        if(nMultiValNum == 0)   // 变长profile字段
        {
            bVariableLen = true;
        }
        int32_t nValueCount = nMultiValNum; // 定长多值profile字段
        index_lib::ProfileMultiValueIterator profileItor; // 多值profile字段读取迭代器
        int32_t nReadCount = 0;
        int32_t nRestrictCount = _restrictInfos.size();
        bool bFound = false;
        StatisticKey statKey;

        RestrictValue **ppRestrictValue = (RestrictValue **)NEW_VEC(pMemPool, RestrictValue*,
                                                                    nRestrictCount);
        if(unlikely(ppRestrictValue == NULL))
        {
            TWARN("构建多维统计限定字段值信息对象失败");
            return KS_ENOMEM;
        }

        for(uint32_t i = 0; i < nDocSize; )
        {
            uint32_t nDocId = pDocIds[i];
            if(unlikely(nDocId == INVALID_DOCID))
            {
                ++i;
                continue; 
            }
            nReadCount++;

            for(int32_t k = 0; k < nRestrictCount; ++k)
            {
                RestrictFieldInfo *pRestrictFieldInfo = _restrictInfos[k];
                ppRestrictValue[k] = NEW(pMemPool, RestrictValue);
                if(unlikely(ppRestrictValue[k] == NULL))
                {
                    TWARN("构建多维统计限定字段值信息对象失败");
                    return KS_ENOMEM;
                }
                bFound = readRestrictAttrInfo(nDocId, pRestrictFieldInfo, ppRestrictValue[k]);
                if(!bFound) // 只要任意限定条件不满足，立即退出
                {
                    break;
                }
            }
            if(!bFound)
            {
                i += nStep;
                continue;
            }

            // 找到一个满足限定条件的doc, 开始进行处理
            if(unlikely(nMultiValNum == 1)) // 单值profile字段
            {
                ret = getSingleAttrValue(nDocId, pProfileField, eType, statKey);
                if(ret == 0)
                {
                    mergeStatKey(statKey, ppRestrictValue, nRestrictCount);
                }
            }
            else
            {
                _pProfileAccessor->getRepeatedValue(nDocId, pProfileField, profileItor);
                if(bVariableLen)    // 变长多值profile字段
                {
                    nValueCount = profileItor.getValueNum();
                }
                for(int32_t j = 0; j < nValueCount; ++j)
                {
                    ret = getMultiAttrValue(profileItor, eType, statKey);
                    if(ret == 0)
                    {
                        mergeStatKey(statKey, ppRestrictValue, nRestrictCount);
                    }
                }
            }
            if(nStatMaxNum > 0 && nReadCount > nStatMaxNum)
            {
                break;
            }
            i += nStep;
        }

        return nReadCount;
    }

    int32_t MultiCountStatistic::processMultiValue(char *szValue,
                                                   PF_DATA_TYPE eType,
                                                   StatMap *pRestrictMap)
    {
        int32_t nValueCount = 0;
        int32_t nItemLen = 0;
        int32_t nLen = strlen(szValue);
        char szItem[MAX_MULTIVALUE_LENGTH];

        for(int32_t i = 0; i < nLen; ++i)
        {
            if(szValue[i] == '|')
            {
                if(nItemLen > 0)
                {
                    szItem[nItemLen] = 0;
                    if(insertValue(szItem, eType, pRestrictMap) == 0)
                    {
                        nItemLen = 0;
                        nValueCount++;
                    }
                } 
            }
            else
            {
                szItem[nItemLen] = szValue[i];
                nItemLen++; 
            }
        }
        if(nItemLen > 0)
        {
            szItem[nItemLen] = 0;
            if(insertValue(szItem, eType, pRestrictMap) == 0)
            {
                nItemLen = 0;
                nValueCount++;
            }
        }

        return nValueCount;
    }

    int32_t MultiCountStatistic::insertValue(char * szItem,
                                             PF_DATA_TYPE eType,
                                             StatMap *pRestrictMap)
    {
        if(unlikely(szItem == NULL || szItem[0] == '\0'))
        {
            return -1;
        }

        MemPool *pMemPool = _pContext->getMemPool();
        StatisticKey statKey;
        statKey.eFieldType = eType;
        switch(eType)
        {
            case DT_INT8:
            case DT_INT16:
            case DT_INT32:
            case DT_INT64:
            case DT_KV32:
                statKey.uFieldValue.lvalue = atoll(szItem);
                break;
            case DT_UINT8:
            case DT_UINT16:
            case DT_UINT32:
            case DT_UINT64:
                statKey.uFieldValue.ulvalue = (uint64_t)atoll(szItem);
                break;
            case DT_FLOAT:
                statKey.uFieldValue.fvalue = atof(szItem);
                break;
            case DT_DOUBLE:
                statKey.uFieldValue.dvalue = atof(szItem);
                break;
            case DT_STRING:
                statKey.uFieldValue.svalue = UTIL::replicate(szItem, pMemPool,
                                                             strlen(szItem));
                break;
            default:
                break;
        }

        pRestrictMap->insert(statKey, NULL);

        return KS_SUCCESS;
    }

    int32_t MultiCountStatistic::mergeStatKey(StatisticKey &statKey,
                                              RestrictValue **ppRestrictValue,
                                              int32_t nRestrictCount)
    {
        MemPool *pMemPool = _pContext->getMemPool();
        char buffer[MAX_MULTIVALUE_LENGTH] = {0};
        int32_t nLen = 0;
        int32_t nFirstPos = 0;
        int32_t nSecondPos = 0;

        transStatKey(statKey);
        char *szValue = statKey.uFieldValue.svalue;
        strncpy(buffer, szValue, strlen(szValue));
        nLen +=  strlen(szValue);
        buffer[nLen] = '\0';
        nFirstPos = nLen;
        statKey.eFieldType = DT_STRING;
        
        for(int32_t i = 0; i < ppRestrictValue[0]->nCount; ++i)
        {
            StatisticKey firstKey = ppRestrictValue[0]->pStatKeys[i];

            transStatKey(firstKey);
            buffer[nFirstPos] = '\0';
            strcat(buffer, ";");
            strcat(buffer, firstKey.uFieldValue.svalue);
            nSecondPos = nFirstPos + strlen(firstKey.uFieldValue.svalue) + 1;

            if(nRestrictCount < 2)  // 只有一个限定字段
            {
                statKey.uFieldValue.svalue = UTIL::replicate(buffer, pMemPool,
                                                             strlen(buffer));
                insertStatMap(statKey);
            }
            else  // 有两个限定字段
            {
                for(int32_t j = 0; j < ppRestrictValue[1]->nCount; ++j)
                {
                    StatisticKey secondKey = ppRestrictValue[1]->pStatKeys[j];

                    transStatKey(secondKey);
                    buffer[nSecondPos] = '\0';
                    strcat(buffer, ";");
                    strcat(buffer, secondKey.uFieldValue.svalue);
                    
                    statKey.uFieldValue.svalue = UTIL::replicate(buffer, pMemPool, strlen(buffer));
                    insertStatMap(statKey);
                }
            }
        }

        return KS_SUCCESS;
    }

}
