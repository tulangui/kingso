/**
 * @file SingletonSegregatedPool.h
 * @brief declare SingleSegregatedPool class
 *
 * @version 1.0.0
 * @date 2009.1.20
 * @author ruijie.guo
 */
#ifndef __SINGLETONSEGREGATEDPOOL_H
#define __SINGLETONSEGREGATEDPOOL_H

#include "Singleton.h"

namespace pool{

template<class T,unsigned RequestedSize>
class SegregatedPoolCreator
{
public:
    static void create(T*& ptr)
    {
        ptr = new T(RequestedSize);
        static Destroyer<T> destroyer(ptr);
    }
};

template<unsigned RequestedSize,typename PoolType,typename LockType = Mutex>
class SingletonSegregatedPool : public Singleton<PoolType,LockType,SegregatedPoolCreator<PoolType,RequestedSize> >
{
public:
    typedef PoolType pool_type;
    typedef LockType lock_type;
};

}///end of namespace pool

#endif
