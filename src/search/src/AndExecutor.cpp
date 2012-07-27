#include "AndExecutor.h"

#include "PostExecutor.h"
#include "SearchDef.h"
#include "FilterFactory.h"
#include "util/PriorityQueue.h"

using namespace util;
using namespace index_lib;

namespace search {

bool compareExecutor(Executor *p1, Executor *p2)
{
    return ( p1->getTermsCnt() >= p2->getTermsCnt() );
}

typedef bool(*FuncPtr)(Executor *p1, Executor *p2);


AndExecutor::AndExecutor() : _is_all_post(true)
{
    _executor_type = ET_AND;
}

AndExecutor::~AndExecutor()
{

}

uint32_t AndExecutor::getTermsCnt()
{
    uint32_t count = 0xFFFFFFFF;
    for(int32_t i = 0; i < _executor_size; i++) {
        if(count > _executors[i]->getTermsCnt()) {
            count = _executors[i]->getTermsCnt();
        }
    }
    return count;
}
/*按照索引的长度排序，短的链条排在前面。
  如果一个post和一个or做and，也是谁短谁放前面。
  扁平化后，and的直接孩子不可能是and了，or也一样
*/
void AndExecutor::sortExecutor()
{
    int32_t executors_size = _executors.size();
    if (1 == executors_size) {
        return;
    } else {
        FuncPtr cmp = compareExecutor;
        PriorityQueue<Executor*, FuncPtr> pq(cmp);
        pq.initialize(_executor_size);
        for (int32_t i = 0; i < executors_size; i++) {
            pq.push(_executors[i]);
        }
        _executors.clear();
        for (int32_t i = 0; i < executors_size; i++) {
            _executors.pushBack(pq.pop());
        }
    }
}

Executor *AndExecutor::getOptExecutor(Executor           *p_parent, 
                                      framework::Context *p_context)
{
    Executor* p_executor = NULL;
    //递归调用自己的孩子继续优化 
    for(int32_t i = 0; i < _executor_size; i++) {
        p_executor = _executors[i]->getOptExecutor(this, p_context);
        if(p_executor) {
            _executors[i] = p_executor;
        }
    }
    //扁平化，如果and的父亲是and，则将这个and删掉，自己的孩子给父亲
    if(p_parent && p_parent->getExecutorType() == ET_AND) {
        for(int32_t i = 1 ; i < _executor_size; i++) {
            p_parent->addExecutor(_executors[i]);
        }
        return _executors[0];
    }
    //and关系时，将短的链放前面。
    sortExecutor();
    //如果and的孩子全是post，则直接操作index_term，省掉post这层。
    if(!p_parent) {     
        for(int32_t i = 0; i < _executor_size; i++) {
            if(_executors[i]->getExecutorType() != ET_POST) {
                _is_all_post = false;
            }
        }
    }
    //如果孩子是post，能转成bitmap,但是最左边的孩子不能转
    for(int32_t i = (_executor_size - 1); i > 0; i--) {
        if(_executors[i]->getExecutorType() == ET_POST) {
            const uint64_t *bitmap = NULL;
            bitmap = ((PostExecutor*)_executors[i])->getBitmapIndex();
            if(bitmap) { 
                PostExecutor * p_post = (PostExecutor*)_executors[i];
                //如果不需要occ的信息的话，可以用bitmap，
                //否则不可以,因为bitmap不能保存occ信息
                if(NULL == p_post->getOccWeightCalculator()
                        || false == p_post->getOccWeightCalculator()->isSubFieldSearch())
                {
                    _bitmaps.pushBack(bitmap);
                    _bitmap_size++;
                    _executors.erase(i);
                    _executor_size--;
                }
            }
        }
    }
    //如果and只有一个孩子了，将这个节点再优化一遍，
    //区别是这次优化，孩子节点的父节点变化了，不是自己了，而是自己的父亲了
    //example: or -> and -> or，当and只有一个孩子or时，把and删除，现在剩下了两个or，
    //孩子or还可以优化掉的，最后只剩下了根节点的or
    if(_executor_size > 1) {    //可以加个判断 if(p_parent == NULL) 可以继续优化，即seek转next
        p_executor = this;
    } else {
        p_executor = _executors[0]->getOptExecutor(p_parent, p_context);
        if(!p_executor) {
            p_executor = _executors[0];
        }
        for(int32_t i = 0; i < _bitmap_size; i++) {
            p_executor->addBitmap(_bitmaps[i]);
        }
    }

    return p_executor;
}

int32_t AndExecutor::truncate(const char             *sort_field,     
                              int32_t                 sort_type, 
                              index_lib::IndexReader *p_index_reader, 
                              index_lib::ProfileDocAccessor * p_doc_accessor,
                              MemPool                *mempool)
{
    int32_t cut_len = 0;
    if(getTermsCnt() < INDEX_CUT_LEN) {
        return cut_len;
    }
    //and只对第一个孩子做截断
    _executors[0]->truncate(sort_field, 
                            sort_type, 
                            p_index_reader, 
                            p_doc_accessor,
                            mempool);

    return getTermsCnt();
}

int32_t AndExecutor::doExecute(uint32_t      cut_len, 
                               uint32_t      init_docid,
                               SearchResult *p_search_result) 
{
    //优化：当所有的孩子均为post时，直接操作index_term，省了多态
    if(_is_all_post) {      
        uint32_t cur_doc_id = init_docid;
        Vector<IndexTerm*> index_terms;
        getIndexTerms(index_terms);
        int32_t index_term_size = index_terms.size();
        if(index_term_size > 0) {
            if(index_term_size == 1) {
                int32_t i  = 0;
                int32_t k  = 0;
                cur_doc_id = index_terms[0]->next();
                if(_bitmap_size > 0) {
                    while(cur_doc_id < INVALID_DOCID) {
                        for(i = 0; i < _bitmap_size; i++) {
                            if(!(_bitmaps[i][cur_doc_id>>6]&bit_mask_tab[cur_doc_id&0x3F])) {
                                break;
                            }
                        }
                        if(i == _bitmap_size) {
                            for(k = 0; k < _filter_size; k++) {
                                if(!(_filters[k]->process(cur_doc_id))) {
                                    break;
                                }
                            }
                            if(k == _filter_size) {
                                p_search_result->nDocIds[p_search_result->nDocSize] = cur_doc_id;
                                p_search_result->nRank[p_search_result->nDocSize] = score(); 
                                p_search_result->nDocSize++;
                                if(unlikely(p_search_result->nDocSize >= cut_len)) {
                                    break;
                                }
                            }
                        }
                        cur_doc_id = index_terms[0]->next();
                    }
                } else {
                    while(cur_doc_id < INVALID_DOCID) {
                        for(k = 0; k < _filter_size; k++) {
                            if(!(_filters[k]->process(cur_doc_id))) {
                                break;
                            }
                        }
                        if(k == _filter_size) {
                            p_search_result->nDocIds[p_search_result->nDocSize] = cur_doc_id;
                            p_search_result->nRank[p_search_result->nDocSize] = score();
                            p_search_result->nDocSize++;
                            if(unlikely(p_search_result->nDocSize >= cut_len)) {
                                break;
                            }
                        }
                        cur_doc_id = index_terms[0]->next();
                    }               
                }
            } else {    //多个index_term做and
                uint32_t temp_doc_id = 0;
                int32_t  i           = 0;
                int32_t  j           = 0;
                int32_t  k           = 0;
                while((cur_doc_id = index_terms[0]->seek(cur_doc_id)) < INVALID_DOCID) {
                    temp_doc_id = cur_doc_id;
                    for(j = 1; j < index_term_size; j++) {
                        cur_doc_id = index_terms[j]->seek(temp_doc_id);
                        if(temp_doc_id != cur_doc_id) {
                            break;
                        }
                    }
                    if(j == index_term_size) {   //该docid在所有的index_term中都存在才行
                        for(i = 0; i < _bitmap_size; i++) {
                            if(!(_bitmaps[i][cur_doc_id>>6]&bit_mask_tab[cur_doc_id&0x3F])) {
                                break;
                            }
                        }
                        if(i == _bitmap_size) { //所有的bitmap中也有该docid
                            for(k = 0; k < _filter_size; k++) {
                                if(!(_filters[k]->process(cur_doc_id))) {
                                    break;
                                }
                            }
                            if(k == _filter_size) {
                                p_search_result->nDocIds[p_search_result->nDocSize] = cur_doc_id;
                                p_search_result->nRank[p_search_result->nDocSize] = score();
                                p_search_result->nDocSize++;
                                if(unlikely(p_search_result->nDocSize >= cut_len)) {
                                    break;
                                }
                            }
                        }
                        cur_doc_id++;
                    }
                }
            }
        }
    } else {    //没有is_all_post优化，正常操作post，而不是index_term,参考seek部分代码注释
        int32_t  k           = 0;
        int32_t  i           = 0;
        uint32_t cur_doc_id  = init_docid;
        uint32_t temp_doc_id = 0;
        while(cur_doc_id < INVALID_DOCID) {
            int32_t index       = 0;
            int32_t match_count = 0;
            int32_t skip_index  = -1;
            do {
                if(skip_index == index) {
                    index++;
                }
                temp_doc_id = _executors[index]->seek(cur_doc_id);
                if (temp_doc_id != cur_doc_id) {
                    cur_doc_id  = temp_doc_id;
                    match_count = 1;
                    skip_index  = index;
                    index       = 0;
                } else {
                    match_count++;
                    index++;
                }
            } while (match_count < _executor_size);
            if(cur_doc_id < INVALID_DOCID) {
                for(i = 0; i < _bitmap_size; i++) {
                    if(!(_bitmaps[i][cur_doc_id>>6]&bit_mask_tab[cur_doc_id&0x3F])) {
                        break;
                    }
                }
                if(i == _bitmap_size) {
                    for(k = 0; k < _filter_size; k++) {
                        if(!(_filters[k]->process(cur_doc_id))) {
                            break;
                        }
                    }
                    if(k == _filter_size) {
                        p_search_result->nDocIds[p_search_result->nDocSize] = cur_doc_id;
                        p_search_result->nRank[p_search_result->nDocSize] = score();
                        p_search_result->nDocSize++;
                        if(unlikely(p_search_result->nDocSize >= cut_len)) {
                            break;
                        }
                    }
                }
                cur_doc_id++;
            }
        }
    }
 
    return KS_SUCCESS;
}

void AndExecutor::print(char *buf)
{
    strcat(buf, "AND[ ");
    for(int32_t i = 0; i < _executor_size; i++)
    {
        _executors[i]->print(buf);
    }
    strcat(buf, "]");
}

}
