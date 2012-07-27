#ifndef __SINGLETON_H
#define __SINGLETON_H

#include "Guard.h"

namespace pool{

template<class T>
class Destroyer
{
public:
    Destroyer(T* pObj)
        : m_pObj(pObj)
    {
    }
    ~Destroyer()
    {
        if(m_pObj)
            delete m_pObj;
        m_pObj = NULL;
    }
private:
    T* m_pObj;
};

template<class T,class CreatePolicy,class LockType>
class Singleton;

template<class T>
class DefaultCreator
{
public:
    static void create(T*& ptr)
    {
        ptr = new T;
        static Destroyer<T> destroyer(ptr);
    }
};

template<class T,class LockType = Mutex,class CreatePolicy = DefaultCreator<T> >
class Singleton
{
public:
    typedef T value_type;
    typedef CreatePolicy create_policy;
    typedef LockType lock_type;
public:
    /**
     * get instance
     */
    static T* instance();
private:
    Singleton();
    Singleton(const Singleton<T,LockType,CreatePolicy>&);
    void operator=(const Singleton<T,LockType,CreatePolicy>&);
};

//////////////////////////////////////////
///implementation

template<class T,class LockType,class CreatePolicy>
T* Singleton<T,LockType,CreatePolicy>::instance()
{
    static T* ptr = 0;    
    static LockType lock;

    if(!ptr) 
    {
        Guard<LockType> l(lock);
        if(!ptr)        
            CreatePolicy::create(ptr);
    }
    return const_cast<T*>(ptr);
}

}//end of namespace pool
#endif
