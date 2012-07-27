#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "UnitTestFramework.h"
#include "PooledSTLAllocator.h"

class PooledSTLAllocatorTestCase : public UnitTestCase
{
public:
    PooledSTLAllocatorTestCase()
        : UnitTestCase("PooledSTLAllocator")
    {}
    virtual ~PooledSTLAllocatorTestCase(){}
    
    void run()
    {
        std::vector<int,pool::PooledSTLAllocator<int> >intVec1;
        for(int i = 0; i < 11; i++)
            intVec1.push_back(i);
        intVec1.pop_back();
        TEST_ASSERT(intVec1.size() == 10);
        intVec1.clear();
       /* 
        ///Fixed size chunk policy with mmap/munmap allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::FixedSizeChunkPolicy<pool::MMapAllocator> > >intVec2;
        for(int i = 0; i < 10; i++)
            intVec2.push_back(i);
        intVec2.clear();
        
        ///Fixed size chunk policy with new/delete allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::FixedSizeChunkPolicy<pool::NewDeleteAllocator> > >intVec3;
        for(int i = 0; i < 10; i++)
            intVec3.push_back(i);
        TEST_ASSERT(intVec3.size() == 10);
        intVec3.clear();
        
        ///Fixed size chunk policy with malloc/free allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::FixedSizeChunkPolicy<pool::MallocFreeAllocator> > >intVec4;
        for(int i = 0; i < 11; i++)
            intVec4.push_back(i);
        TEST_ASSERT(intVec4.size() == 11);
        intVec4.clear();
        
        ///exponential increase chunk policy with mmap/munmap allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::ExpIncChunkPolicy<pool::MMapAllocator> > >intVec5;
        for(int i = 0; i < 10; i++)
            intVec5.push_back(i);
        TEST_ASSERT(intVec5.size() == 10);
        intVec5.clear();
        
        ///exponential increase chunk policy with new/delete allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > >intVec6;
        for(int i = 0; i < 10; i++)
            intVec6.push_back(i);
        TEST_ASSERT(intVec6.size() == 10);
        intVec6.clear();
        
        ///exponential increase chunk policy with malloc/free allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::FixedSizeChunkPolicy<pool::MallocFreeAllocator> > >intVec7;
        for(int i = 0; i < 11; i++)
            intVec7.push_back(i);
        TEST_ASSERT(intVec7.size() == 11);
        intVec7.clear();
        
        ///adaptive statistic policy with mmap/munmap allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::AdaptiveStatPolicy<pool::MMapAllocator> > >intVec8;
        for(int i = 0; i < 10; i++)
            intVec8.push_back(i);
        TEST_ASSERT(intVec8.size() == 10);
        intVec8.clear();
        
        ///adaptive statistic policy with new/delete allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > >intVec9;
        for(int i = 0; i < 10; i++)
            intVec9.push_back(i);
        TEST_ASSERT(intVec9.size() == 10);
        intVec9.clear();
        
        ///adaptive statistic policy with malloc/free allocator
        std::vector<int,pool::PooledSTLAllocator<int,pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > >intVec10;
        for(int i = 0; i < 11; i++)
            intVec10.push_back(i);
        TEST_ASSERT(intVec10.size() == 11);
        intVec10.clear();*/
    }
};
