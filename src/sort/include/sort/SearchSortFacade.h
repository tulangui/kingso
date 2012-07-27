#ifndef __SEARCHER_SORT_FACADE_H_
#define __SEARCHER_SORT_FACADE_H_

#include "index_lib/ProfileManager.h"
#include "commdef/SearchResult.h"
#include "queryparser/Param.h"
#include "framework/Context.h"
#include "sort/SortResult.h"


#define SORT sort_framework

namespace sort_framework {

class SortConfig;
class AppManage;

class SearchSortFacade
{
public:
    SearchSortFacade();
    ~SearchSortFacade();
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
     * @param context, 框架提供的资源类
     * @param param, queryparser对query的解析数据
     * @praram sr, 检索结果
     * @param ret, sort之后得到的结果
     * @return 0, success
     * @return else, failed
     */
    int doProcess(const framework::Context &context,
            const queryparser::Param &param,
            SearchResult &sr,
            sort_framework::SortResult &ret);
    /**
     * 获取该排序链条的属性（目前只有截断属性）
     * @param param, queryparser对query的解析数据
     * @return string, 属性字符串
     * @return NULL, failed
     */
    const char *getBranchAttr(const queryparser::Param &param) const;
private:
    SortConfig *_pConfig;
    AppManage *_pAppManage;
};

}
#endif
