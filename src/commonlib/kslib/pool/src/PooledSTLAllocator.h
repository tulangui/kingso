/**
 * @file PooledSTLAllocator.h
 * @brief declare PooledSTLAllocator class
 *
 * @version 1.1.0
 * @date 2008.12.29
 * @author ruijie.guo
 */

#ifndef __POOLEDSTLALLOCATOR_H
#define __POOLEDSTLALLOCATOR_H

#include <limits>
#include "Pool.h"
#include "SingletonSegregatedPool.h"
#include "SynchronizedSSPool.h"

namespace pool{
    
/**
 * @class PooledSTLAllocator
 * @brief the class is used to replace the default allocator of stl containers
 */
template <typename T,typename PoolType = SynchronizedSSPool<ExpIncChunkPolicy<NewDeleteAllocator> > >
class PooledSTLAllocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;

    typedef PoolType pool_type;

    template <class U> 
        struct rebind { typedef PooledSTLAllocator<U,PoolType> other; };
public: 
    PooledSTLAllocator(){}

    pointer address(reference x) const 
    {return &x;}
    const_pointer address(const_reference x) const 
    {return &x;}

    pointer allocate(size_type size, const void* hint = 0)
    {
        return static_cast<pointer>(SingletonSegregatedPool<sizeof(T),PoolType>::instance()->alloc(size));
    }

    template <class U> PooledSTLAllocator(const PooledSTLAllocator<U>&){}
  
    void deallocate(pointer p, size_type n)
    {
        SingletonSegregatedPool<sizeof(T),PoolType>::instance()->orderedFree(p, n);
    }

    void deallocate(void *p, size_type n)
    {
        SingletonSegregatedPool<sizeof(T),PoolType>::instance()->orderedFree(p, n);
    }

    size_type max_size() const throw() {return std::numeric_limits<size_type>::max();}

    void construct(pointer p, const T& val)
    {
        new(static_cast<void*>(p)) T(val);
    }

    void construct(pointer p)
    {
        new(static_cast<void*>(p)) T();
    }
    
    void destroy(pointer p)
    {
        p->~T();
    }
};

template <typename T, typename U,typename PoolType>
inline bool operator==(const PooledSTLAllocator<T,PoolType>&, const PooledSTLAllocator<U,PoolType>){return true;}

template <typename T, typename U,typename PoolType>
inline bool operator!=(const PooledSTLAllocator<T,PoolType>&, const PooledSTLAllocator<U,PoolType>){return false;}

//////////////////////////////////////////////
///
template <typename PoolType> class PooledSTLAllocator<void,PoolType>
{
public:
    typedef void* pointer;
    typedef const void* const_pointer;
    // reference to void members are impossible.
    typedef void value_type;

    template <class U> 
        struct rebind { typedef PooledSTLAllocator<U,PoolType> other; };
};    

}
#endif

