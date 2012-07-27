#include <stdio.h>
#include <time.h>
#include <vector>
#include <map>
#include "ObjectPool.h"
#include "backend/_MemPool.h"
#include "SimpleSTLAllocator.h"
#include "SimpleSegregatedPool.h"
#include "PooledSTLAllocator.h"

class Object
{
public:
	Object()
    {
        NUM_OBJECTS++;
    }
	~Object()
    {
        NUM_OBJECTS--;
    }

	void access(){memset(m_buf,0,1024*sizeof(char));};
protected:
	char m_buf[1024];
public:
    static int NUM_OBJECTS;
};

int Object::NUM_OBJECTS = 0;

int main(int argc, char* argv[])
{
    
    //===examples for pool ===/
    
    ///example for pool,using default allocator and allocate policy 
    pool::Pool<> pool1;
    pool1.setChunkSize(512);///set chunk size to 512 bytes
    char* p = (char*)pool1.alloc(10);///alloc 10 bytes from pool
    memset(p,0,10); 
    const pool::MemStat st = pool1.getMemStat();///get pool state
    printf("state of pool1:total:%d,free:%d,used:%d\n",st.getTotalBytes(),st.getBytesFree(),st.getBytesUsed() );
    pool1.release();///release pool

    ///example for pool,using malloc/free allocator and fixed size chunk policy 
    pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > pool2;
    p = (char*)pool2.alloc(10);///alloc 10 bytes from pool
    pool2.release();///release pool

    ///example for pool,using new/delete allocator and fixed size chunk policy 
    pool::Pool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > pool3;
    p = (char*)pool3.alloc(10);///alloc 10 bytes from pool
    pool3.release();///release pool

    ///example for pool,using mmap/munmap allocator and fixed size chunk policy 
    pool::Pool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > pool4;
    p = (char*)pool4.alloc(10);///alloc 10 bytes from pool
    pool4.release();///release pool
    
    ///example for pool,using new/delete allocator and exponential increase chunk policy 
    pool::Pool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > pool5;
    p = (char*)pool5.alloc(10);///alloc 10 bytes from pool
    pool5.release();///release pool
    
    ///example for pool,using new/delete allocator and adaptive statistic policy 
    pool::Pool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > pool6;
    p = (char*)pool6.alloc(10);///alloc 10 bytes from pool
    pool6.release();///release pool
    
    //===examples for object pool ===/
    
    ///example for object pool, using default allocator and allocate policy
    pool::ObjectPool<> objPool1;

    ///new object from object pool
    Object* pObj = pool::newObj<Object>(&objPool1);
    Object* pObjArray = pool::newArray<Object>(&objPool1,10);
    int* pIntArray = pool::newVec<int>(&objPool1,100);

    ///example for object pool, using default allocator and allocate policy
    pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > objPool2;

    ///new object from object pool
    pObj = pool::newObj<Object>(&objPool2);
    pObjArray = pool::newArray<Object>(&objPool2,10);
    pIntArray = pool::newVec<int>(&objPool2,100);

    ///example for object pool, using default allocator and allocate policy
    pool::ObjectPool<>* pObjPool = pool::ObjectPool<>::getPool();

    ///new object from object pool
    pObj = pool::newObj<Object>(pObjPool);
    pObjArray = pool::newArray<Object>(pObjPool,10);
    pIntArray = pool::newVec<int>(pObjPool,100);
    delete pObjPool;
    
    //===examples for MemPool ===/
    ///example for MemPool which is a backend class
    MemPool mp;
    pObj = NEW(&mp,Object);
    pObjArray = NEW_ARRAY(&mp,Object,10);
    pIntArray = NEW_VEC(&mp,int,100);
    
    //===examples for stl allocators ===/
    ///example for replacing std::vector allocator with simple allocator 
    std::vector<int,pool::SimpleSTLAllocator<int> >intVec;
    for(int i = 0; i < 10; i++)
        intVec.push_back(i);
        
    ///example for replacing std::vector allocator with simple allocator,using mmap/munmap to allocate memory 
    std::vector<int,pool::SimpleSTLAllocator<int,pool::MMapAllocator> >intVec2;
    for(int i = 0; i < 10; i++)
        intVec2.push_back(i);

    std::map<int,int,std::less<int>,pool::SimpleSTLAllocator<std::pair<const int,int>,pool::MMapAllocator > > intMap1;
    for(int i = 0; i < 10; i++)
        intMap1.insert(std::make_pair(i,i + 1));
    
    ///example for replacing std::vector allocator with pool allocator 
    std::vector<int,pool::PooledSTLAllocator<int> >intVec3;
    for(int i = 0; i < 10; i++)
        intVec3.push_back(i);
    ///pop the last element, the memory of the element is NOT freed until the pool is destroyed or released.
    intVec3.pop_back();
    
    ///example for replacing std::map allocator with pool allocator,using exponential increase chunk policy
    std::map<int,int,std::less<int>,pool::PooledSTLAllocator<std::pair<const int,int> > > intMap2;
    for(int i = 0; i < 10; i++)
        intMap2.insert(std::make_pair(i,i + 1));

    return 0;
}
