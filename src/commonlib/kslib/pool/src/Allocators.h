/**
 * @file Allocators.h
 * @brief declare user allocators used for pool and stl allocators 
 *
 * @version 1.0.0
 * @date 2009.1.4
 * @author ruijie.guo
 */

#ifndef __ALLOCATORS_H
#define __ALLOCATORS_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <new>
#ifndef _WIN32
#include <sys/mman.h>
#endif

namespace pool{

///new/delete allocator
struct NewDeleteAllocator
{
    static void* malloc(const size_t nBytes){
        return static_cast<void*>(new (std::nothrow) char[nBytes]);
    }
    static void free(void* ptr,const size_t /*nBytes*/){
        delete[] (char*)ptr;
    }
};

///malloc/free allocator
struct MallocFreeAllocator
{
    static void* malloc(const size_t nBytes){
        return ::malloc(nBytes);
    }
    static void free(void* ptr,const size_t /*nBytes*/){
        ::free(ptr);
    }
};

//mmap/munmap allocator
struct MMapAllocator
{
    static void* malloc(const size_t nBytes){
        return mmap(NULL, nBytes,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1, 0);
    }
    static void free(void* ptr,const size_t nBytes){
        munmap(ptr,nBytes);
    }
};

}

#endif

