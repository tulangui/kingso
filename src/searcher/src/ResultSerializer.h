/*********************************************************************
 * $Author: taiyi.zjh $
 *
 * $LastChangedBy: taiyi.zjh $
 * 
 * $Revision: 12996 $
 *
 * $LastChangedDate: 2012-06-26 13:05:18 +0800 (Tue, 26 Jun 2012) $
 *
 * $Id: ResultSerializer.h 12996 2012-06-26 05:05:18Z taiyi.zjh $
 *
 * $Brief: Result序列化类，用于searcher框架调用 $
 ********************************************************************/

#ifndef KINGSO_SEARCHER_RESULTSERIALIZER_H
#define KINGSO_SEARCHER_RESULTSERIALIZER_H

#include "commdef/ClusterResult.h"
#include "util/namespace.h"
#include "util/MemPool.h"
#include "queryparser/QPResult.h"
#include "statistic/StatisticResult.h"
#include "sort/SortResult.h"

class Document;

class ResultSerializer {
    public:
        int32_t serialClusterResult(commdef::ClusterResult *pClusterResult,
                char * &pOutData, uint32_t &pOutSize, 
                const char *pDflCmpr);
        //Need MemPool to store result
        int32_t deserialClusterResult(const char *pData, uint32_t size,
                MemPool *pHeap, commdef::ClusterResult &out, 
                const char *pDflCmpr);
        //Need MemPool to store result
        int32_t deserialDetailResult(const char *pData, uint32_t size,
                MemPool *pHeap, Document ** &docs, 
                uint32_t &count, const char *pDflCmpr);
};
bool translateV3(const char *pV3Str, Document ** &ppDocument, uint32_t &docsCount, MemPool *heap);
#endif
