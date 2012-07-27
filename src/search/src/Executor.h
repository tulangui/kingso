/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 464 $
 *
 * $LastChangedDate: 2011-04-08 14:44:54 +0800 (五, 08  4月 2011) $
 *
 * $Id: Executor.h 464 2011-04-08 06:44:54Z boluo.ybb $
 *
 * $Brief: 渐进式关系查询器接口类 $
 ********************************************************************
 */

#ifndef _KINGSO_EXECUTOR_H_
#define _KINGSO_EXECUTOR_H_

#include "search/Search.h"
#include "FilterFactory.h"
#include "FilterInterface.h"
#include "index_lib/IndexTerm.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/DocIdManager.h"

#include "util/Vector.h"
#include "util/MemPool.h"
#include "commdef/commdef.h"
#include "framework/Context.h"
#include "commdef/SearchResult.h"

#include <stdint.h>
#include <string.h>

namespace search {

class Executor
{
public:
    Executor(); 

    virtual ~Executor();

    /**
     * 添加executor对象，用来做seek归并
     * @param [in] p_executor 归并器Executor类型指针
     */
    virtual void addExecutor(Executor *p_executor);

    /**
     * 归并seek接口，输入参数docid，
     * 输出当前第一个等于或大于输入参数的docid值
     * @param [in]  docid docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    virtual uint32_t seek(uint32_t doc_id) = 0;

    /**
     * first rank接口，每找到一个search result docId的时候调用一次，
     * 计算该docId的分数
     * @return 当前docId的分数
     */
    virtual int32_t score() = 0;

    /**
     * 返回需要归并的倒排长度，用于sortExecutor和初始化docid集合
     * @return 倒排长度
     */
    virtual uint32_t getTermsCnt() = 0;

    /**
     * 调整语法归并节点，索引底层seek可以优化为next
     * @param [in]  p_parent 父节点指针 
     * @param [in]  p_context 框架context
     * @param [in]  typep 父节点类型 
     * @return 优化后的Executor，NULL无须优化
     */
    virtual Executor* getOptExecutor(Executor           *p_parent, 
                                     framework::Context *p_context);

    /**
     * 索引截断性能优化
     * @param [in]  sort_field 排序字段名
     * @param [in]  sort_type 排序类型 (0:正排 1:倒排)
     * @param [in/out]  p_index_reader 索引指针 
     * @param [in]  p_doc_accessor 正排索引接口指针（用于别名查询）
     * @param [in]  mempool mempool 
     * @return 阶段后长度
     */
    virtual int32_t truncate(const char             *sort_field,     
                             int32_t                 sort_type, 
                             index_lib::IndexReader *p_index_reader,
                             index_lib::ProfileDocAccessor *p_doc_accessor,
                             MemPool                *mempool);

    /**
     * 当该executor为根节点时执行该方法，批量归并，直接返回结果集
     * @param [in]   cut_len  截断长度
     * @param [in]   init_doc_id    初始doc_id，增量时有用
     * @param [out]  p_search_result 输出search结果集结构
     * @return 0 成功 非0 错误码
     */
    virtual int32_t doExecute(uint32_t      cut_len, 
                              uint32_t      init_docid,
                              SearchResult *p_search_result);


    /**
     * 将所有token的索引指针都移到init_docid
     * @param init_docid 起始docId
     */
    void setPos(uint32_t init_docid);

    /**
     * 逐层返回叶子节点IndexTerm，用于扁平化层次减少调用深度
     * @param [out] index_terms 输出index_term数组
     */
    virtual void getIndexTerms(util::Vector<index_lib::IndexTerm*> &index_terms); 
 
    /**
     * 添加filter类对象
     * @param [in] p_fiter FilterInterface对象指针
     */
    virtual void addFilter(FilterInterface* p_filter); 
 
    /**
     * 添加bitmap插件
     * @param [in] p_bitmap bitmap指针
     */
    virtual void addBitmap(const uint64_t *pBitmap);

    /**
     * @return 返回executor类型：post、and、or、not
     */
    ExecutorType getExecutorType();
    
    /**
     * 打印语法结构信息
     * @param [out] buf 保存executor数打印信息
     */
    virtual void print(char *buf);
    
public:
    util::Vector<const uint64_t*>  _bitmaps;            /* bitmap过滤集合 */
    int32_t                        _bitmap_size;        /* bitmap个数 */
protected:
    util::Vector<FilterInterface*> _filters;            /* 插件集合 */
    util::Vector<Executor*>        _executors;          /* 保存executor数组 */
    ExecutorType                   _executor_type;      /* execuor类型 */ 
    int32_t                        _executor_size;      /* executor数量 */
    int32_t                        _filter_size;        /* 插件个数 */
};

}

#endif

