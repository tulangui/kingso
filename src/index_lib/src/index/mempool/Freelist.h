#ifndef  __DLIST_H_
#define  __DLIST_H_

//#include <ul_def.h>

#include <stdint.h>
#include "LogicMemSpace.h"

typedef uint32_t u_int; 

namespace index_mem
{

struct freelist_t
{
    u_int next_key;
	u_int key;
	int size[0];
};

u_int freelist_insert(LogicMemSpace * mem, u_int &root_key, freelist_t *node);
u_int freelist_pop(LogicMemSpace * mem, u_int &root_key, freelist_t **node);

};

#endif  //__DLIST_H_

/* vim: set ts=4 sw=4 sts=4 tw=100 */
