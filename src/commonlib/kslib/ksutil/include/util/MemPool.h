/**
 * @file    MemPool.h
 * @author  王炜 <santai.ww@taobao.com>
 * @version 1.0
 * @section Date:           2011-03-09
 * @section Description:    基础库Pool（内存分配器）的包装类。
 * @section Others:         
 * @section Usage:
 *     MemPool的功能：负责内存的分配，提供分配空间接口，不提供内存的释放接口，
 *     即用户只管申请内存不用考虑内存的释放，在Pool销毁或reset()时统一释放。
 *     （1）头文件
 *         #include "util/MemPool.h"
 *     （2）推荐用法：
 *     首先构造MemPool对象
 *         MemPool objMemPool;
 *         MemPool *pMemPool = &objMemPool;
 *     分配基本类型（如：char, int, float）的数组
 *         int32_t nCount = 10;
 *         char *pszArray = NEW_VEC(pMemPool, char, nCount);
 *         int32_t *pnArray = NEW_VEC(pMemPool, int32_t, nCount);
 *         float *pfArray = NEW_VEC(pMemPool, float, nCount);
 *     假设有一个类：
 *         class MyClass {...};
 *     构造一个对象，无参构造。
 *         MyClass *pMyClass = NEW(pMemPool, MyClass);
 *     构造一个对象，有参构造。
 *         MyClass *pMyClass = NEW(pMemPool, MyClass)(param1, param2);
 *     构造对象数组，仅支持无参构造。
 *         MyClass *pMyClassArray = NEW_ARRAY(pMemPool, MyClass, nCount);
 * @section Copyright (C), 2000-2010, Taobao Tech. Co., Ltd.
 */

#ifndef _UTIL_MEMPOOL_H
#define _UTIL_MEMPOOL_H

#include <pool/_MemPool.h>
#include "util/namespace.h"
#include "util/Log.h"

using pool::MemMonitor;

#define MMONITOR_BEGIN(p) try{ MemMonitorAutoControler mmc(p->getMonitor());
#define MMONITOR_BEGIN_NOEXCEPTION try{
#define MMONITOR_TRACK(p,nSize) if(p && p->getMonitor()) p->getMonitor()->memAllocated(nSize);
#define MMONITOR_END(p,e,b) }\
                          catch(const MemMonitor::OverflowException& oe){\
                              TERR("Memory in one search is overload (used:%d,max:%d)", oe.getAllocatedSize(), p->getMonitor()->getMaxMem()); \
                              e = b;\
                          }

class MemMonitorAutoControler
{
private:
    MemMonitor * m_pMemMonitor;
    bool last;
public:
    MemMonitorAutoControler(MemMonitor * mm = NULL):m_pMemMonitor(mm),last(false)
    {
        if(m_pMemMonitor) {
            last = m_pMemMonitor->getExceptionStat();
            m_pMemMonitor->enableException();
        }
    }
    ~MemMonitorAutoControler()
    {
        stop();	
    }
    void stop()
    {
        if(m_pMemMonitor) {
            if(!last) {
                m_pMemMonitor->disableException();
            }
            m_pMemMonitor = NULL;
        }
    }
};

#endif
