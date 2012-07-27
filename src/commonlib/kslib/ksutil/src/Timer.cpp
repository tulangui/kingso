#include "util/Timer.h"
#include <stdio.h>

UTIL_BEGIN;

Timer::Timer()
{
}

Timer::~Timer()
{
}

uint64_t Timer::getTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (static_cast<uint64_t>(t.tv_sec));
}

uint64_t Timer::getMilliTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (static_cast<uint64_t>(t.tv_sec) * 1000ul
        + static_cast<uint64_t>(t.tv_usec) / 1000ul);
}

uint64_t Timer::getMicroTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (static_cast<uint64_t>(t.tv_sec) * 1000000ul
        + static_cast<uint64_t>(t.tv_usec));
}

void Timer::startTimer()
{
    gettimeofday(&m_Timer, NULL);
}

uint64_t Timer::stopTimer()
{
    struct timeval stopTimer;
    gettimeofday(&stopTimer, NULL);
    return (static_cast<uint64_t>(stopTimer.tv_sec - m_Timer.tv_sec));
}

void Timer::startMilliTimer()
{
    gettimeofday(&m_MilliTimer, NULL);
}

uint64_t Timer::stopMilliTimer()
{
    struct timeval stopTimer;
    gettimeofday(&stopTimer, NULL);
    return (static_cast<uint64_t>(stopTimer.tv_sec - m_MilliTimer.tv_sec) * 1000ul
        + static_cast<uint64_t>(stopTimer.tv_usec - m_MilliTimer.tv_usec) / 1000ul);
}

void Timer::startMicroTimer()
{
    gettimeofday(&m_MicroTimer, NULL);
}

uint64_t Timer::stopMicroTimer()
{
    struct timeval stopTimer;
    gettimeofday(&stopTimer, NULL);
    return (static_cast<uint64_t>(stopTimer.tv_sec - m_MicroTimer.tv_sec) * 1000000ul
        + static_cast<uint64_t>(stopTimer.tv_usec - m_MicroTimer.tv_usec));
}

UTIL_END;

