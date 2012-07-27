#include "Util/SDM.h"
#include "sys/time.h"

using namespace std;
using namespace sort_framework;
using namespace sort_application;

namespace sort_util
{

    uint64_t getCurrentTime()
    {
        struct timeval t;
        gettimeofday(&t, NULL);
        return (static_cast<uint64_t>(
                    t.tv_sec) * static_cast<int64_t>(1000000) +
                static_cast<int64_t>(t.tv_usec));
    }

    //在searcher上使用
    SDM::SDM(MemPool *mempool,SortQuery *pQuery, const SearchResult *pSearchRes) : _mempool(mempool),
    _curNo(0),
    _pSearchRes(pSearchRes),
    _pQuery(pQuery),
    _docsFound(0),
    _estDocsFound(0),
    _estFactor(1.0),
    _docNum(0),
    _uniqNullCount(0),
    _startNo(0),
    _retNum(0),
    _allRetNum(0),
    _pResArray(NULL),
    _curResNum(0),
    _pSortCmpItem(NULL),
    _pSDMReader(NULL),
    _keyRow(NULL)
    {
        _pSDMReader = NEW(_mempool, SDMReader)(_mempool);
        const char *pTmp = _pQuery->getParam("s");
        if (pTmp != NULL)
            _startNo = atoi(pTmp);
        else
            _startNo = 0;

        pTmp = _pQuery->getParam("n");
        if (pTmp != NULL)
            _retNum = atoi(pTmp);
        else
            _retNum = 0;
        _allRetNum = _startNo + _retNum;
        _pSortCmpItem = NEW(_mempool, SortCompareItem);
        if (_pSearchRes != NULL) {
            _docsFound = pSearchRes->nDocFound;
            _estDocsFound = pSearchRes->nEstimateDocFound;
            if(_docsFound == 0) {
                _estFactor = 0;
            }
            else {
                _estFactor = (double)_estDocsFound / (double)_docsFound;
            }
        }
    }

    SDM::SDM(MemPool *mempool,SortQuery* pQuery) : _mempool(mempool),
    _curNo(0),
    _pSearchRes(NULL),
    _pQuery(pQuery),
    _docsFound(0),
    _estDocsFound(0),
    _estFactor(1.0),
    _docNum(0),
    _uniqNullCount(0),
    _startNo(0),
    _retNum(0),
    _allRetNum(0),
    _pResArray(NULL),
    _curResNum(0),
    _pSortCmpItem(NULL),
    _pSDMReader(NULL),
    _keyRow(NULL)
    {
        _pSDMReader = NEW(_mempool, SDMReader)(_mempool);
        const char *pTmp = _pQuery->getParam("s");
        if (pTmp != NULL)
            _startNo = atoi(pTmp);
        else
            _startNo = 0;

        pTmp = _pQuery->getParam("n");
        if (pTmp != NULL)
            _retNum = atoi(pTmp);
        else
            _retNum = 0;
        _allRetNum = _startNo + _retNum;
        _pSortCmpItem = NEW(_mempool, SortCompareItem);
        _keyRow = NULL;
    }


    SDM::SDM(MemPool *mempool) : _mempool(mempool),
    _curNo(0),
    _pSearchRes(NULL),
    _pQuery(NULL),
    _docsFound(0),
    _estDocsFound(0),
    _now(0),
    _estFactor(1.0),
    _docNum(0),
    _uniqNullCount(0),
    _startNo(0),
    _retNum(0),
    _allRetNum(0),
    _pResArray(NULL),
    _curResNum(0),
    _pSortCmpItem(NULL),
    _pSDMReader(NULL),
    _keyRow(NULL)
    {
        _pSortCmpItem = NEW(_mempool, SortCompareItem);
    }

    int32_t SDM::init(SortQuery* pQuery)
    {
        _pQuery = pQuery;
        _pSDMReader = NEW(_mempool, SDMReader)(_mempool);
        const char *pTmp = _pQuery->getParam("s");
        if (pTmp != NULL)
            _startNo = atoi(pTmp);
        else
            _startNo = 0;

        pTmp = _pQuery->getParam("n");
        if (pTmp != NULL)
            _retNum = atoi(pTmp);
        else
            _retNum = 0;
        _allRetNum = _startNo + _retNum;
        _pSortCmpItem = NEW(_mempool, SortCompareItem);
        _keyRow = NULL;
        return 0;
    }

    SDM_ROW* SDM::getFirstRankRow()
    {
        SortCompare* pCompare = _cmpMap.begin()->second;
        if (pCompare == NULL) {
            return NULL;
        }
        return pCompare->getRow(0);
    }

    SDM_ROW* SDM::getSecondRankRow()
    {
        SortCompare* pCompare = _cmpMap.begin()->second;
        if (pCompare == NULL) {
            return NULL;
        }
        return pCompare->getRow(1);
    }

    SDM_ROW* SDM::FindCopyRow(const char *rowname)
    {
        if (rowname == NULL) {
            return NULL;
        }
        string sTmp = "copy_";
        sTmp += rowname;
        return FindRow(sTmp.c_str());
    }

    SDM_ROW* SDM::FindRow(const char *rowname)
    {
        if (rowname == NULL) {
            return NULL;
        }
        std::map<std::string, SDM_ROW*>::const_iterator it = _rowMap.find(rowname);
        if (it ==  _rowMap.end()) {
            return NULL;
        }
        return it->second;
    }

    SDM_ROW* SDM::AddTmpRow(SDM_ROW* pSrcSDM)
    {
        if (!pSrcSDM) {
            return NULL;
        }
        PF_DATA_TYPE type = pSrcSDM->dataType;
        SDM_ROW *pTmpRow = NULL;
        switch(type){
            case DT_UINT8: {
                               pTmpRow = NEW(_mempool,TMP_SDM_ROW<uint8_t>)(_mempool, pSrcSDM);
                               break;
                           }
            case DT_UINT16: {
                                pTmpRow = NEW(_mempool,TMP_SDM_ROW<uint16_t>)(_mempool, pSrcSDM);
                                break;
                            }
            case DT_UINT32: {
                                pTmpRow = NEW(_mempool,TMP_SDM_ROW<uint32_t>)(_mempool, pSrcSDM);
                                break;
                            }
            case DT_UINT64: {
                                pTmpRow = NEW(_mempool,TMP_SDM_ROW<uint64_t>)(_mempool, pSrcSDM);
                                break;
                            }
            case DT_INT8: {
                              pTmpRow = NEW(_mempool,TMP_SDM_ROW<int8_t>)(_mempool, pSrcSDM);
                              break;
                          }
            case DT_INT16: {
                               pTmpRow = NEW(_mempool,TMP_SDM_ROW<int16_t>)(_mempool, pSrcSDM);
                               break;
                           }
            case DT_INT32: {
                               pTmpRow = NEW(_mempool,TMP_SDM_ROW<int32_t>)(_mempool, pSrcSDM);
                               break;
                           }
            case DT_INT64: {
                               pTmpRow = NEW(_mempool,TMP_SDM_ROW<int64_t>)(_mempool, pSrcSDM);
                               break;
                           }
            case DT_FLOAT: {
                               pTmpRow = NEW(_mempool,TMP_SDM_ROW<float>)(_mempool, pSrcSDM);
                               break;
                           }
            case DT_DOUBLE: {
                                pTmpRow = NEW(_mempool,TMP_SDM_ROW<double>)(_mempool, pSrcSDM);
                                break;
                            }
            case DT_STRING: {
                                pTmpRow = NEW(_mempool,TMP_SDM_ROW<char *>)(_mempool, pSrcSDM);
                                break;
                            }
            default:
                            return NULL;
        }
        return pTmpRow;
    }

    SDM_ROW* SDM::AddCopyRow(SDM_ROW* pSrcSDM)
    {
        if (!pSrcSDM) {
            return NULL;
        }
        PF_DATA_TYPE type = pSrcSDM->dataType;
        SDM_ROW *pCopyRow = NULL;
        switch(type){
            case DT_UINT8: {
                               pCopyRow = NEW(_mempool,COPY_SDM_ROW<uint8_t>)(_mempool, pSrcSDM); 
                               break;
                           }
            case DT_UINT16: {
                                pCopyRow = NEW(_mempool,COPY_SDM_ROW<uint16_t>)(_mempool, pSrcSDM); 
                                break;
                            }
            case DT_UINT32: {
                                pCopyRow = NEW(_mempool,COPY_SDM_ROW<uint32_t>)(_mempool, pSrcSDM); 
                                break;
                            }
            case DT_UINT64: {
                                pCopyRow = NEW(_mempool,COPY_SDM_ROW<uint64_t>)(_mempool, pSrcSDM); 
                                break;
                            }
            case DT_INT8: {
                              pCopyRow = NEW(_mempool,COPY_SDM_ROW<int8_t>)(_mempool, pSrcSDM); 
                              break;
                          }
            case DT_INT16: {
                               pCopyRow = NEW(_mempool,COPY_SDM_ROW<int16_t>)(_mempool, pSrcSDM); 
                               break;
                           }
            case DT_INT32: {
                               pCopyRow = NEW(_mempool,COPY_SDM_ROW<int32_t>)(_mempool, pSrcSDM); 
                               break;
                           }
            case DT_INT64: {
                               pCopyRow = NEW(_mempool,COPY_SDM_ROW<int64_t>)(_mempool, pSrcSDM); 
                               break;
                           }
            case DT_FLOAT: {
                               pCopyRow = NEW(_mempool,COPY_SDM_ROW<float>)(_mempool, pSrcSDM); 
                               break;
                           }
            case DT_DOUBLE: {
                                pCopyRow = NEW(_mempool,COPY_SDM_ROW<double>)(_mempool, pSrcSDM); 
                                break;
                            }
            case DT_STRING: {
                                pCopyRow = NEW(_mempool,COPY_SDM_ROW<char *>)(_mempool, pSrcSDM);
                                break;
                            }
            default:
                            return NULL;
        }
        pSrcSDM->setCopy(pCopyRow);
        _rowMap.insert(std::make_pair(pCopyRow->rowName, pCopyRow));
        _rows.push_back(pCopyRow);
        return pCopyRow;
    }

    SDM_ROW* SDM::AddVirtRow(const char* name,PF_DATA_TYPE type, int size)
    {
        if (name == NULL)
            return NULL;
        SDM_ROW *pNewRow; 
        if ((pNewRow = FindRow(name))){
            if (pNewRow->rowType == RT_VIRTUAL){
                return pNewRow;
            }
            else{
                return NULL;
            }
        }
        switch(type){
            case DT_UINT8: {
                               pNewRow = NEW(_mempool,VIR_SDM_ROW<uint8_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_UINT16: {
                                pNewRow = NEW(_mempool,VIR_SDM_ROW<uint16_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_UINT32: {
                                pNewRow = NEW(_mempool,VIR_SDM_ROW<uint32_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_UINT64: {
                                pNewRow = NEW(_mempool,VIR_SDM_ROW<uint64_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_INT8: {
                              pNewRow = NEW(_mempool,VIR_SDM_ROW<int8_t>)(_mempool, name,type, size);
                              break;
                          }
            case DT_INT16: {
                               pNewRow = NEW(_mempool,VIR_SDM_ROW<int16_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_INT32: {
                               pNewRow = NEW(_mempool,VIR_SDM_ROW<int32_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_INT64: {
                               pNewRow = NEW(_mempool,VIR_SDM_ROW<int64_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_FLOAT: {
                               pNewRow = NEW(_mempool,VIR_SDM_ROW<float>)(_mempool, name,type, size);
                               break;
                           }
            case DT_DOUBLE: {
                                pNewRow = NEW(_mempool,VIR_SDM_ROW<double>)(_mempool, name,type, size);
                                break;
                            }
            case DT_STRING: {
                                pNewRow = NEW(_mempool,VIR_SDM_ROW<char *>)(_mempool, name,type, size);
                                break;
                            }
            default:
                            return NULL;
        }
        _rowMap.insert(std::make_pair(pNewRow->rowName, pNewRow));
        _rows.push_back(pNewRow);
        return pNewRow;
    }

    SDM_ROW *SDM::AddProfRow(const char* name)
    {
        if (name == NULL)
            return NULL;
        SDM_ROW *pNewRow; 
        if ((pNewRow = FindRow(name))){
            if (pNewRow->rowType == RT_PROFILE){
                return pNewRow;
            }
            else{
                return NULL;
            }
        }
        Field_Info FieldInfo;
        if ( _pSDMReader->getFieldInfo(name, FieldInfo) < 0)
            return NULL;
        PF_DATA_TYPE type = FieldInfo.profField->getAppType();
        switch(type){
            case DT_UINT8: {
                               pNewRow = NEW(_mempool,PRF_SDM_ROW<uint8_t>)(_mempool, name, _pSDMReader);
                               break;
                           }
            case DT_UINT16: {
                                pNewRow = NEW(_mempool,PRF_SDM_ROW<uint16_t>)(_mempool, name, _pSDMReader);
                                break;
                            }
            case DT_UINT32: {
                                pNewRow = NEW(_mempool,PRF_SDM_ROW<uint32_t>)(_mempool, name, _pSDMReader);
                                break;
                            }
            case DT_UINT64: {
                                pNewRow = NEW(_mempool,PRF_SDM_ROW<uint64_t>)(_mempool, name, _pSDMReader);
                                break;
                            }
            case DT_INT8: {
                              pNewRow = NEW(_mempool,PRF_SDM_ROW<int8_t>)(_mempool, name, _pSDMReader);
                              break;
                          }
            case DT_INT16: {
                               pNewRow = NEW(_mempool,PRF_SDM_ROW<int16_t>)(_mempool, name, _pSDMReader);
                               break;
                           }
            case DT_INT32: {
                               pNewRow = NEW(_mempool,PRF_SDM_ROW<int32_t>)(_mempool, name, _pSDMReader);
                               break;
                           }
            case DT_INT64: {
                               pNewRow = NEW(_mempool,PRF_SDM_ROW<int64_t>)(_mempool, name, _pSDMReader);
                               break;
                           }
            case DT_FLOAT: {
                               pNewRow = NEW(_mempool,PRF_SDM_ROW<float>)(_mempool, name, _pSDMReader);
                               break;
                           }
            case DT_DOUBLE: {
                                pNewRow = NEW(_mempool,PRF_SDM_ROW<double>)(_mempool, name, _pSDMReader);
                                break;
                            }
            default:
                            return NULL;
        }
        if ((!pNewRow) || (!pNewRow->isOk())) {
            return NULL;
        }
        _rowMap.insert(std::make_pair(pNewRow->rowName, pNewRow));
        _rows.push_back(pNewRow);
        return pNewRow;
    }

    SDM_ROW *SDM::AddOtherRow(const char* name, PF_DATA_TYPE type, int size)
    {
        if (name == NULL)
            return NULL;
        SDM_ROW *pNewRow = NULL;
        if ((pNewRow = FindRow(name))){
            if (pNewRow->rowType == RT_OTHER){
                return pNewRow;
            }
            else{
                return NULL;
            }
        }
        switch(type){
            case DT_UINT8: {
                               pNewRow = NEW(_mempool,OTH_SDM_ROW<uint8_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_UINT16: {
                                pNewRow = NEW(_mempool,OTH_SDM_ROW<uint16_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_UINT32: {
                                pNewRow = NEW(_mempool,OTH_SDM_ROW<uint32_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_UINT64: {
                                pNewRow = NEW(_mempool,OTH_SDM_ROW<uint64_t>)(_mempool, name,type, size);
                                break;
                            }
            case DT_INT8: {
                              pNewRow = NEW(_mempool,OTH_SDM_ROW<int8_t>)(_mempool, name,type, size);
                              break;
                          }
            case DT_INT16: {
                               pNewRow = NEW(_mempool,OTH_SDM_ROW<int16_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_INT32: {
                               pNewRow = NEW(_mempool,OTH_SDM_ROW<int32_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_INT64: {
                               pNewRow = NEW(_mempool,OTH_SDM_ROW<int64_t>)(_mempool, name,type, size);
                               break;
                           }
            case DT_FLOAT: {
                               pNewRow = NEW(_mempool,OTH_SDM_ROW<float>)(_mempool, name,type, size);
                               break;
                           }
            case DT_DOUBLE: {
                                pNewRow = NEW(_mempool,OTH_SDM_ROW<double>)(_mempool, name,type, size);
                                break;
                            }
            case DT_STRING: {
                                pNewRow = NEW(_mempool,OTH_SDM_ROW<char *>)(_mempool, name,type, size);
                                break;
                            }
            default:
                            return NULL;
        }
        _rowMap.insert(std::make_pair(pNewRow->rowName, pNewRow));
        _rows.push_back(pNewRow);
        return pNewRow;
    }

    SDM_ROW *SDM::AddRow(const char *name, bool isCopy){
        if (name == NULL)
            return NULL;
        SDM_ROW *pRow = FindRow(name);
        if (pRow) {
            if (isCopy) {
                return AddCopyRow(pRow);
            }
            else {
                return pRow;
            }
        }
        else if (_pSearchRes == NULL)
            return NULL;
        if ((strcasecmp(name, "RL")==0) || (strcasecmp(name, "rl")==0)) {
            pRow = FindRow(MNAME_RELESCORE);
            if (pRow) {
                return pRow;
            }
        }
        SDM_ROW *pReRow = NULL;
        if (strcasecmp(name, MNAME_DOCID)==0) {
            pReRow = AddVirtRow(MNAME_DOCID, DT_UINT32, sizeof(uint32_t));
            int nDocFound = _pSearchRes->nDocFound;
            int nDocSize = _pSearchRes->nDocSize;
            int validNum = 0;
            pReRow->Alloc(nDocFound);
            uint32_t *pKeyDocid = (uint32_t *)pReRow->getBuff();
            if (!pKeyDocid)
                return NULL;
            //pKeyDocid = _pSearchRes->nDocIds;
            //pReRow->setDocNum(nDocFound);
            for (int32_t i=0; i<nDocSize; i++) {
                if (_pSearchRes->nDocIds[i] != INVALID_DOCID) { //过滤无效值
                    pKeyDocid[validNum++] = _pSearchRes->nDocIds[i];
                }
            }
            pReRow->setDocNum(validNum);
        }
        else if ((strcasecmp(name, MNAME_RELESCORE)==0) ||
                (strcasecmp(name, "RL")==0) ||
                (strcasecmp(name, "rl")==0)){
            pReRow = AddVirtRow(MNAME_RELESCORE, DT_UINT32, sizeof(uint32_t));
            SDM_ROW *pKeyRow = getKeyRow();
            if (!pKeyRow) {
                return NULL;
            }
            int nDocFound = _pSearchRes->nDocFound;
            int nDocSize = _pSearchRes->nDocSize;
            int keyValidNum = pKeyRow->getDocNum();
            int validNum = 0;
            pReRow->Alloc(keyValidNum);
            uint32_t *pRl = (uint32_t *)pReRow->getBuff();
            if (!pRl)
                return NULL;
            for (int32_t i=0; i<nDocSize; i++){
                if (_pSearchRes->nDocIds[i] != INVALID_DOCID){ //过滤无效值
                    pRl[validNum++] = _pSearchRes->nRank[i];
                }
            }
            if (keyValidNum != validNum){
                return NULL;
            }
            pReRow->setDocNum(validNum);
        } 
        else if(strcasecmp(name, MNAME_LEVELFIELD) == 0) {
            pReRow = AddVirtRow(MNAME_LEVELFIELD, DT_INT32, sizeof(int32_t));
        }
        else {
            SDM_ROW *pKeyRow = getKeyRow();
            if (!pKeyRow){
                return NULL;
            }
            int keyValidNum = pKeyRow->getDocNum();
            pReRow = AddProfRow(name);//其它默认为PROFILE
            if (!pReRow)
                return NULL;
            pReRow->setDocNum(keyValidNum);
            pReRow->Alloc(keyValidNum);
            void *pTest = pReRow->getBuff();
            if (!pTest)     //失败
                return NULL;
        }
        if ((_keyRow) && (pReRow)){
            pReRow->setDocIds((uint32_t *)_keyRow->getBuff());
        }
        if ((strcasecmp(name, MNAME_DOCID )==0) && (pReRow)){
            pReRow->setDocIds((uint32_t *)pReRow->getBuff());
        }
        return pReRow;
    }

    //合并SDM
    int32_t SDM::Append(SDM **ppSDM, uint32_t size) {
        if (!ppSDM || size<=0) {
            return -1;
        }
        uint32_t rowDocNum = 0;
        uint32_t newDocNum = 0;
        uint32_t newLenSize = 0;
        const char *newName = NULL;
        sort_util::SDM_ROW *pSDMRow = NULL;
        sort_util::SDM_ROW *pSDMRowTmp = NULL;
        PF_DATA_TYPE dataType;
        ROW_TYPE rowType;
        //generate sdm row and calculate size
        for (uint32_t i=0; i<size; i++) {
            if (!ppSDM[i])
                continue;
            addDocNum(ppSDM[i]->getDocNum());
            _uniqNullCount += ppSDM[i]->getUniqNullCount();
            for (uint32_t j=0; j<ppSDM[i]->_rows.size(); j++) {
                pSDMRow = ppSDM[i]->_rows[j];
                newDocNum = pSDMRow->getDocNum();
                newLenSize = pSDMRow->getLenSize();
                newName = pSDMRow->rowName.c_str();
                rowType = pSDMRow->getRowType();
                dataType = pSDMRow->getDataType();
                if (rowType == RT_OTHER) {
                    pSDMRowTmp = AddOtherRow(newName, dataType, newLenSize);
                }
                else {
                    pSDMRowTmp = AddVirtRow(newName, dataType, newLenSize);
                }
                if (!pSDMRowTmp) {
                    continue;
                }
                pSDMRowTmp->addDocNum(newDocNum);
            }
        }
        Alloc();
        for (uint32_t i=0; i<size; i++) {
            if (!ppSDM[i])
                continue;
            for (uint32_t j=0; j<ppSDM[i]->_rows.size(); j++) {
                pSDMRow = ppSDM[i]->_rows[j];
                newName = pSDMRow->rowName.c_str();
                pSDMRowTmp = FindRow(newName);
                if (!pSDMRowTmp) {
                    continue;
                }
                memcpy((char *)pSDMRowTmp->getCurAddr(), 
                        (char *)pSDMRow->getBuff(), 
                        pSDMRow->getDocNum() * pSDMRow->getLenSize());
                pSDMRowTmp->addCurNum(pSDMRow->getDocNum());
            }
        }
        return 0;
    }

    int32_t SDM::FilterInvalidID(uint32_t nNum) {
        uint32_t nDocNum = getDocNum();
        SDM_ROW *pRow = NULL;
        SDM_ROW *pRowTmp = NULL;
        SDM_ROW *pDocIDRow = NULL;
        uint32_t rowDocNum = 0;
        uint32_t newDocNum = 0;
        uint32_t newLenSize = 0;
        uint32_t *pDocsID = NULL;
        const char *newName = NULL;
        PF_DATA_TYPE dataType;
        ROW_TYPE rowType = RT_VIRTUAL;
        if (nNum > 0) {
            nDocNum = nDocNum<nNum?nDocNum:nNum;
        }
        pDocIDRow = FindRow(MNAME_DOCID);
        if (pDocIDRow == NULL) {
            return -1;
        }
        pDocsID = (uint32_t *)pDocIDRow->getBuff();
        if (pDocsID == NULL) {
            return -1;
        }
        uint32_t nOffset = 0;
        //遍历每个SDM ROW
        for (uint32_t i=0; i<_rows.size(); i++) {
            pRow = _rows[i];
            pRowTmp = AddTmpRow(pRow);
            if (pRow->rowType == RT_OTHER) {
                continue;
            }
            else {
                pRowTmp->Alloc(nDocNum);
                nOffset = 0;
                for (uint32_t j=0; j<nDocNum; j++) {
                    if (pDocsID[j] == INVALID_DOCID) {
                        continue;
                    }
                    memcpy((char *)pRowTmp->getValAddr(nOffset), 
                            (char *)pRow->getValAddr(j), 
                            pRowTmp->getLenSize());
                    nOffset++;
                }
                pRowTmp->setDocNum(nOffset);
                pRow->setBuff((char *)pRowTmp->getBuff());
                pRow->setDocNum(nOffset);
                pDocIDRow = FindRow(MNAME_DOCID);
                if (pDocIDRow == NULL) {
                    return -1;
                }
                pRow->setDocIds((uint32_t *)pDocIDRow->getBuff());
            }
        }
        _docNum = nOffset;
        //Reset key row
        setKeyRow(getKeyRow());
        return nOffset;
    }

    void SDM::Alloc() 
    {
        for (uint32_t i=0; i<_rows.size(); i++) {
            _rows[i]->Alloc();
        }
        return ;
    }

    void SDM::Load()
    {
        for (uint32_t i=0; i<_rows.size(); i++) {
            _rows[i]->Load();
        }
        return ;
    }

    void SDM::Dump(FILE *pFile, bool bPrint)
    {
        if (!bPrint) {
            return;
        }
        FILE *pTmpFile = stderr;
        if (pFile != NULL) {
            pTmpFile = pFile;
        }
        fprintf(pFile, "\n##############################SDM DUMP BEGIN##############################\n");
        fprintf(pFile, "size=%d, docnum=%d, docsfound=%d, estdocsfound=%d\n", _rows.size(), getDocNum(), _docsFound, _estDocsFound);
        for (uint32_t i=0; i<_rows.size(); i++) {
            _rows[i]->Dump(pFile);
        }
        fprintf(pFile, "\nsort array list\n");
        for(uint32_t i=0; i<getDocNum(); i++) {
            fprintf(pFile, "%10d", _pSortCmpItem->pSortArray[i]);

        }
        fprintf(pFile, "\n");
        fprintf(pFile, "\n##############################SDM DUMP END################################\n");
        return;
    }

    int SDM::AddCmp(CompareInfo* pCmpInfo, SortQuery* pQuery)
    {
        SortCompare* pNewCmp = GetCmp(pCmpInfo);
        if (!pNewCmp){
            pNewCmp = NEW(_mempool, SortCompare)(_mempool, pCmpInfo, _pSortCmpItem);
            if (!pNewCmp){
                return -1;
            }
            if (pNewCmp->init(this, pQuery) <0) {
                TLOG("error in init SortCompare");
                return -1;
            }
            _cmpList.push_back(pNewCmp);
            _cmpMap.insert(std::make_pair(pCmpInfo, pNewCmp));
            return 0;
        }
        return 0;
    }

    SortCompare * SDM::GetCmp(CompareInfo* pCmpInfo)
    {
        std::map<CompareInfo*, SortCompare*>::const_iterator iter;
        iter = _cmpMap.find(pCmpInfo);
        if (iter != _cmpMap.end()){
            return iter->second;
        }
        return NULL;
    }

    int SDM::setKeyRow(SDM_ROW* pKeyRow)
    {
        if (!pKeyRow)
            return -1;
        _keyRow = pKeyRow;
        _docNum = _keyRow->getDocNum();
        _pSortCmpItem->docNum = _docNum;
        _pSortCmpItem->curDocNum = _docNum;
        _pSortCmpItem->pSortArray = NEW_VEC(_mempool, int32_t, _pSortCmpItem->docNum);
        for (int32_t i=0; i<_docNum; i++){
            _pSortCmpItem->pSortArray[i] = i;
        }
        _pSortCmpItem->sortBegNo = 0;
        _pSortCmpItem->sortEndNo = _docNum;

        //设置结果集合
        _pResArray = NEW_VEC(_mempool, uint32_t, _allRetNum);
        for (int32_t i=0; i<_allRetNum; i++) {
            _pResArray[i] = i;
        }
        _curResNum = 0;
        return 0;
    }

    int32_t SDM::Pop(uint32_t num) {
        if (num > _pSortCmpItem->curDocNum) {
            num = _pSortCmpItem->curDocNum;
        }
        if (num > (_allRetNum-_curResNum)) {
            num = _allRetNum-_curResNum;
        }
        if (num <=0) {
            return 0;
        }
        memcpy(&_pResArray[_curResNum], &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo], num*sizeof(uint32_t));
        _pSortCmpItem->sortBegNo += num;
        _pSortCmpItem->curDocNum -= num;
        _curResNum += num;
        return num;
    }

    int32_t SDM::serialize(char *str, uint32_t size) {
        uint32_t serializeCount = 0;
        int32_t totalLength = 0;
        uint32_t tmpLength = 0;
        uint32_t nTmpDocNum = 0;
        uint32_t nTypeSize = 0;
        PF_DATA_TYPE type;
        ROW_TYPE rowType;
        if (str == NULL) {
            return getSerialSize();
        }
        char *ptr = str;
        //set uniq null count
        totalLength += securityCopyTo(ptr, size, &_uniqNullCount, sizeof(uint32_t));
        //set docsFound
        totalLength += securityCopyTo(ptr, size, &_docsFound, sizeof(uint32_t));
        //set estDocsFound
        totalLength += securityCopyTo(ptr, size, &_estDocsFound, sizeof(uint32_t));
        //set startNo
        totalLength += securityCopyTo(ptr, size, &_startNo, sizeof(uint32_t));
        //set retNum
        totalLength += securityCopyTo(ptr, size, &_retNum, sizeof(uint32_t));
        //set allRetNum
        totalLength += securityCopyTo(ptr, size, &_allRetNum, sizeof(uint32_t));
        //set curResNum
        totalLength += securityCopyTo(ptr, size, &_curResNum, sizeof(uint32_t));
        //set debug info query len
        tmpLength = _sDebugInfo.length();
        totalLength += securityCopyTo(ptr, size, &tmpLength, sizeof(uint32_t));
        //set debug info query
        totalLength += securityCopyTo(ptr, size, _sDebugInfo.c_str(), tmpLength);
        for (uint32_t i=0; i<_rows.size(); i++) {
            /*
               if (!_rows[i]->needSerialize())
               continue;
               */
            if (_rows[i]->getDocNum() <= 0) {
                continue;
            }
            serializeCount++;
        }
        //set row num
        totalLength += securityCopyTo(ptr, size, &serializeCount, sizeof(uint32_t));
        for (uint32_t i=0; i<_rows.size(); i++) {
            /*
               if (!_rows[i]->needSerialize())
               continue;
               */
            if (_rows[i]->getDocNum() <= 0) {
                continue;
            }
            //set row name
            tmpLength = _rows[i]->rowName.length();
            totalLength += securityCopyTo(ptr, size, &tmpLength, sizeof(uint32_t));
            totalLength += securityCopyTo(ptr, size, _rows[i]->rowName.c_str(), tmpLength);
            //set row type
            rowType = _rows[i]->rowType;
            totalLength += securityCopyTo(ptr, size, &rowType, sizeof(ROW_TYPE));
            //set data type
            type = _rows[i]->dataType;
            totalLength += securityCopyTo(ptr, size, &type, sizeof(PF_DATA_TYPE));
            //set data type size
            nTypeSize = _rows[i]->getLenSize();
            totalLength += securityCopyTo(ptr, size, &nTypeSize, sizeof(uint32_t));
            //get doc num
            nTmpDocNum = (_rows[i]->getRowType()==RT_OTHER) ? _rows[i]->getDocNum():_curResNum;
            if (type != DT_STRING) {
                //set row length
                tmpLength = _rows[i]->getLenSize() * nTmpDocNum;
                totalLength += securityCopyTo(ptr, size, &tmpLength, sizeof(uint32_t));            
                //set row
                totalLength += securityCopyTo(ptr, size, _rows[i]->getBuff(), tmpLength);
            }
            else {
                totalLength += securityCopyTo(ptr, size, &nTmpDocNum, sizeof(uint32_t));
                //DT_STRING类型的需特殊处理
                char **pRowPtr = (char **)_rows[i]->getBuff();
                uint32_t nTmpRowLength = 0;
                if (pRowPtr == NULL) {
                    continue;
                }
                for (int32_t h=0; h<nTmpDocNum; h++) {
                    nTmpRowLength = 0;
                    if (pRowPtr[h] != NULL) {
                        nTmpRowLength = strlen(pRowPtr[h]);
                    }
                    //Set row length
                    totalLength += securityCopyTo(ptr, size, &nTmpRowLength, sizeof(uint32_t));
                    //Set row
                    if (nTmpRowLength > 0)
                        totalLength += securityCopyTo(ptr, size, pRowPtr[h], nTmpRowLength);
                }
            }
        }
        return totalLength;
    }

    int32_t SDM::getSerialSize() {
        if ((_docNum == 0) || (_curResNum == 0)) {
            return 0;
        }
        uint32_t nTmpDocNum = 0;
        uint32_t totalLength = 0;
        //add uniq null count size
        totalLength += sizeof(uint32_t);
        //set docsFound
        totalLength += sizeof(uint32_t);
        //set estDocsFound
        totalLength += sizeof(uint32_t);
        //set startNo
        totalLength += sizeof(uint32_t);
        //set retNum
        totalLength += sizeof(uint32_t);
        //set allRetNum
        totalLength += sizeof(uint32_t);
        //set curResNum
        totalLength += sizeof(uint32_t);
        //set debug info query len
        totalLength += sizeof(uint32_t);
        //set debug info query
        totalLength += _sDebugInfo.length();
        //get rownum size
        totalLength += sizeof(uint32_t);
        for (uint32_t i=0; i<_rows.size(); i++)
        {
            if (_rows[i]->getDocNum() <= 0) {
                continue;
            }
            /*
               if (!_rows[i]->needSerialize()) {
               continue;
               }
               */
            //get row name size
            totalLength += sizeof(uint32_t);
            totalLength += _rows[i]->rowName.length();
            //get row type
            totalLength += sizeof(ROW_TYPE);
            //get data type
            totalLength += sizeof(PF_DATA_TYPE);
            //get data type size
            totalLength += sizeof(uint32_t);
            //get doc num
            nTmpDocNum = (_rows[i]->getRowType()==RT_OTHER) ? _rows[i]->getDocNum():_curResNum;
            if (_rows[i]->dataType != DT_STRING) {
                //set row length
                totalLength += sizeof(uint32_t);
                totalLength += _rows[i]->getLenSize() * nTmpDocNum;
            } 
            else {
                //Set Doc Num
                totalLength += sizeof(uint32_t);
                char **pRowPtr = (char **)_rows[i]->getBuff();
                uint32_t nTmpRowLength = 0;
                if (pRowPtr == NULL) {
                    continue;
                }
                for (int32_t h=0; h<nTmpDocNum; h++) {
                    nTmpRowLength = 0;
                    if (pRowPtr[h] != NULL)
                        nTmpRowLength = strlen(pRowPtr[h]);
                    //Set row length
                    totalLength += sizeof(uint32_t);
                    //Set row
                    totalLength += nTmpRowLength;
                }
            }
        }
        return totalLength;    
    }


    int32_t SDM::deserialize(char *str, uint32_t size) {
        if (NULL == str || 0 == size) {
            return 0;
        }
        uint32_t len = size;
        uint32_t total=0;
        char *ptr = str;
        uint32_t serializeCount = 0;
        uint32_t fieldsize = 0;
        uint32_t rowsize = 0;
        uint32_t docnum = 0;
        uint32_t tmpLength = 0;
        uint32_t nTmpRowLength = 0;
        uint32_t nTmpDocNum = 0;
        SDM_ROW *tmpRow = NULL;
        ROW_TYPE rowType;
        PF_DATA_TYPE type;
        //get uniq null count
        total += securityCopyFrom(&_uniqNullCount, sizeof(uint32_t), ptr, len);
        //get docsFound
        total += securityCopyFrom(&_docsFound, sizeof(uint32_t), ptr, len);
        //get estDocsFound
        total += securityCopyFrom(&_estDocsFound, sizeof(uint32_t), ptr, len);
        //get startNo
        total += securityCopyFrom(&_startNo, sizeof(uint32_t), ptr, len);
        //get retNum
        total += securityCopyFrom(&_retNum, sizeof(uint32_t), ptr, len);
        //get allRetNum
        total += securityCopyFrom(&_allRetNum, sizeof(uint32_t), ptr, len);
        //get curResNum
        total += securityCopyFrom(&_curResNum, sizeof(uint32_t), ptr, len);
        //get debug info query len
        total += securityCopyFrom(&tmpLength, sizeof(uint32_t), ptr, len);
        if (tmpLength != 0) {
            //set debug info query
            char *pDebug = NEW_VEC(_mempool, char, tmpLength+1);
            memset(pDebug , 0x0, tmpLength+1);
            total += securityCopyFrom(pDebug, tmpLength, ptr, len);
            _sDebugInfo = pDebug;
        }
        //get row num
        total += securityCopyFrom(&serializeCount, sizeof(uint32_t), ptr, len);
        for (uint32_t i=0; i<serializeCount; i++) {
            char *pName = NULL;
            uint32_t namelength = 0;
            //get row name length
            total += securityCopyFrom(&namelength, sizeof(uint32_t), ptr, len);//名字长度
            pName = NEW_VEC(_mempool, char, namelength+1);
            //get row name
            total += securityCopyFrom(pName, namelength, ptr, len);//名字长度
            pName[namelength] = '\0';
            //get row type
            total += securityCopyFrom(&rowType, sizeof(ROW_TYPE), ptr, len);
            //get data type
            total += securityCopyFrom(&type, sizeof(PF_DATA_TYPE), ptr, len);//属性信息
            //get data type size
            total += securityCopyFrom(&fieldsize, sizeof(uint32_t), ptr, len);//获取内容长度
            if (type != DT_STRING) {
                //get row size
                total += securityCopyFrom(&rowsize, sizeof(uint32_t), ptr, len);//获取内容长度
                //create row
                tmpRow = AddVirtRow(pName, type, fieldsize);
                if (tmpRow == NULL) {
                    return 0;
                }
                if (strncmp(pName, MNAME_DOCID, strlen(MNAME_DOCID)) == 0)
                    docnum = rowsize/fieldsize;
                tmpRow->setDocNum(rowsize/fieldsize);
                tmpRow->Alloc();
                //get row content
                total += securityCopyFrom(tmpRow->getBuff(), rowsize, ptr, len);//获取内容
            }
            else {
                //get doc num
                total += securityCopyFrom(&nTmpDocNum, sizeof(uint32_t), ptr, len);
                //Create row
                tmpRow = AddVirtRow(pName, type, fieldsize);
                if (tmpRow == NULL) {
                    return 0;
                }
                tmpRow->setDocNum(nTmpDocNum);
                tmpRow->Alloc();
                char **pTmpString = (char **)tmpRow->getBuff();
                for (int32_t h=0; h<nTmpDocNum; h++) {
                    total += securityCopyFrom(&nTmpRowLength, sizeof(uint32_t), ptr, len);
                    if (nTmpRowLength <= 0) {
                        pTmpString[h] = NULL; 
                        continue;
                    }
                    pTmpString[h] = NEW_VEC(_mempool, char, nTmpRowLength+1);
                    if (pTmpString[h] == NULL)
                        return -1;
                    total += securityCopyFrom(pTmpString[h], nTmpRowLength, ptr, len); 
                    pTmpString[h][nTmpRowLength] = '\0';
                }
            }
            if (rowType == RT_OTHER) {
                tmpRow->setRowType(RT_OTHER);
            }
        }
        setDocNum(docnum);
        return total;
    }

    int32_t SDM::FullRes() {
        SDM_ROW *pRowTmp = NULL;
        SDM_ROW *pRow = NULL;
        for (uint32_t i=0; i<_rows.size(); i++) {
            pRow = _rows[i];
            if (pRow->rowType == RT_OTHER) {
                continue;
            }
            else {
                pRowTmp = AddTmpRow(pRow);
                pRowTmp->Alloc(_curResNum);
                if ((_rows[i]->getRowType()==RT_PROFILE) && !_rows[i]->isLoad()) {
                    //未读取的profile字段，需要在这里读取
                    for (uint32_t j=0; j<_curResNum; j++) {
                        pRow->Read(_pResArray[j]);
                        memcpy((char *)pRowTmp->getValAddr(j),
                                (char *)pRow->getValAddr(_pResArray[j]),
                                pRowTmp->getLenSize());
                    }
                }
                else {
                    if (pRow->getDataType() != DT_STRING) {
                        for (uint32_t j=0; j<_curResNum; j++) {
                            //pRowTmp->Set(j, pRow->Get(_pResArray[j]));
                            memcpy((char *)pRowTmp->getValAddr(j),
                                    (char *)pRow->getValAddr(_pResArray[j]),
                                    pRowTmp->getLenSize());
                        }
                    }
                    else {
                        char **ppTmp = (char **)pRowTmp->getBuff();
                        char **ppRow = (char **)pRow->getBuff();
                        if (!ppTmp || !ppRow)
                            continue;
                        for (uint32_t j=0; j<_curResNum; j++) {
                            ppTmp[j] = ppRow[_pResArray[j]];
                        }
                    }
                }
                pRowTmp->setDocNum(_curResNum);
                pRow->setBuff((char *)pRowTmp->getBuff());
                pRow->setDocNum(_curResNum);
                _docNum = _curResNum;
            }
        }
        return 0;
    }

    /*int SDM::PrePare()
      {
      }*/

}
