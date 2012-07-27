#include "framework/ParallelCounter.h"
#include "util/ScopedLock.h"
#include "framework/Session.h"

FRAMEWORK_BEGIN;

bool ParallelCounter::inc() 
{
    MUTEX_LOCK(_lock);
    _total++;
    if (_cur == 0) {
        _seriateErrorCount = 0;
        _seriateTimeoutCount = 0;
    }
    if (!isOK() || (_max > 0 && _cur >= _max)) {
        return false;
    }
    _cur++;
    //stablizing the system when it is attacked by abnormal queries	
    _lastServiceTime = Session::getCurrentTime();
    // ended
    MUTEX_UNLOCK(_lock);
    return true;
}

bool ParallelCounter::dec(uint64_t cost, bool error) 
{
    MUTEX_LOCK(_lock);
    if (_cur == 0) {
        return false;
    }
    _cur--;
    if (_timeout > 0 && cost >= _timeout && _cur!=0) {
        _timeoutCount++;
        _seriateTimeoutCount++;
    } 
    else {
        _seriateTimeoutCount = 0;
    }
    if (error && _cur!=0) {
        _errorCount++;
        _seriateErrorCount++;
    } 
    else {
        _seriateErrorCount = 0;
    }
    MUTEX_UNLOCK(_lock);
    return true;
}

bool ParallelCounter::stat(uint64_t start, uint64_t end)
{
    uint64_t use = end - start;

    MUTEX_LOCK(_statLock);
    _allStatCount++;
    _allStatTime += use;

    if(end - _startStatTime > 1000 * 1000 * 30) { // 30秒统计一次
        uint32_t allTime = (start - _startStatTime) / (1000 * 1000);
        if(allTime == 0) allTime = 1;
        _lastQps = _allStatCount / allTime;
        _lastLatency = _allStatTime / _allStatCount;

        _allStatTime = 0;
        _allStatCount = 0;
        _startStatTime = end;
    }
    MUTEX_UNLOCK(_statLock);
    return true;
}

FRAMEWORK_END;

