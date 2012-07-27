#include <stdio.h>

#include "Freelist.h"

namespace index_mem
{

u_int freelist_insert(LogicMemSpace * mem, u_int &root_key, freelist_t *node)
{
    if (mem == NULL) {
        return (u_int)(-1);
    }

    node->next_key = root_key;
    root_key = node->key;
    return node->key;
}

u_int freelist_pop(LogicMemSpace * mem, u_int &root_key, freelist_t **node)
{
    if (mem == NULL || root_key == ((u_int)(-1))) {
        return (u_int)(-1);
    }

    freelist_t * root = (freelist_t *)mem->addr(root_key);
    *node  = root;
    root_key = root->next_key;
    return root->next_key;
}

};
/* vim: set ts=4 sw=4 sts=4 tw=100 */
