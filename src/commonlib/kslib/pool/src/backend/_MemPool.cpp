#include "_MemPool.h"

MemPool::MemPool(size_t msize, bool isAutoInc)
    : m_pMonitor(NULL)
{
    m_pPool = new pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator, 90> > ();
}

MemPool::~MemPool() 
{
    delete m_pPool;
    if (m_pMonitor) 
    {
        m_pMonitor->reset();
        m_pMonitor = NULL;
    }    
}

double MemPool::getMemPoolStat()
{
    pool::MemStat st = m_pPool->getMemStat();
    return st.getUsageRatio();
}

size_t MemPool::reset() 
{
    if (m_pMonitor) 
    {
        m_pMonitor->reset();
    }
    
	return m_pPool->release();
}

