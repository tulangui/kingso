#include "AppManage.h"
#include "Util/SortCompare.h"
#include "Application/HeapSort.h"
#include <map>
#include <vector>
using namespace sort_application;
using namespace sort_util;
namespace sort_framework{
#define TRUNCATE "truncate"

    AppManage::AppManage()
    {
    }

    AppManage::~AppManage()
    {
        uint32_t i; 
        for (i=0; i<_sortAppList.size(); i++){
            delete _sortAppList[i]->pModeHandle;
            delete _sortAppList[i];
        }
    }

    SortModeBase* AppManage::CreateModule(const char * pModeName)
    {
        SortModeBase* pret = NULL;
        if (!pModeName)
            return NULL;
        else if (strcasecmp("HeapSort",pModeName) == 0){
            pret = new HeapSort;
        }
        return pret;
    }

    int AppManage::Init(SortConfig & sortcfg)
    {
        uint32_t i = 0;
        const std::vector<ApplicationInfo*> & appInfoList = sortcfg.getAppInfoList(); 

        for (i=0; i<appInfoList.size();i++) {
            SortModeBase* pMod = CreateModule(appInfoList[i]->Type);
            if (pMod) {
                pMod->init(appInfoList[i]->xNode, &_Operate);
                AppHandle* pTmpHandle= new AppHandle;
                pTmpHandle->pAppName = appInfoList[i]->Name;
                pTmpHandle->pModeName = appInfoList[i]->Type;
                pTmpHandle->isFinal = appInfoList[i]->isFinal;
                pTmpHandle->isTruncate = appInfoList[i]->isTruncate;
                pTmpHandle->pCmpInfo = appInfoList[i]->pCmpInfo;
                pTmpHandle->pCondInfos = &appInfoList[i]->CondInfos;
                pMod->setCmpInfo(appInfoList[i]->pCmpInfo);
                pTmpHandle->pModeHandle = pMod;
                _sortAppList.push_back(pTmpHandle);
            }
            else {
                TLOG("Create sort %s failed.", appInfoList[i]->Type);
            }
        }
        return 0;
    }

    const char *AppManage::getBranchAttr(SortConfig *pConfig, SortQuery *pQuery)
    {
        if (!pQuery || !pConfig)
            return 0;
        uint32_t i = 0;
        uint32_t j = 0;
        int renum = 0;
        std::string szCondName;
        std::string szCondNameTmp;
        std::string szCondVal;
        std::string szCondType;
        std::string szBranchSpec;
        bool isMatch;
        for (i=0; i<_sortAppList.size(); i++){
            isMatch = true;
            std::vector<ConditionInfo*> * pCondInfos = _sortAppList[i]->pCondInfos;
            for (j=0; j<pCondInfos->size(); j++){
                szCondName = (*pCondInfos)[j]->Name;
                szCondType = (*pCondInfos)[j]->Type;
                if (szCondName == "*")
                    continue;
                if (szCondType == "noparam") {
                    if (pQuery->findParam(szCondName, szCondVal)) {
                        isMatch = false;
                        break;
                    }
                    else {
                        continue;
                    }
                }
                StringSplit ssName(szCondName, "|");
                for (int32_t k=0; k<ssName.size(); k++) {
                    if (!pQuery->findParam(ssName[k], szCondVal)) {
                        if (!strcmp(ssName[k], "ps") || !strcmp(ssName[k], "ss")) {
                            szCondNameTmp = "_";
                            szCondNameTmp += ssName[k];
                            if (!pQuery->findParam(szCondNameTmp, szCondVal)) {
                                isMatch = false;
                                continue;
                            }
                        }
                        else {
                            isMatch = false;
                            continue;
                        }
                    }
                    if (((*pCondInfos)[j]->Value) &&
                            (szCondVal != (*pCondInfos)[j]->Value))
                    {
                        isMatch = false;
                        continue;
                    }
                    isMatch = true;
                    break;
                }
                if (!isMatch) {
                    break;
                }
            }
            if (isMatch){
                if (szBranchSpec != "") {
                    szBranchSpec += " ";
                }
                szBranchSpec += _sortAppList[i]->pAppName;
                if (_sortAppList[i]->isFinal) {
                    break;
                }
            }
        }
        if (szBranchSpec != "") {
            const std::vector<CustomInfo*> vCusInfo = pConfig->getCusInfoList();
            for (int32_t i=0; i < vCusInfo.size(); i++) {
                if (szBranchSpec == vCusInfo[i]->BrancheSpec) {
                    if (vCusInfo[i]->isTruncate) {
                        return TRUNCATE;
                    }
                    return NULL;
                }
            }
        }
        return NULL;
    }

    int AppManage::DecisionMaker(SortConfig & sortcfg, SortQuery* pQuery, std::vector<AppHandle*>& curAppList )
    {
        if (!pQuery)
            return 0;
        uint32_t i,j;
        int renum = 0;
        std::string szCondName;
        std::string szCondNameTmp;
        std::string szCondVal;
        std::string szCondType;
        bool isMatch;
        for (i=0; i<_sortAppList.size(); i++){
            isMatch = true;
            std::vector<ConditionInfo*> * pCondInfos = _sortAppList[i]->pCondInfos; 
            for (j=0; j<pCondInfos->size(); j++){
                szCondName = (*pCondInfos)[j]->Name;
                szCondType = (*pCondInfos)[j]->Type;
                if (szCondName == "*")
                    continue;
                if (szCondType == "noparam") {
                    if (pQuery->findParam(szCondName, szCondVal)) {
                        isMatch = false;
                        break;
                    }
                    else {
                        continue;
                    }
                }
                StringSplit ssName(szCondName, "|");
                for (int32_t k=0; k<ssName.size(); k++) {
                    if (!pQuery->findParam(ssName[k], szCondVal)) {
                        if (!strcmp(ssName[k], "ps") || !strcmp(ssName[k], "ss")) {
                            szCondNameTmp = "_";
                            szCondNameTmp += ssName[k];
                            if (!pQuery->findParam(szCondNameTmp, szCondVal)) {
                                isMatch = false;
                                continue;
                            }
                        }
                        else {
                            isMatch = false;
                            continue;
                        }
                    }
                    if (((*pCondInfos)[j]->Value) &&
                            (szCondVal != (*pCondInfos)[j]->Value))
                    {
                        isMatch = false;
                        continue;
                    }
                    isMatch = true;
                    break;
                }
                if (!isMatch) {
                    break;
                }
            }
            if (isMatch){
                curAppList.push_back(_sortAppList[i]);
                renum++;
                if (_sortAppList[i]->isFinal) {
                    break;
                }
            }
        }
        return renum;
    }

    int32_t transDocIdToNId(SDM *pSDM)
    {
        if (pSDM == NULL) {
            TLOG("transDocIdToNId, input pSDM is NULL");
            return -1;
        }
        index_lib::ProfileManager *pm = index_lib::ProfileManager::getInstance();
        if (!pm) {
            TLOG("Get ProfileManager instance error");
            return -1;
        }
        index_lib::ProfileDocAccessor *pd = pm->getDocAccessor();
        if (!pd) {
            TLOG("Get DocAccessor error");
            return -1;
        }
        const index_lib::ProfileField * pf = pd->getProfileField("nid");
        if (!pf) {
            TLOG("Get nid ProfileField error");
            return -1;
        }
        SDM_ROW *pDocsIdRow = pSDM->FindRow(MNAME_DOCID);
        if (pDocsIdRow == NULL) {
            TLOG("Get doc id row error.");
            return -1;
        }
        uint32_t *pDocsId = (uint32_t *)pDocsIdRow->getBuff();
        //添加Nid数组到pSDM中
        sort_util::SDM_ROW *pNidRow =
            pSDM->AddVirtRow(MNAME_NID, DT_INT64, sizeof(int64_t));
        if (!pNidRow) {
            TLOG("Add nid row error");
            return -1;
        }
        int64_t nNid = 0;
        uint32_t nDocNum = pSDM->getDocNum();
        pNidRow->Alloc(nDocNum, sizeof(int64_t));
        if (nDocNum == 0) {
            return 0;
        }
        for (uint32_t i=0; i<nDocNum; i++) {
            if (pDocsId[i] == INVALID_DOCID) {
                nNid = INVALID_NID;
            }
            else {
                nNid = pd->getInt64(pDocsId[i], pf);
            }
            pNidRow->Set(i, (double)nNid);
        }
        //添加uniq2中的Nid数组
        sort_util::SDM_ROW *pUniqIIDocID = pSDM->FindRow(MNAME_UNIQII_INFO_ID);
        if (pUniqIIDocID != NULL) {
            sort_util::SDM_ROW *pUniqIINidRow =
                pSDM->AddOtherRow(MNAME_UNIQII_INFO_NID, DT_INT64, sizeof(int64_t));
            if (!pUniqIINidRow) {
                TLOG("Add uniqII nid row error");
                return -1;
            }
            int64_t nNid = 0;
            uint32_t *pDocsId = (uint32_t *)pUniqIIDocID->getBuff();
            uint32_t nDocNum = pUniqIIDocID->getDocNum();
            pUniqIINidRow->Alloc(nDocNum, sizeof(int64_t));
            for (uint32_t i=0; i<nDocNum; i++) {
                if (pDocsId[i] == INVALID_DOCID) {
                    nNid = INVALID_NID;
                }
                else {
                    nNid = pd->getInt64(pDocsId[i], pf);
                }
                pUniqIINidRow->Set(i, (double)nNid);
            }
        }
        return 0;
    }

    SDM* AppManage::doProcessOnSearcher(SortConfig & sortcfg, SortQuery* pQuery, 
            SearchResult &searchRes, MemPool *mempool)
    {
        int32_t ret,i;
        int32_t retNum = 0;
        std::vector<AppHandle*> curAppList; 
        SDM* pSDM = NEW(mempool, SDM)(mempool, pQuery, &searchRes);
        //如果DocsFound为0，不需要再走sort流程
        if (pSDM->getDocsFound() == 0) {
            return pSDM;
        }
        //生成排序链条
        ret = DecisionMaker(sortcfg, pQuery, curAppList);
        //设置主键
        SDM_ROW* pRow = pSDM->AddRow(MNAME_DOCID);
        if (!pRow) {
            TLOG("Load MNAME_DOCID error");
            return NULL;
        }
        pSDM->setKeyRow(pRow);
        if (ret > 0) {
            //准备compare
            for (i=0; i<ret; i++) {
                if (pSDM->AddCmp(curAppList[i]->pCmpInfo, pQuery)<0){
                    TLOG("error in AddCmp");
                    return NULL;
                }
            }
            for (i=0; i<ret; i++){
                curAppList[i]->pModeHandle->prepare(pSDM );
            }
            for (i=0; i<ret; i++){
                retNum = curAppList[i]->pModeHandle->process(pSDM, pQuery, &searchRes, mempool);
                if (retNum < 0) {
                    TLOG("error in appname = %s, modename = %s", curAppList[i]->pAppName, curAppList[i]->pModeName);
                    return NULL;
                }
            }
        }
        //fullresult
        pSDM->FullRes();

        //根据docid生成nid
        if (transDocIdToNId(pSDM) != 0) {
            TLOG("transDocIdToNId error");
            return NULL;
        }
        return pSDM;
    }

    SDM* AppManage::doProcessOnMerger(SortConfig &sortcfg, SortQuery *pQuery, 
            commdef::ClusterResult **ppcr, uint32_t arrSize, MemPool *mempool)
    {
        int32_t i = 0;
        int32_t offset = 0;
        int32_t num = 0;
        int32_t retNum = 0;
        uint32_t nDocsFound = 0;
        uint32_t nEstimateDocsFound = 0;
        bool isBlender = false;
        if (arrSize == 1) {
            isBlender = true;
        }
        std::vector<AppHandle*> curAppList; 
        //生成排序链
        num = DecisionMaker(sortcfg, pQuery, curAppList);
        SDM *pSDM = NEW(mempool, SDM)(mempool, pQuery);
        SDM *pSDMTMP = NULL;
        uint32_t nDocNum = 0;
        SDM **ppSDM = NEW_VEC(mempool, SDM *, arrSize);
        memset(ppSDM, 0x0, sizeof(SDM *) * arrSize);
        uint32_t nSdmNum = 0;
        //合并各列数据
        for (uint32_t i=0; i<arrSize; i++) {
            if (!ppcr[i]) {
                continue;
            }
            nDocsFound += ppcr[i]->_nDocsFound;
            nEstimateDocsFound += ppcr[i]->_nEstimateDocsFound;
            if (!ppcr[i]->_pSortResult || !ppcr[i]->_pSortResult->_sdm) {
                continue;
            }
            pSDMTMP = ppcr[i]->_pSortResult->_sdm;
            nDocNum = pSDMTMP->getDocNum();
            //Create ServerID Row
            sort_util::SDM_ROW *pSidRow = pSDMTMP->FindRow(MNAME_SERVERID);
            if (pSidRow == NULL) {
                pSidRow = pSDMTMP->AddVirtRow(MNAME_SERVERID, DT_UINT32, sizeof(uint32_t));
            }
            pSidRow->Alloc(nDocNum, sizeof(uint32_t));
            for (uint32_t j=0; j<nDocNum; j++) {
                pSidRow->Set(j, (double)i);
            }
            //Create Pos Row
            sort_util::SDM_ROW *pPosRow = pSDMTMP->FindRow(MNAME_RESULT_POS);
            if (pPosRow == NULL) {
                pPosRow = pSDMTMP->AddVirtRow(MNAME_RESULT_POS, DT_UINT32, sizeof(uint32_t));
            }
            pPosRow->Alloc(nDocNum, sizeof(uint32_t));
            for (uint32_t j=0; j<nDocNum; j++) {
                pPosRow->Set(j, (double)offset);
                offset++;
            }
            ppSDM[nSdmNum++] = pSDMTMP;
        }
        if (nSdmNum <= 0) {
            SDM *pSDM = NEW(mempool, SDM)(mempool);
            pSDM->setDocsFound(nDocsFound);
            pSDM->setEstimateDocsFound(nEstimateDocsFound);
            pSDM->setDocNum(0);
            return pSDM;
        }
        if (nDocsFound <= 0) {
            return pSDM;
        }
        pSDM->Append(ppSDM, nSdmNum);
        pSDM->setDocsFound(nDocsFound);
        pSDM->setEstimateDocsFound(nEstimateDocsFound);
        //设置主键
        SDM_ROW* pRow = pSDM->FindRow(MNAME_NID);
        if (!pRow) {
            return NULL;
        }
        pSDM->setKeyRow(pRow);

        if (num > 0) {
            //准备compare
            for (i=0; i<num; i++) {
                if (pSDM->AddCmp(curAppList[i]->pCmpInfo, pQuery)<0) {
                    TLOG("Add compare error");
                    return NULL;
                }
            }
            for (i=0; i<num; i++) {
                retNum = curAppList[i]->pModeHandle->process(pSDM, pQuery, NULL, mempool);
                //TLOG("appname = %s, modename = %s", curAppList[i]->pAppName, curAppList[i]->pModeName);
                if (retNum < 0) {
                    return NULL;
                }
            }
        }
        //fullresult
        pSDM->FullRes();
        return pSDM;
    }
}
