/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 533 $
 *
 * $LastChangedDate: 2011-04-11 16:49:25 +0800 (一, 11  4月 2011) $
 *
 * $Id : Search.h 40 2011-03-18 08:41:43Z boluo.ybb $
 *
 * $Brief: Search 模块框架 $
 ********************************************************************
 */

#ifndef KINGSO_SEARCH_H
#define KINGSO_SEARCH_H

#include "queryparser/FilterList.h"
#include "queryparser/QPResult.h"
#include "queryparser/qp_syntax_tree.h"

#include "index_lib/IndexReader.h"
#include "index_lib/IndexIncManager.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProvCityManager.h"

#include "sort/SearchSortFacade.h"

#include "mxml.h"
#include "util/Vector.h"
#include "framework/Context.h"
#include "commdef/SearchResult.h"



using namespace queryparser;
namespace search {

class Executor;
class ObjExecutor;
class FilterFactory;
class CfgManager;
class MemCache;

enum ExecutorType
{
	ET_POST,	// POST
	ET_OCCPOST,	// POST
	ET_AND,	//AND
 	ET_OR,	// OR
	ET_NOT,	// NOT
    ET_NONE
}; 

enum IndexMode
{
    IDX_MODE_ALL,   //全量
    IDX_MODE_ADD,    //增量
    IDX_MODE_FULL   //全量+增量
};


//search模块框架类,提供search与框架的接口
// 框架调用该类的方法驱动search模块
// 全局唯一，多线程可重入
class Search
{
public:

    Search(); 
    virtual ~Search();

    /**
     * 框架初始使用， 获取所需指针
     * @param [in] config config文件的xml node
     * @return 0 正常 非0 错误码
     */
    int init(mxml_node_t *config);

    /**
     * 框架调用的process， search的主处理流程
     * @param [in] p_context 框架提供的资源
     * @param [in] p_qp_result queryparser的输入结果集
     * @param [in] p_sort_facade sort的类对象，用于截断
     * @param [out] p_search_result, search输出结果集 
     * @param [in] mode 查询模式，指定全量、增量或者是全部
     * @param [in] init_docid 初始的docid 
     * @return 0 正常 非0 错误码
     */
    int doQuery(framework::Context               *p_context, 
                queryparser::QPResult            *p_qp_result, 
                sort_framework::SearchSortFacade *p_sort_facade, 
                SearchResult                     *p_search_result, 
                IndexMode                         mode        = IDX_MODE_FULL, 
                uint32_t                          init_docid  = 0);

private:
    /**
     * 生成叶子节点Executor
     * @param [in]  field_name  索引字段名
     * @param [in]  field_value 字段值
     * @param [out] is_ignore   stopword是否忽略 
     * @param [in]  mempool     mempool
     * @param [in]  mode        全量or增量
     * @return Executor 叶子节点的Executor指针
     */ 
    Executor* getPostExecutor(const char *field_name, 
                              const char *field_value, 
                              bool       &is_ignore,  
                              MemPool    *mempool, 
                              IndexMode   mode);
    
    /**       
     * 解析Delete Map的filter，生成对应Obj插入ObjExecutor中
     * @param [in]      p_context 框架提供的资源
     * @param [in/out]  p_root_executor 将生成obj加入这里
     * @param [in]      olu_type 过滤条件
     * @param [out]     node_num 插入obj个数
     * @return 0 正常 非0 错误码
     */        
    int parseDelMapFilter(framework::Context *p_context, 
                          Executor           *p_root_executor);
    
    /**
     * 解析profile的filter，生成对应Obj插入
     * @param [in]      p_context 框架提供的资源
     * @param [in]      p_qp_result queryparser提供的结果 
     * @param [in/out]  p_root_executor 将生成obj加入这里
     * @param [in]      olu_type 过滤条件
     * @param [in]      mempool mempool 
     * @return 0 正常 非0 错误码
     */
    int parseAttrFilter(framework::Context    *p_context,      
                        queryparser::QPResult *p_qp_result, 
                        Executor              *p_root_executor, 
                        MemPool               *mempool);
    /**
     * 解析queryparse生成的Filter，生成>对应Executor
     * @param [in]  p_context 框架提供的资源
     * @param [in]  p_qp_result queryparser提供的结果 
     * @param [in]  mempool mempool 
     * @return 0 正常 非0 错误码
     */
    int parseFilter(framework::Context    *p_context,       
                    queryparser::QPResult *p_qp_result, 
                    Executor              *p_root_executor, 
                    MemPool               *mempool); 
    /**
     * 检查字段是否在检索和过滤中使用，用于判断是否可以做索引截断
     * @param [in]  field 待检查字段名
     * @param [in]  p_qp_result queryparser提供的结果 
     * @return true 可以截断 false 不能截断
     */
    bool checkTruncate(const char *field, queryparser::QPResult *p_qp_result);

    /**
     * 对字段进行截断 
     * @param [in]  mempool mempool 
     * @param [in]  p_qp_result queryparser提供的结果 
     * @param [in]  p_sort_facade sort的类对象，用于截断
     * @param [out] p_root_executor 将生成obj加入这里
     * @param [out] truncate_len    截断之前的长度 
     * @return true 可以截断 false 不能截断
     */
    int32_t doTruncate(MemPool                          *mempool, 
                       queryparser::QPResult            *p_qp_result, 
                       sort_framework::SearchSortFacade *p_sort_result, 
                       Executor                         *p_root_executor);

    
    /**
     * 对字段进行截断 
     * @param [in]  tree queryparser的检索结果：检索语法树 
     * @param [in]  is_leaf, 语法树的根节点是否为叶子节点 
     * @param [out] is_ignore   stopword是否忽略 
     * @param [in]  mempool mempool 
     * @param [in] mode 查询模式，指定全量、增量或者是全部
     * @return 根据queryparser的检索树，生成的搜索语法树 
     */
    Executor* copyQpTree(qp_syntax_node_t *root,      
                         bool            &is_ignore, 
                         MemPool         *mempool, 
                         IndexMode        index_mode);
    /**
     * 取得整个语法树中的 进行 and not 计算的节点个数
     * @param[in]  p_qp_resutl queryParser的输出结果集
     * @return  节点个数
     */
    int  getAndNodeNum( queryparser::QPResult *p_qp_result);

    bool checkSyntaxTruncate(const char *field, qp_syntax_node_t *root);

    int  getAndNodeNum( qp_syntax_node_t *root , int &node_num);
private:

    index_lib::ProfileDocAccessor       *_profile_accessor;		    //Profile DocAccessor指针
    index_lib::IndexReader              *_p_index_reader;		    //index reader指针
    index_lib::IndexIncManager          *_p_index_inc_manager;		//增量index reader指针
    index_lib::DocIdManager::DeleteMap  *_p_del_map;                //delete map
    index_lib::DocIdManager             *_p_docid_manager;          // DocIdManager对象指针
    index_lib::ProvCityManager          *_p_prov_city_manager;      // 行政区编码管理器
    FilterFactory                       *_p_filter_factory;	        //obj的Factory
    CfgManager                          *_p_config_manager;         // 配置信息
};

}
#endif
