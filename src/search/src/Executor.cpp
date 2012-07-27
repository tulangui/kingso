#include "Executor.h"

using namespace util;

namespace search {

Executor::Executor() : _bitmap_size(0), 
                       _executor_size(0), 
                       _filter_size(0), 
                       _executor_type(ET_NONE)
{
}

Executor::~Executor()
{
}

ExecutorType Executor::getExecutorType()
{
    return _executor_type;
}
void Executor::addExecutor(Executor *p_executor)
{
    _executors.pushBack(p_executor);
    _executor_size++;
}

Executor *Executor::getOptExecutor(Executor           *p_parent, 
                                   framework::Context *p_context)
{
    return NULL;
}

int32_t Executor::truncate(const char             *sort_field,     
                           int32_t                 sort_type, 
                           index_lib::IndexReader *p_index_reader, 
                           index_lib::ProfileDocAccessor *p_doc_accessor,
                           MemPool                *mempool)
{
    return 0;
}


int32_t Executor::doExecute(uint32_t      cut_len, 
                            uint32_t      init_docid,
                            SearchResult *p_search_result) 
{
    return KS_SUCCESS;
}

void Executor::setPos(uint32_t init_docid)
{
    if(init_docid > 0) {
        Vector<IndexTerm*> index_terms;
        getIndexTerms(index_terms);
        for(uint32_t i=0; i<index_terms.size(); i++) {
            index_terms[i]->setpos(init_docid);
        }
    }
}

void Executor::getIndexTerms(util::Vector<index_lib::IndexTerm*> &index_terms)
{
    for(int32_t i = 0; i < _executor_size; i++) {
        _executors[i]->getIndexTerms(index_terms);
    }
}

void Executor::addFilter(FilterInterface *p_filter)
{
    _filters.pushBack(p_filter);
    _filter_size++;
}

void Executor::addBitmap(const uint64_t *p_bitmap)
{
    _bitmaps.pushBack(p_bitmap);
    _bitmap_size++;
}

void Executor::print(char *buf)
{
    strcat(buf, "NONE ");
}

}
