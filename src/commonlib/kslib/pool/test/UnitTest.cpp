#include "PoolTest.h"
#include "SimpleSTLAllocatorTest.h"
#include "PooledSTLAllocatorTest.h"
#include "ObjectPoolTest.h"
#include "MemMonitorTest.h"

int main()
{
    UnitTestFramework& runner = UnitTestFramework::getInstance();
    runner.addCase(new PoolTestCase());
    runner.addCase(new SimpleSTLAllocatorTestCase());
    runner.addCase(new PooledSTLAllocatorTestCase());
    runner.addCase(new ObjectPoolTestCase());
    runner.addCase(new MemMonitorTestCase());
    runner.run();
    return 0;
}
