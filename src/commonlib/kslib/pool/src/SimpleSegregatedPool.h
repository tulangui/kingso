/**
 * @file SimpleSegregatedPool.h
 * @brief declare SimpleSegregatedPool class
 *
 * @version 1.0.0
 * @date 2009.1.15
 * @author ruijie.guo
 */

#ifndef __SIMPLESEGREGATEDPOOL_H
#define __SIMPLESEGREGATEDPOOL_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <functional>
#include "PoolDefine.h"
#include "MonotoneChunkPolicy.h"
#include "ExpIncChunkPolicy.h"
#include "AdaptiveStatPolicy.h"

namespace pool{
    
/**
 * @class SimpleSegregatedPool
 * @brief implementation of simple segregated storage strategy.
 */
template<typename AllocPolicy = ExpIncChunkPolicy<NewDeleteAllocator> >
class SimpleSegregatedPool : public Pool<AllocPolicy>
{
public:
    typedef AllocPolicy alloc_policy;
private:
    SimpleSegregatedPool(const SimpleSegregatedPool&);
    void operator=(const SimpleSegregatedPool&);
public:
    /** constructor */
    SimpleSegregatedPool(const size_t nRequestSize);
    /** destructor */
    ~SimpleSegregatedPool();
public:
    /** 
     * is the pool empty or not
     * @return true if empty
     */
    bool isEmpty(){return (m_pFirst == 0);}

    /**
     * allocate one memory chunk from this pool
     * @return allocated memory address
     */
    void* alloc();

    /**
     * allocate n memory chunk from this pool
     * @param n number of chunks
     * @return allocated memory address
     */
    void* alloc(const size_t n);

    /**
     * free memory chunk which was previously returned from alloc()
     * @param pAddr chunk to free
     */
    void free(void* const pAddr);

    /**
     * free memory chunk which was previously returned from alloc()
     * @param pAddr chunk to free
     * @param n number of chunk
     */
    void free(void* const pAddr, const size_t n);

    /*
     * ordered free memory chunk which was previously returned from alloc()
     * @param pAddr chunk to free
     */
    void orderedFree(void* const pAddr);
    
    /*
     * ordered free memory chunk which was previously returned from alloc()
     * @param pAddr chunk to free
     */
    void orderedFree(void* const pAddr,const size_t n);

    /**
     * release all allocated memory in Pool
     * @return number of bytes released
     */
    size_t release();
protected:
    /*
     * find previous node in free list
     * @param pAddr pointer to find
     * @return previous node 
     */
    void* findPrev(void* const pAddr);
    
    /**
     * grow the list
     * @param min chunks
     */
    void  grow(const size_t n = 0);
    
    /**
     * try to allocate n memory chunk from this pool
     * @param n number of chunks
     * @return allocated memory address
     */
    void* tryAlloc(void*& pStart,const size_t n);
protected:
    /** 
     * get next node of pAddr
     * @param pAddr pointer
     * @return next node 
     */
    static void*& nextOf(void* const pAddr)
    {return *(static_cast<void**>(pAddr));}

    /** 
     * segregate a chunk to partitions
     * @param pChunk memory chunk
     * @param nChunkSize size of chunk
     * @param nRequestedSize size of partition
     * @param pEndNode the last node to link to
     * @return first node of partitions
     */
    static void* segregate(void* pChunk,size_t nChunkSize,size_t nRequestedSize,void* pEndNode = NULL);
protected:
    void* m_pFirst; ///first free node
    size_t m_nNextSize;///next alloc size
    size_t m_nRequestSize;///request size
};

///////////////////////////////////////
///implementation

template<typename AllocPolicy >
SimpleSegregatedPool<AllocPolicy>::SimpleSegregatedPool(const size_t nRequestSize)
    : Pool<AllocPolicy>(nRequestSize)
    , m_pFirst(NULL)
    , m_nNextSize(5)
{
    m_nRequestSize = ( (nRequestSize > sizeof(void*) ) ? nRequestSize : sizeof(void*) );
}

template<typename AllocPolicy >
SimpleSegregatedPool<AllocPolicy>::~SimpleSegregatedPool()
{
    release();
}

template<typename AllocPolicy >
inline void* SimpleSegregatedPool<AllocPolicy>::alloc()
{
    if( isEmpty() )
        grow();
    void* const pRet = m_pFirst;
    m_pFirst = nextOf(m_pFirst);
    return pRet;
}

template<typename AllocPolicy >
inline void* SimpleSegregatedPool<AllocPolicy>::alloc(const size_t n)
{
    if( isEmpty() )
        grow(n);
    void** pStart = &m_pFirst;
    void* pIter;
    do
    {
        if (nextOf(*pStart) == 0)///no free mem
        {
            size_t nAlloc = (m_nRequestSize << m_nNextSize);
            size_t nReqSize = n * m_nRequestSize;
            if(nAlloc <= nReqSize)
            {
                return Pool<AllocPolicy>::alloc(nReqSize);///alloc from Pool
            }
            else
            {
                void* pChunk = Pool<AllocPolicy>::alloc(nAlloc);
                m_pFirst = segregate((char*)pChunk + nReqSize,nAlloc - nReqSize,m_nRequestSize,m_pFirst);
                return pChunk;
            }
            
            m_nNextSize++;
        }
        pIter = tryAlloc(*pStart, n);
    } while (pIter == 0);
    void* const pRet = nextOf(*pStart);
    nextOf(*pStart) = nextOf(pIter);
    return pRet;
}

template<typename AllocPolicy >
inline void* SimpleSegregatedPool<AllocPolicy>::tryAlloc(void*& pStart,const size_t n)
{
    void* pIter = nextOf(pStart);
    size_t nn = (size_t)n;
    while (--nn != 0)
    {
        void* pNext = nextOf(pIter);
        if (pNext != static_cast<char *>(pIter) + m_nRequestSize)
        {
            // next == 0 (end-of-list) or non-contiguous chunk found
            pStart = pIter;
            return NULL;
        }
        pIter = pNext;
    }
    return pIter;
}

template<typename AllocPolicy >
inline void SimpleSegregatedPool<AllocPolicy>::free(void* const pAddr)
{
    nextOf(pAddr) = m_pFirst;
    m_pFirst = pAddr;
}

template<typename AllocPolicy >
inline void SimpleSegregatedPool<AllocPolicy>::free(void* const pAddr,const size_t n)
{
    m_pFirst = segregate(pAddr,n * m_nRequestSize,m_nRequestSize,m_pFirst);
}

template<typename AllocPolicy >
inline void SimpleSegregatedPool<AllocPolicy>::orderedFree(void* const pAddr)
{
    void* const pLoc = findPrev(pAddr);
    if(!pLoc)
    {
        free(pAddr);
    }
    else
    {
        nextOf(pAddr) = nextOf(pLoc);
        nextOf(pLoc) = pAddr;
    }
}

template<typename AllocPolicy >
inline void SimpleSegregatedPool<AllocPolicy>::orderedFree(void* const pAddr,const size_t n)
{
    void* const pLoc = findPrev(pAddr);
    if(!pLoc)
    {
        m_pFirst = segregate(pAddr,n * m_nRequestSize,m_nRequestSize,m_pFirst);
    }
    else
    {
        nextOf(pLoc) = segregate(pAddr,n * m_nRequestSize,m_nRequestSize,nextOf(pLoc) );
    }
}

template<typename AllocPolicy >
void* SimpleSegregatedPool<AllocPolicy>::findPrev(void* const pAddr)
{
    if(isEmpty() || std::greater<void*>()(m_pFirst,pAddr) )
        return NULL;
    void* pIter = m_pFirst;
    while(true)
    {
        if(nextOf(pIter) == 0 || std::greater<void*>()(nextOf(pIter),pAddr) )
            return pIter;
        pIter = nextOf(pIter);
    }
}

template<typename AllocPolicy >
void* SimpleSegregatedPool<AllocPolicy>::segregate(void* pChunk,size_t nChunkSize,size_t nRequestedSize,void* pEndNode )
{
    char* pOld = static_cast<char*>(pChunk) + ( ( nChunkSize - nRequestedSize)/nRequestedSize ) * nRequestedSize;
    nextOf(pOld) = pEndNode;///pOld point to the end node
    if(pOld == pChunk)//only one partition
        return pChunk;
    for(char* pIter = pOld - nRequestedSize; pIter != pChunk; pOld = pIter,pIter -= nRequestedSize)
        nextOf(pIter) = pOld;
    nextOf(pChunk) = pOld;
    return pChunk;
}

template<typename AllocPolicy >
void SimpleSegregatedPool<AllocPolicy>::grow(const size_t n)
{
    size_t nAlloc = (m_nRequestSize << m_nNextSize);
    size_t nReqSize = n * m_nRequestSize;
    if(nAlloc < nReqSize)
        nAlloc = nReqSize;
    void* pChunk = Pool<AllocPolicy>::alloc(nAlloc);
    m_pFirst = segregate(pChunk,nAlloc,m_nRequestSize,m_pFirst);
    m_nNextSize++;
}

template<typename AllocPolicy >
size_t SimpleSegregatedPool<AllocPolicy>::release()
{
    m_pFirst = 0;
    m_nNextSize = 5;
    return Pool<AllocPolicy>::release();
}

}//end of namespace pool

#endif
