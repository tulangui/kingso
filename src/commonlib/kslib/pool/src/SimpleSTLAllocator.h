/**
 * @file SimpleSTLAllocator.h
 * @brief declare SimpleAllcator class
 *
 * @version 1.0.0
 * @date 2008.12.29
 * @author ruijie.guo
 */

#ifndef __SIMPLESTLALLOCATOR_H
#define __SIMPLESTLALLOCATOR_H

#include <limits>
#include "Allocators.h"

namespace pool{
    
/**
 * @class SimpleSTLAllocator
 * @brief the class is used to replace the default allocator of stl containers
 */
template <typename T,typename Allocator = NewDeleteAllocator>
class SimpleSTLAllocator
{
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef T value_type;
    typedef Allocator allocator;

    template <class U> 
        struct rebind { typedef SimpleSTLAllocator<U,Allocator> other; };
public: 
    SimpleSTLAllocator(){}

    pointer address(reference x) const 
    {return &x;}
    const_pointer address(const_reference x) const 
    {return &x;}

    pointer allocate(size_type size, const void* hint = 0)
    {
        return static_cast<pointer>(allocator::malloc(size*sizeof(T)));
    }

    template <class U> SimpleSTLAllocator(const SimpleSTLAllocator<U,Allocator>&){}
  
    void deallocate(pointer p, size_type n)
    {
        allocator::free(p, n);
    }

    void deallocate(void *p, size_type n)
    {
        allocator::free(p, n);
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
    
    bool operator==(const SimpleSTLAllocator &) const
    { return true; }
    bool operator!=(const SimpleSTLAllocator &) const
    { return false; }
};

//////////////////////////////////////////////
///
template <typename Allocator> class SimpleSTLAllocator<void,Allocator>
{
public:
    typedef void* pointer;
    typedef const void* const_pointer;
    // reference to void members are impossible.
    typedef void value_type;

    template <class U> 
        struct rebind { typedef SimpleSTLAllocator<U,Allocator> other; };
};    

}
#endif

