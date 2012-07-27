/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 491 $
 *
 * $LastChangedDate: 2011-04-08 16:06:30 +0800 (五, 08  4月 2011) $
 *
 * $Id: PostExecutor.h 491 2011-04-08 08:06:30Z boluo.ybb $
 *
 * $Brief: 渐进式底层索引查询器接口类 $
 ********************************************************************
 */

#ifndef _POST_EXECUTOR_H_
#define _POST_EXECUTOR_H_

#include "Executor.h"
#include "OccWeightCalculator.h"
#include "index_lib/IndexTerm.h"

namespace search {

class PostExecutor : public Executor
{
public:
    
    PostExecutor();

    virtual ~PostExecutor();

    /**
     * 归并seek接口，输入参数docid，输出当前第一个等于或大于输入参数的docid值
     * @param [in]  docid docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    inline virtual uint32_t seek(uint32_t doc_id);

    /**
     * first rank接口，每找到一个search result docId的时候调用一次，计算该docId的分数
     * @return 当前docid的分数
     */
    inline virtual int32_t score();

    /**
     * 返回需要归并的倒排长度，用于sortExecutor
     * @return 倒排长度
     */
    virtual uint32_t getTermsCnt();

    /**
     * 设置索引查询器
     * @param [in] p_indexTerm IndexTerm对象指针
     * @param [in] field_name IndexTerm field_name
     * @param [in] field_value IndexTerm token_name 
     */
    void setIndexTerm(index_lib::IndexTerm *p_index_term, 
                      const char           *field_name, 
                      const char           *field_value);

    /**
     * 设置occ算分器
     * @param [in] p_calculator Occ算分器
     */
    void setOccWeightCalculator(OccWeightCalculator *p_calculator);

    OccWeightCalculator *getOccWeightCalculator() {return _p_calculator;}
    
    /**
     * 获取occ
     * @param [out] occ_num occ个数
     * @return occ数组 NULL
     */
    inline uint8_t *getOcc(int32_t &occ_num); 
 
    /**
     * 获取bitmap指针
     * @return bitmap指针
     */ 
    virtual const uint64_t *getBitmapIndex(); 
 
    /**
     * 调整语法归并节点，索引底层seek可以优化为next
     * @param [in]  p_parent 父节点指针 
     * @param [in]  p_context 框架context
     * @param [in]  typep 父节点类型 
     * @return 优化后的Executor，NULL无须优化
     */
    virtual Executor *getOptExecutor(Executor           *p_parent, 
                                     framework::Context *p_context);



    /**
     * 索引截断性能优化
     * @param [in]  sort_field 排序字段名
     * @param [in]  sort_type 排序类型 (0:正排 1:倒排)
     * @param [in/out]  p_index_reader 索引指针 
     * @param [in]  p_doc_accessor  正排索引指针
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
     * 逐层返回叶子节点IndexTerm，用于扁平化层次减少调用深度
     * @param [out] index_terms 输出index_term数组
     */
    virtual void getIndexTerms(util::Vector<index_lib::IndexTerm*> &index_terms); 

    /**
     * 打印语法结构信息
     */
    virtual void print(char *buf);

protected:
    index_lib::IndexTerm *_p_index_term;         /* 索引查询器 */
    OccWeightCalculator  *_p_calculator;       /* occ算分器 */
    const char           *_field;
    const char           *_token;
    
};

uint32_t PostExecutor::seek(uint32_t doc_id)
{
    if(_bitmap_size > 0) {
        int32_t  i          = 0;
        uint32_t cur_doc_id = doc_id;
        do {
            cur_doc_id = _p_index_term->seek(cur_doc_id);
            if(cur_doc_id >= INVALID_DOCID) {
                break;
            }
            for(i = 0; i < _bitmap_size; i++) {
                if(!(_bitmaps[i][cur_doc_id>>6]&bit_mask_tab[cur_doc_id&0x3F])) {
                    cur_doc_id++;
                    break;
                }
            }
        } while(cur_doc_id < INVALID_DOCID && i != _bitmap_size);
        return cur_doc_id;
    } else {
        return _p_index_term->seek(doc_id);
    }
}

uint8_t* PostExecutor::getOcc(int32_t &occ_num)
{
    return _p_index_term->getOcc(occ_num);
}

int32_t PostExecutor::score()
{
    if(!_p_calculator) {
        return 0;
    }
    int32_t  occ_num    = 0;
    uint8_t *p_occ_list = _p_index_term->getOcc(occ_num);
    if(p_occ_list) {
        return _p_calculator->getScore(p_occ_list, occ_num);
    } else {
        return 0;
    }
}

}

#endif

