#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "match_tree.h"


match_tree_t *match_tree_create(){
    match_tree_t *ptree = 0;
    ptree = (match_tree_t*)malloc(sizeof(match_tree_t));
    if(!ptree){
        return NULL;
    }
    memset(ptree,0,sizeof(match_tree_t));
    ptree->root = (mt_node_t *)malloc(sizeof(mt_node_t));
    if(!ptree->root){
        free(ptree);
        ptree = NULL;
        return NULL;
    }
    memset(ptree->root,0,sizeof(mt_node_t));
    return ptree;
}

int match_tree_insert(match_tree_t *ptree,const char *name,void *data){
    if(!name||!ptree){
        return -1;
    }
    mt_node_t *pnode = ptree->root;
    mt_node_t *pnew = NULL;
    unsigned int is_null = 1;
    const unsigned char *p = (const unsigned char *)name;
    for(;*p;p++){
        is_null = 0;
        if(pnode->childs[*p]==NULL){
            pnew = (mt_node_t*)malloc(sizeof(mt_node_t));
            if(!pnew){
                return -1;
            }
            memset(pnew,0,sizeof(mt_node_t));
            pnode->childs[*p]=pnew;
            pnode = pnew;
        }
        else{
            pnode = pnode->childs[*p];
        }
    }
    if(is_null){
        return -1;
    }
    if(pnode->is_final){
        fprintf(stderr,"Match tree. insert error.\n");
        return -1;
    }
    pnode->is_final = 1;
    pnode->data = data;
    return 0;
}

static void match_tree_free_node(mt_node_t *pnode){
    if(!pnode){
        return;
    }
    int i = 0;
    for(i = 0;i<256;i++){
        if(pnode->childs[i]){
            match_tree_free_node(pnode->childs[i]);
            free(pnode->childs[i]);
            pnode->childs[i]=NULL;
        }
    }
    return;
}

void match_tree_destroy(match_tree_t *ptree){
    if(!ptree){
        return;
    }
    match_tree_free_node(ptree->root);
    free(ptree->root);
    free(ptree);
    return;
}
