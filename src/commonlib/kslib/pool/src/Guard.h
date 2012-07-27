#ifndef __POOL_GUARD_H
#define __POOL_GUARD_H
#include "Mutex.h"

namespace pool{

template<class LockType = Mutex>
class Guard
{
public:
    typedef LockType lock_type;
public:
    explicit Guard(lock_type& lock)
        : m_locker(lock)
    {
        lock.lock();
    }
    ~Guard()
    {
        m_locker.unlock();
    }
private:
    Guard(const Guard&);
    void operator=(const Guard&);
private:
    lock_type& m_locker;
};

} //end of namespace pool

#endif
