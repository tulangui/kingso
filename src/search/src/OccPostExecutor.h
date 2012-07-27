/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 491 $
 *
 * $LastChangedDate: 2011-04-08 16:06:30 +0800 (五, 08  4月 2011) $
 *
 * $Id: SinglePostExecutor.h 491 2011-04-08 08:06:30Z boluo.ybb $
 *
 * $Brief: 单渐进式底层索引查询器接口类 $
 ********************************************************************
 */

#ifndef _OCC_POST_EXECUTOR_H_
#define _OCC_POST_EXECUTOR_H_

#include "PostExecutor.h"
#include "OccFilter.h"

namespace search {

class OccPostExecutor : public PostExecutor
{
public:
    OccPostExecutor();

    virtual ~OccPostExecutor();

    /**
     * 归并seek接口，输入参数docid，
     * 输出当前第一个等于或大于输入参数的docid值
     * @param [in]  docid docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    inline virtual uint32_t seek(uint32_t doc_id);

    void setOccFilter(OccFilter *p_occ_filter);
    
    virtual Executor* getOptExecutor(Executor *p_parent, framework::Context *p_context);
    
    inline bool isFilted();
    
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
    
private:
    OccFilter *_p_occ_filter;
};

uint32_t OccPostExecutor::seek(uint32_t doc_id)
{
    int32_t  i          = 0;
    uint32_t cur_doc_id = doc_id;
 
    if(_bitmap_size > 0) {
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
        } while(cur_doc_id < INVALID_DOCID && i != _bitmap_size && isFilted());
        return cur_doc_id;
    } else {
        do {
            cur_doc_id = _p_index_term->seek(cur_doc_id);
            if(cur_doc_id >= INVALID_DOCID) {
                break;
            }
            if(isFilted()) {
                cur_doc_id ++; 
            } else {
                break;
            }
        } while(cur_doc_id < INVALID_DOCID);
    }
    return cur_doc_id;
}

bool OccPostExecutor::isFilted()
{
    if(!_p_occ_filter) {
        return false;
    }
    int32_t  occ_num    = 0;
    uint8_t *p_occ_list = _p_index_term->getOcc(occ_num);
    if(p_occ_list) {
        return _p_occ_filter->doFilter(p_occ_list, occ_num);
    } else {
        return 0;
    }
}


}

#endif

