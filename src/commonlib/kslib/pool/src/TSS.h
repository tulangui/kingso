/**
 * @file TSS.h
 * @brief declare TSS class
 *
 * @version 1.0.0
 * @date 2009.1.21
 * @author ruijie.guo
 */

#ifndef __TSS_H
#define __TSS_H

#ifndef _WIN32
#   include <pthread.h>
#else
#   include <windows.h>
#endif

namespace pool{

#ifndef _WIN32
template<typename T>
class TSS
{
public:
    TSS()
    {
        if(pthread_key_create(&m_key,0) != 0)
        {
            assert(0);
        }
    }
    
    ~TSS()
    {
        pthread_key_delete(m_key);
    }
public:
    /**
     * get value
     * @return value
     */
    T get()const
    {
        return reinterpret_cast<T>(pthread_getspecific(m_key));
    }

    /**
     * store value 
     * @param value T
     */
    T set(T value)
    {
        T oldValue = get();
        pthread_setspecific(m_key,(void*)value);
        return oldValue;
    }
protected:
    pthread_key_t m_key;
};
#else

template<typename T>
class TSS
{
public:
    TSS()
    {
        m_key = ::TlsAlloc();
        m_bValid = (m_key != 0xFFFFFFFF);
    }
    
    ~TSS()
    {
        if(m_bValid)
            ::TlsFree(m_key);
    }
public:
    /**
     * get value
     * @return value
     */
    T get()const
    {
        assert(m_bValid);
        return reinterpret_cast<T>(::TlsGetValue(m_key));
    }

    /**
     * store value 
     * @param value T
     */
    T set(T value)
    {
        T oldValue = get();
        :TlsSetValue(m_key,(LPVOID)value);
        return oldValue;
    }
protected:
    DWORD m_key;
    bool m_bValid;
};
#endif

}///end of pool

#endif
