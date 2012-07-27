#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include "UnitTestFramework.h"
#include "ObjectPool.h"
#include "backend/_MemPool.h"

class TestObject
{
public:
	TestObject()
    {
        NUM_TESTOBJECTS++;
    }
	~TestObject()
    {
        NUM_TESTOBJECTS--;
    }

	void access(){memset(m_buf,0,1024*sizeof(char));};
protected:
	char m_buf[1024];
public:
    static int NUM_TESTOBJECTS;
};

int TestObject::NUM_TESTOBJECTS = 0;

class ObjectPoolTestCase : public UnitTestCase
{
public:
    ObjectPoolTestCase()
        : UnitTestCase("ObjectPool")
    {}
    virtual ~ObjectPoolTestCase(){}
    
    void run()
    {
        {
            ///using default allocator and allocate policy
            pool::ObjectPool<> objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            objPool.release();
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 0);
        }
        
        {
            ///using adaptive statistic policy with mmap/munmap allocator
            pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using adaptive statistic policy with new/delete allocator
            pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using adaptive statistic policy with malloc/free allocator
            pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using fixed size chunk  policy with mmap/munmap allocator
            pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using fixed size chunk  policy with new/delete allocator
            pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using fixed size chunk policy with malloc/free allocator
            pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using exponential increase chunk policy with mmap/munmap allocator
            pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using exponential increase chunk policy with new/delete allocator
            pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///using exponential increase chunk policy with malloc/free allocator
            pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > objPool;
            ///new TestObject from TestObject pool
            TestObject* pObj = pool::newObj<TestObject>(&objPool);
            TestObject* pObjArray = pool::newArray<TestObject>(&objPool,10);
	    pObjArray->access();
            int* pIntArray = pool::newVec<int>(&objPool,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
        }

        {
            ///example for MemPool which is a backend class
            MemPool mp;
            TestObject* pObj = NEW(&mp,TestObject);
            TestObject* pObjArray = NEW_ARRAY(&mp,TestObject,10);
	    pObjArray->access();
            int* pIntArray = NEW_VEC(&mp,int,100);
            
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 11);
            mp.reset();
            TEST_ASSERT(TestObject::NUM_TESTOBJECTS == 0);
        }
    }
};
