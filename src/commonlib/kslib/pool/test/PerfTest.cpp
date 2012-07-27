#include <stdio.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <map>
#include "ObjectPool.h"
#include "PooledSTLAllocator.h"

void perfTest();
void perfTestFromFile(const char* szFile);
void allocatorPerfTest(const size_t nTestTimes);

class Object
{
public:
	Object(){}
	~Object(){}

	void access()
    {
        for(size_t i = 0; i < 1024;i++)
            m_buf[i] = 0;
    }
protected:
	char m_buf[1024];
};

void usage()
{
    std::cout << "Usage:" << std::endl;
    std::cout << "      " << "perftest -pool [test file path]:test pool" << std::endl;
    std::cout << "      " << "perftest -allocator [test times]:test stl allocator" << std::endl;
    std::cout << "      " << "perftest -help :show help." << std::endl;
}

int main(int argc, char* argv[])
{
    if(argc == 1)
        perfTest();
    else if(argc >= 2)
    {
        if(!strcmp(argv[1],"-allocator") )
        {
            if(argc >= 3)
                allocatorPerfTest( atoi(argv[2]) );
            else 
                allocatorPerfTest( 1000000 );
        }
        else if(argc == 3 && !strcmp(argv[1],"-pool") )
            perfTestFromFile(argv[2]);
        else if(!strcmp(argv[1],"-help") )
            usage();
        else
            usage();
    }
}

template<class PoolType>
void perfTestObjectPool(PoolType* pObjPool,const char* szDesc)
{
    printf("%s\n",szDesc);
    size_t nTotal = 0,nUsed = 0,nReleaseTime = 0,nMax = 0,nMin = sizeof(Object);
	time_t t = time(NULL);
    size_t a;
	Object* pObj = NULL;
	Object* pObj2 = NULL;
	char* pBuf = NULL;
    printf("begin at :%d\n",t);


	for(int i = 0;i < 10000;i++)
	{
	    srand(time(0));
		for(int j = 0;j< 100; j++)
		{
			pObj = pool::newObj<Object>(pObjPool);
			pObj->access();
			a = rand() % 1000;
		    pObj2 = pool::newArray<Object>(pObjPool,a);
            if(a*sizeof(Object) > nMax )
                nMax = a * sizeof(Object);
			for(int n = 0;n < a;n++)
				pObj2[n].access();
			a = rand() % 1000000;
			pBuf = pool::newVec<char>(pObjPool,a);
            for(size_t i = 0;i < a; i++)
                *pBuf++ = 0;
            if(a < nMin)
                nMin = a;
            if(a > nMax )
                nMax = a;
		}
        const pool::MemStat st = pObjPool->getMemStat();
        nTotal += st.getTotalBytes();
        nUsed += st.getBytesUsed();
		pObjPool->release();
        nReleaseTime ++;
	}
	time_t t2 = time(NULL);
	printf("end at:%d\n",t2);
    printf("Total bytes:%lu,used bytes:%lu,usage ratio:%.2f\n",nTotal,nUsed,(double)nUsed/(double)nTotal);
    if(nReleaseTime > 0)
        printf("Releas Time:%d,max alloc bytes:%lu(%lu MB),min alloc bytes:%lu,avg bytes/pool:%lu(%lu MB)\n",nReleaseTime,nMax,nMax/1024/1024,nMin,nTotal/nReleaseTime,nTotal/nReleaseTime/1024/1024);
	printf("Total time:%d\n\n",time(NULL) - t);
}

void perfTest()
{
    {
	    char szDesc[] = "===ObjectPool performance test (MonotoneChunkPolicy with malloc/free allocator)";
	    pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > >(&objPool,szDesc);
	}
    
    {
	    char szDesc[] = "===ObjectPool performance test (MonotoneChunkPolicy with new/delete allocator)";
	    pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > >(&objPool,szDesc);
	}

    {
	    char szDesc[] = "===ObjectPool performance test (MonotoneChunkPolicy with mmap/munmap allocator)";
	    pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > >(&objPool,szDesc);
	}

    {
	    char szDesc[] = "===ObjectPool performance test (ExpIncChunkPolicy with malloc/free allocator)";
	    pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > >(&objPool,szDesc);
	}
    
    {
	    char szDesc[] = "===ObjectPool performance test (ExpIncChunkPolicy with new/delete allocator)";
	    pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > >(&objPool,szDesc);
	}

    {
	    char szDesc[] = "===ObjectPool performance test (ExpIncChunkPolicy with mmap/munmap allocator)";
	    pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > >(&objPool,szDesc);
	}
    
    {
	    char szDesc[] = "===ObjectPool performance test (AdaptiveStatPolicy with malloc/free allocator)";
	    pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > >(&objPool,szDesc);
	}
    
    {
	    char szDesc[] = "===ObjectPool performance test (AdaptiveStatPolicy with new/delete allocator)";
	    pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > >(&objPool,szDesc);
	}

    {
	    char szDesc[] = "===ObjectPool performance test (AdaptiveStatPolicy with mmap/munmap allocator)";
	    pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > objPool;
	    perfTestObjectPool<pool::ObjectPool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > >(&objPool,szDesc);
	}
}

template<class PoolType>
void perfTestPool(PoolType* pPool,const char* szTestFile,const char* szDesc)
{
    int fd = ::open(szTestFile,O_RDONLY,0644);
    if(fd  == -1)
    {
        std::cout << "Open test file failed." << std::endl;
        return;
    }
    struct stat statInfo;
    fstat(fd,&statInfo);
    size_t nFileLen = statInfo.st_size;
    std::vector<char*> frees;
    char* szContent = (char*)mmap(0,nFileLen,PROT_READ,MAP_SHARED,fd,0);
    size_t nCurPos = 0;
	time_t t = time(NULL);
    size_t nTotal = 0,nUsed = 0,nReleaseTime = 0,nMax = 0,nMin = (size_t)-1;
    std::cout << szDesc << std::endl;
    std::cout << "begin at:" << t << std::endl;
    while(nCurPos < nFileLen)
    {
        if(szContent[nCurPos] == '#')
        {
            if(pPool)
            {
                const pool::MemStat st = pPool->getMemStat();
                nTotal += st.getTotalBytes();
                nUsed += st.getBytesUsed();
                pPool->release();
                nCurPos += 2;
                nReleaseTime++;
            }
            else 
            {
                nCurPos += 2;
                for(std::vector<char*>::iterator iter = frees.begin();iter != frees.end();iter++)
                    ::free(*iter);
                frees.clear();
            }
                
        }
        else 
        {
            char* pTmp;
            int nAllocBytes = strtol( &(szContent[nCurPos]),&pTmp,10);
            if(nAllocBytes > 0)
            {
                if(pPool)
                {
                    char* p = (char*)pPool->alloc((const size_t)nAllocBytes);
                    *p = 0;
                }
                else
                {
                    char* p = (char*)::malloc(nAllocBytes);
                    *p = 0;
                    frees.push_back(p);
                }
            }
            nCurPos += (pTmp - (szContent + nCurPos) + 1);
            if(nAllocBytes > nMax)
                nMax = nAllocBytes;
            if(nAllocBytes > 0 && nAllocBytes < nMin)
                nMin = nAllocBytes;
        }
    }//end while
    if(pPool)
    {
        const pool::MemStat st = pPool->getMemStat();
        nTotal += st.getTotalBytes();
        nUsed += st.getBytesUsed();
    }
    else
    {
        for(std::vector<char*>::iterator iter = frees.begin();iter != frees.end();iter++)
            ::free(*iter);
        frees.clear();
    }
	
    time_t t2 = time(NULL);
	printf("end at:%d\n",t2);
    printf("Total bytes:%lu(%lu MB),used bytes:%lu(%lu MB),usage ratio:%.2f\n",nTotal,nTotal/1024/1024,nUsed,nUsed/1024/1024,(double)nUsed/(double)nTotal);
    if(nReleaseTime > 0)
        printf("Release Time:%d,max alloc bytes:%lu(%lu MB),min alloc bytes:%lu,avg bytes/pool:%lu(%lu MB)\n",nReleaseTime,nMax,nMax/1024/1024,nMin,nTotal/nReleaseTime,nTotal/nReleaseTime/1024/1024);
	printf("Total time:%d\n\n",t2 - t);
    
    munmap(szContent,nFileLen);
    ::close(fd);
}

void perfTestFromFile(const char* szTestFile)
{
    {
        char szDesc[] = "===Pool performance test ( malloc/free allocator)";
        perfTestPool<pool::Pool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > >(NULL,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (AdaptiveStatPolicy with malloc/free allocator)";
        pool::Pool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > pool;
        perfTestPool<pool::Pool<pool::AdaptiveStatPolicy<pool::MallocFreeAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (AdaptiveStatPolicy with new/delete allocator)";
        pool::Pool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > pool;
        perfTestPool<pool::Pool<pool::AdaptiveStatPolicy<pool::NewDeleteAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (AdaptiveStatPolicy with mmap/munmap allocator)";
        pool::Pool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > pool;
        perfTestPool<pool::Pool<pool::AdaptiveStatPolicy<pool::MMapAllocator> > >(&pool,szTestFile,szDesc);
    }

    {
        char szDesc[] = "===Pool performance test (ExpIncChunkPolicy with malloc/free allocator)";
        pool::Pool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > pool;
        perfTestPool<pool::Pool<pool::ExpIncChunkPolicy<pool::MallocFreeAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (ExpIncChunkPolicy with new/delete allocator)";
        pool::Pool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > pool;
        perfTestPool<pool::Pool<pool::ExpIncChunkPolicy<pool::NewDeleteAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (ExpIncChunkPolicy with mmap/munmap allocator)";
        pool::Pool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > pool;
        perfTestPool<pool::Pool<pool::ExpIncChunkPolicy<pool::MMapAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (MonotoneChunkPolicy with malloc/free allocator)";
        pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > pool;
        perfTestPool<pool::Pool<pool::MonotoneChunkPolicy<pool::MallocFreeAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (MonotoneChunkPolicy with new/delete allocator)";
        pool::Pool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > pool;
        perfTestPool<pool::Pool<pool::MonotoneChunkPolicy<pool::NewDeleteAllocator> > >(&pool,szTestFile,szDesc);
    }
    
    {
        char szDesc[] = "===Pool performance test (MonotoneChunkPolicy with mmap/munmap allocator)";
        pool::Pool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > pool;
        perfTestPool<pool::Pool<pool::MonotoneChunkPolicy<pool::MMapAllocator> > >(&pool,szTestFile,szDesc);
    }
}

void allocatorPerfTest(const size_t nTestTimes)
{
    {
        std::cout << "==vector with default allocator" << std::endl;
        size_t a;
        time_t t = time(NULL);
        std::cout << "begin at:" << t << std::endl;
        {
            std::vector<int> intVec;
            for(size_t i = 0;i < nTestTimes;i++)
            {
			    a = rand() % 3;
                if(a == 0 && !intVec.empty() )
                    intVec.pop_back();
                else 
                    intVec.push_back(i);
            }
        }
	    time_t t2 = time(NULL);
	    std::cout << "end at:" << t2 << std::endl;
	    std::cout << "Total time:" << t2 - t << std::endl << std::endl;
    }
    
    {
        std::cout << "==vector with simple segregated storage allocator" << std::endl;
        size_t a;
        time_t t = time(NULL);
        std::cout << "begin at:" << t << std::endl;
        {
            std::vector<int,pool::PooledSTLAllocator<int> > intVec;
            for(size_t i = 0;i < nTestTimes;i++)
            {
			    a = rand() % 3;
                if(a == 0 && !intVec.empty() )
                    intVec.pop_back();
                else 
                    intVec.push_back(i);
            }
        }
	    time_t t2 = time(NULL);
	    std::cout << "end at:" << t2 << std::endl;
	    std::cout << "Total time:" << t2 - t << std::endl << std::endl;
    }
}

