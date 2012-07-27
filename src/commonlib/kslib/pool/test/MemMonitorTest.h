#include <stdio.h>
#include <stdlib.h>
#include "UnitTestFramework.h"
#include "MemMonitor.h"
#include "backend/_MemPool.h"

class MemMonitorTestCase : public UnitTestCase
{
public:
    class Object
    {
    public:
        Object() {
            a[0] = 16;            
        }
        ~Object() {
        }
        
        bool isInit() const {
            return a[0] == 16;
        }
        
    protected:
        int a[100];        
    };
    
public:
    MemMonitorTestCase()
        : UnitTestCase("MemMonitor")
    {}
    virtual ~MemMonitorTestCase(){}
    
    void run()
    {
        {
            MemPool mp;        
            TEST_ASSERT(mp.getMonitor() == NULL);
            pool::MemMonitor mm(&mp, 100 * 1024 * 1024);
            TEST_ASSERT(mp.getMonitor() != NULL);
            TEST_ASSERT(mm.getExceptionStat() == false);
            
            try
            {
                mp.alloc(10 * 1024 * 1024, NULL);
                mp.alloc(100 * 1024 * 1024, NULL);
                TEST_ASSERT(mp.getAllocatedMemSize() == 110  * 1024 * 1024);                
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                TEST_ASSERT(false);
            }

            mm.enableException();            
            bool bOver = false;            
            try
            {
                mp.alloc(100 * 1024 * 1024, NULL);
                TEST_ASSERT(mp.getAllocatedMemSize() == 210  * 1024 * 1024);
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                bOver = true;                
            }
            TEST_ASSERT(bOver);                        
        }        

        {
            MemPool mp;        
            TEST_ASSERT(mp.getMonitor() == NULL);
            pool::MemMonitor mm(&mp, 100 * 1024 * 1024);
            TEST_ASSERT(mp.getMonitor() != NULL);
            TEST_ASSERT(mm.getExceptionStat() == false);
            
            try
            {
                int *a = NEW(&mp, int);
                TEST_ASSERT(mp.getAllocatedMemSize() == sizeof(int));                
                int *aa = NEW_VEC(&mp, int, 10);
                TEST_ASSERT(mp.getAllocatedMemSize() == 11 * sizeof(int));
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                TEST_ASSERT(false);
            }
        }
        
        {
            MemPool mp;        
            TEST_ASSERT(mp.getMonitor() == NULL);
            pool::MemMonitor mm(&mp, 100 * 1024 * 1024);
            TEST_ASSERT(mp.getMonitor() != NULL);
            TEST_ASSERT(mm.getExceptionStat() == false);
            
            try
            {
                Object *a = NEW(&mp, Object);
                TEST_ASSERT(mp.getAllocatedMemSize() == sizeof(Object));                
                Object *aa = NEW_ARRAY(&mp, Object, 10);
                TEST_ASSERT(mp.getAllocatedMemSize() == 11 * sizeof(Object));
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                TEST_ASSERT(false);
            }
        }        

        {
            MemPool mp;        
            TEST_ASSERT(mp.getMonitor() == NULL);
            pool::MemMonitor mm(&mp, 100 * 1024 * 1024);
            TEST_ASSERT(mp.getMonitor() != NULL);
            TEST_ASSERT(mm.getExceptionStat() == false);
            
            try
            {
                Object *a = newType<Object>(&mp);
                TEST_ASSERT(mp.getAllocatedMemSize() == sizeof(Object));                
                Object *aa = newArray<Object>(&mp, 10);
                TEST_ASSERT(mp.getAllocatedMemSize() == 11 * sizeof(Object));
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                TEST_ASSERT(false);
            }
        }        

        {
            MemPool mp;        
            TEST_ASSERT(mp.getMonitor() == NULL);
            
            try
            {
                Object *a = NEW(&mp, Object);
                TEST_ASSERT(mp.getAllocatedMemSize() == 0);                
                Object *aa = NEW_ARRAY(&mp, Object, 10);
                TEST_ASSERT(mp.getAllocatedMemSize() == 0);
            }
            catch(const pool::MemMonitor::OverflowException& )
            {
                TEST_ASSERT(false);
            }
        }        
    }
};
