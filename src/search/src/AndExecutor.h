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
 * $Id: AndExecutor.h 498 2011-04-08 08:37:41Z boluo.ybb $
 *
 * $Brief: 渐进式AND关系查询器 $
 ********************************************************************
 */

#ifndef _AND_EXECUTOR_H_
#define _AND_EXECUTOR_H_

#include "Executor.h"
#include "commdef/commdef.h"

namespace search {

class AndExecutor : public Executor
{
public:
    AndExecutor();
 
    virtual ~AndExecutor();

    /**
     * 归并seek接口，输入参数docid，输出当前第一个等于或大于输入参数的docid值
     * @param [in]  docid docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    inline virtual uint32_t seek(uint32_t doc_id);

    /**
     * first rank接口，每找到一个search result docId的时候调用一次，计算该docId的分数
     * @return 当前docId的分数
     */
    inline virtual int32_t score();

    /**
     * 根据倒排长度对内部executor做排序，以便提高归并效率
    */
    void sortExecutor();

    /**
     * 返回需要归并的倒排长度，用于sortExecutor
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
    virtual Executor* getOptExecutor(Executor           *p_parent, 
                                     framework::Context *p_context);

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
    virtual int32_t doExecute(uint32_t      cut_len, 
                              uint32_t      init_docid,
                              SearchResult *p_search_result);

    /**
     * 打印语法结构信息
     */
    virtual void print(char *buf);
protected:
    bool _is_all_post;      //该and_executor下的所有孩子节点经过扁平化之后是否全为post_executor
};

/*  and关系的seek方法，每次do while需要找出一个doc,该节点下面会有几个孩子
    找docid的方法为，先从最左边的孩子中seek出一个docid，叫做tempid，
    然后右面的孩子节点根据tempid去seek
    如果右面的孩子seek出的docid和tempid相同，再右面的孩子继续用这个tempid去seek。
    如果不相同，从头来，即从最左边的孩子开始seek，
    不过seek的docid为用tempid去seek时得到的不同的那个docid。
    结束条件为，从左到右seek到的tempid都相同。这是返回这个tempid，作为结果
*/
uint32_t AndExecutor::seek(uint32_t doc_id)
{   
    uint32_t cur_doc_id = doc_id;
    if(_bitmap_size > 0) {
        int32_t i = 0;
        do  {
            int32_t index       = 0;    //第几个孩子节点在seek
            int32_t match_count = 0;    //有几个孩子节点seek出的docid相同
            int32_t skip_index  =-1;    //如果第二个孩子节点seek的docid不同，
                                        //则从头开始seek，这次到第二个孩子节点直接跳过就行了
            do  {
                if(skip_index == index) {
                    index++;
                }
                uint32_t temp_docid = _executors[index]->seek(cur_doc_id);
                if (temp_docid  != cur_doc_id) {    //不相同，从头来
                    cur_doc_id  = temp_docid;
                    match_count = 1;
                    skip_index  = index;
                    index       = 0;
                } else {
                    ++match_count;
                    index++;
                }
            } while (INVALID_DOCID > cur_doc_id && match_count < _executor_size);
            if(INVALID_DOCID <= cur_doc_id) {
                break;
            }
            for(i = 0; i < _bitmap_size; i++) {
                if(!(_bitmaps[i][cur_doc_id>>6] & bit_mask_tab[cur_doc_id&0x3F])) {
                    cur_doc_id++;
                    break;
                }
            }
        } while(i != _bitmap_size);
    } else {
        int32_t index       = 0;
        int32_t match_count = 0;
        int32_t skip_index  = -1;
        do  {
            if(skip_index == index) {
                index++;
            }
            uint32_t temp_docid = _executors[index]->seek(cur_doc_id);
            if (temp_docid  != cur_doc_id) {
                cur_doc_id  = temp_docid;
                match_count = 1;
                skip_index  = index;
                index       = 0;
            } else {
                ++match_count;
                index++;
            }
        } while (INVALID_DOCID > cur_doc_id && match_count < _executor_size);
    }
    return cur_doc_id;
}

int32_t AndExecutor::score()
{
    uint32_t ret = 0;
    int weight = 0;
    for(int i=0; i<_executor_size; ++i) {
        weight = _executors[i]->score();
        if(weight < 0) {
            return -1;
        } else {
            ret += weight;
        }
    }
    return ret; 
}

}

#endif

