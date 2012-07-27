#ifndef _SORT_MODULE_BASE_H_
#define _SORT_MODULE_BASE_H_
#include <mxml.h>
#include "SortConfig.h"
#include "SortQuery.h"
#include "commdef/SearchResult.h"
#include "Util/SDM.h"
#include "Util/Operate/SortOperate.h"

using namespace sort_util;
namespace sort_application{
class SortModeBase
{
public:
    SortModeBase(){}
    //SortModeBase(const char* pszName): _pszName(pszName) {}
    virtual ~SortModeBase(){}
    virtual int init(mxml_node_t* xNode, SortOperate* _pOperate) = 0;
    virtual int prepare(SDM* pSDM) = 0;
    virtual int process(SDM* pSDM, sort_framework::SortQuery* pQuery,
            SearchResult *pResult, MemPool *mempool) = 0;
    const char * GetAppName();
    void setCmpInfo(sort_framework::CompareInfo* pCompareInfo){ _pCompareInfo = pCompareInfo; }
protected:
//    const char * _pszName;
    sort_framework::CompareInfo* _pCompareInfo;
    SortOperate* _pOperate;
};

}
#endif
