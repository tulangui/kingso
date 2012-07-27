/**
 * @file MemMonitor.h
 * @brief declare MemMonitor class used for memory allocation monitor 
 *
 * @version 1.0.0
 * @date 2009.5.4
 * @author ruijie.guo
 */
#ifndef __MEMMONITOR_H
#define __MEMMONITOR_H

#include <stddef.h>

class MemPool;

namespace pool {

class MemMonitor
{
public:
    class OverflowException
    {
    public:
        OverflowException(size_t nAllocatedSize)
            : m_nAllocatedSize(nAllocatedSize) {
        }
    public:
        size_t getAllocatedSize() const {
            return m_nAllocatedSize;
        }
    protected:
        size_t m_nAllocatedSize;
    };
         
public:
    MemMonitor(MemPool* pMemPool,size_t nMaxMem);
    ~MemMonitor();
public:
    bool  isMemAvailable() const {
        return m_nCurUsedMem < m_nMaxUsedMem;
    }
         
    inline bool memAllocated(size_t nSize)
    {
        m_nCurUsedMem += nSize;
        return m_nCurUsedMem >= m_nMaxUsedMem && m_bEnableException ? false : true;
    }
         
    void enableException() {
        m_bEnableException = true;
    }
         
    void disableException() {
        m_bEnableException = false;
    }
         
    bool getExceptionStat() const {
        return m_bEnableException;
    }
         
    //get used memory size
    size_t getUsedMemSize() const {
        return m_nCurUsedMem;
    }
         
    size_t getMaxMem() const {
        return m_nMaxUsedMem;
    }
         
    void  reset() {
        m_nCurUsedMem = 0;
    }
protected:
    size_t      m_nCurUsedMem;
    size_t      m_nMaxUsedMem;
    MemPool*      m_pMemPool;
    bool m_bEnableException;
};

}//end of namespace pool

#endif
