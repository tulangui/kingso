/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 368 $
 *
 * $LastChangedDate: 2011-04-06 13:49:09 +0800 (Wed, 06 Apr 2011) $
 *
 * $Id: MemPoolFactory.h 368 2011-04-06 05:49:09Z zhuangyuan $
 *
 * $Brief: memory pool factory $
 *******************************************************************
 */

#ifndef _MEM_POOL_FACTORY_H_
#define _MEM_POOL_FACTORY_H_

#include "framework/namespace.h"
#include "util/ThreadLock.h"
#include "util/MemPool.h"
#include "util/namespace.h"

FRAMEWORK_BEGIN;

class MemPoolFactory {
    public:
        /**
         * @brief constructor
         */
        MemPoolFactory();

        /**
         * @brief desconstructor
         */
        virtual ~MemPoolFactory();
    public:
        /**
         * @brief get memory pool pointer
         * @param key the key such as thread id
         * @return memory pool pointer, NULL for fail
         */
        MemPool *make(uint64_t key);
    private:
        std::map<uint64_t, MemPool*> _mp;
        UTIL::RWLock _lock;
};

FRAMEWORK_END;

#endif //_MEM_POOL_FACTORY_H_
