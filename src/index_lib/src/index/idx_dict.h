/********************************************************************************
 * Describe : a string hash table, with 3 unsigned integer values.
 *          : surpport 'ADD', 'SAVE', 'LOAD' operation
 *          : surpport dynamic increse
 *          : using 64 bit signature to stand for string, witch increase speed and
 *          : decrease memory usage.
 *
 * Author   : Shituo.lyc, lightningchain@163.com
 *
 * Create   : 2010-11-22
 *******************************************************************************/
#ifndef IDX_DICT_H
#define IDX_DICT_H

#include "util/idx_sign.h"
#include <time.h>
//#include "IndexFieldSyncManager.h"

// data structure define here
//
// hash表的节点结构体
#pragma pack (1)
typedef struct _idict_node_t {
	uint64_t          sign;      // key: 64 bit sign
	uint32_t          cuint1;    // value1
	uint32_t          cuint2;    // value2
	uint32_t          cuint3;    // value3
	unsigned int      next;      // point to next node with the same hash
} idict_node_t;
#pragma pack ()

// hash表结构体
typedef struct _idx_dict_t {
    idict_node_t     *block;       // 节点数组
    idict_node_t     *old_block;   // 双buffer删除数组
    time_t            canFreeTime; // 删除时间
    unsigned int      block_pos;   // 节点数组已使用的个数
	unsigned int      block_size;  // 节点数组的大小
	unsigned int     *hashtab;     // hash数组，数组的每个元素指向hash值为数组下标的一个链表
	unsigned int      hashsize;    // hash数组的大小
    void             *syncMgr;     // 持久化操作管理队列
} idx_dict_t;


// functions defined here
//

/*
 * func : create an idx_dict_t struct
 *
 * args : hashsize, the hash table size
 *      : nodesize, the node array size
 *
 * ret  : NULL, error;
 *      : else, pointer the the idx_dict_t struct
 */
idx_dict_t*   idx_dict_create(const int hashsize, const int nodesize);


/*
 * func : load idx_dict_t from disk file
 *
 * args : path, file
 *
 * ret  : NULL, error
 *      : else, pointer to idx_dict_t struct
 */
idx_dict_t*   idx_dict_load(const char *path, const char *file);

/*
 * func : free a idx_dict_t struct
 */
void     idx_dict_free(idx_dict_t *pdict);

/*
 * func : reset the hash table;
 */
void     idx_dict_reset(idx_dict_t *pdict);

/*
 * func : save idx_dict_t to disk file
 *
 * args : pdict, the idx_dict_t pointer
 *      : path, file, dest path and file
 *
 * ret  : 0, succeed;
 *        -1, error.
 */
int      idx_dict_save(idx_dict_t *pdict, const char *path, const char *file);

/*
 * func : hash table optimize
 *
 * args : pdict, the idx_dict_t pointer
 *
 * ret  : NULL failed, success 优化后的字典
 */
idx_dict_t* idx_dict_optimize(idx_dict_t *pdict);

/*
 * func : add a node to the hash table;
 *
 * args : pdict, pointer to idx_dict_t
 *      : node , pointer to input node
 *
 * ret  : 1,  find a same key, value changed;
 *      : 0,  find NO same key, new node added,
 *      : -1, error;
 */
int      idx_dict_add(idx_dict_t *pdict, idict_node_t *node);

/*
 * func : find node in hash table by signature
 *
 * args : pdict, pointer to hash table
 *      : key,keylen, string and it's length
 *
 * ret  : NULL, not found
 *      : else, pointer to the founded node
 */
idict_node_t* idx_dict_find(idx_dict_t *pdict, const char *key, const int keylen);

/*
 * func : find node in hash table by signature
 *
 * args : pdict, pointer to hash table
 *      : sign,  64 bit string signature
 *
 * ret  : NULL, not found
 *      : else, pointer to the founded node
 */
idict_node_t* idx_dict_find(idx_dict_t *pdict, uint64_t sign);

/*
 * func : get the first node in hash table
 *
 * args : pos, return the position of the first node
 *
 * ret  : NULL, can NOT get first node, hash table empty
 *      : else, pointer to the first node
 */
idict_node_t* idx_dict_first(idx_dict_t *pdict, unsigned int *pos);

/*
 * func : get next node from the hash table
 *
 * args : pos, the start position, get node after pos,
 *
 * ret  : NULL, can NOT get next NODE, reach the end.
 *      : else, pointer to the next NODE
 */
idict_node_t* idx_dict_next(idx_dict_t *pdict, unsigned int *pos);

/*
 * func : 根据给定的值，在升序数组中返回区间上限的下标
 *
 * args : int32_t array 升序数组
 * args : int32_t num   数组个数
 * args : int32_t value 要查找的数值
 *
 * ret  : -1 failed, >= 0 success
 */
int32_t idx_find_value(int32_t* array, int32_t num, int32_t value);

/*
 * func : 获取字典中最大冲突数
 * args : pdict 字典
 * args : collisionNum out 发生冲突的数量
 * ret  : 返回最大冲突数量
 */
int32_t idx_get_max_collision(idx_dict_t *pdict, int32_t& collisionNum);

/*
 * func : 检查并释放可以回收的旧block空间
 * args : pdict 字典
 */
void    idx_dict_free_oldblock(idx_dict_t *pdict);
#endif
