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
 * $Id: NotExecutor.h 498 2011-04-08 08:37:41Z boluo.ybb $
 *
 * $Brief: 渐进式NOT关系查询器 $
 ********************************************************************
 */

#ifndef _NOT_EXECUTOR_H_
#define _NOT_EXECUTOR_H_

#include "Executor.h"
#include "SearchDef.h"
#include "commdef/commdef.h"

namespace search {

class NotExecutor : public Executor
{
public:
    NotExecutor();

    virtual ~NotExecutor();

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
     * 返回需要归并的倒排长度，用于sortExecutor
     * @return 倒排长度
     */
    virtual uint32_t getTermsCnt();

    /**
     * 设定not executor
     * @param [in] p_not_executor 用于做not关系归并的Executor对象指针
     */
    void setNotExecutor(Executor *p_not_executor); 
 
    /**
     * 调整语法归并节点，索引底层seek可以优化为next
     * @param [in]  p_parent 父节点指针 
     * @param [in]  p_context 框架context
     * @param [in]  typep 父节点类型 
     * @return 优化后的Executor，NULL无须优化
     */
    virtual Executor* getOptExecutor(Executor* p_parent, framework::Context* p_context);
 
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
    Executor                     *_p_not_executor;       /* not executor */
    uint32_t                      _not_doc_id;            /* 记录当前not docid */
    util::Vector<const uint64_t*> _not_bitmaps;           /* not逻辑bitmap过滤集合 */
    int32_t                       _not_bitmap_size;
    bool                          _is_init_not_docid;     /* 是否已经初始化notid */

};

uint32_t NotExecutor::seek(uint32_t doc_id)
{
    int32_t  i         = 0;
    int32_t  j         = 0;
    uint32_t cur_docid = _executors[0]->seek(doc_id);
    if(_not_bitmap_size > 0) {
        if(_bitmap_size > 0) {
            while(cur_docid < INVALID_DOCID) {
                for(j = 0; j < _not_bitmap_size; j++) {
                    //cur_docid不能在_not_bitmaps中
                    if(_not_bitmaps[j][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F]) {
                        break;
                    }
                }
                if(j == _not_bitmap_size) {     //该docid不在_not_bitmap中
                    for(i = 0; i < _bitmap_size; i++) {
                        if(!(_bitmaps[i][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F])) {
                            break;
                        }
                    }
                    if(i == _bitmap_size) {
                        break;
                    }
                }
                cur_docid = _executors[0]->seek(++cur_docid);
            }
        } else {
            while(cur_docid < INVALID_DOCID) {
                for(j = 0; j < _not_bitmap_size; j++) {
                    if(_not_bitmaps[j][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F]) {
                        break;
                    }
                }
                if(j == _not_bitmap_size) {
                    break;
                }
                cur_docid = _executors[0]->seek(++cur_docid);
            }
        }
    } else {
        if(unlikely(!_is_init_not_docid)) {
            _not_doc_id = _p_not_executor->seek(0);
            _is_init_not_docid = true;
        }
        if(_bitmap_size > 0) {
            while(cur_docid < INVALID_DOCID) {
                //当左孩子中seek的docid，比右孩子中seek的docid小的时候，该docid符合条件
                //每次while循环，保证出来一个符合条件的docid
                //因为如果cur_docid < _not_doc_id，肯定符合条件。
                //否则分两种情况，1、相等，_executor[0]去seek
                //2、cur_docid > _not_doc_id， _not_executor去seek
                while(cur_docid >= _not_doc_id) {
                    if(cur_docid == _not_doc_id) {       //在右孩子中找到了，该docid不符合
                        cur_docid = _executors[0]->seek(++cur_docid);
                        if(cur_docid >= INVALID_DOCID) {
                            break;
                        }
                    } else {        //cur_docid > _not_doc_id， _not_executor去seek
                        _not_doc_id = _p_not_executor->seek(cur_docid);
                    }
                }
                if(cur_docid >= INVALID_DOCID) {
                    break;
                }
                for(i = 0; i < _bitmap_size; i++) {
                    if(!(_bitmaps[i][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F])) {
                        break;
                    }
                }
                if(i == _bitmap_size) {
                    break;
                }
                cur_docid = _executors[0]->seek(++cur_docid);
            }
        } else {
            if(cur_docid >= INVALID_DOCID) {
                return cur_docid;
            }
            while(cur_docid >= _not_doc_id) {
                if(cur_docid == _not_doc_id) {
                    cur_docid = _executors[0]->seek(++cur_docid);
                    if(cur_docid >= INVALID_DOCID) {
                        break;
                    }
                } else {
                    _not_doc_id = _p_not_executor->seek(cur_docid);
                }
            }
        }
    }
    return cur_docid;
}

int32_t NotExecutor::score()
{
    return _executors[0]->score();
}

}

#endif

