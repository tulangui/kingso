#include "PostExecutor.h"

#include "SinglePostExecutor.h"

using namespace index_lib;

namespace search {

PostExecutor::PostExecutor() : _p_index_term(NULL), 
                               _p_calculator(NULL), 
                               _field(NULL), 
                               _token(NULL)
{
    _executor_type = ET_POST;
}

PostExecutor::~PostExecutor()
{
}

uint32_t PostExecutor::getTermsCnt()
{
    return _p_index_term->getDocNum();
}

void PostExecutor::setIndexTerm(IndexTerm  *p_index_term, 
                                const char *field_name, 
                                const char *field_value)
{
    _p_index_term = p_index_term;
	_field        = field_name;
    _token        = field_value;
}

void PostExecutor::setOccWeightCalculator(OccWeightCalculator *p_calculator)
{
    _p_calculator = p_calculator;
}

const uint64_t* PostExecutor::getBitmapIndex()
{
    return _p_index_term->getBitMap();
}

Executor *PostExecutor::getOptExecutor(Executor           *p_parent, 
                                       framework::Context *p_context)
{
    //1、post自己是根节点 2、父亲节点是OR,且为根节点。
    //3、父亲是not，且自己是左节点
    //才做优化：post转single，目的是seek转next
    if(!p_parent ) { 
        MemPool *mempool = p_context->getMemPool();
        if (!mempool) {
            TERR("no mempool : PostExecutor::getOptExecutor");
            return NULL;
        }
        SinglePostExecutor *p_executor = NEW(mempool, SinglePostExecutor)();
        if(!p_executor) {
            TERR("alloc mem for singlepost executor failed!");
            return NULL;
        }
        p_executor->setIndexTerm(_p_index_term, _field, _token);
        p_executor->setOccWeightCalculator(_p_calculator);
        for(int32_t i = 0; i < _bitmap_size; i++) {
            p_executor->addBitmap(_bitmaps[i]);
        }
        return p_executor;
    }
    return NULL;
}

int32_t PostExecutor::truncate(const char             *sort_field,     
                               int32_t                 sort_type, 
                               index_lib::IndexReader *p_index_reader, 
                               index_lib::ProfileDocAccessor *p_doc_accessor,
                               MemPool                *mempool)
{
    const ProfileField * p_sort = p_doc_accessor->getProfileField(sort_field);
    IndexTerm *p_cut_term = p_index_reader->getTerm(mempool, 
                                                    _field, 
                                                    _token,   
                                                    (p_sort != NULL)?p_sort->name:sort_field, 
                                                    sort_type);
    if(p_cut_term && p_cut_term->getTermInfo()->docNum > 0) {
        _p_index_term = p_cut_term;
    }
    return getTermsCnt();
}

int32_t PostExecutor::doExecute(uint32_t      cut_len, 
                                uint32_t      init_docid,
                                SearchResult *p_search_result) 
{
    if(_bitmap_size > 0) {
        int32_t  i = 0;
        int32_t  k = 0;
        uint32_t doc_id = _p_index_term->next();
        while(doc_id < INVALID_DOCID) {
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
            doc_id = _p_index_term->next();
        }
    } else {
        int32_t  k      = 0;
        uint32_t doc_id = _p_index_term->next();
        while(doc_id < INVALID_DOCID) {
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
            doc_id = _p_index_term->next();
        }
    }
 
    return KS_SUCCESS;
}

void PostExecutor::getIndexTerms(util::Vector<index_lib::IndexTerm*> &index_terms)
{
    index_terms.pushBack(_p_index_term);
}

void PostExecutor::print(char *buf)
{
    strcat(buf, "POST:");
    strcat(buf, _token);
    strcat(buf, ";");
}

}
