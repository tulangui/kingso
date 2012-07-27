#ifndef __SORT_CONFIG_H_
#define __SORT_CONFIG_H_
#include <mxml.h>
#include "SortCfgInfo.h"
#include "SortQuery.h"
#include "util/Log.h"
namespace sort_framework {

#define XML_COMPARES_FLAG "compares"
#define XML_COMPARE_FLAG "compare"
#define XML_APPLICATIONS_FLAG "applications"
#define XML_APPLICATION_FLAG "application"
#define XML_CUSTOMS_FLAG "customs"
#define XML_CUSTOM_FLAG "custom"

class SortConfig
{
public:
    SortConfig();
    ~SortConfig();
    int parse(const char* filename);
    int parseCompare(mxml_node_t* xnode);
    int parseApplication(mxml_node_t* xnode);
    int parseCustom(mxml_node_t* xnode);
    const std::vector<ApplicationInfo*> & getAppInfoList() const { return _AppInfoList; }
    //未来增加get
    const std::vector<CustomInfo*> & getCusInfoList() const { return _CusInfoList; }
private:
    mxml_node_t * _pTopNode;
    std::vector<ApplicationInfo*> _AppInfoList;
    std::vector<CompareInfo*> _CmpInfoList;
    std::vector<CustomInfo*> _CusInfoList;
    CmpFieldInfo* _pDefCmpFiled;
    ApplicationInfo* GetAppInfo(const char* pAppName);
    CompareInfo* GetCmpInfo(const char* pCmpName);
    void splitString(const char * p, std::vector<std::string> & out);
};

}
#endif
