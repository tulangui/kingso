#include "util/MemPool.h"

static void * mempool_alloc(MemPool * heap, uint32_t size) {
    return NEW_VEC(heap, char, size);
}

