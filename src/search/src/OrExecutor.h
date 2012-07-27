/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 498 $
 *
 * $LastChangedDate: 2011-04-08 16:37:41 +0800 (五, 08  4月 2011) $
 *
 * $Id: OrExecutor.h 498 2011-04-08 08:37:41Z boluo.ybb $
 *
 * $Brief: 渐进式OR关系查询器 $
 ********************************************************************
 */

#ifndef _OR_EXECUTOR_H_
#define _OR_EXECUTOR_H_

#include "Executor.h"
#include "SearchDef.h"
#include "commdef/commdef.h"
#include <stdint.h> 
#include <limits.h> 

namespace search {

class OrExecutor : public Executor
{
public:
    OrExecutor(); 

    virtual ~OrExecutor();

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
    inline virtual uint32_t seek(uint32_t doc_id);

    /**
     * first rank接口，每找到一个search result docId的时候调用一次，
     * 计算该docId的分数
     * @return 当前docId的分数
     */
    inline virtual int32_t score();

    /**
     * 返回需要归并的倒排长度，用于sortExecutor和初始化docid集合
     * @return 倒排长度
     */
    virtual uint32_t getTermsCnt(); 
 
    /**
     * 调整语法归并节点，索引底层seek可以优化为next
     * @param [in]  p_parent 父节点指针 
     * @param [in]  p_context 框架context
     * @param [in]  typep 父节点类型 
     * @return 优化后的Executor，NULL无须优化
     */
    virtual Executor* getOptExecutor(Executor *p_parent, framework::Context *p_context);
    
    /**
     * 索引截断性能优化
     * @param [in]  sort_field 排序字段名
     * @param [in]  sort_type 排序类型 (0:正排 1:倒排)
     * @param [in/out]  p_index_reader 索引指针 
     * @param [in]  p_doc_accessor    正排索引访问接口
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
     * @param [out]  p_search_result 输出search结果集结构
     * @param [in]   cut_len  截断长度
     * @param [in]   init_doc_id    初始doc_id，增量时有用
     * @return 0 成功 非0 错误码
     */
    inline virtual int32_t doExecute(uint32_t      cut_len, 
                                     uint32_t      init_docid,
                                     SearchResult *p_search_result);



    /**
     * 打印语法结构信息
     */
    virtual void print(char *buf);

protected:
    util::Vector<uint32_t> _p_doc_ids;  //保存所有孩子executor的当前docid
    int32_t _index;                     //最小的docid的下标
};

//or节点下面的多个executor做seek操作，所有孩子直接seek，返回最小的那个docid
uint32_t OrExecutor::seek(uint32_t doc_id)
{
    uint32_t cur_id  = doc_id;
    uint32_t cur_min = INVALID_DOCID;
    int32_t  i       = 0;
    if(_bitmap_size > 0) {
        do {
            cur_min = INVALID_DOCID;
            for(i = 0; i < _executor_size; i++) {
                if(_p_doc_ids[i] < cur_id || unlikely(cur_id == 0)) {  
                    _p_doc_ids[i] = _executors[i]->seek(cur_id); 
                    if(unlikely(_p_doc_ids[i] >= INVALID_DOCID)) {
                        _executors.erase(i);
                        _p_doc_ids.erase(i);
                        _executor_size--;
                        i--;
                        continue; 
                    }
                }
                if(cur_min > _p_doc_ids[i]) {
                    cur_min = _p_doc_ids[i];
                    _index = i;
                }
            }
            cur_id   = cur_min;
            if(cur_id >= INVALID_DOCID) {
                break;
            }
            for(i = 0; i < _bitmap_size; i++) {
                if(!(_bitmaps[i][cur_id>>6] & bit_mask_tab[cur_id&0x3F])) {
                    cur_id++;
                    break;
                }
            }
        } while(i != _bitmap_size); 
    } else {
        for(i = 0; i < _executor_size; i++) {
            if(_p_doc_ids[i]  < cur_id || unlikely(cur_id == 0)) {  
                _p_doc_ids[i] = _executors[i]->seek(cur_id); 
                if(unlikely(_p_doc_ids[i] >= INVALID_DOCID)) {
                    _executors.erase(i);
                    _p_doc_ids.erase(i);
                    _executor_size--;
                    i--;
                    continue; 
                }
            }
            if(cur_min > _p_doc_ids[i]) {
                cur_min = _p_doc_ids[i];
                _index = i;
            }
        }
        cur_id   = cur_min;
    }
    return cur_id;
}

int32_t OrExecutor::doExecute(uint32_t      cut_len, 
                              uint32_t      init_doc_id,
                              SearchResult *p_search_result)
{
    int32_t  i       = 0;
    int32_t  k       = 0;
    uint32_t doc_id  = init_doc_id;
    uint32_t pre_min = 0;               //seek之前的最小doc_id
    while(doc_id < INVALID_DOCID) {
        uint32_t cur_min = INVALID_DOCID;  //seek之后的最小doc_id
        //每次for循环，将所有最小的doc_id都进行一次seek，产出下一个最小的docid
        for(i = 0; i < _executor_size; i++) {
            //如果多个孩子的docid都最小，所有的都要seek
            if(_p_doc_ids[i]  == pre_min) {  
                _p_doc_ids[i] = _executors[i]->seek(doc_id); 
                //优化：有一个孩子到索引链尾巴，将该孩子去掉
                if(unlikely(_p_doc_ids[i] >= INVALID_DOCID)) {
                    _executors.erase(i);
                    _p_doc_ids.erase(i);
                    _executor_size--;
                    i--;
                    continue; 
                }
            }
            if(cur_min > _p_doc_ids[i]) {
                cur_min = _p_doc_ids[i];
                _index = i;
            }
        }
        doc_id  = cur_min; 
        pre_min = cur_min;
        if(doc_id < INVALID_DOCID) {
            for(i = 0; i < _bitmap_size; i++) {
                if(!(_bitmaps[i][doc_id>>6]&bit_mask_tab[doc_id&0x3F])) {
                    break;
                }
            }
            if(i == _bitmap_size) {
                for(k = 0; k < _filter_size; k++) {
                    if(!(_filters[k]->process(doc_id))) {
                        break;
                    }
                }
                if(k == _filter_size) {
                    p_search_result->nDocIds[p_search_result->nDocSize] = doc_id;
                    p_search_result->nRank[p_search_result->nDocSize] = score();
                    p_search_result->nDocSize++;
                    if(unlikely(p_search_result->nDocSize >= cut_len)) {
                        break;
                    }
                }
            }
            doc_id++;
        }
    }

    return KS_SUCCESS;
}

int32_t OrExecutor::score()
{
    int32_t ret = _executors[_index]->score();
    uint32_t min_doc_id = _p_doc_ids[_index];
    for(int i=0; i<_executor_size; i++) {
        if(_p_doc_ids[i] == min_doc_id) {
            int32_t weight = _executors[i]->score();
            if(weight > ret) {
                ret = weight;
            }
        }
    }
    return ret;
}

}

#endif

