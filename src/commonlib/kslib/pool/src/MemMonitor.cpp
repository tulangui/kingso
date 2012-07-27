#include "MemMonitor.h"
#include "backend/_MemPool.h"

namespace pool {

MemMonitor::MemMonitor(MemPool* pMemPool,size_t nMaxMem)
    : m_nCurUsedMem(0)
    , m_nMaxUsedMem(nMaxMem * 1024 * 1024)
    , m_pMemPool(pMemPool)
    , m_bEnableException(false)
{
    pMemPool->setMonitor(this);
}
         
MemMonitor::~MemMonitor()
{
    if(m_pMemPool)
    {
        m_pMemPool->setMonitor(NULL);
        m_pMemPool = NULL;
    }
}

}//end of namespace pool
