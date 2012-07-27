#include "DebugInfo.h"

DebugInfo::DebugInfo(const queryparser::Param *pParam):
    _debugInfoDocs(NULL),
    _nDocsCount(0),
    _debugInfoQuery(NULL),
    _debugInfoGlobal(NULL),
    _isDebug(false),
    _isDebugTrace(false),
    _nDebugLevel(0)
{
    const char *pTmp = NULL;
    if (pParam == NULL)
        return;
    pTmp = pParam->getParam("debug");
    if ((pTmp!=NULL) && (strcasecmp(pTmp, "TRUE")==0)) {
        _isDebugTrace = true;
        pTmp = pParam->getParam("debuglevel");
        if (pTmp != NULL) {
            _nDebugLevel = atoi(pTmp);
            if (_isDebugTrace && (_nDebugLevel>=1))
                _isDebug = true;
        }
    }
    return;
}

int32_t DebugInfo::genDebugInfo(sort_framework::SortResult *pSortResult)
{
    if (pSortResult == NULL)
        return KS_EFAILED;
    int32_t num = pSortResult->getDebugInfoQuery(_debugInfoQuery);
    if (num < 0) {
        return KS_EFAILED;
    }
    num = pSortResult->getDebugInfoDocs(_debugInfoDocs);
    if (num < 0) {
        return KS_EFAILED;
    }
    _nDocsCount = num;
    /*
       std::ostringstream ossdebug;
       ossdebug << "Merge_Phase1DispatchLatency=";
       ossdebug << (_session.getCurrentTime() - _session.getLastTime()) / 1000 << "ms\n";
       for (int idebug = 0; idebug < req->size(); idebug++) {
       FRAMEWORK::ANETRequest *anetreq = req->getANETRequest(idebug);
       clustermap::CMISNode *node = anetreq->getRequestNode();
       ossdebug << "Search=" << node->m_ip << "\n";
       }
       std::string strdebug = ossdebug.str();
       _debugInfoGlobal = NEW_VEC(heap, char, strdebug.size()+1);
       memcpy(_debugInfoGlobal, strdebug.c_str(), strdebug.size());
       _debugInfoGlobal[strdebug.size()] = '\0';
       */
    return KS_SUCCESS;
}


