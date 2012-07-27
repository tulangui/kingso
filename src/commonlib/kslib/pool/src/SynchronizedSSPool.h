/**
 * @file SynchronizedSSPool.h
 * @brief declare SynchronizedSSPool class
 *
 * @version 1.0.0
 * @date 2009.1.16
 * @author ruijie.guo
 */

#ifndef __SYNCHRONIZEDSSPOOL_H
#define __SYNCHRONIZEDSSPOOL_H

#include "SimpleSegregatedPool.h"
#include "Guard.h"

namespace pool{
    
/**
 * @class SynchronizedSSPool
 * @brief thread-safe version of SimpleSegregatedPool.
 */
template<typename AllocPolicy = ExpIncChunkPolicy<NewDeleteAllocator>,typename LockType = Mutex >
class SynchronizedSSPool : public SimpleSegregatedPool<AllocPolicy>
{
public:
    typedef AllocPolicy alloc_policy;
    typedef LockType lock_type;
private:
    SynchronizedSSPool(const SynchronizedSSPool&);
    void operator=(const SynchronizedSSPool&);
public:
    /** constructor */
    SynchronizedSSPool(const size_t nReqSize);
public:
    /**
     * allocate memory from this pool
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
private:
    lock_type m_lock;
};

///////////////////////////////////////
///implementation

template<typename AllocPolicy,typename LockType >
SynchronizedSSPool<AllocPolicy,LockType>::SynchronizedSSPool(const size_t nReqSize)
    : SimpleSegregatedPool<AllocPolicy>(nReqSize)
{
}

template<typename AllocPolicy,typename LockType >
inline void* SynchronizedSSPool<AllocPolicy,LockType>::alloc()
{
    Guard<LockType> g(m_lock);
    return SimpleSegregatedPool<AllocPolicy>::alloc();
}

template<typename AllocPolicy,typename LockType >
inline void* SynchronizedSSPool<AllocPolicy,LockType>::alloc(const size_t n)
{
    Guard<LockType> g(m_lock);
    return SimpleSegregatedPool<AllocPolicy>::alloc(n);
}

template<typename AllocPolicy ,typename LockType >
inline void SynchronizedSSPool<AllocPolicy,LockType>::free(void* const pAddr)
{
    Guard<LockType> g(m_lock);
    SimpleSegregatedPool<AllocPolicy>::free(pAddr);
}

template<typename AllocPolicy ,typename LockType >
inline void SynchronizedSSPool<AllocPolicy,LockType>::free(void* const pAddr,const size_t n)
{
    Guard<LockType> g(m_lock);
    SimpleSegregatedPool<AllocPolicy>::free(pAddr,n);
}

template<typename AllocPolicy ,typename LockType >
inline void SynchronizedSSPool<AllocPolicy,LockType>::orderedFree(void* const pAddr)
{
    Guard<LockType> g(m_lock);
    SimpleSegregatedPool<AllocPolicy>::orderedFree(pAddr);
}

template<typename AllocPolicy ,typename LockType >
inline void SynchronizedSSPool<AllocPolicy,LockType>::orderedFree(void* const pAddr,const size_t n)
{
    Guard<LockType> g(m_lock);
    SimpleSegregatedPool<AllocPolicy>::orderedFree(pAddr,n);
}

template<typename AllocPolicy ,typename LockType >
size_t SynchronizedSSPool<AllocPolicy,LockType>::release()
{
    Guard<LockType> g(m_lock);
    return  SimpleSegregatedPool<AllocPolicy>::release();
}

}//end of namespace pool

#endif

