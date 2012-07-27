#ifndef __POOL_MUTEX_H
#define __POOL_MUTEX_H

#include <pthread.h>

namespace pool{

class Mutex
{
public:
    Mutex()
    {
        ::pthread_mutex_init(&m_mutex,NULL);
    }
    ~Mutex()
    {
        ::pthread_mutex_destroy(&m_mutex);
    }

    void lock()
    {
        ::pthread_mutex_lock(&m_mutex);
    }

    void unlock()
    {
        ::pthread_mutex_unlock(&m_mutex);
    }
private:
    pthread_mutex_t m_mutex;
};
    
class NullMutex
{
public:
    NullMutex()
    {
    }
    ~NullMutex()
    {
    }

    void lock()
    {
    }

    void unlock()
    {
    }
};

}//end of namespace pool

#endif

