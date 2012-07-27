/**
 * @file ObjectPool.h
 * @brief declare ObjectPool class
 *
 * @version 1.0.0
 * @date 2008.12.29
 * @author ruijie.guo
 */

#ifndef __OBJECTPOOL_H
#define __OBJECTPOOL_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "Pool.h"
#include "TSS.h"

namespace pool{
    
//for apply object array
struct ArrayHeader
{
    size_t count;
};

//constructor traits
template<class T>
struct ConstructorTraits
{
    static void destruct(void *data)
    {
         ((T*)data)->~T();
    }

    static T* constructArray(T* data, size_t count)
    {
        for (size_t i = 0; i < count; i++){
            new ( (T*)data + i) T;
        }
        return data;
    }

    static void destructArray(void *data)
    {
        ArrayHeader *ah = (ArrayHeader*)data;
        //note that data contains a header here
        T * realData = (T*)(ah + 1);
        for (size_t i = 0; i < ah->count; i++){
            ((T*)(realData+i))->~T();
        }
    }

    static T * constructVec(T* data, size_t count)
    {
        return data; //simple array don't need construct
    }

    static void destructVec(void* data)
    {
        //null;
    }
};      

template<typename AllocPolicy = MonotoneChunkPolicy<> >
class ObjectPool : public Pool< AllocPolicy >
{
public:
    typedef void (*fnDestructor)(void* data);
    typedef void* (*fnArrayConstructor)(void* data, size_t nCount);

public:
   //destruct node define
   struct DestructNode
   {
       fnDestructor     destructor;
       DestructNode*    prev;
   };
public:
    ObjectPool (size_t nInitSize = Pool< AllocPolicy >::DEFAULT_INIT_SIZE); 
    virtual ~ObjectPool();
public:
    
    /**
     * alloc an object from Pool
     * @param nBytes number of bytes of object
     * @param destructor destructor function of object
     * @return allocated object address
     */     
    void*   newObj(const size_t nBytes,fnDestructor destructor);

    /** 
     * allocate an object array
     * @param nBytesOfObj number of bytes of object
     * @param nCount element count of array
     * @param constructor constructor function of object
     * @param destructor destructor function of object
     * @return allocated array address
     */
    //void*   newArray(const size_t nBytesOfObj,const size_t nCount,fnArrayConstructor constructor,fnDestructor destructor);
   
    /**
     * release all allocated memory in Pool,over wirte the parent function
     * @return number of bytes released
     */
    size_t   release(); 
public:
    /**
     * get thread-local object pool
     * @return object pool for current thread
     */
    static ObjectPool<AllocPolicy>* getPool();
protected:
    DestructNode*   m_pDestructNode;///destructor node chain
protected:
    static TSS<ObjectPool<AllocPolicy>* > sm_tss;//thread specific storage
};

//////////////////////////////////////////////////
///implementation
template<typename AllocPolicy>
TSS<ObjectPool<AllocPolicy>* > ObjectPool<AllocPolicy>::sm_tss;

template<typename AllocPolicy>
ObjectPool<AllocPolicy>::ObjectPool(size_t nInitSize)
    : Pool<AllocPolicy>(nInitSize)
    , m_pDestructNode(NULL)
{
}

template<typename AllocPolicy>
ObjectPool<AllocPolicy>::~ObjectPool()
{
    //invoke destructor of object
    while (m_pDestructNode != NULL) {
        if (m_pDestructNode->destructor != NULL) {
            m_pDestructNode->destructor(m_pDestructNode + 1);
        }
        m_pDestructNode = m_pDestructNode->prev;
    }
}

template<typename AllocPolicy>
void* ObjectPool<AllocPolicy>::newObj(const size_t nBytes,fnDestructor destructor)
{
    size_t nAllocSize = nBytes;
	if(destructor)
		nAllocSize += sizeof(DestructNode) ;

    //construct a destructor node, and chain it
    void* ptr = Pool<AllocPolicy>::alloc(nAllocSize);
	if(!destructor)
		return ptr;
	DestructNode* pNode = (DestructNode*) ptr;
    pNode->destructor = destructor;
    pNode->prev = m_pDestructNode;
    m_pDestructNode = pNode;
    return (void*)(pNode + 1); //the actual memory position for users    
}

// template<typename AllocPolicy>
// void* ObjectPool<AllocPolicy>::newArray(const size_t nBytesOfObj,const size_t nCount,fnArrayConstructor constructor,fnDestructor destructor)
// {
//     if (nCount == 0) return NULL;
//     void* data = newObj(nBytesOfObj * nCount + sizeof(ArrayHeader),destructor);

//     ArrayHeader* pAH = (ArrayHeader*)data;
//     pAH->count = nCount;
//     return (void*)constructor( (pAH + 1), nCount );
    
// }

template<typename AllocPolicy>
size_t ObjectPool<AllocPolicy>::release()
{
    //invoke destructor of object
    while (m_pDestructNode != NULL) {
        if (m_pDestructNode->destructor != NULL) {
            m_pDestructNode->destructor(m_pDestructNode + 1);
        }
        m_pDestructNode = m_pDestructNode->prev;
    }
   return Pool<AllocPolicy>::release(); 
}
//static
template<typename AllocPolicy>
inline ObjectPool<AllocPolicy>* ObjectPool<AllocPolicy>::getPool()
{
    ObjectPool<AllocPolicy>* pPool = NULL;
    pPool = sm_tss.get();
    if(!pPool)
    {
        pPool = new ObjectPool<AllocPolicy>();
        sm_tss.set(pPool);
    }
    return pPool;
}

///////////////////////////////////////////////////////////
//utility functions and defines

///allocate object array
template <typename T>
inline T * newArray(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
     if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

     //    if (nCount == 0) return NULL;
     //return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
     //   , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
     if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );
 //    if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

///for exponent incremental chunk allocate policy
template <typename T>
inline T * newArray(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );


//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//        , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

///for adaptive statistic allocate policy
template <typename T>
inline T * newArray(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator, 90> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

template <typename T>
inline T * newArray(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator, 90> >* pObjPool, size_t nCount)
{
if (nCount == 0) return NULL;
     void* data = pObjPool->newObj(sizeof(T) * nCount + sizeof(ArrayHeader),pool::ConstructorTraits< T >::destructArray);

     ArrayHeader* pAH = (ArrayHeader*)data;
     pAH->count = nCount;
     return pool::ConstructorTraits< T >::constructArray((T*) (pAH + 1), nCount );

//     if (nCount == 0) return NULL;
//     return (T*)pObjPool->newArray(sizeof(T),nCount,(fnArrayConstructor)pool::ConstructorTraits< T >::constructArray
//         , pool::ConstructorTraits< T >::destructArray);
}

///allocate vector for basic type such as int,bool,...
template <typename T>
inline T* newVec(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}

template <typename T>
inline T* newVec(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> >* pObjPool, size_t nCount)
{
    if (nCount == 0) return NULL;
    return (T*)pObjPool->newObj(sizeof(T) * nCount,NULL); //don't need constructor
}


///allocate object
template <typename T>
inline T* newObj(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator,90> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator,90> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator,90> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator,80> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator,80> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator,80> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator,70> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator,70> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

template <typename T>
inline T* newObj(pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator,70> > * pObjPool)
{
    return new (pObjPool->newObj(sizeof(T),pool::ConstructorTraits< T >::destruct)) T;
}

}//end of namespace pool

#endif

