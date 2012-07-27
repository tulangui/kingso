#include "OrExecutor.h"

namespace search {

OrExecutor::OrExecutor() : _index(0)
{
    _executor_type = ET_OR;
}

OrExecutor::~OrExecutor()
{

}

uint32_t OrExecutor::getTermsCnt()
{
    uint32_t nCount = 0;
    for(int32_t i = 0; i < _executor_size; i++)
    {
        nCount += _executors[i]->getTermsCnt();
    }
    return nCount;
}

void OrExecutor::addExecutor(Executor *p_executor)
{
    _executors.pushBack(p_executor);
    _executor_size++;
    _p_doc_ids.pushBack(0);
}

Executor* OrExecutor::getOptExecutor(Executor           *p_parent, 
                                     framework::Context *p_context)
{
    Executor* p_executor = NULL;
    for(int32_t i = 0; i < _executor_size; i++) {
        p_executor = _executors[i]->getOptExecutor(this, p_context);
        if(p_executor) {
            _executors[i] = p_executor;
        }
    }
    if(p_parent && p_parent->getExecutorType() == ET_OR) {
        for(int32_t i = 1 ; i < _executor_size; i++) {
            p_parent->addExecutor(_executors[i]);
        }
        return _executors[0];
    }
    if(!p_parent) {     //or为跟节点时，or下面的post可以转成single,即seek转next
        for(int32_t i = 0; i < _executor_size; i++) {
            p_executor = _executors[i]->getOptExecutor(NULL, p_context); 
            if(p_executor) {
                _executors[i] = p_executor;
            }
        }
    }
    return NULL;
}

int32_t OrExecutor::truncate(const char              *sort_field,     
                              int32_t                 sort_type, 
                              index_lib::IndexReader *p_index_reader, 
                              index_lib::ProfileDocAccessor *p_doc_accessor,
                              MemPool                *mempool)
{
    for(int32_t i = 0; i < _executor_size; i++) {
        if(_executors[i]->getTermsCnt() > INDEX_CUT_LEN) {
            _executors[i]->truncate(sort_field, 
                                    sort_type, 
                                    p_index_reader, 
                                    p_doc_accessor,
                                    mempool);
        }
    }
    return getTermsCnt();
}

void OrExecutor::print(char *buf)
{
    strcat(buf, "OR[ ");
    for(int32_t i = 0; i < _executor_size; i++) {
        _executors[i]->print(buf);
    }
    strcat(buf, "]");
}

}
