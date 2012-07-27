#include "ResultSerializer.h"
#include <stdio.h>
#include <stdlib.h>
#include "commdef/ClusterResult.h"
#include "framework/namespace.h"
#include "framework/Compressor.h"
#include "Document.h"

bool translateV3(const char *pV3Str, Document ** &ppDocument, 
        uint32_t &docsCount, MemPool *heap);

int32_t ResultSerializer::serialClusterResult(commdef::ClusterResult *pClusterResult,
        char * &outData, uint32_t &outSize, const char *dflCmpr=NULL) {
    uint32_t paraSize = 0;
    uint32_t statSize = 0;
    uint32_t sortSize = 0;
    uint32_t uniqSize = 0;
    uint32_t srcSize = 0;
    uint32_t orsSize = 0;
    char * ptr = NULL;
    //CategorySerializer stat_serial;
    if(!pClusterResult) {
        outData = NULL;
        outSize = 0;
        return KS_SUCCESS;
    }
    queryparser::QPResult *pQPResult = pClusterResult->_pQPResult;
    statistic::StatisticResult *pStatisticResult = pClusterResult->_pStatisticResult;
    sort_framework::SortResult *pSortResult = pClusterResult->_pSortResult;
    //DocsSearch,DocsFound,DocsRestrict,paraSize,
    //statSize,sortSize,uniqSize,srcSize,orsSize
    outSize = sizeof(uint32_t) * 10;
    //计算序列化后，各部分所需空间大小
    paraSize = pQPResult ? pQPResult->getSerialSize() : 0;
    statSize = pStatisticResult ? pStatisticResult->getSerialSize() : 0;
    sortSize = pSortResult ? pSortResult->serialize(NULL, 0) : 0;
    srcSize = pClusterResult->_szSrcBuffer ?
        pClusterResult->_nSrcBufLen : 0;
    orsSize = pClusterResult->_szOrsBuffer ?
        pClusterResult->_nOrsBufferLen : 0;
    //计算序列化后，所需空间大小
    outSize += paraSize + statSize + sortSize + uniqSize + srcSize + orsSize;
    outData = (char*) ::malloc(outSize);
    if((!outData)) {
        return KS_EFAILED;
    }
    ptr = outData;
    *(uint32_t*)ptr = pClusterResult->_nDocsSearch;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = pClusterResult->_nDocsFound;
     ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = pClusterResult->_nEstimateDocsFound;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = pClusterResult->_nDocsRestrict;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = paraSize;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = statSize;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = sortSize;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = uniqSize;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = srcSize;
    ptr += sizeof(uint32_t);
    *(uint32_t*)ptr = orsSize;
    ptr += sizeof(uint32_t);
    if(paraSize > 0) {
        if(pQPResult->serialize(ptr, paraSize) != paraSize) {
            TWARN("serialize QPResult error");
            ::free(outData);
            outData = NULL;
            return KS_EFAILED;
        }
        ptr += paraSize;
    }
    if(statSize > 0) {
        if(pStatisticResult->serialize(ptr,
                    statSize) != statSize) {
            TWARN("serialize StatisticResult error");
            ::free(outData);
            outData = NULL;
            return KS_EFAILED;
        }
        ptr += statSize;
    }
    if(sortSize > 0) {
        if(pSortResult->serialize(ptr, sortSize) != sortSize) {
            TWARN("serialize SortResult error");
            ::free(outData);
            outData = NULL;
            return KS_EFAILED;
        }
        ptr += sortSize;
    }
    if(uniqSize > 0) {
        memcpy(ptr, pClusterResult->_szUniqBuffer, uniqSize);
        ptr += uniqSize;
    }
    if(srcSize > 0) {
        memcpy(ptr, pClusterResult->_szSrcBuffer, srcSize);
        ptr += srcSize;
    }
    if(orsSize > 0) {
        memcpy(ptr, pClusterResult->_szOrsBuffer, orsSize);
        ptr += orsSize;
    }
    //compress
    FRAMEWORK::Compressor *compr;
    FRAMEWORK::compress_type_t cmpr_t = FRAMEWORK::ct_none;
    if ((dflCmpr!=NULL) && (strcmp(dflCmpr,"Z")==0)) {
        cmpr_t = FRAMEWORK::ct_zlib;
    }
    //Do not use MemPool
    compr = FRAMEWORK::CompressorFactory::make(cmpr_t, NULL);
    if (compr != NULL) {
        char *cmpr = NULL;
        uint32_t cmprSize = 0;
        int32_t co_ret = compr->compress(outData, outSize, cmpr, cmprSize);
        FRAMEWORK::CompressorFactory::recycle(compr);
        ::free(outData);
        outData = NULL;
        if ((co_ret!=KS_SUCCESS)||(cmpr==NULL)||(cmprSize==0)) {
            ::free(cmpr);
            cmpr = NULL;
            return KS_EFAILED;
        }
        outData = cmpr;
        outSize = cmprSize;
    }
    //compress end
    return KS_SUCCESS;
}

int32_t ResultSerializer::deserialClusterResult(const char *data, 
        uint32_t size,
        MemPool *pHeap,
        commdef::ClusterResult &out,
        const char *dflCmpr=NULL) 
{
    uint32_t paraSize = 0;
    uint32_t sortSize = 0;
    if ((data == NULL) || (size == 0) || (pHeap == NULL)) {
        return KS_EFAILED;
    }
    //uncompress
    char *uncmpr = NULL;
    uint32_t uncmprSize = 0;
    FRAMEWORK::Compressor *compr;
    FRAMEWORK::compress_type_t cmpr_t = FRAMEWORK::ct_none;
    if ((dflCmpr!=NULL) && (strcmp(dflCmpr,"Z")==0)) {
        cmpr_t = FRAMEWORK::ct_zlib;
    }
    compr = FRAMEWORK::CompressorFactory::make(cmpr_t, pHeap);
    if (compr != NULL) {
        int32_t co_ret = compr->uncompress(data, size, uncmpr, uncmprSize);
        FRAMEWORK::CompressorFactory::recycle(compr);
        if ((co_ret!=KS_SUCCESS)||(uncmpr==NULL)||(uncmprSize==0)) {
            return KS_EFAILED;
        }
        data = uncmpr;
        size = uncmprSize;
    }
    //uncompress end
    memset(&out, 0, sizeof(commdef::ClusterResult));
    if(size < sizeof(uint32_t) * 10) {
        return KS_EFAILED;
    }
    out._nDocsSearch = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nDocsFound = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nEstimateDocsFound = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nDocsRestrict = *(uint32_t*)data;
    data += sizeof(uint32_t);
    paraSize = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nStatBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    sortSize = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nUniqBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nSrcBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nOrsBufferLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    if(sizeof(uint32_t) * 10 + paraSize + out._nStatBufLen +
            sortSize + out._nUniqBufLen + 
            out._nSrcBufLen + out._nOrsBufferLen != size) 
    {
        return KS_EFAILED;
    }
    if(paraSize > 0) {
        out._pQPResult = NEW(pHeap, queryparser::QPResult)(pHeap);
        if (out._pQPResult == NULL) {
            TWARN("out of Memory");
            return KS_ENOMEM;
        }
        if (out._pQPResult->deserialize((char*)data, paraSize, pHeap) != KS_SUCCESS)
        {
            TWARN("deserialize QPResult error, paraSize=%d.", paraSize);
            return KS_EFAILED;
        }
        data += paraSize;
    }
    else {
        out._pQPResult = NULL;
    }
    if(out._nStatBufLen > 0) {
        out._szStatBuffer = (char*)data;
        out._pStatisticResult = NEW(pHeap, statistic::StatisticResult)();
        if (out._pStatisticResult == NULL) {
            TWARN("out of Memory");
            return KS_ENOMEM;
        }
        if (out._pStatisticResult->deserialize(out._szStatBuffer,
                    out._nStatBufLen, pHeap) != KS_SUCCESS)
        {
            TWARN("deserialize Statistic Result error.");
            return KS_EFAILED;
        }
        data += out._nStatBufLen;
    } else {
        out._szStatBuffer = NULL;
    }
    if(sortSize > 0) {
        out._pSortResult = NEW(pHeap, sort_framework::SortResult)(pHeap);
        if (out._pSortResult == NULL) {
            TWARN("out of Memory");
            return KS_ENOMEM;
        }
        if (!out._pSortResult->deserialize((char *)data, sortSize)) {
            TWARN("deserialize SortResult error.");
            return KS_EFAILED;
        }
        data += sortSize;
    } else {
        out._pSortResult = NULL;
    }
    if(out._nUniqBufLen > 0) {
        out._szUniqBuffer = (char*)data;
        data += out._nUniqBufLen;
    } else {
        out._szUniqBuffer = NULL;
    }
    if(out._nSrcBufLen > 0) {
        out._szSrcBuffer = (char*)data;
        data += out._nSrcBufLen;
    } else {
        out._szSrcBuffer = NULL;
    }
    if(out._nOrsBufferLen > 0) {
        out._szOrsBuffer = (char*)data;
        data += out._nOrsBufferLen;
    } else {
        out._szOrsBuffer = NULL;
    }
    return KS_SUCCESS;
}

int32_t ResultSerializer::deserialDetailResult(const char *data, uint32_t size,
        MemPool *pHeap, Document ** &docs, uint32_t &count, const char *dflCmpr=NULL)
{
    uint32_t cost = 0;
    uint32_t i = 0;
    uint32_t j = 0;
    int32_t nFieldCount = 0;
    int32_t ret = KS_EFAILED;
    if (!pHeap || !data || size <= sizeof(int) * 2) {
        docs = NULL;
        count = 0;
        return KS_SUCCESS;
    }
    //uncompress begin
    char *uncmpr = NULL;
    uint32_t uncmprSize = 0;
    FRAMEWORK::Compressor *compr;
    FRAMEWORK::compress_type_t cmpr_t = FRAMEWORK::ct_none;
    if ((dflCmpr!=NULL) && (strcmp(dflCmpr,"Z")==0)) {
        cmpr_t = FRAMEWORK::ct_zlib;
    }
    compr = FRAMEWORK::CompressorFactory::make(cmpr_t, pHeap);
    if (compr != NULL) {
        int32_t co_ret = compr->uncompress(data, size, uncmpr, uncmprSize);
        FRAMEWORK::CompressorFactory::recycle(compr);
        if ((co_ret!=KS_SUCCESS)||(uncmpr==NULL)||(uncmprSize==0)) {
            return KS_EFAILED;
        }
        data = uncmpr;
        size = uncmprSize;
    }
    //uncompress end
    if (!translateV3(data, docs, count, pHeap)) {
        return KS_EFAILED;
    }
    return KS_SUCCESS;
}

bool translateV3(const char *pV3Str, Document ** &ppDocument, uint32_t &docsCount, MemPool *heap)
{
    if (!pV3Str || !*pV3Str) {
        return false;
    }
    int32_t totalLen = strlen(pV3Str);
    const char *pHead = pV3Str;
    const char *pTmp = pV3Str;
    const char *pBefore = NULL;
    const char *pTotalFieldCountBegin = NULL;
    const char *pTotalFieldCountEnd = NULL;
    const char *pFieldCountBegin = NULL;
    const char *pFieldCountEnd = NULL;
    char tmpBuff[2048];
    uint32_t totalFieldCount = 0;
    uint32_t fieldCount = 0;
    uint32_t len = 0;
    uint32_t offset = 0;
    //get totalfieldcount
    pTotalFieldCountBegin = pHead;
    pTmp = strchr(pTmp+1, 0x01);
    if (pTmp == NULL) {
        return false;
    }
    pTotalFieldCountEnd = pTmp;
    len = pTotalFieldCountEnd - pTotalFieldCountBegin;
    memcpy(tmpBuff, pTotalFieldCountBegin, len);
    tmpBuff[len] = '\0';
    totalFieldCount = atoi(tmpBuff);
    //get fieldcount
    pTmp = strchr(pTmp, 0x01);
    if (pTmp == NULL) {
        TWARN("Get fieldcount begin error");
        return false;
    }
    pFieldCountBegin = pTmp + 1;
    pTmp = strchr(pTmp+1, 0x01);
    if (pTmp == NULL) {
        TWARN("Get fieldcount end error");
        return false;
    }
    pFieldCountEnd = pTmp;
    len = pFieldCountEnd - pFieldCountBegin;
    memcpy(tmpBuff, pFieldCountBegin, len);
    tmpBuff[len] = '\0';
    fieldCount = atoi(tmpBuff);
    //caculate document number
    docsCount = totalFieldCount/fieldCount - 1;
    //create ppDocument array
    ppDocument = NEW_VEC(heap, Document *, docsCount);
    if (ppDocument == NULL) {
        TWARN("out of Memory");
        return KS_ENOMEM;
    }
    char **ppFieldName = NEW_VEC(heap, char *, fieldCount);
    if (ppFieldName == NULL) {
        TWARN("out of Memory");
        return KS_ENOMEM;
    }
    //get field name
    for (int32_t i=0; i<fieldCount; i++) {
        pBefore = pTmp + 1;
        pTmp = strchr(pTmp+1, 0x01);
        if (pTmp == NULL) {
            TWARN("Get field name end error");
            return false;
        }
        len = pTmp - pBefore;
        char *pFieldName = NEW_VEC(heap, char, len+1);
        if (pFieldName == NULL) {
            TWARN("out of Memory");
            return KS_ENOMEM;
        }
        memcpy(pFieldName, pBefore, len);
        pFieldName[len] = '\0';
        ppFieldName[i] = pFieldName;
    }
    //create document array
    for (int32_t i=0; i<docsCount; i++) {
        ppDocument[i] = NEW(heap, Document)(heap);
        if (ppDocument[i] == NULL) {
            TWARN("out of Memory");
            return KS_ENOMEM;
        }
        ppDocument[i]->init(fieldCount);
    }
    //add field to document
    for (int32_t i=0; i<docsCount; i++) {
        for (int32_t j=0; j<fieldCount; j++) {
            pBefore = pTmp + 1;
            pTmp = strchr(pTmp+1, 0x01);
            if (pTmp == NULL) {
                TWARN("get field value end error");
                return false;
            }
            len = pTmp - pBefore;
            memcpy(tmpBuff, pBefore, len);
            tmpBuff[len] = '\0';
            //name, value
            ppDocument[i]->addField(ppFieldName[j], tmpBuff);
        }
    }
    return true;
}

