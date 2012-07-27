#include "NotExecutor.h"
#include "PostExecutor.h"
#include "FilterFactory.h"

namespace search {

NotExecutor::NotExecutor() : _p_not_executor(NULL), 
                             _not_doc_id(0), 
                             _not_bitmap_size(0), 
                             _is_init_not_docid(false)
{
    _executor_type = ET_NOT;
}

NotExecutor::~NotExecutor()
{

}

uint32_t NotExecutor::getTermsCnt()
{
    return _executors[0]->getTermsCnt();
}

void NotExecutor::setNotExecutor(Executor *p_not_executor)
{
    _p_not_executor = p_not_executor;
}

Executor *NotExecutor::getOptExecutor(Executor           *p_parent, 
                                      framework::Context *p_context)
{
    Executor *p_executor = NULL;
    p_executor = _executors[0]->getOptExecutor(this, p_context);
    if(p_executor) {
        _executors[0] = p_executor;
    }
    p_executor = _p_not_executor->getOptExecutor(this, p_context);
    if(p_executor) {
        _p_not_executor = p_executor;
    }
    const uint64_t *p_bitmap = NULL; 
    if(_p_not_executor->getExecutorType() == ET_POST 
            && _p_not_executor->_bitmap_size == 0) 
    {
        p_bitmap = ((PostExecutor*)_p_not_executor)->getBitmapIndex();
        if(p_bitmap) { 
            PostExecutor * p_post = (PostExecutor*)_p_not_executor;
            if(NULL == p_post->getOccWeightCalculator() 
                    || false == p_post->getOccWeightCalculator()->isSubFieldSearch())
            {
                _not_bitmaps.pushBack(p_bitmap);
                _not_bitmap_size = _not_bitmaps.size();
            }
        }
    }
    if(!p_parent) { //如果左孩子是post，seek转next，如果是and，不可以
        p_executor = _executors[0]->getOptExecutor(NULL, p_context);
        if(p_executor) {
            _executors[0] = p_executor;
        }
    }

    return NULL;
}

int32_t NotExecutor::truncate(const char             *sort_field,     
                              int32_t                 sort_type, 
                              index_lib::IndexReader *p_index_reader, 
                              index_lib::ProfileDocAccessor *p_doc_accessor,
                              MemPool                *mempool)
{
    if(getTermsCnt() > INDEX_CUT_LEN) {
        _executors[0]->truncate(sort_field, 
                                sort_type, 
                                p_index_reader, 
                                p_doc_accessor,
                                mempool);
    }
    return getTermsCnt();
}

int32_t NotExecutor::doExecute(uint32_t      cut_len, 
                               uint32_t      init_docid,
                               SearchResult *p_search_result) 
{ 
    int32_t  i         = 0;
    int32_t  j         = 0;
    int32_t  k         = 0;
    uint32_t cur_docid = init_docid;
    if(_not_bitmap_size > 0) {
        while((cur_docid = _executors[0]->seek(cur_docid)) < INVALID_DOCID) {
            for(j = 0; j < _not_bitmap_size; j++) {
                if(_not_bitmaps[j][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F]) {
                    break;
                }
            }
            if(j == _not_bitmap_size) { //docid不在_not_bitmap中
                for(i = 0; i < _bitmap_size; i++) {
                    if(!(_bitmaps[i][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F])) {
                        break;
                    }
                }
                if(i == _bitmap_size) {
                    for(k = 0; k < _filter_size; k++) {
                        if(!(_filters[k]->process(cur_docid))) {
                            break;
                        }
                    }
                    if(k == _filter_size) {
                        p_search_result->nDocIds[p_search_result->nDocSize] = cur_docid;
                        p_search_result->nRank[p_search_result->nDocSize] = score();
                        p_search_result->nDocSize++;
                        if(unlikely(p_search_result->nDocSize >= cut_len)) {
                            break;
                        }
                    }
                }
            }
            cur_docid++;
        }
        return KS_SUCCESS;
    }
    //右孩子不是bitmap
    if(!_is_init_not_docid) {
        _not_doc_id = _p_not_executor->seek(0);
        _is_init_not_docid = true;
    }
    //参考seek部分注释
    while((cur_docid = _executors[0]->seek(cur_docid)) < INVALID_DOCID) {
        while(cur_docid >= _not_doc_id) {
            if(cur_docid == _not_doc_id) {
                cur_docid = _executors[0]->seek(++cur_docid);
                if(cur_docid >= INVALID_DOCID) {
                    return KS_SUCCESS;
                }
            } else {
                _not_doc_id = _p_not_executor->seek(cur_docid);
            }
        } 
        for(i = 0; i < _bitmap_size; i++) {
            if(!(_bitmaps[i][cur_docid>>6]&bit_mask_tab[cur_docid&0x3F])) {
                break;
            }
        }
        if(i == _bitmap_size) {
            for(k = 0; k < _filter_size; k++) {
                if(!(_filters[k]->process(cur_docid))) {
                    break;
                }
            }
            if(k == _filter_size) {
                p_search_result->nDocIds[p_search_result->nDocSize] = cur_docid;
                p_search_result->nRank[p_search_result->nDocSize] = score();
                p_search_result->nDocSize++;
                if(unlikely(p_search_result->nDocSize >= cut_len)) {
                    break;
                }
            }
        }
        cur_docid++;
    }
    return KS_SUCCESS;
}

void NotExecutor::print(char *buf)
{
    strcat(buf, "NOT[ ");
    _executors[0]->print(buf);
    _p_not_executor->print(buf);
    strcat(buf, "]");
}

}
