/*********************************************************************
 * $Author: qiaoyin.dzh $
 *
 * $LastChangedBy: qiaoyin.dzh $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: StorableMessage.h 2577 2011-03-09 01:50:55Z qiaoyin.dzh $
 *
 * $Brief: 可存储的消息对象定义 . $
 ********************************************************************/

#ifndef _STORABLEMESSAGE_H_
#define _STORABLEMESSAGE_H_

#include <stdio.h>
#include <stdint.h>
#include "update/update_def.h"

namespace update {

/** 
 * @breaf: 内存释放函数原型 
 */
typedef void (*fn_free)(void*);

/**
 * 可储存的消息, 表征结构化数据, 实际序列化与反序列化操作另外完成.
 * 其中ptr域与freeFn必须配套赋值，否则可能出现内存泄漏.
 */
struct StorableMessage
{ 
    StorableMessage() : ptr(0),max(0),freeFn(0),size(0),
        action(ACTION_NONE),cmpr('N'),nid(0),seq(0),distribute(0){
    }
    ~StorableMessage() {
        if(ptr && freeFn) freeFn(ptr);
        ptr = NULL;
    }

    static void non_free(void*) { }

    char* ptr;              //结构序列化后起始地址, 字节流
    uint32_t max;           //内存地址大小
    fn_free freeFn;         //ptr释放方法，若为空则不在析构时释放.

    // 需要保存在index中
    uint32_t size;          //结构序列化后大小
    uint8_t action;         //操作类型，因排重操作可能与消息体中的类型不一致
    char cmpr;              //压缩标志
    uint64_t nid;           //宝贝id  
    uint64_t seq;           //数据编号  
    uint64_t distribute;    //分发id

    private:
    /**
     * 禁止拷贝构造和赋值函数, 因可能存在深度拷贝.
     */
    StorableMessage(const StorableMessage &);
    StorableMessage& operator=(const StorableMessage &);
};

}

#endif //_STORABLEMESSAGE_H_
