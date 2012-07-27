#include "Common.h"
#include "IndexTermActualize.h"
#include "IndexFieldReader.h"
#include "util/Timer.h"
#include "index_lib/IndexTerm.h"

INDEX_LIB_BEGIN

#define IDX_GET_BASE(doc_id)  ((doc_id) >> 16)
#define IDX_GET_DIFF(doc_id)  ((doc_id) &  0x0000FFFF)

static uint8_t BITMAP_NEXT_TABLE[256][9] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 1, 0, 0, 0, 0, 0, 0, 2},
    {2, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 2, 0, 0, 0, 0, 0, 0, 2},
    {1, 2, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 2, 0, 0, 0, 0, 0, 3},
    {3, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 3, 0, 0, 0, 0, 0, 0, 2},
    {1, 3, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 3, 0, 0, 0, 0, 0, 3},
    {2, 3, 0, 0, 0, 0, 0, 0, 2},
    {0, 2, 3, 0, 0, 0, 0, 0, 3},
    {1, 2, 3, 0, 0, 0, 0, 0, 3},
    {0, 1, 2, 3, 0, 0, 0, 0, 4},
    {4, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 4, 0, 0, 0, 0, 0, 0, 2},
    {1, 4, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 4, 0, 0, 0, 0, 0, 3},
    {2, 4, 0, 0, 0, 0, 0, 0, 2},
    {0, 2, 4, 0, 0, 0, 0, 0, 3},
    {1, 2, 4, 0, 0, 0, 0, 0, 3},
    {0, 1, 2, 4, 0, 0, 0, 0, 4},
    {3, 4, 0, 0, 0, 0, 0, 0, 2},
    {0, 3, 4, 0, 0, 0, 0, 0, 3},
    {1, 3, 4, 0, 0, 0, 0, 0, 3},
    {0, 1, 3, 4, 0, 0, 0, 0, 4},
    {2, 3, 4, 0, 0, 0, 0, 0, 3},
    {0, 2, 3, 4, 0, 0, 0, 0, 4},
    {1, 2, 3, 4, 0, 0, 0, 0, 4},
    {0, 1, 2, 3, 4, 0, 0, 0, 5},
    {5, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 5, 0, 0, 0, 0, 0, 0, 2},
    {1, 5, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 5, 0, 0, 0, 0, 0, 3},
    {2, 5, 0, 0, 0, 0, 0, 0, 2},
    {0, 2, 5, 0, 0, 0, 0, 0, 3},
    {1, 2, 5, 0, 0, 0, 0, 0, 3},
    {0, 1, 2, 5, 0, 0, 0, 0, 4},
    {3, 5, 0, 0, 0, 0, 0, 0, 2},
    {0, 3, 5, 0, 0, 0, 0, 0, 3},
    {1, 3, 5, 0, 0, 0, 0, 0, 3},
    {0, 1, 3, 5, 0, 0, 0, 0, 4},
    {2, 3, 5, 0, 0, 0, 0, 0, 3},
    {0, 2, 3, 5, 0, 0, 0, 0, 4},
    {1, 2, 3, 5, 0, 0, 0, 0, 4},
    {0, 1, 2, 3, 5, 0, 0, 0, 5},
    {4, 5, 0, 0, 0, 0, 0, 0, 2},
    {0, 4, 5, 0, 0, 0, 0, 0, 3},
    {1, 4, 5, 0, 0, 0, 0, 0, 3},
    {0, 1, 4, 5, 0, 0, 0, 0, 4},
    {2, 4, 5, 0, 0, 0, 0, 0, 3},
    {0, 2, 4, 5, 0, 0, 0, 0, 4},
    {1, 2, 4, 5, 0, 0, 0, 0, 4},
    {0, 1, 2, 4, 5, 0, 0, 0, 5},
    {3, 4, 5, 0, 0, 0, 0, 0, 3},
    {0, 3, 4, 5, 0, 0, 0, 0, 4},
    {1, 3, 4, 5, 0, 0, 0, 0, 4},
    {0, 1, 3, 4, 5, 0, 0, 0, 5},
    {2, 3, 4, 5, 0, 0, 0, 0, 4},
    {0, 2, 3, 4, 5, 0, 0, 0, 5},
    {1, 2, 3, 4, 5, 0, 0, 0, 5},
    {0, 1, 2, 3, 4, 5, 0, 0, 6},
    {6, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 6, 0, 0, 0, 0, 0, 0, 2},
    {1, 6, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 6, 0, 0, 0, 0, 0, 3},
    {2, 6, 0, 0, 0, 0, 0, 0, 2},
    {0, 2, 6, 0, 0, 0, 0, 0, 3},
    {1, 2, 6, 0, 0, 0, 0, 0, 3},
    {0, 1, 2, 6, 0, 0, 0, 0, 4},
    {3, 6, 0, 0, 0, 0, 0, 0, 2},
    {0, 3, 6, 0, 0, 0, 0, 0, 3},
    {1, 3, 6, 0, 0, 0, 0, 0, 3},
    {0, 1, 3, 6, 0, 0, 0, 0, 4},
    {2, 3, 6, 0, 0, 0, 0, 0, 3},
    {0, 2, 3, 6, 0, 0, 0, 0, 4},
    {1, 2, 3, 6, 0, 0, 0, 0, 4},
    {0, 1, 2, 3, 6, 0, 0, 0, 5},
    {4, 6, 0, 0, 0, 0, 0, 0, 2},
    {0, 4, 6, 0, 0, 0, 0, 0, 3},
    {1, 4, 6, 0, 0, 0, 0, 0, 3},
    {0, 1, 4, 6, 0, 0, 0, 0, 4},
    {2, 4, 6, 0, 0, 0, 0, 0, 3},
    {0, 2, 4, 6, 0, 0, 0, 0, 4},
    {1, 2, 4, 6, 0, 0, 0, 0, 4},
    {0, 1, 2, 4, 6, 0, 0, 0, 5},
    {3, 4, 6, 0, 0, 0, 0, 0, 3},
    {0, 3, 4, 6, 0, 0, 0, 0, 4},
    {1, 3, 4, 6, 0, 0, 0, 0, 4},
    {0, 1, 3, 4, 6, 0, 0, 0, 5},
    {2, 3, 4, 6, 0, 0, 0, 0, 4},
    {0, 2, 3, 4, 6, 0, 0, 0, 5},
    {1, 2, 3, 4, 6, 0, 0, 0, 5},
    {0, 1, 2, 3, 4, 6, 0, 0, 6},
    {5, 6, 0, 0, 0, 0, 0, 0, 2},
    {0, 5, 6, 0, 0, 0, 0, 0, 3},
    {1, 5, 6, 0, 0, 0, 0, 0, 3},
    {0, 1, 5, 6, 0, 0, 0, 0, 4},
    {2, 5, 6, 0, 0, 0, 0, 0, 3},
    {0, 2, 5, 6, 0, 0, 0, 0, 4},
    {1, 2, 5, 6, 0, 0, 0, 0, 4},
    {0, 1, 2, 5, 6, 0, 0, 0, 5},
    {3, 5, 6, 0, 0, 0, 0, 0, 3},
    {0, 3, 5, 6, 0, 0, 0, 0, 4},
    {1, 3, 5, 6, 0, 0, 0, 0, 4},
    {0, 1, 3, 5, 6, 0, 0, 0, 5},
    {2, 3, 5, 6, 0, 0, 0, 0, 4},
    {0, 2, 3, 5, 6, 0, 0, 0, 5},
    {1, 2, 3, 5, 6, 0, 0, 0, 5},
    {0, 1, 2, 3, 5, 6, 0, 0, 6},
    {4, 5, 6, 0, 0, 0, 0, 0, 3},
    {0, 4, 5, 6, 0, 0, 0, 0, 4},
    {1, 4, 5, 6, 0, 0, 0, 0, 4},
    {0, 1, 4, 5, 6, 0, 0, 0, 5},
    {2, 4, 5, 6, 0, 0, 0, 0, 4},
    {0, 2, 4, 5, 6, 0, 0, 0, 5},
    {1, 2, 4, 5, 6, 0, 0, 0, 5},
    {0, 1, 2, 4, 5, 6, 0, 0, 6},
    {3, 4, 5, 6, 0, 0, 0, 0, 4},
    {0, 3, 4, 5, 6, 0, 0, 0, 5},
    {1, 3, 4, 5, 6, 0, 0, 0, 5},
    {0, 1, 3, 4, 5, 6, 0, 0, 6},
    {2, 3, 4, 5, 6, 0, 0, 0, 5},
    {0, 2, 3, 4, 5, 6, 0, 0, 6},
    {1, 2, 3, 4, 5, 6, 0, 0, 6},
    {0, 1, 2, 3, 4, 5, 6, 0, 7},
    {7, 0, 0, 0, 0, 0, 0, 0, 1},
    {0, 7, 0, 0, 0, 0, 0, 0, 2},
    {1, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 1, 7, 0, 0, 0, 0, 0, 3},
    {2, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 2, 7, 0, 0, 0, 0, 0, 3},
    {1, 2, 7, 0, 0, 0, 0, 0, 3},
    {0, 1, 2, 7, 0, 0, 0, 0, 4},
    {3, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 3, 7, 0, 0, 0, 0, 0, 3},
    {1, 3, 7, 0, 0, 0, 0, 0, 3},
    {0, 1, 3, 7, 0, 0, 0, 0, 4},
    {2, 3, 7, 0, 0, 0, 0, 0, 3},
    {0, 2, 3, 7, 0, 0, 0, 0, 4},
    {1, 2, 3, 7, 0, 0, 0, 0, 4},
    {0, 1, 2, 3, 7, 0, 0, 0, 5},
    {4, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 4, 7, 0, 0, 0, 0, 0, 3},
    {1, 4, 7, 0, 0, 0, 0, 0, 3},
    {0, 1, 4, 7, 0, 0, 0, 0, 4},
    {2, 4, 7, 0, 0, 0, 0, 0, 3},
    {0, 2, 4, 7, 0, 0, 0, 0, 4},
    {1, 2, 4, 7, 0, 0, 0, 0, 4},
    {0, 1, 2, 4, 7, 0, 0, 0, 5},
    {3, 4, 7, 0, 0, 0, 0, 0, 3},
    {0, 3, 4, 7, 0, 0, 0, 0, 4},
    {1, 3, 4, 7, 0, 0, 0, 0, 4},
    {0, 1, 3, 4, 7, 0, 0, 0, 5},
    {2, 3, 4, 7, 0, 0, 0, 0, 4},
    {0, 2, 3, 4, 7, 0, 0, 0, 5},
    {1, 2, 3, 4, 7, 0, 0, 0, 5},
    {0, 1, 2, 3, 4, 7, 0, 0, 6},
    {5, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 5, 7, 0, 0, 0, 0, 0, 3},
    {1, 5, 7, 0, 0, 0, 0, 0, 3},
    {0, 1, 5, 7, 0, 0, 0, 0, 4},
    {2, 5, 7, 0, 0, 0, 0, 0, 3},
    {0, 2, 5, 7, 0, 0, 0, 0, 4},
    {1, 2, 5, 7, 0, 0, 0, 0, 4},
    {0, 1, 2, 5, 7, 0, 0, 0, 5},
    {3, 5, 7, 0, 0, 0, 0, 0, 3},
    {0, 3, 5, 7, 0, 0, 0, 0, 4},
    {1, 3, 5, 7, 0, 0, 0, 0, 4},
    {0, 1, 3, 5, 7, 0, 0, 0, 5},
    {2, 3, 5, 7, 0, 0, 0, 0, 4},
    {0, 2, 3, 5, 7, 0, 0, 0, 5},
    {1, 2, 3, 5, 7, 0, 0, 0, 5},
    {0, 1, 2, 3, 5, 7, 0, 0, 6},
    {4, 5, 7, 0, 0, 0, 0, 0, 3},
    {0, 4, 5, 7, 0, 0, 0, 0, 4},
    {1, 4, 5, 7, 0, 0, 0, 0, 4},
    {0, 1, 4, 5, 7, 0, 0, 0, 5},
    {2, 4, 5, 7, 0, 0, 0, 0, 4},
    {0, 2, 4, 5, 7, 0, 0, 0, 5},
    {1, 2, 4, 5, 7, 0, 0, 0, 5},
    {0, 1, 2, 4, 5, 7, 0, 0, 6},
    {3, 4, 5, 7, 0, 0, 0, 0, 4},
    {0, 3, 4, 5, 7, 0, 0, 0, 5},
    {1, 3, 4, 5, 7, 0, 0, 0, 5},
    {0, 1, 3, 4, 5, 7, 0, 0, 6},
    {2, 3, 4, 5, 7, 0, 0, 0, 5},
    {0, 2, 3, 4, 5, 7, 0, 0, 6},
    {1, 2, 3, 4, 5, 7, 0, 0, 6},
    {0, 1, 2, 3, 4, 5, 7, 0, 7},
    {6, 7, 0, 0, 0, 0, 0, 0, 2},
    {0, 6, 7, 0, 0, 0, 0, 0, 3},
    {1, 6, 7, 0, 0, 0, 0, 0, 3},
    {0, 1, 6, 7, 0, 0, 0, 0, 4},
    {2, 6, 7, 0, 0, 0, 0, 0, 3},
    {0, 2, 6, 7, 0, 0, 0, 0, 4},
    {1, 2, 6, 7, 0, 0, 0, 0, 4},
    {0, 1, 2, 6, 7, 0, 0, 0, 5},
    {3, 6, 7, 0, 0, 0, 0, 0, 3},
    {0, 3, 6, 7, 0, 0, 0, 0, 4},
    {1, 3, 6, 7, 0, 0, 0, 0, 4},
    {0, 1, 3, 6, 7, 0, 0, 0, 5},
    {2, 3, 6, 7, 0, 0, 0, 0, 4},
    {0, 2, 3, 6, 7, 0, 0, 0, 5},
    {1, 2, 3, 6, 7, 0, 0, 0, 5},
    {0, 1, 2, 3, 6, 7, 0, 0, 6},
    {4, 6, 7, 0, 0, 0, 0, 0, 3},
    {0, 4, 6, 7, 0, 0, 0, 0, 4},
    {1, 4, 6, 7, 0, 0, 0, 0, 4},
    {0, 1, 4, 6, 7, 0, 0, 0, 5},
    {2, 4, 6, 7, 0, 0, 0, 0, 4},
    {0, 2, 4, 6, 7, 0, 0, 0, 5},
    {1, 2, 4, 6, 7, 0, 0, 0, 5},
    {0, 1, 2, 4, 6, 7, 0, 0, 6},
    {3, 4, 6, 7, 0, 0, 0, 0, 4},
    {0, 3, 4, 6, 7, 0, 0, 0, 5},
    {1, 3, 4, 6, 7, 0, 0, 0, 5},
    {0, 1, 3, 4, 6, 7, 0, 0, 6},
    {2, 3, 4, 6, 7, 0, 0, 0, 5},
    {0, 2, 3, 4, 6, 7, 0, 0, 6},
    {1, 2, 3, 4, 6, 7, 0, 0, 6},
    {0, 1, 2, 3, 4, 6, 7, 0, 7},
    {5, 6, 7, 0, 0, 0, 0, 0, 3},
    {0, 5, 6, 7, 0, 0, 0, 0, 4},
    {1, 5, 6, 7, 0, 0, 0, 0, 4},
    {0, 1, 5, 6, 7, 0, 0, 0, 5},
    {2, 5, 6, 7, 0, 0, 0, 0, 4},
    {0, 2, 5, 6, 7, 0, 0, 0, 5},
    {1, 2, 5, 6, 7, 0, 0, 0, 5},
    {0, 1, 2, 5, 6, 7, 0, 0, 6},
    {3, 5, 6, 7, 0, 0, 0, 0, 4},
    {0, 3, 5, 6, 7, 0, 0, 0, 5},
    {1, 3, 5, 6, 7, 0, 0, 0, 5},
    {0, 1, 3, 5, 6, 7, 0, 0, 6},
    {2, 3, 5, 6, 7, 0, 0, 0, 5},
    {0, 2, 3, 5, 6, 7, 0, 0, 6},
    {1, 2, 3, 5, 6, 7, 0, 0, 6},
    {0, 1, 2, 3, 5, 6, 7, 0, 7},
    {4, 5, 6, 7, 0, 0, 0, 0, 4},
    {0, 4, 5, 6, 7, 0, 0, 0, 5},
    {1, 4, 5, 6, 7, 0, 0, 0, 5},
    {0, 1, 4, 5, 6, 7, 0, 0, 6},
    {2, 4, 5, 6, 7, 0, 0, 0, 5},
    {0, 2, 4, 5, 6, 7, 0, 0, 6},
    {1, 2, 4, 5, 6, 7, 0, 0, 6},
    {0, 1, 2, 4, 5, 6, 7, 0, 7},
    {3, 4, 5, 6, 7, 0, 0, 0, 5},
    {0, 3, 4, 5, 6, 7, 0, 0, 6},
    {1, 3, 4, 5, 6, 7, 0, 0, 6},
    {0, 1, 3, 4, 5, 6, 7, 0, 7},
    {2, 3, 4, 5, 6, 7, 0, 0, 6},
    {0, 2, 3, 4, 5, 6, 7, 0, 7},
    {1, 2, 3, 4, 5, 6, 7, 0, 7},
    {0, 1, 2, 3, 4, 5, 6, 7, 8}
};


template<typename T>
static uint32_t bsearch(const T * list, uint32_t num, T& key)
{
    int32_t begin = 0;
    int32_t end = num - 1;
    int32_t mid;

    while(begin < end) {
        mid = begin + ((end - begin)>>1);
        if(list[mid] > key) {
            end = mid -1;
        } else if(list[mid] < key) {
            begin = mid + 1;
        } else {
            return mid;
        }
    }

    if(list[begin] < key) { // 有监视哨存在，不会越界
        return begin+1;
    }
    return begin;
}


static uint32_t bsearch(const idx2_nzip_unit_t * list, uint32_t num, uint32_t key)
{
    int32_t begin = 0;
    int32_t end = num - 1;
    int32_t mid;

    while(begin < end) {
        mid = begin + ((end - begin)>>1);
        if(list[mid].doc_id > key) {
            end = mid -1;
        } else if(list[mid].doc_id < key) {
            begin = mid + 1;
        } else {
            return mid;
        }
    }

    if(list[begin].doc_id < key) { // 有监视哨存在，不会越界
        return begin+1;
    }
    return begin;
}


static uint32_t bsearch(const idx2_zip_skip_unit_t * list, uint32_t num, uint32_t key)
{
    int32_t begin = 0;
    int32_t end = num - 1;
    int32_t mid;

    while(begin < end) {
        mid = begin + ((end - begin)>>1);
        if(list[mid].doc_id_base > key) {
            end = mid -1;
        } else if(list[mid].doc_id_base < key) {
            begin = mid + 1;
        } else {
            return mid;
        }
    }

    if(list[begin].doc_id_base < key) { // 有监视哨存在，不会越界
        return begin+1;
    }
    return begin;
}


static uint32_t bsearch(const idx2_zip_list_unit_t * list, uint32_t num, uint32_t key)
{
    int32_t begin = 0;
    int32_t end = num - 1;
    int32_t mid;

    while(begin < end) {
        mid = begin + ((end - begin)>>1);
        if(list[mid].doc_id_diff> key) {
            end = mid -1;
        } else if(list[mid].doc_id_diff < key) {
            begin = mid + 1;
        } else {
            return mid;
        }
    }

    if(list[begin].doc_id_diff < key) { // 有监视哨存在，不会越界
        return begin+1;
    }
    return begin;
}


IndexTerm::IndexTerm()
{
    _pBitMap = NULL;       // 倒排链的bitmap格式存储
    _invertList = NULL;

    _cIndexTermInfo.docNum = 0;      // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = 0;      // occ链表中含有的occ数量
    _cIndexTermInfo.zipType = 0;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
    _cIndexTermInfo.bitmapFlag = 0;  // 是否有bitmap索引，0: 没有 1： 有

    _cIndexTermInfo.delRepFlag = 0;  // 是否排重信息，0: 不排重， 1： 排重
    _cIndexTermInfo.bitmapLen = 0;   // bitmap索引长度(如果存在)
    _cIndexTermInfo.maxOccNum = 0;   // field本身限制的最多支持的occ数量
    _cIndexTermInfo.maxDocNum = 0;   // 总的doc数量(容量)
    _cIndexTermInfo.not_orig_flag = 0;
}

IndexTerm::~IndexTerm()
{
}

uint32_t IndexTerm::seek(uint32_t docId)
{
    return INVALID_DOCID;
}

uint8_t* const IndexTerm::getOcc(int32_t &count)
{
    count = 0;
    return NULL;
}

uint32_t IndexTerm::next()
{
    return INVALID_DOCID;
}
uint32_t IndexTerm::next(int32_t step)
{
    return INVALID_DOCID;
}
uint32_t IndexTerm::setpos(uint32_t docId)
{
    return INVALID_DOCID;
}

// 非压缩  one occ
IndexUnZipOneOccTerm::IndexUnZipOneOccTerm()
{
    _cIndexTermInfo.zipType = 1;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
    _cIndexTermInfo.maxOccNum = 1;   // field本身限制的最多支持的occ数量
}

IndexUnZipOneOccTerm::~IndexUnZipOneOccTerm()
{
}

int IndexUnZipOneOccTerm::init(uint32_t docNum, uint32_t maxDocNum,
        uint32_t maxOccNum , uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;         // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    _first = (idx2_nzip_unit_t*)_invertList;
    _begin = _first;
    _end   = _begin + docNum;
    _nextFirstFlag = true;

    return 0;
}

uint32_t IndexUnZipOneOccTerm::seek(uint32_t docId)
{
    if (unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }
    while (_begin->doc_id < docId) {
        _begin ++;
    }
    return _begin->doc_id;
}

uint8_t* const IndexUnZipOneOccTerm::getOcc(int32_t &count)
{
    count = 1;
    return &_begin->occ;
}

uint32_t IndexUnZipOneOccTerm::next()
{
    if (unlikely(_begin >= _end)) {
        return INVALID_DOCID;
    }
    if (unlikely(_nextFirstFlag)) {
        _nextFirstFlag = false;
        return _begin->doc_id;
    }

    ++_begin;
    return _begin->doc_id;
}
uint32_t IndexUnZipOneOccTerm::next(int32_t step)
{
    _beginNext += step;
    if (_beginNext >= _end) {
        return INVALID_DOCID;
    }
    return  _beginNext->doc_id;
}
uint32_t IndexUnZipOneOccTerm::setpos(uint32_t docId)
{
//    TNode<uint32_t, uint8_t> * list = (TNode<uint32_t, uint8_t>*)_invertList;
//    uint32_t pos = bsearch<uint32_t, uint8_t>(list, _cIndexTermInfo.docNum, docId);
    uint32_t pos = bsearch(_first, _cIndexTermInfo.docNum, docId);

    _begin = _first + pos;
    _nextFirstFlag = true;

    return _begin->doc_id;
}

// 非压缩  one occ
IndexUnZipOneOccTermOpt::IndexUnZipOneOccTermOpt()
{
    _cIndexTermInfo.zipType = 1;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
    _cIndexTermInfo.maxOccNum = 1;   // field本身限制的最多支持的occ数量
}

IndexUnZipOneOccTermOpt::~IndexUnZipOneOccTermOpt()
{
}

int IndexUnZipOneOccTermOpt::init(uint32_t docNum, uint32_t maxDocNum,
        uint32_t maxOccNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;         // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    _first = (idx2_nzip_unit_t*)_invertList;
    _begin = _first;
    _end   = _begin + docNum;
    _nextFirstFlag = true;

    return 0;
}

uint32_t IndexUnZipOneOccTermOpt::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    // 低位采用4倍跳读，首先判定最后四个doc是否可以为监视哨
    if(likely(_end - _begin > 4)) {
        // 统计发现4个之内命中占有很大比例，优先比较下
        if(_begin[3].doc_id < docId) {
            // 判定监视哨，以4倍速定位
            if(_end[-4].doc_id > docId) {
                while(_begin[3].doc_id < docId) {
                    _begin += 4;
                }
            } else { // 只有可能在最后4个内命中
                if(_end[-1].doc_id < docId) {
                    return INVALID_DOCID;
                } else {
                    _begin = _end - 4;
                }
            }
        }
        // 确定命中位置，返回查询结果
        if(_begin[0].doc_id >= docId) {
            return _begin->doc_id;
        } else if(_begin[1].doc_id >= docId) {
            _begin += 1;
            return _begin->doc_id;
        } else if(_begin[2].doc_id >= docId) {
            _begin += 2;
            return _begin->doc_id;
        } else {
            _begin += 3;
            return _begin->doc_id;
        }
    }
    while(_begin < _end && _begin->doc_id < docId) {
        _begin++;
    }
    // 命中返回当前docid，非命中返回比当前大的最小的docid（可能是INVALID_DOCID)
    if(_begin < _end) return _begin->doc_id;
    return INVALID_DOCID;
}

uint8_t* const IndexUnZipOneOccTermOpt::getOcc(int32_t &count)
{
    count = 1;
    return &_begin->occ;
}

uint32_t IndexUnZipOneOccTermOpt::next()
{
    if (unlikely(_begin >= _end)) {
        return INVALID_DOCID;
    }
    if (unlikely(_nextFirstFlag)) {
        _nextFirstFlag = false;
        return _begin->doc_id;
    }

    ++_begin;
    return _begin->doc_id;
}
uint32_t IndexUnZipOneOccTermOpt::next(int32_t step)
{
    _beginNext += step;
    if (_beginNext >= _end) {
        return INVALID_DOCID;
    }
    return  _beginNext->doc_id;
}

uint32_t IndexUnZipOneOccTermOpt::setpos(uint32_t docId)
{
//    TNode<uint32_t, uint8_t> * list = (TNode<uint32_t, uint8_t>*)_invertList;
//    uint32_t pos = bsearch<uint32_t, uint8_t>(list, _cIndexTermInfo.docNum, docId);

    uint32_t pos = bsearch(_first, _cIndexTermInfo.docNum, docId);

    _begin = _first + pos;
    _nextFirstFlag = true;

    return _begin->doc_id;
}

// 非压缩 多occ
IndexUnZipMulOccTerm::IndexUnZipMulOccTerm()
{
    _cIndexTermInfo.zipType = 1;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexUnZipMulOccTerm::~IndexUnZipMulOccTerm()
{
}

int IndexUnZipMulOccTerm::init(uint32_t docNum, uint32_t maxDocNum,
        uint32_t maxOccNum, MemPool* pMemPool, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;         // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    _first = (idx2_nzip_unit_t*)_invertList;
    _begin = _first;
    _end   = _begin + docNum;
    _pOcc  = NEW_ARRAY(pMemPool, uint8_t, maxOccNum);

    if(unlikely(NULL == _pOcc)) {
        return -1;
    }
    _lastDocId = INVALID_DOCID;

    return 0;
}

uint32_t IndexUnZipMulOccTerm::seek(uint32_t docId)
{
    if (unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }
    while (_begin->doc_id < docId) {
        _begin ++;
    }
    return _begin->doc_id;
}

uint8_t* const IndexUnZipMulOccTerm::getOcc(int32_t &count)
{
    idx2_nzip_unit_t* begin = _begin;
    uint32_t docId = begin->doc_id;

    if(unlikely(_first->doc_id == docId)) { // 只有第一次才有可能
        begin = _first;
    } else { // 找到第一个不重复的docid
        do {
            --begin;
        } while(begin->doc_id == docId);
        ++begin;
    }

    // cp occ
    count = 0;
    uint8_t* pOcc = _pOcc;
    do {
        *pOcc++ = begin->occ;
        ++count;
        ++begin;
    } while(begin->doc_id == docId);

    return _pOcc;
}

uint32_t IndexUnZipMulOccTerm::next()
{
    if (unlikely(_begin >= _end)) {
        return INVALID_DOCID;
    }

    if(_begin->doc_id != _lastDocId) {
        _lastDocId = _begin->doc_id;
        return _begin->doc_id;
    }

    // 过滤掉重复的docid
    do {
        ++_begin;
    } while(_begin->doc_id == _lastDocId);
    _lastDocId = _begin->doc_id;
    return _begin->doc_id;
}
uint32_t IndexUnZipMulOccTerm::next(int32_t step)
{
    _beginNext += step;
    if (_beginNext >= _end) {
        return INVALID_DOCID;
    }
    return  _beginNext->doc_id;
}

uint32_t IndexUnZipMulOccTerm::setpos(uint32_t docId)
{
//    TNode<uint32_t, uint8_t> * list = (TNode<uint32_t, uint8_t>*)_invertList;
//    uint32_t pos = bsearch<uint32_t, uint8_t>(list, _cIndexTermInfo.docNum, docId);

    uint32_t pos = bsearch(_first, _cIndexTermInfo.docNum, docId);

    _begin = _first + pos;
    _lastDocId = INVALID_DOCID;

    return _begin->doc_id;
}

// 非压缩  无occ
IndexUnZipNullOccTerm::IndexUnZipNullOccTerm()
{
    _cIndexTermInfo.zipType = 1;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexUnZipNullOccTerm::~IndexUnZipNullOccTerm()
{
}

int IndexUnZipNullOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if (unlikely(NULL == _invertList)) {
        return -1;
    }
    _begin = (uint32_t*)_invertList;
    _end = _begin + docNum;
    return 0;
}

uint32_t IndexUnZipNullOccTerm::seek(uint32_t docId)
{
    if (unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    while (*_begin < docId) {
        _begin ++;
    }

    return *_begin;
}

uint32_t IndexUnZipNullOccTerm::next()
{
    if (likely(_begin < _end)) {
        return  *_begin++;
    }
    return INVALID_DOCID;
}
uint32_t IndexUnZipNullOccTerm::next(int32_t step)
{
    _beginNext += step;
    if(_beginNext >= _end) {
        return INVALID_DOCID;
    }
    return *_beginNext;
}

uint32_t IndexUnZipNullOccTerm::setpos(uint32_t docId)
{
    uint32_t * list = (uint32_t*)_invertList;
    uint32_t pos = bsearch<uint32_t>(list, _cIndexTermInfo.docNum, docId);

    _begin = list + pos;
    return *_begin;
}

// 非压缩  无occ + skip read
IndexUnZipNullOccTermOpt::IndexUnZipNullOccTermOpt()
{
    _cIndexTermInfo.zipType = 1;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexUnZipNullOccTermOpt::~IndexUnZipNullOccTermOpt()
{
}

int IndexUnZipNullOccTermOpt::init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if (unlikely(NULL == _invertList)) {
        return -1;
    }
    _begin = (uint32_t*)_invertList;
    _end = _begin + docNum;
    return 0;
}

uint32_t IndexUnZipNullOccTermOpt::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    // 低位采用4倍跳读，首先判定最后四个doc是否可以为监视哨
    if(likely(_end - _begin > 4)) {
        // 统计发现4个之内命中占有很大比例，优先比较下
        if(_begin[3] < docId) {
            // 判定监视哨，以4倍速定位
            if(_end[-4] > docId) {
                while(_begin[3] < docId) {
                    _begin += 4;
                }
            } else { // 只有可能在最后4个内命中
                if(_end[-1] < docId) {
                    return INVALID_DOCID;
                } else {
                    _begin = _end - 4;
                }
            }
        }
        // 确定命中位置，返回查询结果
        if(_begin[0] >= docId) {
            return *_begin;
        } else if(_begin[1] >= docId) {
            _begin += 1;
            return *_begin;
        } else if(_begin[2] >= docId) {
            _begin += 2;
            return *_begin;
        } else {
            _begin += 3;
            return *_begin;
        }
    }
    while(_begin < _end && *_begin < docId) {
        _begin++;
    }
    // 命中返回当前docid，非命中返回比当前大的最小的docid（可能是INVALID_DOCID)
    if(_begin < _end) return *_begin;
    return INVALID_DOCID;
}

uint32_t IndexUnZipNullOccTermOpt::next()
{
    if (likely(_begin < _end)) {
        return  *_begin++;
    }
    return INVALID_DOCID;
}
uint32_t IndexUnZipNullOccTermOpt::next(int32_t step)
{
    _beginNext += step;
    if(_beginNext >= _end) {
        return INVALID_DOCID;
    }
    return *_beginNext;
}

uint32_t IndexUnZipNullOccTermOpt::setpos(uint32_t docId)
{
    uint32_t * list = (uint32_t*)_invertList;
    uint32_t pos = bsearch<uint32_t>(list, _cIndexTermInfo.docNum, docId);

    _begin = list + pos;
    return *_begin;
}

//  压缩  one occ
IndexZipOneOccTerm::IndexZipOneOccTerm()
{
    _cIndexTermInfo.maxOccNum = 1;   // field本身限制的最多支持的occ数量
    _cIndexTermInfo.zipType = 2;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexZipOneOccTerm::~IndexZipOneOccTerm()
{
}

int IndexZipOneOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;          // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;          // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;    // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum+63)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_zip_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (idx2_zip_list_unit_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _begin = _first;
    _end   = _first + _pSkipBegin[1].off;
    _nextFirstFlag = true;

    return 0;
}

uint32_t IndexZipOneOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    uint32_t nDocIdBase = IDX_GET_BASE(docId);

    if(_pSkipBegin->doc_id_base < nDocIdBase) { // 查找的docid不在当前块内
        // 最后一块为监视哨，根据倒数第二块情况，将其设为监视哨
        if(unlikely(_pSkipEnd[-1].doc_id_base < nDocIdBase)) {
            return INVALID_DOCID;
        }

        _pSkipBegin++;
        // 监视哨存在下，循环不再需要判断边界问题
        while (_pSkipBegin[1].doc_id_base < nDocIdBase) {
            _pSkipBegin +=2;
        }
        if(_pSkipBegin->doc_id_base < nDocIdBase) {
            _pSkipBegin++;
        }
        _begin = _first + _pSkipBegin->off;
        _end   = _first + _pSkipBegin[1].off;
    }

    if (likely(_pSkipBegin->doc_id_base == nDocIdBase)) { // 高位命中，在块内查找低位
        uint32_t nDocIdDiff = IDX_GET_DIFF(docId);

        // 低位采用4倍跳读，首先判定最后四个doc是否可以为监视哨
        if(likely(_end - _begin > 4)) {
            // 统计发现4个之内命中占有很大比例，优先比较下
            if(_begin[3].doc_id_diff < nDocIdDiff) {
                // 判定监视哨，以4倍速定位
                if(_end[-4].doc_id_diff > nDocIdDiff) {
                    while(_begin[3].doc_id_diff < nDocIdDiff) {
                        _begin += 4;
                    }
                } else { // 只有可能在最后4个内命中
                    if(_end[-1].doc_id_diff < nDocIdDiff) {
                        _pSkipBegin++;
                        _begin = _end;
                        _end   = _first + _pSkipBegin[1].off;
                        return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
                    } else {
                        _begin = _end - 4;
                    }
                }
            }
            // 确定命中位置，返回查询结果
            if(_begin[0].doc_id_diff >= nDocIdDiff) {
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else if(_begin[1].doc_id_diff >= nDocIdDiff) {
                _begin += 1;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else if(_begin[2].doc_id_diff >= nDocIdDiff) {
                _begin += 2;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else {
                _begin += 3;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            }
        }
        while(_begin < _end && _begin->doc_id_diff < nDocIdDiff) {
            _begin++;
        }
        if (_begin == _end) {
            _pSkipBegin++;
            _end   = _first + _pSkipBegin[1].off;
        }
    }
    // 命中返回当前docid，非命中返回比当前大的最小的docid（可能是INVALID_DOCID)
    return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
}

uint8_t* const IndexZipOneOccTerm::getOcc(int32_t &count)
{
    count = 1;
    return &_begin->occ;
}

uint32_t IndexZipOneOccTerm::next()
{
    if (unlikely(_nextFirstFlag)) { // 第一次取docid
        _nextFirstFlag = false;
        return  _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
    }

    // 除第一次取之外，都要先自增，然后取值
    _begin++;
    if (unlikely(_begin >= _end)) {
        _pSkipBegin++;
        _end = _first + _pSkipBegin[1].off;
    }

    return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
}
uint32_t IndexZipOneOccTerm::next(int32_t step)
{
    _beginNext += step;
    while(_beginNext >= _end) {
        _pSkipBegin++;
        if (_pSkipBegin >= _pSkipEnd) {
            return INVALID_DOCID;
        }
        _end = _first + _pSkipBegin[1].off;
    }

    return _beginNext->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
}

uint32_t IndexZipOneOccTerm::setpos(uint32_t docId)
{
    _nextFirstFlag = true;
    uint16_t base = IDX_GET_BASE(docId);

//    TNode<uint16_t, uint32_t> * list = (TNode<uint16_t, uint32_t> *)(_head + 1);
//    uint32_t pos = bsearch<uint16_t, uint32_t>(list, _head->block_num, base);

    idx2_zip_skip_unit_t * list = (idx2_zip_skip_unit_t *)(_head + 1);
    uint32_t pos = bsearch(list, _head->block_num, base);

    if(unlikely(pos >= _head->block_num)) {
        _pSkipBegin = _pSkipEnd;
        _begin =  _first + _pSkipBegin->off;
        _end = _begin;
        return INVALID_DOCID;
    }
    _pSkipBegin = (idx2_zip_skip_unit_t*)(list + pos);
    _begin =  _first + _pSkipBegin->off;
    _end = _first + _pSkipBegin[1].off;

    if(_pSkipBegin->doc_id_base == base) {
        uint16_t diff = IDX_GET_DIFF(docId);
        if(_end - _begin < 8) {
            while(_begin < _end && _begin->doc_id_diff < diff) {
                _begin ++;
            }
        } else {
            pos = bsearch(_begin, _end - _begin, diff);
            _begin += pos;
        }
        if(_begin >= _end) {
            _pSkipBegin++;
            _end = _first + _pSkipBegin[1].off;
        }
    }
    return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
}

//  压缩  one occ
IndexP4DOneOccTerm::IndexP4DOneOccTerm()
{
    _cIndexTermInfo.maxOccNum = 1;   // field本身限制的最多支持的occ数量
    _cIndexTermInfo.zipType = 2;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexP4DOneOccTerm::~IndexP4DOneOccTerm()
{
}

int IndexP4DOneOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;          // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;          // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;    // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum+63)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_p4d_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (uint8_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _occBegin = (uint8_t*)((char*)_first + _head->doc_list_len);
    _occPos = 0;
    _curOcc = _occBegin[_occPos];

    _idxComp.clearDecode();
    int nBlockSize = (_cIndexTermInfo.docNum > P4D_BLOCK_SIZE) ? P4D_BLOCK_SIZE : _cIndexTermInfo.docNum;
    _begin = _idxComp.decompression(_first + _pSkipBegin->off, nBlockSize);
    _end = _begin + nBlockSize;
    _curId = *_begin;

    return 0;
}

uint32_t IndexP4DOneOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    if(unlikely(docId > _pSkipEnd[-1].doc_id_base)) {
        return INVALID_DOCID;
    }

    idx2_p4d_skip_unit_t* pSkipCur = _pSkipBegin;
    while(pSkipCur->doc_id_base < docId)
    {
        pSkipCur++;
    }
    if(pSkipCur != _pSkipBegin)
    {
        _idxComp.clearDecode();
        int nBlockSize = P4D_BLOCK_SIZE;
        if(unlikely((pSkipCur + 1)  == _pSkipEnd))
            nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
        _begin = _idxComp.decompression(_first + pSkipCur->off, nBlockSize);
        _end = _begin + nBlockSize;
        _curId = *_begin;
        _pSkipBegin = pSkipCur;
        idx2_p4d_skip_unit_t *pSkipFirst = (idx2_p4d_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
        _occPos = (_pSkipBegin - pSkipFirst) * P4D_BLOCK_SIZE;
    }

    while(_curId < docId)
    {
        _begin++;
        _curId += *_begin;
        _occPos++;
    }
    _curOcc = _occBegin[_occPos];

    return  _curId;
}

uint8_t* const IndexP4DOneOccTerm::getOcc(int32_t &count)
{
    count = 1;
    return &_curOcc;
}

uint32_t IndexP4DOneOccTerm::next()
{
    uint32_t ret = _curId;
    _begin++;
    if (unlikely(_begin >= _end))
    {
        _pSkipBegin++;
        if(likely(_pSkipBegin < _pSkipEnd))
        {
            _idxComp.clearDecode();
            int nBlockSize = P4D_BLOCK_SIZE;
            if(unlikely((_pSkipBegin + 1)  == _pSkipEnd))
                nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
            _begin = _idxComp.decompression(_first+_pSkipBegin->off, nBlockSize);
            _end = _begin + nBlockSize;
            _curId = *_begin;
        }
        else
        {
            _curId = INVALID_DOCID;
        }
    }
    else
    {
        _curId += *_begin;
    }
    _curOcc = _occBegin[_occPos++];

    return ret;
}

//  压缩  有多个occ
IndexZipMulOccTerm::IndexZipMulOccTerm()
{
    _cIndexTermInfo.maxOccNum = 256;   // field本身限制的最多支持的occ数量
    _cIndexTermInfo.zipType = 2;       // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexZipMulOccTerm::~IndexZipMulOccTerm()
{
}

int IndexZipMulOccTerm::init(uint32_t docNum, uint32_t maxDocNum,
        uint32_t maxOccNum, MemPool* pMemPool, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;          // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;          // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;    // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum+64)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;          // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_zip_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (idx2_zip_list_unit_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _begin = _first;
    _end   = _first + _pSkipBegin[1].off;
    _pOcc  = NEW_ARRAY(pMemPool, uint8_t, getMaxOccNum());

    if(!_pOcc) {
        return -1;
    }
    _lastDocIdDiff = INVALID_DOCID;

    return 0;
}

uint32_t IndexZipMulOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    uint32_t nDocIdBase = IDX_GET_BASE(docId);

    if(_pSkipBegin->doc_id_base < nDocIdBase) {
        if(unlikely(_pSkipEnd[-1].doc_id_base < nDocIdBase)) {
            return INVALID_DOCID;
        }

        _pSkipBegin++;
        while (_pSkipBegin[1].doc_id_base < nDocIdBase) {
            _pSkipBegin +=2;
        }
        if(_pSkipBegin->doc_id_base < nDocIdBase) {
            _pSkipBegin++;
        }
        //    if (_pSkipBegin >= _pSkipEnd) {
        //      return INVALID_DOCID;
        //    }
        _begin = _first + _pSkipBegin->off;
        _end   = _first + _pSkipBegin[1].off;
    }

    if (likely(_pSkipBegin->doc_id_base == nDocIdBase)) {
        uint32_t nDocIdDiff = IDX_GET_DIFF(docId);

        if(likely(_end - _begin > 4)) {
            if(_begin[3].doc_id_diff < nDocIdDiff) {
                if(_end[-4].doc_id_diff > nDocIdDiff) {
                    while(_begin[3].doc_id_diff < nDocIdDiff) {
                        _begin += 4;
                    }
                } else {
                    if(_end[-1].doc_id_diff < nDocIdDiff) {
                        _pSkipBegin++;
                        _begin = _end;
                        _end   = _first + _pSkipBegin[1].off;
                        return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
                    } else {
                        _begin = _end - 4;
                    }
                }
            }
            if(_begin[0].doc_id_diff >= nDocIdDiff) {
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else if(_begin[1].doc_id_diff >= nDocIdDiff) {
                _begin += 1;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else if(_begin[2].doc_id_diff >= nDocIdDiff) {
                _begin += 2;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            } else {
                _begin += 3;
                return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
            }
        }
        while(_begin < _end && _begin->doc_id_diff < nDocIdDiff) {
            _begin++;
        }
        if (_begin == _end) {
            _pSkipBegin++;
            _end   = _first + _pSkipBegin[1].off;
        }
    }
    return (_pSkipBegin->doc_id_base << 16) + _begin->doc_id_diff;
}

uint8_t* const IndexZipMulOccTerm::getOcc(int32_t &count)
{
    idx2_zip_list_unit_t* begin = _first + _pSkipBegin->off;
    uint16_t doc_id_diff = _begin->doc_id_diff;

    // 找到第一个不重复的docid
    if (likely(doc_id_diff != begin->doc_id_diff)) {
        begin = _begin - 1;
        while(begin->doc_id_diff == doc_id_diff) {
            --begin;
        }
        ++begin;
    }

    // cp occ
    count = 0;
    uint8_t* pOcc = _pOcc;
    do {
        *pOcc++ = begin->occ;
        ++count;
        ++begin;
    } while(begin < _end && begin->doc_id_diff == doc_id_diff);

    return _pOcc;
}

uint32_t IndexZipMulOccTerm::next()
{
    if (likely(_begin < _end)) {
        // 当前的docid 上次获取的不同，直接返回
        if(_lastDocIdDiff != _begin->doc_id_diff) {
            _lastDocIdDiff = _begin->doc_id_diff;
            return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
        }

        // 过滤掉重复的docid, 没有判断块儿边界，最差的情况是第一块儿的最后一个id和第二块儿
        // 开始的docid相同，_begin 多走了几个docid，这种情况应该是很少发生的，正常情况下都
        // 少了块儿边界判断，应该是划算的
        do {
            _begin++;
        } while(_lastDocIdDiff == _begin->doc_id_diff);
        if(likely(_begin < _end)) {
            _lastDocIdDiff = _begin->doc_id_diff;
            return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
        }
    }
    // 判断是否换块
    _pSkipBegin++;
    if (unlikely(_pSkipBegin >= _pSkipEnd)) {
        return INVALID_DOCID;
    }

    _begin = _end;
    _end = _begin + (_pSkipBegin[1].off - _pSkipBegin[0].off);
    _lastDocIdDiff = _begin->doc_id_diff;
    return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
}
uint32_t IndexZipMulOccTerm::next(int32_t step)
{
    return INVALID_DOCID;
}
uint32_t IndexZipMulOccTerm::setpos(uint32_t docId)
{
    _lastDocIdDiff = INVALID_DOCID;
    uint16_t base = IDX_GET_BASE(docId);

    idx2_zip_skip_unit_t * list = (idx2_zip_skip_unit_t *)(_head + 1);
    uint32_t pos = bsearch(list, _head->block_num, base);

    if(unlikely(pos >= _head->block_num)) {
        _pSkipBegin = _pSkipEnd;
        _begin =  _first + _pSkipBegin->off;
        _end = _begin;
        return INVALID_DOCID;
    }
    _pSkipBegin = (idx2_zip_skip_unit_t*)(list + pos);
    _begin =  _first + _pSkipBegin->off;
    _end = _first + _pSkipBegin[1].off;

    if(_pSkipBegin->doc_id_base == base) {
        uint16_t diff = IDX_GET_DIFF(docId);
        if(_end - _begin < 8) {
            while(_begin < _end && _begin->doc_id_diff < diff) {
                _begin ++;
            }
        } else {
            pos = bsearch(_begin, _end - _begin, diff);
            _begin += pos;
        }
        if(_begin >= _end) {
            _pSkipBegin++;
            _end = _first + _pSkipBegin[1].off;
        }
    }
    return _begin->doc_id_diff + (_pSkipBegin->doc_id_base<<16);
}

IndexP4DMulOccTerm::IndexP4DMulOccTerm()
{
    _cIndexTermInfo.maxOccNum = 1;   // field本身限制的最多支持的occ数量
    _cIndexTermInfo.zipType = 2;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexP4DMulOccTerm::~IndexP4DMulOccTerm()
{
}

int IndexP4DMulOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxOccNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;          // doc链表中含有的docid数量
    _cIndexTermInfo.occNum = docNum;          // occ链表中含有的occ数量
    _cIndexTermInfo.maxDocNum = maxDocNum;    // 总的doc数量(容量)
    _cIndexTermInfo.maxOccNum = maxOccNum;    // 单个docid对应的最大occ数量
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum+63)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;                  // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_p4d_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (uint8_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _occBegin = (uint8_t*)((char*)_first + _head->doc_list_len);
    _occPos = 0;
    _curOcc = _occBegin;

    _idxComp.clearDecode();
    int nBlockSize = (_cIndexTermInfo.docNum > P4D_BLOCK_SIZE) ? P4D_BLOCK_SIZE : _cIndexTermInfo.docNum;
    _begin = _idxComp.decompression(_first + _pSkipBegin->off, nBlockSize);
    _end = _begin + nBlockSize;
    _curId = *_begin;
    _occCount = 1;

    return 0;
}

uint32_t IndexP4DMulOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    if(unlikely(docId > _pSkipEnd[-1].doc_id_base)) {
        return INVALID_DOCID;
    }

    idx2_p4d_skip_unit_t* pSkipCur = _pSkipBegin;
    while(pSkipCur->doc_id_base < docId)
    {
        pSkipCur++;
    }
    if(pSkipCur != _pSkipBegin)
    {
        _idxComp.clearDecode();
        int nBlockSize = P4D_BLOCK_SIZE;
        if(unlikely((pSkipCur + 1)  == _pSkipEnd))
            nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
        _begin = _idxComp.decompression(_first + pSkipCur->off, nBlockSize);
        _end = _begin + nBlockSize;
        _curId = *_begin;
        _pSkipBegin = pSkipCur;
        idx2_p4d_skip_unit_t *pSkipFirst = (idx2_p4d_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
        _occPos = (_pSkipBegin - pSkipFirst) * P4D_BLOCK_SIZE;
    }

    while(_curId < docId)
    {
        _begin++;
        _curId += *_begin;
        _occPos++;
    }
    _curOcc = _occBegin + _occPos;

    uint32_t ret = _curId;
    _occCount = 0;
    do
    {
        _begin++;
        if (unlikely(_begin >= _end))
        {
            _pSkipBegin++;
            if(likely(_pSkipBegin < _pSkipEnd))
            {
                _idxComp.clearDecode();
                int nBlockSize = P4D_BLOCK_SIZE;
                if(unlikely((_pSkipBegin + 1)  == _pSkipEnd))
                    nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
                _begin = _idxComp.decompression(_first+_pSkipBegin->off, nBlockSize);
                _end = _begin + nBlockSize;
                _curId = *_begin;
            }
            else
            {
                _curId = INVALID_DOCID;
            }
        }
        else
        {
            _curId += *_begin;
        }
        _occPos++;
        _occCount++;
    }
    while(ret == _curId && _curId != INVALID_DOCID);

    return  ret;
}

uint8_t* const IndexP4DMulOccTerm::getOcc(int32_t &count)
{
    count = _occCount;
    return _curOcc;
}

uint32_t IndexP4DMulOccTerm::next()
{
    uint32_t ret = _curId;
    _curOcc = _occBegin + _occPos;
    _occCount = 0;
    do
    {
        _begin++;
        if (unlikely(_begin >= _end))
        {
            _pSkipBegin++;
            if(likely(_pSkipBegin < _pSkipEnd))
            {
                _idxComp.clearDecode();
                int nBlockSize = P4D_BLOCK_SIZE;
                if(unlikely((_pSkipBegin + 1)  == _pSkipEnd))
                    nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
                _begin = _idxComp.decompression(_first+_pSkipBegin->off, nBlockSize);
                _end = _begin + nBlockSize;
                _curId = *_begin;
            }
            else
            {
                _curId = INVALID_DOCID;
            }
        }
        else
        {
            _curId += *_begin;
        }
        _occPos++;
        _occCount++;
    }
    while(ret == _curId && _curId != INVALID_DOCID);

    return ret;
}

//  压缩  无occ
IndexZipNullOccTerm::IndexZipNullOccTerm()
{
    _cIndexTermInfo.zipType = 2;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexZipNullOccTerm::~IndexZipNullOccTerm()
{
}

int IndexZipNullOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                    // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum + 63)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;          // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;           // bitmap索引长度(如果存在)
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_zip_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (uint16_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _begin = _first;
    _end = _first + _pSkipBegin[1].off;

    return 0;
}

uint32_t IndexZipNullOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    uint32_t nDocIdBase = IDX_GET_BASE(docId);

    if(_pSkipBegin->doc_id_base < nDocIdBase) {
        if(unlikely(_pSkipEnd[-1].doc_id_base < nDocIdBase)) {
            return INVALID_DOCID;
        }

        _pSkipBegin++;
        while (_pSkipBegin[1].doc_id_base < nDocIdBase) {
            _pSkipBegin +=2;
        }
        if(_pSkipBegin->doc_id_base < nDocIdBase) {
            _pSkipBegin++;
        }
        _begin = _first + _pSkipBegin->off;
        _end   = _first + _pSkipBegin[1].off;
    }

    if (likely(_pSkipBegin->doc_id_base == nDocIdBase)) {
        uint32_t nDocIdDiff = IDX_GET_DIFF(docId);

        if(likely(_end - _begin > 4)) {
            if(_begin[3] < nDocIdDiff) {
                if(_end[-4] > nDocIdDiff) {
                    while(_begin[3] < nDocIdDiff) {
                        _begin += 4;
                    }
                } else {
                    if(_end[-1] < nDocIdDiff) {
                        _pSkipBegin++;
                        _begin = _end;
                        _end   = _first + _pSkipBegin[1].off;
                        return (_pSkipBegin->doc_id_base << 16) + *_begin;
                    } else {
                        _begin = _end - 4;
                    }
                }
            }
            if(_begin[0] >= nDocIdDiff) {
                return (_pSkipBegin->doc_id_base << 16) + *_begin;
            } else if(_begin[1] >= nDocIdDiff) {
                _begin += 1;
                return (_pSkipBegin->doc_id_base << 16) + *_begin;
            } else if(_begin[2] >= nDocIdDiff) {
                _begin += 2;
                return (_pSkipBegin->doc_id_base << 16) + *_begin;
            } else {
                _begin += 3;
                return (_pSkipBegin->doc_id_base << 16) + *_begin;
            }
        }
        while(_begin < _end && *_begin < nDocIdDiff) {
            _begin++;
        }
        if (_begin == _end) {
            _pSkipBegin++;
            _end   = _first + _pSkipBegin[1].off;
        }
    }
    return (_pSkipBegin->doc_id_base << 16) + *_begin;
}

uint32_t IndexZipNullOccTerm::next()
{
    if (unlikely(_begin >= _end)) {
        _pSkipBegin++;
        if (unlikely(_pSkipBegin >= _pSkipEnd)) {
            return INVALID_DOCID;
        }

        _begin = _end;
        _end = _begin + (_pSkipBegin[1].off - _pSkipBegin[0].off);
    }

    return (_pSkipBegin->doc_id_base<<16) + *_begin++;
}
uint32_t IndexZipNullOccTerm::next(int32_t step)
{
    _beginNext += step;
    while(_beginNext >= _end) {
        _pSkipBegin++;
        if (_pSkipBegin >= _pSkipEnd) {
            return INVALID_DOCID;
        }
        _end = _first + _pSkipBegin[1].off;
    }

    return *_beginNext + (_pSkipBegin->doc_id_base<<16);
}

uint32_t IndexZipNullOccTerm::setpos(uint32_t docId)
{
    uint16_t base = IDX_GET_BASE(docId);

    idx2_zip_skip_unit_t * list = (idx2_zip_skip_unit_t *)(_head + 1);
    uint32_t pos = bsearch(list, _head->block_num, base);

    if(unlikely(pos >= _head->block_num)) {
        _pSkipBegin = _pSkipEnd;
        _begin =  _first + _pSkipBegin->off;
        _end = _begin;
        return INVALID_DOCID;
    }
    _pSkipBegin = (idx2_zip_skip_unit_t*)(list + pos);
    _begin =  _first + _pSkipBegin->off;
    _end = _first + _pSkipBegin[1].off;

    if(_pSkipBegin->doc_id_base == base) {
        uint16_t diff = IDX_GET_DIFF(docId);
        if(_end - _begin < 8) {
            while(_begin < _end && *_begin < diff) {
                _begin ++;
            }
        } else {
            pos = bsearch<uint16_t>(_begin, _end - _begin, diff);
            _begin += pos;
        }
        if(_begin >= _end) {
            _pSkipBegin++;
            _end = _first + _pSkipBegin[1].off;
        }
    }
    return *_begin + (_pSkipBegin->doc_id_base<<16);
}


//  压缩  无occ
IndexP4DNullOccTerm::IndexP4DNullOccTerm()
{
    _cIndexTermInfo.zipType = 2;     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
}

IndexP4DNullOccTerm::~IndexP4DNullOccTerm()
{
}

int IndexP4DNullOccTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;         // doc链表中含有的docid数量
    _cIndexTermInfo.maxDocNum = maxDocNum;   // 总的doc数量(容量)
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(_pBitMap) {
        _cIndexTermInfo.bitmapFlag = 1;                    // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = (maxDocNum + 63)>>6;   // bitmap索引长度(如果存在)
    } else {
        _cIndexTermInfo.bitmapFlag = 0;          // 是否有bitmap索引，0: 没有 1： 有
        _cIndexTermInfo.bitmapLen = 0;           // bitmap索引长度(如果存在)
    }

    _head = (idx2_zip_header_t *)_invertList;

    _pSkipBegin = (idx2_p4d_skip_unit_t*)(_invertList + sizeof(idx2_zip_header_t));
    _pSkipEnd = _pSkipBegin + _head->block_num;
    if (_pSkipBegin >= _pSkipEnd) {
        return -1;
    }

    _first = (uint8_t*)((char*)_pSkipBegin + _head->skip_list_len);
    _idxComp.clearDecode();
    int nBlockSize = (_cIndexTermInfo.docNum > P4D_BLOCK_SIZE) ? P4D_BLOCK_SIZE : _cIndexTermInfo.docNum;
    _begin = _idxComp.decompression(_first + _pSkipBegin->off, nBlockSize);
    _end = _begin + nBlockSize;
    _curId = *_begin;

    return 0;
}

uint32_t IndexP4DNullOccTerm::seek(uint32_t docId)
{
    if(unlikely(docId >= INVALID_DOCID)) {
        return INVALID_DOCID;
    }

    if(unlikely(docId > _pSkipEnd[-1].doc_id_base)) {
        return INVALID_DOCID;
    }

    idx2_p4d_skip_unit_t* pSkipCur = _pSkipBegin;
    while(pSkipCur->doc_id_base < docId)
    {
        pSkipCur++;
    }
    if(pSkipCur != _pSkipBegin)
    {
        _pSkipBegin = pSkipCur;
        _idxComp.clearDecode();
        int nBlockSize = P4D_BLOCK_SIZE;
        if(unlikely((_pSkipBegin + 1)  == _pSkipEnd))
        {
            nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
        }
        _begin = _idxComp.decompression(_first+_pSkipBegin->off, nBlockSize);
        _end = _begin + nBlockSize;
        _curId = *_begin;
    }

    while(_curId < docId)
    {
        _begin++;
        _curId += *_begin;
    }

    return  _curId;
}

uint32_t IndexP4DNullOccTerm::next()
{
    uint32_t ret = _curId;
    _begin++;
    if (unlikely(_begin >= _end))
    {
        _pSkipBegin++;
        if(likely(_pSkipBegin < _pSkipEnd))
        {
            _idxComp.clearDecode();
            int nBlockSize = P4D_BLOCK_SIZE;
            if(unlikely((_pSkipBegin + 1)  == _pSkipEnd))
            {
                nBlockSize = _cIndexTermInfo.docNum - (_head->block_num - 1) * P4D_BLOCK_SIZE;
            }
            _begin = _idxComp.decompression(_first+_pSkipBegin->off, nBlockSize);
            _end = _begin + nBlockSize;
            _curId = *_begin;
        }
        else
        {
            _curId = INVALID_DOCID;
        }
    }
    else
    {
        _curId += *_begin;
    }

    return ret;
}

// bitmap type
IndexBitmapTerm::IndexBitmapTerm()
{
    _cIndexTermInfo.occNum = 0;
    _cIndexTermInfo.zipType = 0;
    _cIndexTermInfo.bitmapFlag = 1;
    _cIndexTermInfo.delRepFlag = 0;
    _cIndexTermInfo.maxOccNum = 0;

}

IndexBitmapTerm::~IndexBitmapTerm()
{
}

int IndexBitmapTerm::init(uint32_t docNum, uint32_t maxDocNum, uint32_t maxDocId, uint32_t not_orig_flag)
{
    _cIndexTermInfo.docNum = docNum;
    _cIndexTermInfo.bitmapLen = (maxDocNum+63)>>6;
    _cIndexTermInfo.maxDocNum = maxDocNum;
    _cIndexTermInfo.not_orig_flag = not_orig_flag;

    if(NULL == _pBitMap) {
        return KS_EFAILED;
    }
    _maxDocId = maxDocId;

    const uint64_t* bitmap = _pBitMap;
    while(*bitmap == 0) bitmap++;

    _lastDocId = (bitmap - _pBitMap)<<6;
    _begin = BITMAP_NEXT_TABLE[*(uint8_t*)bitmap];
    _end = _begin + BITMAP_NEXT_TABLE[*(uint8_t*)bitmap][8];
    return 0;
}

uint32_t IndexBitmapTerm::seek(uint32_t docId)
{
    if(unlikely(docId > _maxDocId)) {
        return INVALID_DOCID;
    }

    uint64_t* bitmap = (uint64_t*)(_pBitMap + (docId>>6));
    if(*bitmap & bit_mask_tab[docId&0x3F]) {
        return docId;
    }

    // 找到一个docId
    uint32_t pos = docId&0x3F;
    for(++pos, ++docId; pos < 64; ++pos, ++docId) {
        if(*bitmap & bit_mask_tab[pos]) {
            return docId;
        }
    }
    bitmap++;

    // 到接下来的int64里面去找
    while(*bitmap == 0) {
        bitmap++;
        docId += 64;
    }
    for(pos = 0; pos < 64; ++pos, ++docId) {
        if(*bitmap & bit_mask_tab[pos]) {
            return docId;
        }
    }
    return INVALID_DOCID;
}


uint32_t IndexBitmapTerm::next()
{
    if(_begin < _end) {
        return _lastDocId + *_begin++;
    }

    _lastDocId += 8;
    if(unlikely(_lastDocId > _maxDocId)) {
        return INVALID_DOCID;
    }

    uint8_t* bitmap = (uint8_t*)_pBitMap + (_lastDocId>>3);

    for(int32_t i = 0; i < 8 ; ++i) {
        if(*bitmap) {
            _begin = BITMAP_NEXT_TABLE[*bitmap];
            _end = _begin + BITMAP_NEXT_TABLE[*bitmap][8];
            return _lastDocId + *_begin++;
        }
        ++bitmap;
        _lastDocId += 8;
    }

    while(*(uint64_t*)bitmap == 0) {
        bitmap += 8;
        _lastDocId += 64;
    }
    while(*bitmap == 0) {
        ++bitmap;
        _lastDocId += 8;
    }

    _begin = BITMAP_NEXT_TABLE[*bitmap];
    _end = _begin + BITMAP_NEXT_TABLE[*bitmap][8];

    return _lastDocId + *_begin++;
}
uint32_t IndexBitmapTerm::next(int32_t step)
{
    return INVALID_DOCID;
}

uint32_t IndexBitmapTerm::setpos(uint32_t docId)
{
    return INVALID_DOCID;
}

INDEX_LIB_END

