#include "CountStatistic.h"
#include "util/StringUtility.h"

namespace statistic {

    CountStatistic::CountStatistic(const FRAMEWORK::Context *pContext,
                                   index_lib::ProfileDocAccessor *pProfileAccessor,
                                   StatisticConfig *pStatConf,
                                   StatisticParameter *pStatParam, const queryparser::Param *param)
        : _pContext(pContext), _pProfileAccessor(pProfileAccessor),
          _pStatConf(pStatConf), _pStatParam(pStatParam), _param(param)
    { }

    CountStatistic::~CountStatistic()
    {
       _statMap.clear(); 
    }

    int32_t CountStatistic::doStatistic(SearchResult *pSearchResult,
                                        StatisticResultItem *pStatResItem)
    {
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
        pStatHeader->nCount = _pStatParam->nCount > 0 ? _pStatParam->nCount : pStatResItem->nCount;

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

    int32_t CountStatistic::process(uint32_t nDocId,
                                    StatisticResultItem *pStatResItem)
    {
        return 0;
    }

    int32_t CountStatistic::getSampleStep(uint32_t nDocs)
    {
        int32_t nStatMaxCount = _pStatConf->nStatisticNum;
        int32_t nSampleStatNum = _pStatConf->nSampleStatisticNum;
        int32_t nSampleStatRate = _pStatConf->nSampleStatisticRate;
        int32_t nSampleStatMaxCount = _pStatConf->nSampleStatisticMaxCount;

        int32_t nStep = 1;

        if(unlikely(nStatMaxCount == 0))
        {
            return nStep;
        }

        if(nSampleStatNum > 0 && nDocs >= static_cast<uint32_t>(nSampleStatNum))
        {
            if(nSampleStatMaxCount > 0)
            {
                nStep = (nDocs+nSampleStatMaxCount-1) / nSampleStatMaxCount;
            }
            else if(nSampleStatRate > 0)
            {
                nStep = nSampleStatRate;
            }
        }

        return nStep;
    }

    int32_t CountStatistic::readAttributeInfo(SearchResult *pSearchResult)
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

        const char *szFieldName = _pStatParam->szFieldName;
        const index_lib::ProfileField *pProfileField = 
                                      _pProfileAccessor->getProfileField(szFieldName);
        assert(pProfileField != NULL); // 参数解析时已经做过判断
        PF_DATA_TYPE eType = _pProfileAccessor->getFieldDataType(pProfileField);
        //PF_FIELD_FLAG field_flag = _pProfileAccessor->getFieldFlag(pProfileField);
        bool bVariableLen = false;
        int32_t nMultiValNum = _pProfileAccessor->getFieldMultiValNum(pProfileField);
        if(nMultiValNum == 0)   // 变长profile字段
        {
            bVariableLen = true;
        }
        int32_t nValueCount = nMultiValNum; // 定长多值profile字段
        index_lib::ProfileMultiValueIterator profileItor; // 多值profile字段读取迭代器
        int32_t nReadCount = 0;
        StatisticKey statKey;

        for(uint32_t i = 0; i < nDocSize; )
        {
            uint32_t nDocId = pDocIds[i];
            if(unlikely(nDocId == INVALID_DOCID))
            {
                ++i;
                continue; 
            }
            nReadCount++;

            if(unlikely(nMultiValNum == 1)) // 单值profile字段
            {
                ret = getSingleAttrValue(nDocId, pProfileField, eType, statKey);
                if(ret == 0)
                {
                    insertStatMap(statKey);
                }
            }
            else
            {
                // 设置profile多值字段读取迭代器
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
                        insertStatMap(statKey);
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

    int32_t CountStatistic::getSingleAttrValue(uint32_t nDocId,
                                               const index_lib::ProfileField *pProfileField,
                                               PF_DATA_TYPE eType, StatisticKey &statKey)
    {
        MemPool *pMemPool = _pContext->getMemPool();
        statKey.eFieldType = eType; 
        switch(eType)
        {
            case DT_INT8:
                statKey.uFieldValue.lvalue = _pProfileAccessor->getInt8(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.lvalue == INVALID_INT8))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_INT16:
                statKey.uFieldValue.lvalue = _pProfileAccessor->getInt16(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.lvalue == INVALID_INT16))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_INT32:
                statKey.uFieldValue.lvalue = _pProfileAccessor->getInt32(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.lvalue == INVALID_INT32))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_INT64:
                statKey.uFieldValue.lvalue = _pProfileAccessor->getInt64(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.lvalue == INVALID_INT64))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_UINT8:
                statKey.uFieldValue.ulvalue = _pProfileAccessor->getUInt8(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.ulvalue == INVALID_UINT8))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_UINT16:
                statKey.uFieldValue.ulvalue = _pProfileAccessor->getUInt16(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.ulvalue == INVALID_UINT16))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_UINT32:
                statKey.uFieldValue.ulvalue = _pProfileAccessor->getUInt32(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.ulvalue == INVALID_UINT32))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_UINT64:
                statKey.uFieldValue.ulvalue = _pProfileAccessor->getUInt64(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.ulvalue == INVALID_UINT64))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_FLOAT:
                statKey.uFieldValue.fvalue = _pProfileAccessor->getFloat(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.fvalue == INVALID_FLOAT))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_DOUBLE:
                statKey.uFieldValue.dvalue = _pProfileAccessor->getDouble(nDocId, pProfileField);
                if(unlikely(statKey.uFieldValue.dvalue == INVALID_DOUBLE))
                {
                    return KS_EINVAL;
                }
                break;
            case DT_STRING:
                {
                    const char *szValue = _pProfileAccessor->getString(nDocId, pProfileField);
                    if(unlikely(szValue == NULL))
                    {
                        return KS_EINVAL;
                    }
                    statKey.uFieldValue.svalue = UTIL::replicate(szValue,
                                                                 pMemPool, strlen(szValue));
                    break;
                }
            case DT_KV32:   // kv32类型暂时没有单值，空值暂未设定，先用INVALID_INT64占坑
                statKey.uFieldValue.lvalue = index_lib::ConvFromKV32(_pProfileAccessor->getKV32(nDocId, pProfileField));
                if(unlikely(statKey.uFieldValue.lvalue == INVALID_INT64))
                {
                    return KS_EINVAL;
                }
                break;
            default:
                break;
        }

        return KS_SUCCESS;
    }

    int32_t CountStatistic::getMultiAttrValue(index_lib::ProfileMultiValueIterator &profileItor,
                                              PF_DATA_TYPE eType,
                                              StatisticKey &statKey)
    {
        MemPool *pMemPool = _pContext->getMemPool();
        statKey.eFieldType = eType; 
        switch(eType)
        {
            case DT_INT8:
                statKey.uFieldValue.lvalue = profileItor.nextInt8();
                break;
            case DT_INT16:
                statKey.uFieldValue.lvalue = profileItor.nextInt16();
                break;
            case DT_INT32:
                statKey.uFieldValue.lvalue = profileItor.nextInt32();
                break;
            case DT_INT64:
                statKey.uFieldValue.lvalue = profileItor.nextInt64();
                break;
            case DT_UINT8:
                statKey.uFieldValue.ulvalue = profileItor.nextUInt8();
                break;
            case DT_UINT16:
                statKey.uFieldValue.ulvalue = profileItor.nextUInt16();
                break;
            case DT_UINT32:
                statKey.uFieldValue.ulvalue = profileItor.nextUInt32();
                break;
            case DT_UINT64:
                statKey.uFieldValue.ulvalue = profileItor.nextUInt64();
                break;
            case DT_FLOAT:
                statKey.uFieldValue.fvalue = profileItor.nextFloat();
                break;
            case DT_DOUBLE:
                statKey.uFieldValue.dvalue = profileItor.nextDouble();
                break;
            case DT_STRING:
                {
                    const char *szValue = profileItor.nextString();
                    if(unlikely(szValue == NULL))
                    {
                        return KS_EINVAL;
                    }
                    statKey.uFieldValue.svalue = UTIL::replicate(szValue,
                                                                 pMemPool, strlen(szValue));
                    break;
                }
            case DT_KV32:
                statKey.uFieldValue.lvalue = index_lib::ConvFromKV32(profileItor.nextKV32());
                break;
            default:
                break;
        }

        return KS_SUCCESS;
    }

    int32_t CountStatistic::insertStatMap(StatisticKey &statKey)
    {
        StatIterator statItor;
        if((statItor = _statMap.find(statKey)) != _statMap.end())
        {
            statItor->value->nCount++;
        }
        else
        {
            StatisticItem *pStatItem = NEW(_pContext->getMemPool(), StatisticItem);
            if(unlikely(pStatItem == NULL))
            {
                TWARN("构建统计参数子句结果中的统计项实例失败");
                return KS_EINVAL; 
            }
            pStatItem->statKey = statKey;
            pStatItem->nCount = 1;

            _statMap.insert(statKey, pStatItem);
        }

        return KS_SUCCESS;
    }

}
