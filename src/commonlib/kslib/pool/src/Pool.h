/**
 * @file Pool.h
 * @brief declare Pool class
 *
 * @version 1.0.0
 * @date 2008.12.25
 * @author ruijie.guo
 */

#ifndef __POOL_H
#define __POOL_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "PoolDefine.h"
#include "MonotoneChunkPolicy.h"
#include "ExpIncChunkPolicy.h"
#include "AdaptiveStatPolicy.h"

namespace pool{

template<typename AllocPolicy = AdaptiveStatPolicy<> >
class Pool
{               
public:
    typedef AllocPolicy alloc_policy;   //memory allocate strategy
    
    const static size_t DEFAULT_INIT_SIZE = 128*1024;//128 KB
public:
    /**
     * constructor
     * @param nInitSize init Pool size
     */
    Pool (size_t nInitSize = DEFAULT_INIT_SIZE); 
    virtual ~Pool();
private:
    Pool(const Pool&);
    void operator=(const Pool&);
public:
    /**
     * allocate memory from Pool
     * @param nBytes number of bytes need to allocate
     * @return address of allocated memory
     */
    void*   alloc(const size_t nBytes);
    
    /** 
     * free memory,dummy function
     */
    void   free(void*, const size_t){}
    
    /**
     * get Pool MemStatus
     * @return MemStatus of Pool
     */
    const MemStat getMemStat()const;

	/**
	 * set memory chunk size
	 * @param nChunkSize chunk size
	 */
	void setChunkSize(const size_t nChunkSize){m_allocPolicy.setChunkSize( nChunkSize );}
    
    /**
     * set alignment size 
     * @param nAlign alignment size
     */
    void   setAlignSize(const size_t nAlign) { m_nAlignSize = nAlign;}    

public:
    /**
     * release all allocated memory in Pool
     * @return number of bytes released
     */
    virtual size_t  release();
protected:
    size_t      m_nAlignSize; ///Alignment length
    MemChunk*   m_pMemChunk;  ///Memory chunk
    
    alloc_policy m_allocPolicy;///allocation policy
    
    static MemChunk DUMMY_CHUNK;///dummy chunk
};

//////////////////////////////////////////////////
//implementation
template<typename AllocPolicy>
MemChunk Pool<AllocPolicy>::DUMMY_CHUNK;

template<typename AllocPolicy>
Pool<AllocPolicy>::Pool (size_t nInitSize)
{
    m_nAlignSize = sizeof(char*);
    m_pMemChunk = m_allocPolicy.allocChunk(nInitSize);
    if(!m_pMemChunk)
        m_pMemChunk = &(Pool::DUMMY_CHUNK);
}

template<typename AllocPolicy>
Pool<AllocPolicy>::~Pool()
{
    m_pMemChunk = NULL;
}

template<typename AllocPolicy>
void* Pool<AllocPolicy>::alloc(const size_t nBytes)
{
    //align memory
    size_t nAllocSize = ((nBytes + m_nAlignSize - 1) & ~(m_nAlignSize - 1)); 
    void* ptr = m_pMemChunk->malloc(nAllocSize);
    if(!ptr)
	{
        m_pMemChunk = m_allocPolicy.allocChunk(nAllocSize);
        ptr = m_pMemChunk->malloc(nAllocSize);
    }
    return ptr;
}

template<typename AllocPolicy>
const MemStat Pool<AllocPolicy>::getMemStat()const
{
    return m_allocPolicy.getMemStat();
}

template<typename AllocPolicy>
size_t Pool<AllocPolicy>::release()
{
    size_t nTotalSize = m_allocPolicy.release();
    m_pMemChunk = &(Pool::DUMMY_CHUNK);
    return nTotalSize;
}

}//end of namespace pool

#endif

