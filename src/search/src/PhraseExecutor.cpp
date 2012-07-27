#include "PhraseExecutor.h"
#include "commdef/commdef.h"

namespace search {

PhraseExecutor::PhraseExecutor() : _ppOcc(NULL), _pOccNum(NULL), _pStarts(NULL), _pHeap(NULL)
{

}

PhraseExecutor::~PhraseExecutor()
{

}

int PhraseExecutor::init(MemPool *pHeap)
{
    if(!pHeap) {
        return KS_ENOENT;
    }
    _pHeap = pHeap;

    _ppOcc = (uint8_t**)NEW_VEC(_pHeap, int32_t*, _executor_size); 
    if(!_ppOcc)
    {
        TERR("Memory alloc failed : PhraseExecutor::init _ppOcc");
        return KS_ENOMEM;
    }
    memset(_ppOcc, 0x0, _executor_size*sizeof(int32_t*));
 
    _pOccNum = (int32_t*)NEW_VEC(_pHeap, int32_t, _executor_size);
    if(!_pOccNum) 
    {
        TERR("Memory alloc failed : PhraseExecutor::init _pOccNum");
        return KS_ENOMEM;
    }
    memset(_pOccNum, 0x0, _executor_size*sizeof(int32_t));

    _pStarts = (int32_t*)NEW_VEC(_pHeap, int32_t, _executor_size);
    if(!_pStarts) 
    {
        TERR("Memory alloc failed : PhraseExecutor::init _pStarts");
        return KS_ENOMEM;
    }
    memset(_pStarts, 0x0, _executor_size*sizeof(int32_t));

    return KS_SUCCESS;
}

Executor* PhraseExecutor::getOptExecutor(Executor* pParent, framework::Context* pContext, ExecutorType exType )
{
    return NULL;
}

int32_t PhraseExecutor::truncate(const char* szSortField, int32_t nSortType, index_lib::IndexReader *pIndexReader, index_lib::ProfileDocAccessor * pAccessor, MemPool *pHeap)
{
    return 0;
}

}
