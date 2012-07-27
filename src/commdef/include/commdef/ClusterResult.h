#ifndef _CLUSTER_RESULT_H_
#define _CLUSTER_RESULT_H_
#include <stdio.h>
#include "util/common.h"

//#define NULL 0

namespace sort_framework {
    class SortResult;
    class UniqIIResult;
}

namespace statistic {
    class StatisticResult;
}

namespace queryparser {
    class QPResult;
}

class Document;

class DebugInfo;

namespace commdef {

class ClusterResult {
    public:
        ClusterResult()
            : _pSortResult(NULL),
            _pQPResult(NULL),
            _pStatisticResult(NULL),
            _nUniqCounts(NULL),
            _pUniqIIResult(NULL),
            _pLightKey(NULL),
            _pNID(NULL),
            _pServerID(NULL),
            _szDistCounts(NULL),
            _ppDocuments(NULL),
            _pDebugInfo(NULL)
    {}
        sort_framework::SortResult *_pSortResult;
        queryparser::QPResult *_pQPResult;
        statistic::StatisticResult *_pStatisticResult;
        uint32_t _nSortBufLen;
        char *_szSortBuffer;           //排序
        uint32_t _nStatBufLen;
        char *_szStatBuffer;           //统计
        int32_t *_nUniqCounts;           //distinct排序时需要uniq(唯一的),即按照制定字段distinct(field)
        sort_framework::UniqIIResult *_pUniqIIResult;
        uint32_t _nUniqBufLen;
        char *_szUniqBuffer;
        uint32_t _nParaBufLen;
        char *_szParaBuffer;
        uint32_t _nSrcBufLen;
        char *_szSrcBuffer;
        uint32_t _nOrsBufferLen;
        char *_szOrsBuffer;
        int32_t _nDocsSearch;
        int32_t _nDocsFound;
        int32_t _nEstimateDocsFound;
        int32_t _nDocsRestrict;
        int32_t _nDocsReturn;
        const char *_szClusterName;
        int32_t nServerId;                //服务器Id
        int32_t *_szDocs;
        char *_pEchoKeys;
        char *_pLightKey;
        int64_t *_pNID;
        int32_t *_pServerID;
        int64_t *_szFirstRanks;
        int64_t *_szSecondRanks;
        int32_t *_szDistCounts;
        int32_t *_szUnionRelates;
        unsigned char *_pOnlineStatus;
        int32_t *_nPos;
        Document **_ppDocuments;
        char *_uniqFieldValue;
        uint64_t *_uniqFieldHash;
        int32_t _nClusterBufLen;
        char * _szClusterBuffer;
        DebugInfo *_pDebugInfo;
};

}
#endif
