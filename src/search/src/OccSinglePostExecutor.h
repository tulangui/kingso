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

#ifndef _OCCSINGLE_POST_EXECUTOR_H_
#define _OCCSINGLE_POST_EXECUTOR_H_

#include "OccPostExecutor.h"

namespace search {

class OccSinglePostExecutor : public OccPostExecutor
{
public:
    OccSinglePostExecutor();

    virtual ~OccSinglePostExecutor();

    /**
     * 归并seek接口，输入参数docid，
     * 输出当前第一个等于或大于输入参数的docid值
     * @param [in]  docid docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    inline virtual uint32_t seek(uint32_t doc_id);

    /**
     * 打印语法结构信息
     */
    virtual void print(char *buf);

};


uint32_t OccSinglePostExecutor::seek(uint32_t doc_id)
{
    uint32_t cur_doc_id = 0;
    if(_bitmap_size > 0) {
        int32_t i = 0;
        do {
            cur_doc_id = _p_index_term->next();
            if(cur_doc_id >= INVALID_DOCID) {
                break;
            }
            for(i = 0; i < _bitmap_size; i++) {
                if(!(_bitmaps[i][cur_doc_id>>6]&bit_mask_tab[cur_doc_id&0x3F])) {
                    break;
                }
            }
        } while(cur_doc_id < INVALID_DOCID && i != _bitmap_size && isFilted());
        return cur_doc_id;
    } else {
        do { 
            cur_doc_id = _p_index_term->next();
        }while(cur_doc_id < INVALID_DOCID && isFilted());
    }
    return cur_doc_id;
}

}

#endif

