#include "util/Timer.h"
#include <stdio.h>    // 仅用于测试
#include <assert.h>   // 仅用于测试

// 测试用例，兼做使用说明。
int main()
{
    // TestCase Group1:
    uint64_t uTime = UTIL::Timer::getTime();
    uint64_t uMilliTime = UTIL::Timer::getMilliTime();
    uint64_t uMicroTime = UTIL::Timer::getMicroTime();
    fprintf(stdout, "Timer::getTime():\t%lu\n", uTime);
    fprintf(stdout, "Timer::getMilliTime():\t%lu\n", uMilliTime);
    fprintf(stdout, "Timer::getMicroTime():\t%lu\n", uMicroTime);
    assert(uMicroTime / 1000ul == uMilliTime);
    assert(uMilliTime / 1000ul == uTime);

    // TestCase Group2:
    UTIL::Timer timer1, timer2;
    timer1.startTimer();
    timer1.startMilliTimer();
    timer1.startMicroTimer();
    timer2.startTimer();
    timer2.startMilliTimer();
    timer2.startMicroTimer();
    // elapse some time
    for (int i = 0; i < 100000000; i++) {
        int j = i * i;
        j *= j;
    }
    //
    uint64_t uElapsedTimer1 = timer1.stopTimer();
    uint64_t uElapsedMilliTimer1 = timer1.stopMilliTimer();
    uint64_t uElapsedMicroTimer1 = timer1.stopMicroTimer();
    // elapse some time
    for (int i = 0; i < 1000000; i++) {
        int j = i * i;
        j *= j;
    }
    //
    uint64_t uElapsedTimer2 = timer2.stopTimer();
    uint64_t uElapsedMilliTimer2 = timer2.stopMilliTimer();
    uint64_t uElapsedMicroTimer2 = timer2.stopMicroTimer();
    //
    fprintf(stdout, "uElapsedTimer1:\t%lu\n", uElapsedTimer1);
    fprintf(stdout, "uElapsedMilliTimer1:\t%lu\n", uElapsedMilliTimer1);
    fprintf(stdout, "uElapsedMicroTimer1:\t%lu\n", uElapsedMicroTimer1);
    fprintf(stdout, "uElapsedTimer2:\t%lu\n", uElapsedTimer2);
    fprintf(stdout, "uElapsedMilliTimer2:\t%lu\n", uElapsedMilliTimer2);
    fprintf(stdout, "uElapsedMicroTimer2:\t%lu\n", uElapsedMicroTimer2);
    assert(uElapsedMicroTimer1 / 1000ul == uElapsedMilliTimer1);
    assert(uElapsedMilliTimer1 / 1000ul == uElapsedTimer1);
    assert(uElapsedMicroTimer2 / 1000ul == uElapsedMilliTimer2);
    assert(uElapsedMilliTimer2 / 1000ul == uElapsedTimer2);
    
    return 0;
}

