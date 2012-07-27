#include "sort/SortResult.h"
#include "util/MemPool.h"
#include "index_lib/DocIdManager.h"
#include "util/Log.h"
#include "Util/SDM.h"

#include <assert.h>

using namespace sort_util;
using namespace sort_application;
using namespace sort_framework;

#define MAXITEMLENGTH 20

    sort_framework::SortResult::SortResult(MemPool *mempool) 
: _mempool(mempool),
    _sdm(NULL)
{
    assert(_mempool);
    _sdm = NEW(_mempool, SDM)(_mempool);
}

sort_framework::SortResult::~SortResult() 
{
} 
uint32_t sort_framework::SortResult::serialize(char *str, uint32_t len) const
{
    return _sdm->serialize(str, len);
}

bool sort_framework::SortResult::deserialize(char *str, uint32_t len)
{
    int32_t length = _sdm->deserialize(str, len);
    return (length>0);
}

int32_t sort_framework::SortResult::FilterInvalidID(uint32_t nNum)
{
    if (_sdm->getKeyRow() == NULL) {
        sort_util::SDM_ROW *pDocIdROW = _sdm->FindRow(MNAME_DOCID);
        if (pDocIdROW == NULL) {
            return -1;
        }
        _sdm->setKeyRow(pDocIdROW);
    }
    int32_t ret = _sdm->FilterInvalidID(nNum);
    _sdm->setCurResNum(_sdm->getDocNum());
    return ret;
}

uint32_t sort_framework::SortResult::getBlockCount()
{
    return _sdm->getDocNum();
}

void sort_framework::SortResult::setBlockCount(uint32_t num)
{
    return _sdm->setDocNum(num);
}

uint32_t sort_framework::SortResult::getDocsFound()
{
    return _sdm->getDocsFound();
}

uint32_t sort_framework::SortResult::getEstimateDocsFound()
{
    return _sdm->getEstimateDocsFound();
}

int32_t sort_framework::SortResult::getNidList(int64_t * & pnid)
{
    if ((_sdm->getDocNum()<= 0) || (_sdm->getCurResNum() <= _sdm->getStartNo())) {
        return 0;
    }
    uint32_t nStart = _sdm->getStartNo();
    sort_util::SDM_ROW *pNidROW = _sdm->FindRow(MNAME_NID);
    if (pNidROW == NULL) {
        TLOG("Get MNAME_NID error");
        return -1;
    }
    int64_t *pResNID = (int64_t *)pNidROW->getBuff();
    if (pResNID == NULL) {
        TLOG("Get MNAME_NID row buf error");
        return -1;
    }
    uint32_t curResNum = _sdm->getCurResNum();
    uint32_t nRetNum = 0;
    int64_t *nidList = NEW_VEC(_mempool, int64_t, curResNum);
    if (!nidList) {
        return -1;
    }
    for (uint32_t i=nStart; i<curResNum; i++) {
        nidList[nRetNum] = pResNID[i];
        nRetNum++;
    }
    pnid = nidList;
    return nRetNum;
}

int32_t sort_framework::SortResult::getDocidList(uint32_t * & pDocid)
{
    uint32_t curResNum = _sdm->getDocNum();
    if (curResNum <= 0) {
        return 0;
    }
    sort_util::SDM_ROW *pNidROW = _sdm->FindRow(MNAME_DOCID);
    if (pNidROW == NULL) {
        return -1;
    }
    uint32_t *pResNID = (uint32_t *)pNidROW->getBuff();
    pDocid = pResNID;
    return curResNum;
}

int sort_framework::SortResult::getSidList(int32_t * & psid)
{
    if ((_sdm->getDocNum()<= 0) || (_sdm->getCurResNum() <= _sdm->getStartNo())) {
        return 0;
    }
    sort_util::SDM_ROW *pSidROW = _sdm->FindRow(MNAME_SERVERID);
    if (pSidROW == NULL) {
        TLOG("Get MNAME_SERVERID error");
        return -1;
    }
    uint32_t *pResNID = (uint32_t *)pSidROW->getBuff();
    if (pResNID == NULL) {
        TLOG("Get MNAME_SERVERID Row buf error");
        return -1;
    }
    uint32_t curResNum = _sdm->getCurResNum();
    uint32_t nStart = _sdm->getStartNo();
    uint32_t nRetNum = 0;
    uint32_t *sidList = NEW_VEC(_mempool, uint32_t, curResNum);
    if (!sidList) {
        return -1;
    }
    for (uint32_t i=nStart; i<curResNum; i++) {
        sidList[nRetNum] = pResNID[i];
        nRetNum++;
    }
    psid = (int32_t *)sidList;
    return nRetNum;
}

int sort_framework::SortResult::getFirstRankList(int64_t * & pfirst)
{
    if ((_sdm->getDocNum()<= 0) || (_sdm->getCurResNum() <= _sdm->getStartNo())) {
        return 0;
    }
    sort_util::SDM_ROW *pFirstRankROW = _sdm->getFirstRankRow();
    if (pFirstRankROW == NULL) {
        return 0;
    }
    uint32_t curResNum = _sdm->getCurResNum();
    uint32_t nStart = _sdm->getStartNo();
    uint32_t nRetNum = 0;
    int64_t *pFirstRankList = NEW_VEC(_mempool, int64_t, curResNum);
    if (!pFirstRankList) {
        return -1;
    }
    PF_DATA_TYPE dataType = pFirstRankROW->getDataType();
    for(int i=nStart; i<curResNum; i++) {
        switch (dataType) {
            case DT_INT8:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((int8_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT8:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((uint8_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT16:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((int16_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT16:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((uint16_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT32:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((int32_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT32:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((uint32_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT64:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((int64_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT64:
                {
                    pFirstRankList[nRetNum] = (int64_t)(((uint64_t *)pFirstRankROW->getBuff())[i]);
                    break;
                }
            case DT_FLOAT: 
            case DT_DOUBLE: 
                {
                    pFirstRankList[nRetNum] = pFirstRankROW->Get(i);
                    break;
                }
            default:
                pFirstRankList[nRetNum] = 0;
        }
        nRetNum++;
    }
    pfirst = pFirstRankList;
    return nRetNum;
}


int sort_framework::SortResult::getSecondRankList(int64_t * & pfirst)
{
    if ((_sdm->getDocNum()<= 0) || (_sdm->getCurResNum() <= _sdm->getStartNo())) {
        return 0;
    }
    sort_util::SDM_ROW *pSecondRankROW = _sdm->getSecondRankRow();
    if (pSecondRankROW == NULL) {
        return 0;
    }
    uint32_t curResNum = _sdm->getCurResNum();
    uint32_t nStart = _sdm->getStartNo();
    uint32_t nRetNum = 0;
    int64_t *pSecondRankList = NEW_VEC(_mempool, int64_t, curResNum);
    if(!pSecondRankList) {
        return -1;
    }
    PF_DATA_TYPE dataType = pSecondRankROW->getDataType();
    for(int i=nStart; i<curResNum; i++) {
        switch (dataType) {
            case DT_INT8:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((int8_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT8:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((uint8_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT16:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((int16_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT16:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((uint16_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT32:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((int32_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT32:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((uint32_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_INT64:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((int64_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_UINT64:
                {
                    pSecondRankList[nRetNum] = (int64_t)(((uint64_t *)pSecondRankROW->getBuff())[i]);
                    break;
                }
            case DT_FLOAT:
            case DT_DOUBLE:
                {
                    pSecondRankList[nRetNum] = pSecondRankROW->Get(i);
                    break;
                }
            default:
                pSecondRankList[nRetNum] = 0;
        }
        nRetNum++;
    }
    pfirst = pSecondRankList;
    return nRetNum;
}

int sort_framework::SortResult::getPosList(int32_t * & pos)
{
    if ((_sdm->getDocNum()<= 0) || (_sdm->getCurResNum() <= _sdm->getStartNo())) {
        return 0;
    }
    sort_util::SDM_ROW *pSidROW = _sdm->FindRow(MNAME_RESULT_POS);
    if (pSidROW == NULL) {
        TLOG("Get MNAME_RESULT_POS error");
        return -1;
    }
    uint32_t *pResNID = (uint32_t *)pSidROW->getBuff();
    if (pResNID == NULL) {
        TLOG("Get MNAME_RESULT_POS row buf error");
        return -1;
    }
    uint32_t curResNum = _sdm->getCurResNum();
    uint32_t nStart = _sdm->getStartNo();
    uint32_t nRetNum = 0;
    uint32_t *sidList = NEW_VEC(_mempool, uint32_t, curResNum);
    if (!sidList) {
        return -1;
    }
    for (uint32_t i=nStart; i<curResNum; i++) {
        sidList[nRetNum] = pResNID[i];
        nRetNum++;
    }
    pos = (int32_t *)sidList;
    return nRetNum;
}

static int putuniqFieldValue(char *FieldValue, int left, void *ptr, PF_DATA_TYPE dataType)
{
    int length = -1;
    switch(dataType){
        case DT_INT8:
        case DT_UINT8: {
                           length = snprintf(FieldValue, left, "%c,", *(uint8_t*)ptr); 
                           break;
                       }
        case DT_INT16:
        case DT_UINT16: {
                            length = snprintf(FieldValue, left, "%d,", *(uint16_t *)ptr); 
                            break;
                        }
        case DT_UINT32: {
                            length = snprintf(FieldValue, left, "%u,", *(uint32_t *)ptr); 
                            break;
                        }
        case DT_INT32: {
                           length = snprintf(FieldValue, left, "%d,", *(int32_t *)ptr); 
                           break;
                       }

        case DT_UINT64: {
                            length = snprintf(FieldValue, left, "%lu,", *(uint64_t *)ptr); 
                            break;
                        }
        case DT_INT64: {
                           length = snprintf(FieldValue, left, "%ld,", *(int64_t *)ptr); 
                           break;
                       }
        case DT_DOUBLE:
        case DT_FLOAT: {
                           length = snprintf(FieldValue, left, "%15.5f,", *(double *)ptr); 
                           break;
                       }
        default:
                       break;
    }
    return length;
}

