#include <stdio.h>
#include <stdlib.h>
#include "UnitTestFramework.h"
#include "Pool.h"

class PoolTestCase : public UnitTestCase
{
public:
    PoolTestCase()
        : UnitTestCase("Pool")
    {}
    virtual ~PoolTestCase(){}
    
    void run()
    {
        {
            ///using default allocate policy and allocator
            pool::Pool<> pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
        }
        
        {
            ///using fixed size chunk policy and new/delete allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
        
        {
            ///using fixed size chunk policy and mmap/munmap allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
        
        {
            ///using fixed size chunk policy and malloc/free allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
        
        {
            ///using exponential increase chunk policy and new/delete allocator
            pool::Pool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
        
        {
            ///using exponential increase chunk policy and mmap/munmap allocator
            pool::Pool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
        
        {
            ///using exponential increase chunk policy and malloc/free allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
            
            pool.setChunkSize(2048);
            p = (char*)pool.alloc(512);
            const pool::MemStat st3 = pool.getMemStat();
            TEST_ASSERT(st3.getTotalBytes() == 2048);
        }
    
        {
            ///using adaptive statistical policy and new/delete allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
        }
        
        {
            ///using adaptive statistical policy and mmap/munmap allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
        }
        
        {
            ///using adaptive statistical policy and malloc/free allocator
            pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > pool;
            char* p = (char*)pool.alloc(1024);
            memset(p,0,1024 * sizeof(char));
            const pool::MemStat st = pool.getMemStat();
            TEST_ASSERT(st.getBytesUsed() == 1024);
            pool.release();
            const pool::MemStat st2 = pool.getMemStat();
            TEST_ASSERT(st2.getBytesUsed() == 0);
        }
    }
};
