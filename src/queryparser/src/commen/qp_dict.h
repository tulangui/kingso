/***********************************************************************************
 * Describe : Query Parser Parameter Dictionary
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-08
 **********************************************************************************/


#ifndef _QP_DICT_H_
#define _QP_DICT_H_


#include "rbtree.h"
#include <stdint.h>
#include "util/MemPool.h"


typedef struct _qp_dict      qp_dict_t;         /* 字典 */
typedef struct _qp_list_node qp_list_node_t;    /* 索引项节点 */
typedef struct _qp_dict_node qp_dict_node_t;    /* hash项节点 */


/* 索引项节点 */
struct _qp_list_node
{
	qp_list_node_t* next;    // 指向下一同hash值索引项节点的指针
	char* name;              // 索引项的key
	int nlen;                // 索引项key的长度
	void* value;             // 索引项的value
	int vlen;                // 索引项value的长度
};

/* hash项节点 */
struct _qp_dict_node
{
	struct rb_node rb;       // 红黑树节点结构
	uint32_t crcval;         // crc32 hash值
	qp_list_node_t* head;    // 指向同hash值索引项链表首节点的指针
};

/* 字典 */
struct _qp_dict
{
	struct rb_root root;       // 红黑树结构
	
	int items;                 // 索引项节点总数
	
	qp_dict_node_t* dnode;     // 遍历红黑树的游标指针
	qp_list_node_t* lnode;     // 遍历某一hash值对应的链表节点的指针
	
	MemPool* mempool;          // 内存池
};


#ifdef __cplusplus
extern "C"{
#endif

/* 计算一个词条最大的内存占用量 */
#define COMPUTE_MEMLEN(name_len, value_len) (sizeof(qp_dict_node_t)+sizeof(qp_list_node_t)+name_len+1+sizeof(int)+value_len)

/**
 * 创建字典
 *
 * @param mempool 内存池
 *
 * @return !NULL 新建立的字典的指针
 *         NULL  创建失败
 */
extern qp_dict_t* qp_dict_create(MemPool* mempool);

/**
 * 销毁字典
 *
 * @param dict 待销毁的字典
 */
extern void qp_dict_destroy(qp_dict_t* dict);

/**
 * 清理字典
 *
 * @param dict 待清理的字典
 */
extern void qp_dict_clean(qp_dict_t* dict);

/**
 * 向字典中插入一个词条
 *
 * @param dict 字典
 * @param name 待插入词条的名字
 * @param nlen 待插入词条的名字长度
 * @param value 待插入词条的值
 * @param vlen 待插入词条的值长度
 *
 * @return ==0 待插入词条的名字已经存在
 *         < 0 插入失败
 *         > 0 插入成功
 */
extern int qp_dict_insert(qp_dict_t* dict, const char* name, int nlen, const void* value, int vlen);

/**
 * 向字典中修改一个词条
 *
 * @param dict 字典
 * @param name 待修改词条的名字
 * @param nlen 待修改词条的名字长度
 * @param value 待修改词条的值
 * @param vlen 待修改词条的值长度
 *
 * @return ==0 待修改词条的名字不存在
 *         < 0 修改失败
 *         > 0 修改成功
 */
extern int qp_dict_update(qp_dict_t* dict, const char* name, int nlen, const void* value, int vlen);

/**
 * 在字典中查找一个词条
 *
 * @param dict 字典
 * @param name 待查找词条的名字
 * @param nlen 待查找词条的名字长度
 * @param value 用于指向查找词条的值的指针
 * @param vlen 用于保存查找词条的值长度
 *
 * @return ==0 待查找词条的名字不存在
 *         < 0 查找出错
 *         > 0 查找成功
 */
extern int qp_dict_select(qp_dict_t* dict, const char* name, int nlen, void** value, int* vlen);

/**
 * 初始化字典遍历游标
 *
 * @param dict 字典
 */
extern void qp_dict_first(qp_dict_t* dict);

/**
 * 获取字典遍历的下一个节点
 *
 * @param dict 字典
 * @param name 用于指向下一词条的名字的指针
 * @param nlen 用于保存下一词条的名字长度
 * @param value 用于指向下一词条的值的指针
 * @param vlen 用于保存下一词条的值长度
 *
 * @return ==0 遍历完成
 *         > 0 成功获取一个词条
 *         < 0 遍历出错
 */
extern int qp_dict_next(qp_dict_t* dict, char** name, int* nlen, void** value, int* vlen);

/**
 * 获取字典中的词条总数
 *
 * @param dict 字典
 *
 * @return ==0 字典中的词条总数
 *         < 0 执行出错
 */
extern int qp_dict_get_count(qp_dict_t* dict);

#ifdef __cplusplus
}
#endif


#endif
