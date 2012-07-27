/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 254 $
 *
 * $LastChangedDate: 2011-03-31 18:00:09 +0800 (Thu, 31 Mar 2011) $
 *
 * $Id: Context.h 254 2011-03-31 10:00:09Z taiyi.zjh $
 *
 * $Brief: context which used in search procedure$
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_CONTEXT_H_
#define _FRAMEWORK_CONTEXT_H_
#include "util/common.h"
#include "util/namespace.h"
#include "util/MemPool.h"
#include "framework/namespace.h"

#define MAX_ERR_LENGTH 256

class MemPool;

FRAMEWORK_BEGIN

class Context{
    public:
        Context():
            _memPool(NULL) 
        {
            *_msg = '\0';
        }
        ~Context() {;}
        /**
         *@brief 获取内存池对象
         *@return 内存池对象指针
         */
        MemPool *getMemPool() const {return _memPool;}
        /**
         *@brief 获取提示信息
         *@return 提示信息
         */
        const char *getErrorMessage() const {return _msg;}
        /**
         *@brief 被调用方（模块）call，出错时在调用本函数，
         *       设置对调用方友好的提示信息
         *@param msg 提示信息
         */
        void setErrorMessage(const char *msg);
    public:
        /**
         *@brief 设置内存池对象
         *@param memPool 内存池对象指针
         */
        void setMemPool(MemPool *memPool) {_memPool = memPool;}
    private:
        MemPool *_memPool;
        char _msg[MAX_ERR_LENGTH];
};

inline void Context::setErrorMessage(const char *msg)
{
    if (msg != NULL)
    {
        snprintf(_msg, MAX_ERR_LENGTH, "%s", msg);
    }
    return;
}

FRAMEWORK_END

#endif //_FRAMEWORK_COMMAND_H_

