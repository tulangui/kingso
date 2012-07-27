#include "statistic/StatisticResult.h"
#include "util/common.h"
#include <string.h>

namespace statistic {

    int32_t StatisticResult::getSerialSize()
    {
        int32_t nStatBufLen = 0;
        if(ppStatResItems != NULL && nCount > 0)
        {
            nStatBufLen += sizeof(int32_t);
            for(int32_t i = 0; i < nCount; ++i)
            {
                nStatBufLen += getSerialSize(ppStatResItems[i]);
            }
        }
        return nStatBufLen;
    }
    
    int32_t StatisticResult::getSerialSize(StatisticResultItem *pStatResItem)
    {
        int32_t nBufLen = 0;
        if(pStatResItem != NULL)
        {
            nBufLen += sizeof(int32_t);
            nBufLen += getSerialSize(pStatResItem->pStatHeader);
            for(int32_t i =0; i < pStatResItem->nCount; ++i)
            {
                nBufLen += getSerialSize(pStatResItem->ppStatItems[i]);
            }
        }
        return nBufLen;
    }

    int32_t StatisticResult::getSerialSize(StatisticHeader *pStatHeader)
    {
        int32_t nBufLen = 0;
        if(pStatHeader != NULL)
        {
            nBufLen += sizeof(int32_t);     // szFieldName的长度
            if(pStatHeader->szFieldName != NULL)
            {
                nBufLen += strlen(pStatHeader->szFieldName);
            }
            nBufLen += sizeof(int32_t);     // nCount
            nBufLen += sizeof(int32_t);     // szSumFieldName的长度
            if(unlikely(pStatHeader->szSumFieldName != NULL))
            {
                nBufLen += strlen(pStatHeader->szSumFieldName);
                nBufLen += sizeof(PF_DATA_TYPE);
            }
        }
        return nBufLen;
    }

    int32_t StatisticResult::getSerialSize(StatisticItem *pStatItem)
    {
        int32_t nBufLen = 0;
        if(pStatItem != NULL)
        {
            nBufLen += sizeof(PF_DATA_TYPE); // statKey的类型
            PF_DATA_TYPE eType = pStatItem->statKey.eFieldType;
            if(eType != DT_STRING)
            {
                nBufLen += sizeof(ValueType);
            }
            else
            {
                nBufLen += sizeof(int32_t); // 字段串类型Key的长度
                nBufLen += (pStatItem->statKey.uFieldValue.svalue ? \
                                strlen(pStatItem->statKey.uFieldValue.svalue) : 0);
            }
            nBufLen += sizeof(int32_t);     // nCount
            nBufLen += sizeof(ValueType);   // nSumCount
        }
        return nBufLen;
    }

    int32_t StatisticResult::serialize(char *buffer, int32_t size)
    {
        if(unlikely(buffer == NULL || size == 0))
        {
            return size + 1;
        }

        int32_t len = 0, cost = 0;
        const char *name = NULL;
        StatisticHeader *pStatHeader = NULL;
        StatisticResultItem *pStatResItem = NULL;
        StatisticItem *pStatItem = NULL;
        
        char *pAddr = buffer;
        cost += sizeof(int32_t);
        if(unlikely(cost > size))
        {
            return size + 1;
        }
        *(int32_t*)pAddr = nCount;
        pAddr += sizeof(int32_t);

        for(int32_t i = 0; i < nCount; ++i)
        {
            pStatResItem = ppStatResItems[i];
            if(unlikely(pStatResItem == NULL))
            {
                continue;
            }
            cost += sizeof(int32_t);
            if(unlikely(cost > size))
            {
                return size + 1;
            }
            *(int32_t*)pAddr = pStatResItem->nCount;
            pAddr += sizeof(int32_t);

            pStatHeader = pStatResItem->pStatHeader;
            if(unlikely(pStatHeader == NULL))
            {
                continue;
            }
            cost += getSerialSize(pStatHeader);
            if(unlikely(cost > size))
            {
                return size + 1;
            }
            name = pStatHeader->szFieldName;
            len = name ? strlen(name) : 0;
            *(int32_t*)pAddr = len;
            pAddr += sizeof(int32_t);
            if(len > 0)
            {
                memcpy(pAddr, name, len);
                pAddr += len;
            }

            *(int32_t*)pAddr = pStatHeader->nCount;
            pAddr += sizeof(int32_t);
            name = pStatHeader->szSumFieldName;
            len = name ? strlen(name) : 0;
            *(int32_t*)pAddr = len;
            pAddr += sizeof(int32_t);
            if(len > 0)
            {
                memcpy(pAddr, name, len);
                pAddr += len;
                *(PF_DATA_TYPE*)pAddr = pStatHeader->sumFieldType;
                pAddr += sizeof(PF_DATA_TYPE);
            }

            for(int32_t j = 0; j < pStatResItem->nCount; ++j)
            {
                pStatItem = pStatResItem->ppStatItems[j];
                if(unlikely(pStatItem == NULL))
                {
                    continue;
                }
                cost += getSerialSize(pStatItem);
                if(unlikely(cost > size))
                {
                    return size + 1;
                }
                
                PF_DATA_TYPE eType = pStatItem->statKey.eFieldType;

                *(PF_DATA_TYPE*)pAddr = eType;
                pAddr += sizeof(PF_DATA_TYPE);
                if(eType != DT_STRING)
                {
                    memcpy(pAddr, &pStatItem->statKey.uFieldValue, sizeof(ValueType));
                    pAddr += sizeof(ValueType);
                }
                else
                {
                    name = pStatItem->statKey.uFieldValue.svalue;
                    len = name ? strlen(name) : 0;
                    *(int32_t*)pAddr = len;
                    pAddr += sizeof(int32_t);
                    if(len > 0)
                    {
                        memcpy(pAddr, name, len);
                        pAddr += len;
                    }
                }
                *(int32_t*)pAddr = pStatItem->nCount;
                pAddr += sizeof(int32_t);
                memcpy(pAddr, &pStatItem->sumCount, sizeof(ValueType));
                pAddr += sizeof(ValueType);
            }
        }

        return cost;
    }

    int32_t StatisticResult::deserialize(char *buffer, int32_t size, MemPool *pMemPool)
    {
        if(unlikely(buffer == NULL || size < static_cast<int32_t>(sizeof(int32_t))))
        {
            return KS_EINVAL;
        }

        int32_t len = 0;
        char *name = NULL;
        StatisticHeader *pStatHeader = NULL;
        StatisticResultItem *pStatResItem = NULL;
        StatisticItem *pStatItem = NULL;

        char *pAddr = buffer;

        nCount = *(int32_t*)pAddr;
        pAddr += sizeof(int32_t);

        ppStatResItems = NEW_VEC(pMemPool, StatisticResultItem*, nCount);
        if(unlikely(ppStatResItems == NULL))
        {
            TWARN("构建统计参数子句结果对象实例失败");
            nCount = 0;
            return KS_ENOMEM;
        }

        for(int32_t i = 0; i < nCount; ++i)
        {
            pStatResItem = NEW(pMemPool, StatisticResultItem);
            if(unlikely(pStatResItem == NULL))
            {
                TWARN("构建统计参数子句结果对象实例失败");
                ppStatResItems = NULL;
                nCount = 0;
                return KS_ENOMEM;
            }
            pStatResItem->nCount = *(int32_t*)pAddr;
            pAddr += sizeof(int32_t);

            pStatHeader = NEW(pMemPool, StatisticHeader);
            if(unlikely(pStatHeader == NULL))
            {
                TWARN("构建统计参数子句结果头信息对象失败");
                ppStatResItems = NULL;
                nCount = 0;
                return KS_ENOMEM;

            }
            len = *(int32_t*)pAddr;
            pAddr += sizeof(int32_t);
            if(len > 0)
            {
                name = NEW_VEC(pMemPool, char, len + 1);
                if(unlikely(name == NULL))
                {
                    TWARN("构建统计参数子句结果头信息中FieldName失败");
                    ppStatResItems = NULL;
                    nCount = 0;
                    return KS_ENOMEM;
                }
                memcpy(name, pAddr, len);
                pAddr += len;
                name[len] = '\0'; 
                pStatHeader->szFieldName = name;
            }
            pStatHeader->nCount = *(int32_t*)pAddr;
            pAddr += sizeof(int32_t);
            
            len = *(int32_t*)pAddr;
            pAddr += sizeof(int32_t);
            if(unlikely(len > 0))
            {
                name = NEW_VEC(pMemPool, char, len + 1);
                if(unlikely(name == NULL))
                {
                    TWARN("构建统计参数子句结果头信息中SumFieldName失败");
                    ppStatResItems = NULL;
                    nCount = 0;
                    return KS_ENOMEM;
                }
                memcpy(name, pAddr, len);
                pAddr += len;
                name[len] = '\0'; 

                pStatHeader->szSumFieldName = name;
                pStatHeader->sumFieldType = *(PF_DATA_TYPE*)pAddr;
                pAddr += sizeof(PF_DATA_TYPE);
            }
            pStatResItem->pStatHeader = pStatHeader;

            if(pStatResItem->nCount > 0)
            {
                pStatResItem->ppStatItems = NEW_VEC(pMemPool,
                                                    StatisticItem*, pStatResItem->nCount);
                if(unlikely(pStatResItem->ppStatItems == NULL))
                {
                    TWARN("构建统计参数子句结果中的统计项实例失败");
                    ppStatResItems = NULL;
                    nCount = 0;
                    return KS_ENOMEM;
                }
                for(int32_t j = 0; j < pStatResItem->nCount; ++j)
                {
                    pStatItem = NEW(pMemPool, StatisticItem);
                    if(unlikely(pStatItem == NULL))
                    {
                        TWARN("构建统计参数子句结果中的统计项实例失败");
                        ppStatResItems = NULL;
                        nCount = 0;
                        return KS_ENOMEM; 
                    }
                    PF_DATA_TYPE eType = *(PF_DATA_TYPE*)pAddr;
                    pAddr += sizeof(PF_DATA_TYPE);
                    pStatItem->statKey.eFieldType = eType;
                    if(eType != DT_STRING)
                    {
                        memcpy(&pStatItem->statKey.uFieldValue, pAddr, sizeof(ValueType));
                        pAddr += sizeof(ValueType);
                    }
                    else
                    {
                        len = *(int32_t*)pAddr;
                        pAddr += sizeof(int32_t);
                        if(len > 0)
                        {
                            name = NEW_VEC(pMemPool, char, len + 1);
                            if(unlikely(name == NULL))
                            {
                                TWARN("构建统计参数子句统计项中的Key失败");
                                ppStatResItems = NULL;
                                nCount = 0;
                                return KS_ENOMEM;
                            }
                            memcpy(name, pAddr, len);
                            pAddr += len;
                            name[len] = '\0'; 

                            pStatItem->statKey.uFieldValue.svalue = name;
                        }
                    }
                    pStatItem->nCount = *(int32_t*)pAddr;
                    pAddr += sizeof(int32_t);
                    memcpy(&pStatItem->sumCount, pAddr, sizeof(ValueType));
                    pAddr += sizeof(ValueType);

                    pStatResItem->ppStatItems[j] = pStatItem;
                }
            }
            ppStatResItems[i] = pStatResItem;
        }

        return KS_SUCCESS;
    }
    
    int32_t StatisticResult::append(MemPool *pMemPool, const StatisticResult *pStatResult)
    {
        assert(pMemPool != NULL);
        
        // 待追加的统计结果作为空，直接返回
        if(unlikely(pStatResult == NULL
                    || pStatResult->nCount <= 0
                    || pStatResult->ppStatResItems == NULL))
        {
            return KS_SUCCESS;
        }
        // 当前统计结果为空，直接用待追加的统计结果作为最终结果
        if(unlikely(nCount == 0 || ppStatResItems == NULL))
        {
            ppStatResItems = pStatResult->ppStatResItems;
            nCount = pStatResult->nCount;
            return KS_SUCCESS;
        }
        
        if(unlikely(nCount != pStatResult->nCount))
        {
            TWARN("追加统计子结果数和原结果数不相等");
            return KS_EFAILED;
        }
        for(int32_t i = 0; i < nCount; ++i)
        {
            // 待追加的统计子句结果为空，继续
            if(unlikely(pStatResult->ppStatResItems[i] == NULL
                        || pStatResult->ppStatResItems[i]->nCount <=0
                        || pStatResult->ppStatResItems[i]->ppStatItems == NULL))
            {
                continue;
            }
            // 当前统计子句结果为空，直接用待追加的统计子句结果作为结果
            if(unlikely(ppStatResItems[i] == NULL
                        || ppStatResItems[i]->nCount <= 0
                        || ppStatResItems[i]->ppStatItems == NULL))
            {
                ppStatResItems[i] = pStatResult->ppStatResItems[i];
            }
            
            // 合并统计子句结果
            StatisticResultItem *pStatResItem = ppStatResItems[i];
            StatisticResultItem *pNewStatResItem = pStatResult->ppStatResItems[i];
            StatisticItem **ppNewStatItems = NEW_VEC(pMemPool, StatisticItem*,
                    pStatResItem->nCount + pNewStatResItem->nCount);
            if(unlikely(ppNewStatItems == NULL))
            {
                TWARN("构建统计参数子句结果中的统计项实例失败");
                return KS_ENOMEM;
            }
            int32_t n = 0;
            for(int32_t j = 0; j < pStatResItem->nCount; ++j,++n)
            {
                ppNewStatItems[n] = pStatResItem->ppStatItems[j];
            }
            for(int32_t j = 0; j < pNewStatResItem->nCount; ++j,++n)
            {
                ppNewStatItems[n] = pNewStatResItem->ppStatItems[j];
            }
            pStatResItem->nCount += pNewStatResItem->nCount;
            pStatResItem->ppStatItems = ppNewStatItems;
        }

        return KS_SUCCESS;
    }


#define STOP '\1'
#define SECTION_STATINFO "stat"
#define MOVE_CURSOR(p, left, all, n) { p += n; left -= n; all += n; }

static uint32_t outputStatKey(char *ptr, uint32_t size, const StatisticKey &statKey)
{
    PF_DATA_TYPE eFieldType = statKey.eFieldType;
    if(eFieldType == DT_STRING)
    {
        uint32_t len = strlen(statKey.uFieldValue.svalue);
        if(unlikely(len > size)) return size + 1;
        memcpy(ptr, statKey.uFieldValue.svalue, len);
        return len;
    }
    else if(eFieldType == DT_INT8 || eFieldType == DT_INT16
            || eFieldType == DT_INT32 || eFieldType == DT_INT64
            || eFieldType == DT_KV32)
    {
        return snprintf(ptr, size, "%lld",static_cast<long long int>(statKey.uFieldValue.lvalue));
    }
    else if(eFieldType == DT_UINT8 || eFieldType == DT_UINT16
            || eFieldType == DT_UINT32 || eFieldType == DT_UINT64 )
    {
        return snprintf(ptr, size, "%llu",static_cast<long long unsigned int>(statKey.uFieldValue.ulvalue));
    }
    else if(eFieldType == DT_DOUBLE)
    {
        return snprintf(ptr, size, "%f",statKey.uFieldValue.dvalue);
    }
    else if(eFieldType == DT_FLOAT)
    {
        return snprintf(ptr, size, "%f",statKey.uFieldValue.fvalue);
    }
/*
    switch(eFieldType)
    {
        case DT_INT8:
        case DT_INT16:
        case DT_INT32:
        case DT_INT64:
        case DT_KV32:
            return snprintf(ptr, size, "%lld",statKey.uFieldValue->lvalue);
            break;
        
        case DT_UINT8:
        case DT_UINT16:
        case DT_UINT32:
        case DT_UINT64:
            return snprintf(ptr, size, "%llu",statKey.uFieldValue->ulvalue);
            break;
    
        case DT_DOUBLE:
            return snprintf(ptr, size, "%f",statKey.uFieldValue->dvalue);
            break;

        case DT_FLOAT:
            return snprintf(ptr, size, "%f",statKey.uFieldValue->fvalue);
            break;

        case DT_STRING:
            return snprintf(ptr, size, "%s",statKey.uFieldValue->svalue);
            break;

        default:
            break;
    }
*/
    return 0;
}

int32_t StatisticResult::outputXMLFormat(char *buffer, int32_t size)
{
    /*     char *ptr = buffer;
           int32_t len = 0
           int32_t nLeftSize = size;
           int32_t nTotalSize = 0;
           */
    char *ptr = buffer;
    uint32_t left = size;
    uint32_t total_size = 0;
    uint32_t len = 0;

    if(nCount > 0 && ppStatResItems != NULL)
    {
        len = snprintf(ptr, left, "<statset count=\"%d\">\n", nCount);
        if(len + total_size > static_cast<uint32_t>(size))
        {
            return size + 1;
        }
        MOVE_CURSOR(ptr, left, total_size, len);
        for(int32_t i = 0; i < nCount; ++i)
        {
            StatisticResultItem *pResultItem = ppStatResItems[i];
            if(unlikely(NULL == pResultItem))
            {
                continue;
            }
            if(unlikely(pResultItem->nCount <= 0))
            {
                len = snprintf(ptr, left, "<statele count=\"0\" key=\"%s\">\n</statele>\n",
                        pResultItem->pStatHeader->szFieldName);
                if (len + total_size > static_cast<uint32_t>(size))
                {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                continue;
            }
            len = snprintf(ptr, left, "<statele count=\"%d\" key=\"%s\">\n",
                    pResultItem->nCount, pResultItem->pStatHeader->szFieldName);
            if (len + total_size > static_cast<uint32_t>(size))
            {
                return size + 1;
            }
            MOVE_CURSOR(ptr, left, total_size, len);
            for (int32_t j=0; j < pResultItem->nCount; ++j)
            {
                if(unlikely(pResultItem->ppStatItems[j]->nCount < 0))
                {
                    break;
                }
                StatisticItem *pItem = pResultItem->ppStatItems[j];
                //StatisticHeader *pHead = pResultItem->pStatHeader;
                len = snprintf(ptr, left, "<statitem count=\"%d\" statid=\"",
                        pItem->nCount);
                if(len + total_size > static_cast<uint32_t>(size)) 
                {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                len = outputStatKey(ptr, left, pItem->statKey); 
                if(len + total_size > static_cast<uint32_t>(size)) 
                {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                len = snprintf(ptr, left, "\"");
                if(len + total_size > static_cast<uint32_t>(size)) 
                {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
        /*
                if(pHead->szSumFieldName != NULL
                        && (pHead->sumFieldType == DT_INT32
                            || pHead->sumFieldType == DT_INT64
                            || pHead->sumFieldType == DT_DOUBLE
                            || pHead->sumFieldType == DT_INT8
                            || pHead->sumFieldType == DT_INT16
                            || pHead->sumFieldType == DT_FLOAT)) {
                    len = snprintf(ptr, left, " sumcnt=\"");
                    if(len + total_size > size) {
                        return size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                    len = outputStatKey(ptr, left,
                            pHead->sumFieldType, &(pItem->sumCount));
                    if(len + total_size > size) {
                        return size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                    len = snprintf(ptr, left, "\"");
                    if(len + total_size > size) {
                        return size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
        */
                len = snprintf(ptr, left, " />\n");
                if(len + total_size > static_cast<uint32_t>(size)) 
                {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
            }
            len = snprintf(ptr, left, "</statele>\n");
            if(len + total_size > static_cast<uint32_t>(size)) 
            {
                return size + 1;
            }
            MOVE_CURSOR(ptr, left, total_size, len);
        }
        len = snprintf(ptr, left, "</statset>\n");
        if(len + total_size > static_cast<uint32_t>(size)) 
        {
            return size + 1;
        }
        MOVE_CURSOR(ptr, left, total_size, len);
    }
    return total_size;
}

int32_t StatisticResult::outputV3Format(char *buffer, int32_t size)
{
    char *ptr = buffer;
    uint32_t left = size;
    uint32_t total_size = 0;
    uint32_t len = 0;
    int32_t n = 1;
    
    if(nCount > 0 && ppStatResItems != NULL)
    {
        for(int32_t i = 0; i < nCount; ++i)
        {
            StatisticResultItem *pResultItem = ppStatResItems[i];
            if(unlikely(NULL == pResultItem))
            {
                continue;
            }
            n += (pResultItem->nCount + 1);
        }
        len = snprintf(ptr, left, "%s%c%d%c%d%c",
                SECTION_STATINFO, STOP, n, STOP, 
                nCount, STOP);
        if (len + total_size > static_cast<uint32_t>(size)) {
            return size + 1;
        }
        MOVE_CURSOR(ptr, left, total_size, len);
        for(int32_t i = 0; i < nCount; ++i)
        {
            StatisticResultItem *pResultItem = ppStatResItems[i];
            if(unlikely(NULL == pResultItem))
            {
                continue;
            }
            len = snprintf(ptr, left, "%d%c", pResultItem->nCount, STOP);
            if(len + total_size > static_cast<uint32_t>(size)) {
                return size + 1;
            }
            MOVE_CURSOR(ptr, left, total_size, len);
            for (int32_t j = 0; j < pResultItem->nCount; ++j)
            {
                if(unlikely(pResultItem->ppStatItems[i]->nCount < 0))
                {
                    break;
                }
                StatisticItem *pItem = pResultItem->ppStatItems[j];
                //StatisticHeader *pHead = pResultItem->pStatHeader;

                len = outputStatKey(ptr, left, pItem->statKey); 
                if(len + total_size > static_cast<uint32_t>(size)) {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
                len = snprintf(ptr, left, ";%d;", pItem->nCount);
                if(len + total_size > static_cast<uint32_t>(size)) {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
          /*
                if (pHead->szSumFieldName != NULL
                        && (pHead->sumFieldType == DT_INT32
                            || pHead->sumFieldType == DT_INT64
                            || pHead->sumFieldType == DT_DOUBLE
                            || pHead->sumFieldType == DT_INT8
                            || pHead->sumFieldType == DT_INT16
                            || pHead->sumFieldType == DT_FLOAT)) {
                    len = outputStatKey(ptr, left,
                            pHead->sumFieldType, &(pItem->sumCount));
                    if(len + total_size > size) {
                        return size + 1;
                    }
                    MOVE_CURSOR(ptr, left, total_size, len);
                }
           */
                len = snprintf(ptr, left, "%c", STOP);
                if(len + total_size > static_cast<uint32_t>(size)) {
                    return size + 1;
                }
                MOVE_CURSOR(ptr, left, total_size, len);
            }
        }
    }
    return total_size;
}

}
