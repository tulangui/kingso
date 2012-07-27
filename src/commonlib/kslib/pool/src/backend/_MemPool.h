#ifndef __BACKEND_MEM_POOL_H_
#define __BACKEND_MEM_POOL_H_

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "ObjectPool.h"
#include "MemMonitor.h"

typedef void (*FnDestructor)(void* data);

/** 
 * @class MemPool
 * @breif this class is designed for backend
 */
class MemPool
{   
public:
    MemPool (size_t msize = 0, bool isAutoInc = true); //memory pool init size, and whether auto alloc new page
    virtual ~MemPool();

public:
    /**
     * set alignment size, this functions is deprecated
     */
    void   setAlignSize(int align) { }

    /**
     * alloc memory from pool
     * @param size number of bytes to allocate
     * @param fn desctructor callback function
     * @return allocated memory address
     */
    inline void* alloc(size_t size, FnDestructor fn);

    /** 
     * reset the mempool
     * @return number of bytes this pool allocated
     */
    virtual size_t   reset(); //delete all memory block but the first one

    /** 
     * get pool state
     * @return utilization ratio
     */
    virtual double getMemPoolStat(); //MemPool status
   
    pool::MemMonitor* getMonitor() {return m_pMonitor;}
    void setMonitor(pool::MemMonitor* pMonitor) {m_pMonitor = pMonitor;}

    bool memAllocated(size_t nByte);    
    size_t getAllocatedMemSize() const;    
public:
    typedef pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator,90> > Pool;
    Pool* m_pPool; ///the actual pool
protected:
    pool::MemMonitor* m_pMonitor;    
};

///////////////////////////////////////////////////////
///
inline bool MemPool::memAllocated(size_t nByte) 
{
    return m_pMonitor ? m_pMonitor->memAllocated(nByte) : true;
}

inline size_t MemPool::getAllocatedMemSize() const 
{
    if (m_pMonitor) 
    {
        return m_pMonitor->getUsedMemSize();
    }
    return 0;    
}

inline void* MemPool::alloc(size_t size, FnDestructor fn)
{
    return memAllocated(size) ? (m_pPool->newObj(size, fn)) : NULL;
}

template <class T>
inline T * newArray(MemPool *pool, size_t count)
{
    return pool->memAllocated(sizeof(T) * count) ? ((T*)pool::newArray<T>(pool->m_pPool,count)) : NULL;
    //->newArray(sizeof(T),count,pool::ConstructorTraits< T >::constructArray,pool::ConstructorTraits< T >::destructArray);
}

template <class T>
inline T * newType(MemPool *pool)
{ 
    return pool->memAllocated(sizeof(T)) ? (new(pool->m_pPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T) : NULL;
}

///allocate object
//#define NEW(MP,X)  new((MP)->m_pPool->newObj(sizeof(X),pool::ConstructorTraits< X >::destruct)) X
#define NEW(MP,X)    new((MP)->alloc(sizeof(X), pool::ConstructorTraits< X >::destruct)) X 
///allocate object array
//#define NEW_ARRAY(MP,X,LEN) (X*)( (MP)->m_pPool->newArray(sizeof(X),LEN,pool::ConstructorTraits< X >::constructArray,pool::ConstructorTraits< X >::destructArray) )
//#define NEW_ARRAY(MP,X,LEN) (X*)( pool::newArray<X>((MP)->m_pPool,LEN))
#define NEW_ARRAY(MP,X,LEN) (X*)( newArray<X>(MP, LEN))

//allocate basic data type array
//NEW_VEC is preferred and only can be used for basic data type array: char,int,float....
#define NEW_VEC(MP,X,LEN) (X*) ( (MP)->alloc(sizeof(X) * (LEN), NULL) )

#endif

