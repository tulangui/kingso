#include "conhash.h"
#include "conhash_code.h"
#include <string.h>
#include <malloc.h>

static conhash_t* conhash_create1(int node_count);
static conhash_t* conhash_create2(int node_count);
extern conhash_t* conhash_create(int node_count)
{
    conhash_t* ch = 0;

    if(node_count<1 || node_count>NODE_COUNT_MAX){
        return 0;
    }

    if(node_count<=8){
        ch = conhash_create1(node_count);
    }
    else{
        ch = conhash_create2(node_count);
    }

    return ch;
}

static conhash_t* conhash_create1(int node_count)
{
    conhash_t* ch = (conhash_t*)malloc(sizeof(conhash_t));
    if(ch){
        memcpy(ch->map, conhash_map[node_count], sizeof(unsigned int)*65536);
        ch->node_count = node_count;
    }
    return ch;
}

static conhash_t* conhash_create2(int node_count)
{
    conhash_t* ch = (conhash_t*)malloc(sizeof(conhash_t));
    if(ch){
        int pernode_count = 65536/node_count;
        int addnode_count = 65536%node_count;
        int i = 0;
        int j = 0;
        int k = 0;
        for(; i<addnode_count; i++){
            for(j=0; j<pernode_count+1; j++){
                ch->map[k++] = i;
            }
        }
        for(; i<node_count; i++){
            for(j=0; j<pernode_count; j++){
                ch->map[k++] = i;
            }
        }
        ch->node_count = node_count;
    }
    return ch;
}

extern void conhash_dest(conhash_t* ch)
{
    if(ch){
        free(ch);
    }
}

extern int conhash_find(conhash_t* ch, uint64_t value, uint64_t status)
{
    uint64_t hash_value = value&0xFFFF;
    uint64_t base = 1ll;
    unsigned int vnode = 0;
    unsigned char node = NODE_COUNT_MAX;
    conhash_result_t result;

    if(!ch){
        return -1;
    }
    if(status==0){
        result.real_hash = result.valid_hash = NODE_COUNT_MAX;
        return result.intv;
    }

    vnode = ch->map[hash_value];
    if(ch->node_count<=8){
        node = vnode_map[ch->node_count][vnode];
        result.real_hash = node;
        while(((base<<node)&status)==0){
            vnode = (vnode+1)%vnode_count[ch->node_count];
            node = vnode_map[ch->node_count][vnode];
            base = 1ll;
        }
        result.valid_hash = node;
    }
    else{
        node = vnode;
        result.real_hash = node;
        while(((base<<node)&status)==0){
            node = (node+1)%(ch->node_count);
            base = 1ll;
        }
        result.valid_hash = node;
    }

    return result.intv;
}
