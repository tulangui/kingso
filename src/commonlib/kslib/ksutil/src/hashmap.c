#include "util/hashmap.h"

#include <malloc.h>
#include <string.h>



typedef struct _hashmap_node_s {
    void *key;
    void *value;
    uint64_t hash;
    struct _hashmap_node_s *next; //同一个桶中下一个元素的指针
//    struct _hashmap_node_s *seq_prev;
    struct _hashmap_node_s *seq_next; //遍历指针，指向下一个元素
} _hashmap_node_t;

typedef struct _hashmap_pool_s {
    _hashmap_node_t *pool;
    uint32_t pool_ptr; //下一个分配的节点指针
    struct _hashmap_pool_s *next; //指向下一块内存池
} _hashmap_pool_t;

typedef struct _hashmap_s {
    uint32_t buckets;
    uint32_t flags;
    _hashmap_node_t **table;
    HM_FUNC_HASH cb_hash;
    HM_FUNC_COMPARE cb_cmp;
    _hashmap_pool_t *pool;
    uint32_t pool_size;
    _hashmap_node_t *seq_head;
    _hashmap_node_t *seq_tail;
    _hashmap_node_t *it_cur;
} _hashmap_t;

inline static _hashmap_node_t *
hashmap_get_node(_hashmap_t * map) {


    if ((!map->pool) || (map->pool->pool_ptr >= map->pool_size)) {

        _hashmap_pool_t *pool =
            (_hashmap_pool_t *) malloc(sizeof(_hashmap_pool_t));
        if (!pool) {
            return NULL;
        }
        pool->pool =
            (_hashmap_node_t *) malloc(sizeof(_hashmap_node_t) *
                                       map->pool_size);
        if (!pool->pool) {
            free(pool);
            return NULL;
        }
        pool->pool_ptr = 0;
        pool->next = map->pool;
        map->pool = pool;
    }
    return map->pool->pool + (map->pool->pool_ptr++);
}

hashmap_t
hashmap_create(uint32_t buckets, uint32_t flags, HM_FUNC_HASH cb_hash,
               HM_FUNC_COMPARE cb_cmp) {

    if (!cb_hash || !cb_cmp) {
        return NULL;
    }

    _hashmap_t *map = NULL;
    _hashmap_node_t **table = NULL;
    map = (_hashmap_t *) malloc(sizeof(_hashmap_t));
    table =
        (_hashmap_node_t **) malloc(sizeof(_hashmap_node_t *) * buckets);
    if (!map || !table) {
        free(map);
        free(table);
        return NULL;
    }

    memset(map, 0, sizeof(_hashmap_t));
    memset(table, 0, sizeof(_hashmap_node_t *) * buckets);

    map->buckets = buckets;
    map->flags = flags;
    map->table = table;
    map->cb_hash = cb_hash;
    map->cb_cmp = cb_cmp;
    map->pool_size = buckets;
    map->pool = NULL;
    map->seq_head = NULL;
    map->seq_tail = NULL;

    return (hashmap_t) map;

}

void
hashmap_destroy(hashmap_t h, HM_FUNC_ON_DESTROY cb, void *user_data) {

    if (!h) {
        return;
    }

    _hashmap_t *map = (_hashmap_t *) h;

    _hashmap_node_t *pnode = NULL;
    _hashmap_node_t *ptmp = NULL;

    for (pnode = map->seq_head; pnode; pnode = ptmp) {
        ptmp = pnode->seq_next;
        if (cb) {
            cb(pnode->key, pnode->value, user_data);
        }
        if ((map->flags & HM_FLAG_POOL) == 0) {
            free(pnode);
        }
    }

    if ((map->flags & HM_FLAG_POOL) != 0) {
        _hashmap_pool_t *pool = NULL;
        _hashmap_pool_t *pool_tmp = NULL;
        for (pool = map->pool; pool; pool = pool_tmp) {
            pool_tmp = pool->next;
            free(pool->pool);
            free(pool);
        }
    }

    free(map->table);
    map->table = NULL;
    free(map);
    map = NULL;

    return;
}

int32_t
hashmap_insert(hashmap_t h, void *key, void *value) {

    if (!h) {
        return -1;
    }

    //不检查key,value是否合法

    _hashmap_t *map = (_hashmap_t *) h;
    _hashmap_node_t *pnode = NULL;
    uint64_t hash = 0;
    unsigned int index = 0;

    hash = map->cb_hash(key);
    index = hash % map->buckets;

    //检查是否已经存在
    for (pnode = map->table[index]; pnode; pnode = pnode->next) {
        if (pnode->hash == hash && map->cb_cmp(key, pnode->key) == 0) {
            //已经存在
            return -1;
        }
    }

    if ((map->flags & HM_FLAG_POOL) != 0) {
        pnode = hashmap_get_node(map);
    }
    else {
        pnode = (_hashmap_node_t *) malloc(sizeof(_hashmap_node_t));
    }
    if (!pnode) {
        return -1;
    }
    memset(pnode, 0, sizeof(_hashmap_node_t));
    pnode->key = key;
    pnode->value = value;
    pnode->hash = hash;
    pnode->next = map->table[index];
    map->table[index] = pnode;

    if (map->seq_tail) {
        map->seq_tail->seq_next = pnode;
        map->seq_tail = pnode;
    }
    else {
        map->seq_head = pnode;
        map->seq_tail = pnode;
    }

    return 0;
}

void *
hashmap_find(hashmap_t h, void *key) {

    if (!h) {
        return NULL;
    }

    //不检查key是否合法
    _hashmap_t *map = (_hashmap_t *) h;
    _hashmap_node_t *pnode = NULL;
    uint64_t hash = 0;
    unsigned int index = 0;

    hash = map->cb_hash(key);

    index = hash % map->buckets;

    //检查是否存在
    for (pnode = map->table[index]; pnode; pnode = pnode->next) {
        if (pnode->hash == hash && map->cb_cmp(key, pnode->key) == 0 ) {
            return pnode->value;
        }
    }
    return NULL;
}

int32_t
hashmap_walk(hashmap_t h, HM_FUNC_WALK cb_walk, void *user_data) {
    if (!h) {
        return -1;
    }
    if (!cb_walk) {
        return -1;
    }

    _hashmap_t *map = (_hashmap_t *) h;
    _hashmap_node_t *pnode = NULL;

    int32_t count = 0;

    for (pnode = map->seq_head; pnode; pnode = pnode->seq_next) {
        if (cb_walk(pnode->key, pnode->value, user_data) != 0) {
            break;
        }
        ++count;
    }

    return count;
}

void
hashmap_it_reset(hashmap_t h) {
    if (!h) {
        return;
    }

    _hashmap_t *map = (_hashmap_t *) h;

    map->it_cur = map->seq_head;
    return;
}

void *
hashmap_it_next(hashmap_t h) {
    if (!h) {
        return NULL;
    }

    _hashmap_t *map = (_hashmap_t *) h;

    if (!map->it_cur) {
        return NULL;
    }

    void *value = map->it_cur->value;
    map->it_cur = map->it_cur->seq_next;
    return value;
}
