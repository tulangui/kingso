#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "UnitTestFramework.h"
#include "SimpleSTLAllocator.h"

class SimpleSTLAllocatorTestCase : public UnitTestCase
{
public:
    SimpleSTLAllocatorTestCase()
        : UnitTestCase("SimpleSTLAllocator")
    {}
    virtual ~SimpleSTLAllocatorTestCase(){}
    
    void run()
    {
        std::vector<int,pool::SimpleSTLAllocator<int> >intVec;
        for(int i = 0; i < 10; i++)
            intVec.push_back(i);
        intVec.clear();
        
        ///mmap/munmap allocator
        std::vector<int,pool::SimpleSTLAllocator<int,pool::MMapAllocator> >intVec2;
        for(int i = 0; i < 10; i++)
            intVec2.push_back(i);
        intVec2.clear();
        
        ///new/delete allocator
        std::vector<int,pool::SimpleSTLAllocator<int,pool::NewDeleteAllocator> >intVec3;
        for(int i = 0; i < 10; i++)
            intVec3.push_back(i);
        TEST_ASSERT(intVec3.size() == 10);
        intVec3.clear();
        
        ///malloc/free allocator
        std::vector<int,pool::SimpleSTLAllocator<int,pool::MallocFreeAllocator> >intVec4;
        for(int i = 0; i < 11; i++)
            intVec4.push_back(i);
        TEST_ASSERT(intVec4.size() == 11);
        intVec4.clear();
    }
};
