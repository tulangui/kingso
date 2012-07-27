#include "queryparser/qp_syntax_tree.h"
#include <stdlib.h>
#include "util/MemPool.h"
#include "util/Log.h"

namespace queryparser {

/* 创建语法树叶子节点 */
extern qp_syntax_node_t* qp_syntax_node_create(MemPool* mempool, const char* field_name, uint32_t field_name_len, 
	const char* field_value, uint32_t field_value_len, NodeLogicType relation, uint32_t field_weight, bool phrase)
{
	qp_syntax_node_t* new_node = NULL;
	
	if(!mempool){
		TNOTE("qp_syntax_leaf_create argument error, return!");
		return 0;
	}
	// 为新叶子节点申请内存空间，并初始化
	new_node = NEW(mempool, qp_syntax_node_t);
	if(new_node){
		new_node->left_child  = NULL;
		new_node->right_child = NULL;
        snprintf(new_node->field_name, MAX_FIELD_NAME_LEN-1, "%s", field_name);
		if(!field_value || field_value_len==0){
			new_node->field_value[0] = 0;
			new_node->field_value_len = 0;
        } else {
            snprintf(new_node->field_value, MAX_FIELD_VALUE_LEN-1, "%s", field_value);
			new_node->field_value_len = strlen(field_value);
		}
        new_node->relation = relation;
    } else {
		TNOTE("NEW qp_syntax_leaf_t error, return!");
	}
	
	return new_node;
}

extern int qp_syntax_tree_append_new_node(MemPool* mempool, qp_syntax_tree_t* tree,
	const char* field_name, uint32_t field_name_len, const char* field_value, uint32_t field_value_len,
	bool phrase, NodeLogicType relation)
{
	qp_syntax_node_t* new_node = 0;
	
	/* 创建一个语法树叶子节点 */
	new_node = qp_syntax_node_create(mempool, field_name, field_name_len, field_value, field_value_len, LT_NONE,  0, phrase);
	if(!new_node){
		TNOTE("qp_syntax_leaf_create error, return!");
		return -1;
	}
	
	/* 将新创建的叶子节点加入语法树 */
	return qp_syntax_tree_append_node(mempool, tree, new_node, relation);
}

/* 创建语法树 */
extern qp_syntax_tree_t* qp_syntax_tree_create(MemPool* mempool)
{
	qp_syntax_tree_t* new_tree = NULL;
	
	if(!mempool){
		TNOTE("qp_syntax_tree_create argument error, return!");
		return 0;
	}
	
	new_tree = NEW(mempool, qp_syntax_tree_t);
	if(new_tree){
        new_tree->root = NULL; 
        new_tree->node_count = 0;
        return new_tree;
    }
	
	return NULL;
}


/* 销毁语法树 */
extern void qp_syntax_tree_destroy(qp_syntax_tree_t* tree)
{
}

/* 向语法树中添加叶子节点 */
extern int qp_syntax_tree_append_node(MemPool* mempool, qp_syntax_tree_t* tree, qp_syntax_node_t* new_node, NodeLogicType relation)
{
	int retval = 0;
	
	if(!mempool || !tree || !new_node){
		TNOTE("qp_syntax_tree_append_leaf argument error, return!");
		return -1;
	}
	// 如果语法树是空的
	if(tree->root == NULL){
		tree->root = new_node;
		tree->node_count ++;
        return 0;
	}
 
	qp_syntax_node_t* new_parent = NULL;
	
	// 创建old_node和new_node的父节点
	new_parent = qp_syntax_node_create(mempool, NULL, 0, NULL, 0, relation, 0, false);
	if(!new_parent){
		TNOTE("_qp_syntax_tree_append1 qp_syntax_node_create error, return!");
		return -1;
	}
	new_parent->left_child  = tree->root;
	new_parent->right_child = new_node;
	new_parent->relation    = relation;

    tree->node_count++;
    tree->root = new_parent;
	
    return 0;
}

/* 向语法树中添加其它语法树中的节点 */
extern int qp_syntax_tree_append_tree(MemPool* mempool, qp_syntax_tree_t* tree, qp_syntax_tree_t* new_tree, NodeLogicType relation)
{
	int retval = 0;
	
	if(!mempool || !tree || !new_tree){
		TNOTE("qp_syntax_tree_append_tree argument error, return!");
		return -1;
	}
	
	// 如果待添加语法树是空的
	if(new_tree->root == NULL){
		return 0;
	}
	
	// 如果当前语法树是空的
	if(tree->root == NULL){
		tree->root = new_tree->root;
		tree->node_count = new_tree->node_count;
        return 0;	
    }
    qp_syntax_node_t *new_parent = NULL; 
    // 创建old_node和new_node的父节点
	new_parent = qp_syntax_node_create(mempool, NULL, 0, NULL, 0, relation, 0, false);
	if(!new_parent){
		TNOTE("_qp_syntax_tree_append1 qp_syntax_node_create error, return!");
		return -1;
	}
	new_parent->left_child  = tree->root;
	new_parent->right_child = new_tree->root;
	new_parent->relation    = relation;
    
    tree->node_count += new_tree->node_count;
    tree->root = new_parent;
	
    return 0;
}
	
/*
extern void qp_syntax_tree_print(qp_syntax_node_t *root)
{
    if(!root) {
        return;
    }
    if(root->left_child == NULL && root->right_child == NULL) {
        TERR("SyntaxTree leaf field_name=%s field_value=%s", 
                root->field_name, root->field_value);
        return;
    } else {
        NodeLogicType relation = root->relation;
        if(relation==LT_AND){
            TERR("SyntaxTree node relation=LT_AND");
        } else if(relation==LT_OR){
            TERR("SyntaxTree node relation=LT_OR");
        } else if(relation==LT_NOT){
            TERR("SyntaxTree node relation=LT_NOT");
        } else{
        }
        qp_syntax_tree_print(root->left_child);
        qp_syntax_tree_print(root->right_child);
    }
}
*/
extern void qp_syntax_tree_print(qp_syntax_node_t *root)
{
    if(!root) {
        return;
    }

    qp_syntax_tree_print(root->left_child);
    qp_syntax_tree_print(root->right_child);

    if(root->left_child == NULL && root->right_child == NULL) {
        TERR("SyntaxTree leaf field_name=%s field_value=%s", 
                root->field_name, root->field_value);
    } else {
        NodeLogicType relation = root->relation;
        if(relation==LT_AND){
            TERR("SyntaxTree node relation=LT_AND");
        } else if(relation==LT_OR){
            TERR("SyntaxTree node relation=LT_OR");
        } else if(relation==LT_NOT){
            TERR("SyntaxTree node relation=LT_NOT");
        } else{
        }
    }
}

}
