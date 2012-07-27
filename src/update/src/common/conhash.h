#ifndef _CONHASH_H_
#define _CONHASH_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NODE_COUNT_MAX 64

/* 一致性hash数据结构 */
typedef struct
{
    unsigned int map[65536];    // 节点在65535个虚拟节点中的分布
    int node_count;             // 节点数量
}
conhash_t;

typedef union
{
    int intv;
    struct{
        unsigned char real_hash;
        unsigned char valid_hash;
    };
}
conhash_result_t;

/* 创建一致性hash结构
 *
 * node_count : 节点数量
 *
 * return : 新创建的结构
 */
extern conhash_t* conhash_create(int node_count);

/* 销毁一致性hash结构
 *
 * ch : 一致性hash结构
 */
extern void conhash_dest(conhash_t* ch);

/* 需找value对应的节点
 *
 * ch     : 一致性hash结构
 * value  : 待hash的数值
 * status : 节点状态
 *
 * return : value对应的节点编号
 */
extern int conhash_find(conhash_t* ch, uint64_t value, uint64_t status);

#ifdef __cplusplus
}
#endif

#endif
