/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: match_tree.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_MATCH_TREE_H_
#define SRC_MATCH_TREE_H_

#include <stdint.h>

typedef struct mt_node_s{
    struct mt_node_s *childs[256];
    void *data;
    uint32_t is_final;
}mt_node_t;

typedef struct match_tree_s{
    mt_node_t *root;
}match_tree_t;

match_tree_t *match_tree_create();
void match_tree_destroy(match_tree_t *ptree);
int match_tree_insert(match_tree_t *ptree,const char *name,void *data);
inline void *match_tree_find(match_tree_t *ptree,const char *name){
    if(!name||!ptree){
        return NULL;
    }
    mt_node_t *pnode = ptree->root;
    const unsigned char *p = (const unsigned char *)name;
    for(;*p;++p){
        if(!pnode->childs[*p]){
            return NULL;
        }
        else{
            pnode = pnode->childs[*p];
        }
    }
    if(pnode->is_final){
        return pnode->data;
    }
    return NULL;
}



#endif //SRC_MATCH_TREE_H_
