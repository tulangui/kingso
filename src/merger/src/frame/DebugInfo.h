#include <string>
#include <sstream>
#include "framework/namespace.h"
#include "sort/SortResult.h"


class DebugInfo 
{
    public:
        DebugInfo(const queryparser::Param *pParam);
        int32_t genDebugInfo(sort_framework::SortResult *pSortResult);
        char *getDebugInfoDocs(uint32_t num) const { return _debugInfoDocs[num]; }
        void setDocsCount(int32_t num) { _nDocsCount = num;  }
        int32_t getDocsCount() const { return _nDocsCount; }
        char *getDebugInfoQuery() const { return _debugInfoQuery; }
        char *getDebugInfoGlobal() const { return _debugInfoGlobal; }
        bool isDebug() const { return _isDebug; }
    //private:
        char **_debugInfoDocs;
        int32_t _nDocsCount;
        char *_debugInfoQuery;
        char *_debugInfoGlobal;
        bool _isDebug;
        bool _isDebugTrace;
        int32_t _nDebugLevel;
};

