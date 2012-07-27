#ifndef _APPLICATION_MANAGE_H_
#define _APPLICATION_MANAGE_H_
#include "SortConfig.h"
#include "sort/SortResult.h"
#include "Application/SortModeBase.h"
#include <map>
#include <vector>
#include "Util/SDM.h"
#include "commdef/ClusterResult.h"

namespace sort_framework {

typedef struct _AppHandle
{
    const char * pAppName;  //应用名称
    const char * pModeName;
    bool isFinal;           //是否是最后的sort
    bool isTruncate;        //是否需要截断
    sort_application::SortModeBase * pModeHandle; //sort对象的句柄
    std::vector<ConditionInfo*> * pCondInfos;   //sort的条件对象数组
    CompareInfo* pCmpInfo;  //比较对象句柄
} AppHandle; 

class AppManage
{
public:
    AppManage();
    ~AppManage();
    /**
     * 初始化 : 事先加载所有的排序类
     * @prarm  sortcfg, 配置对象
     * @return  0, succeed
     * @return  else, failed
     */
    int Init(SortConfig & sortcfg);
    /**
     * searcher排序模块的入口，驱动整个sort模块流程
     * @param sortcfg, sort配置对象
     * @param pQuery, query查询对象
     * @praram searchRes, 检索结果
     * @param mempool, 内存池对象
     * @return pSDM, success
     * @return NULL, failed
     */
    SDM *doProcessOnSearcher(SortConfig &sortcfg, SortQuery *pQuery, 
            SearchResult &searchRes, MemPool *mempool);
    /**
     * merger排序模块的入口，驱动整个sort模块流程
     * @param sortcfg, sort配置对象
     * @param pQuery, query查询对象
     * @param ppcr, 多列searcher的查询结果
     * @param arrSize, searcher结果列数
     * @param mempool, 内存池对象
     * @return pSDM, success
     * @return NULL, failed
     */
    SDM *doProcessOnMerger(SortConfig &sortcfg, SortQuery *pQuery, 
            commdef::ClusterResult **ppcr, uint32_t arrSize, MemPool *mempool);
    /**
     * 获取该排序链条的属性（目前只有截断属性）
     * @param param, queryparser对query的解析数据
     * @return string, 属性字符串
     * @return NULL, failed
     */
    const char *getBranchAttr(SortConfig *pConfig, SortQuery *pQuery);
private:
    std::vector<AppHandle*> _sortAppList;       //所有sort对象的数组
    /**
     * 创建sort对象
     * @param sortModeName, 要创建的sort对象名称
     * @return pSort, sort对象句柄
     * @return NULL, failed
     */
    sort_application::SortModeBase* CreateModule(const char* sortModeName);
    /**
     * 根据查询串，生成排序链
     * @param sortcfg, sort配置对象
     * @param pQuery, sort查询对象
     * @param curAppList, 本次查询的排序链
     * @return 0, success
     * @return else, failed
     */
    int DecisionMaker(SortConfig & sortcfg, SortQuery* pQuery,
            std::vector<AppHandle*>& curAppList);
    SortOperate _Operate;
};

}
#endif
