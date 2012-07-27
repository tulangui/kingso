/***********************************************************************************
 * Describe : Query Parser 语法树
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-08
 **********************************************************************************/


#ifndef _QP_SYNTAX_TREE_H_
#define _QP_SYNTAX_TREE_H_


#include <stdint.h>
#include "util/MemPool.h"
#include "commdef/commdef.h"
namespace queryparser {

typedef struct _qp_syntax_node
{
	char field_name[MAX_FIELD_NAME_LEN];      // 域名
	uint32_t field_name_len;
	char field_value[MAX_FIELD_VALUE_LEN];    // 域值
	uint32_t field_value_len;
	
    _qp_syntax_node* left_child;     // 父节点指针
    _qp_syntax_node* right_child;     // 父节点指针
	
	NodeLogicType relation;       // 左右子节点的关系
}qp_syntax_node_t;


/* 语法树 */
typedef struct 
{
	qp_syntax_node_t *root;     // 根节点
	uint32_t node_count;       // 非叶节点个数
}qp_syntax_tree_t;


#ifdef __cplusplus
extern "C"{
#endif

/**
 * 创建语法树叶子节点
 *
 * @param mempool 内存池
 * @param field_name 该叶子节点对应的域名
 * @param field_name_len 该叶子节点对应的域名长度
 * @param field_value 该叶子节点对应的域值
 * @param field_value_len 该叶子节点对应的域值长度
 * @param field_weight 该叶子节点对应的域权重（暂无应用）
 * @param phrase 该叶子节点对应的域值是短语吗
 *
 * @return !NULL 新建立的叶子节点的指针
 *         NULL  创建失败
 */
extern qp_syntax_node_t* qp_syntax_node_create(MemPool* mempool, const char* field_name, uint32_t field_name_len, 
        const char* field_value, uint32_t field_value_len, NodeLogicType relation, uint32_t field_weight, bool phrase);

extern int qp_syntax_tree_append_new_node(MemPool* mempool, qp_syntax_tree_t* tree,
                                      const char* field_name, uint32_t field_name_len, 
                                      const char* field_value, uint32_t field_value_len,
                                      bool phrase, NodeLogicType relation);
/**
 * 创建语法树
 *
 * @param mempool 内存池
 *
 * @return !NULL 新建立的语法树的指针
 *         NULL  创建失败
 */
extern qp_syntax_tree_t* qp_syntax_tree_create(MemPool* mempool);

/**
 * 向语法树中添加叶子节点
 *
 * @param mempool 内存池
 * @param tree 语法树
 * @param new_leaf 待添加的叶子节点
 * @param relation 该叶子节点与原有语法树的逻辑关系
 *
 * @return ==0 添加成功
 *         < 0 添加失败
 */
extern int qp_syntax_tree_append_node(MemPool* mempool, qp_syntax_tree_t* tree, qp_syntax_node_t* new_leaf, NodeLogicType relation);

/**
 * 向语法树中添加其它语法树中的节点
 *
 * @param mempool 内存池
 * @param tree 语法树
 * @param new_tree 待添加的语法树
 * @param relation 该语法树与原有语法树的逻辑关系
 *
 * @return ==0 添加成功
 *         < 0 添加失败
 */
extern int qp_syntax_tree_append_tree(MemPool* mempool, qp_syntax_tree_t* tree, qp_syntax_tree_t* new_tree, NodeLogicType relation);

extern void qp_syntax_tree_print(qp_syntax_node_t *tree);
#ifdef __cplusplus
}
#endif

}
#endif
