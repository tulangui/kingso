#include "OccPostExecutor.h"

#include "OccSinglePostExecutor.h"

using namespace index_lib;

namespace search {

OccPostExecutor::OccPostExecutor() : _p_occ_filter(NULL)
{
    _executor_type = ET_OCCPOST;
}

OccPostExecutor::~OccPostExecutor()
{
}

void OccPostExecutor::setOccFilter(OccFilter *p_occ_filter)
{
    _p_occ_filter = p_occ_filter; 
}

Executor *OccPostExecutor::getOptExecutor(Executor           *p_parent, 
                                          framework::Context *p_context)
{
    if(!p_parent ) { 
        MemPool *mempool = p_context->getMemPool();
        if (!mempool) {
            TERR("mempoo is null");
            return NULL;
        }
        OccSinglePostExecutor *p_executor = NEW(mempool, OccSinglePostExecutor)();
        if(!p_executor) {
            TERR("alloc mem for occ_single_post_executor falied!");
            return NULL;
        }
        p_executor->setIndexTerm(_p_index_term, _field, _token);
        p_executor->setOccWeightCalculator(_p_calculator);
        p_executor->setOccFilter(_p_occ_filter);
        for(int32_t i = 0; i < _bitmap_size; i++) {
            p_executor->addBitmap(_bitmaps[i]);
        }
        return p_executor;
    }
    return NULL;
}

int32_t OccPostExecutor::doExecute(uint32_t      cut_len, 
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
                if(k == _filter_size && !isFilted()) {
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
            if(k == _filter_size && !isFilted()) {
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


void OccPostExecutor::print(char *buf)
{
    strcat(buf, "OCCPOST:");
    strcat(buf, _token);
    strcat(buf, ";");
}


}
