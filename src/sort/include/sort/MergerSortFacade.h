#ifndef _SORT_MERGER_SORT_H_
#define _SORT_MERGER_SORT_H_
#include <stdint.h>
#include "index_lib/ProfileManager.h"
#include "commdef/SearchResult.h"
#include "queryparser/Param.h"
#include "sort/SortResult.h"
#include "framework/Context.h"
#include "commdef/ClusterResult.h"

#define SORT_BEGIN namespace sort_framework {
#define SORT_END }
#define SORT sort_framework

namespace sort_framework {

class SortConfig;
class AppManage;

class MergerSortFacade
{
public:
    MergerSortFacade();
    ~MergerSortFacade();
public:
    /**
     * 初始化 : 事先加载所有的排序类
     * @prarm  szSortCfgFile, 配置文件路径
     * @return  0, succeed
     * @return  else, failed
     */
    int init( const char *szSortCfgFile);
    /**
     * 排序模块的入口，驱动整个sort模块流程
     * @param context,
     * @param param, queryparser对query的解析数据
     * @praram ppcr, 多列searcher返回的结果
     * @praram arrSize, 返回结果的列数
     * @param ret, sort之后得到的结果
     * @return  0, succeed
     * @return  else, failed
     */
    int doProcess(const framework::Context & context,
            const queryparser::Param & param,
            commdef::ClusterResult **ppcr, uint32_t arrSize, 
            sort_framework::SortResult &ret);

    const char *getBranchAttr(const queryparser::Param &param) const;
private:
    SortConfig *_pConfig;
    AppManage *_pAppManage;
};

}

#endif //_SORT_MERGER_SORT_H_
