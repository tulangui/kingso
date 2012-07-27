
/** \file
 *******************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 748 $
 *
 * $LastChangedDate: 2011-04-14 13:55:03 +0800 (Thu, 14 Apr 2011) $
 *
 * $Id: hashmap.h 748 2011-04-14 05:55:03Z gongyi.cl $
 *
 *******************************************************************
 */
#ifndef _HASH_MAP_H
#define _HASH_MAP_H

#include <stdint.h>
//#include "namespace.h"


//UTIL_BEGIN
#ifdef __cplusplus
extern "C" {
#endif


#define HM_FLAG_POOL 1

    typedef void *hashmap_t;

    /**
     * @brief 比较函数。
     * @return ==0  相等
     * @return <>0   不等
     */
    typedef int32_t(*HM_FUNC_COMPARE) (const void *key1, const void *key2);
    typedef uint64_t(*HM_FUNC_HASH) (const void *key);
    typedef int (*HM_FUNC_WALK) (void *key, void *value, void *user_data);
    typedef void (*HM_FUNC_ON_DESTROY) (void *key, void *value,
                                        void *user_data);

/**
 * @brief 创建hashtable.
 * 
 * @param buckets 指定bucket数量
 * @param flags   HM_FLAG_POOL 使用对象池,可大量减少分配内存次数
 * @param cb_hash 指定hash方法
 * @param cb_cmp  指定value比较方法
 *
 * @return NULL 错误
 * @return NOT_NULL hashmap_t对象
 */
    extern hashmap_t hashmap_create(uint32_t buckets, uint32_t flags,
                                    HM_FUNC_HASH cb_hash,
                                    HM_FUNC_COMPARE cb_cmp);

/**
 * @brief 销毁hashmap
 *
 * @param h      hashmap_create建立的对象
 * @param cb     销毁时将为每一对key/value调用此函数,
 * @param user_data 传入cb的user_data参数
 *
 */
    void hashmap_destroy(hashmap_t h, HM_FUNC_ON_DESTROY cb,
                         void *user_data);

/**
 * @brief 在hashmap插入一个元素
 * 
 * @param h     hashmap_create建立的对象
 * @param key   key
 * @param value value
 *
 * @return 0    成功
 * @return -1   失败
 */
    int32_t hashmap_insert(hashmap_t h, void *key, void *value);

/**
 * @brief 在hashmap中查找
 *
 * @param h     hashmap_create建立的对象
 * @param key   key
 *
 * @return NULL 未找到
 * @return ptr  value指针
 */
    extern void *hashmap_find(hashmap_t h, void *key);

/**
 * @brief 回调方式遍历hashmap所有元素
 * 
 * @param h         hashmap_create建立的对象
 * @param cb_walk   对每一个元素调用cb_walk(),cb_walk返回非0值时停止.
 * @param user_data 传入cb_walk的用户参数
 * $return >=0 数量
 * $return <0 错误
 */
    int32_t hashmap_walk(hashmap_t h, HM_FUNC_WALK cb_walk,
                         void *user_data);

/**
 * @brief 重置一次遍历
 *
 * @param h         hashmap_create建立的对象
 */
    void hashmap_it_reset(hashmap_t h);

/**
 * @brief 迭代方式遍历(以插入顺序),获取下一个元素
 * 
 * @param h         hashmap_create建立的对象
 * @return ptr 元素的value
 * @return NULL 到达末尾
 */
    void *hashmap_it_next(hashmap_t h);

#ifdef __cplusplus
}
#endif
//UTIL_END
#endif
