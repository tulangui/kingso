#include "util/kspack.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define MEMLENBASE 8192 // kspack中起始申请的内存长度

#pragma pack(1)
/* kspack头部信息 */
typedef struct
{
    uint32_t data_size; // kspack数据长度
    uint32_t node_size; // kspack节点数量
}
kshead_t;

#define NAMESIZEBIT  6
#define VALUETYPEBIT 4
#define VLAUESIZEBIT 22
#define NAMESIZEMAX  ((1<<NAMESIZEBIT)-1)
#define VALUETYPEMAX ((1<<VALUETYPEBIT)-1)
#define VALUESIZEMAX ((1<<VLAUESIZEBIT)-1)

/* kspack节点数据结构 */
typedef struct
{
    uint32_t name_size  : NAMESIZEBIT;  // 数据名长度，最大64字节
    uint32_t value_type : VALUETYPEBIT; // 数据类型
    uint32_t value_size : VLAUESIZEBIT; // 数据长度，最大4M字节
    char name_value[0];                 // 名字和数据
}
ksnode_t;
#pragma pack()

/* kspack以读取方式打开时的索引节点结构 */
typedef struct _ksindex
{
    struct _ksindex* next;  // 下一节点指针
    ksnode_t* node;         // 数据节点指针
}
ksindex_t;

/* kspack结构定义 */
struct _kspack
{
    // 公共信息
    void* data;     // kspack数据内存
    kshead_t* head; // 包头信息
    char open_type; // 打开方式

    kspack_t* parent;   // 上层节点信息
    kspack_t* child;    // 正在打开的下层节点信息
    
    // 写入信息
    void* mem;              // 写入的实际内存缓冲区，可能被realloc
    uint32_t mem_size;      // 缓冲区长度
    uint32_t mem_used;      // 已使用长度
    char mem_own;           // 该区域是否被当前kspack系列所拥有
    void** mem_p;           // 指向最外层kspack的mem
    uint32_t* mem_size_p;   // 指向最外层kspack的mem_size
    uint32_t* mem_used_p;   // 指向最外层kspack的mem_used

    ksnode_t* node;         // 指向正在写入的节点

    // 读取信息
    ksindex_t* hashtable[65536];    // 读取时节点的hash索引表
    ksindex_t* index_array;         // 读取时的索引节点

    int index_now;                  // 游标指针
};

static uint32_t kspack_crc32(const void* data, size_t len);
static kspack_t* kspack_open_r(void* data, const int size);
static kspack_t* kspack_open_w(void* data, const int size);

/* 打开一个kspack */
extern kspack_t* kspack_open(const char rw, void* data, const int size)
{
    switch(rw){
        case 'r' :
            return kspack_open_r(data, size);
        case 'R' :
            return kspack_open_r(data, size);
        case 'w' :
            return kspack_open_w(data, size);
        case 'W' :
            return kspack_open_w(data, size);
        default :
            return 0;
    }
}

/* 以只读的方式打开kspack */
static kspack_t* kspack_open_r(void* data, const int size)
{
    kspack_t* pack = 0;
    ksnode_t* node = 0;
    uint32_t crcval = 0;
    uint16_t hashval = 0;
    int node_len = 0;
    char* ptr = 0;
    int len = 0;
    int i = 0;

    if(!data || size<=0){
        return 0;
    }

    // 申请kspack
    pack = (kspack_t*)malloc(sizeof(kspack_t));
    if(!pack){
        return 0;
    }

    // 填写公共信息
    pack->data = data;
    pack->open_type = 'r';
    pack->head = (kshead_t*)(pack->data);
    if(sizeof(kshead_t)+pack->head->data_size!=size){
        free(pack);
        return 0;
    }
    pack->parent = 0;
    pack->child = 0;

    // 填写写入信息
    pack->mem = 0;
    pack->mem_size = 0;
    pack->mem_used = 0;
    pack->mem_own = 0;
    pack->mem_p = 0;
    pack->mem_size_p = 0;
    pack->mem_used_p = 0;
    pack->node = 0;

    // 初始化读取信息
    bzero(pack->hashtable, sizeof(ksindex_t*)*65536);
    pack->index_array = (ksindex_t*)malloc(sizeof(ksindex_t)*(pack->head->node_size));
    if(!pack->index_array){
        free(pack);
        return 0;
    }
    pack->index_now = 0;
    // 构造索引信息
    ptr = (char*)(pack->head+1);
    len = pack->head->data_size;
    for(i=0; i<pack->head->node_size; i++){
        node = (ksnode_t*)ptr;
        node_len = sizeof(ksnode_t)+node->name_size+1+node->value_size;
        if(node_len>len){
            free(pack->index_array);
            free(pack);
            return 0;
        }
        ptr += node_len;
        len -= node_len;
        pack->index_array[i].node = node;
        crcval = kspack_crc32(node->name_value, node->name_size);
        hashval = (crcval&0xFFFF)^((crcval>>16)&0xFFFF);
        pack->index_array[i].next = pack->hashtable[hashval];
        pack->hashtable[hashval] = &(pack->index_array[i]);
    }
    if(len!=0){
        free(pack->index_array);
        free(pack);
        return 0;
    }

    return pack;
}

/* 以只写的方式打开kspack */
static kspack_t* kspack_open_w(void* data, const int size)
{
    kspack_t* pack = 0;

    // 申请kspack结构
    pack = (kspack_t*)malloc(sizeof(kspack_t));
    if(!pack){
        return 0;
    }

    // 填充写入信息
    // 如果是指定内存缓冲区方式打开
    if(data && size>0){
        if(size<sizeof(kshead_t)){
            free(pack);
            return 0;
        }
        pack->mem = data;
        pack->mem_own = 0;
        pack->mem_size = size;
        pack->mem_used = sizeof(kshead_t);
        pack->mem_p = &(pack->mem);
        pack->mem_size_p = &(pack->mem_size);
        pack->mem_used_p = &(pack->mem_used);
    }
    // 如果是自有内存缓冲区方式打开
    else{
        pack->mem = malloc(MEMLENBASE);
        if(!pack->mem){
            free(pack);
            return 0;
        }
        pack->mem_own = 1;
        pack->mem_size = MEMLENBASE;
        pack->mem_used = sizeof(kshead_t);
        pack->mem_p = &(pack->mem);
        pack->mem_size_p = &(pack->mem_size);
        pack->mem_used_p = &(pack->mem_used);
    }

    // 填充公共信息
    pack->data = pack->mem;
    pack->open_type = 'w';
    pack->head = (kshead_t*)(pack->data);
    pack->head->data_size = 0;
    pack->head->node_size = 0;
    pack->parent = 0;
    pack->child = 0;
    pack->node = (ksnode_t*)(pack->head+1);

    // 填充读取信息
    pack->index_array = 0;

    return pack;
}

static kspack_t* kspack_sub_open_r(kspack_t* parent, const char* name, const int nlen);
static kspack_t* kspack_sub_open_w(kspack_t* parent, const char* name, const int nlen);

/* 在一个kspack内部打开一个子kspack */
extern kspack_t* kspack_sub_open(kspack_t* parent, const char* name, const int nlen)
{
    if(!parent || !name || nlen<=0 || nlen>NAMESIZEMAX){
        return 0;
    }

    switch(parent->open_type){
        case 'r' :
            return kspack_sub_open_r(parent, name, nlen);
        case 'w' :
            return kspack_sub_open_w(parent, name, nlen);
        default :
            return 0;
    }
}

/* 在一个kspack内部以只读方式打开一个子kspack */
static kspack_t* kspack_sub_open_r(kspack_t* parent, const char* name, const int nlen)
{
    kspack_t* pack = 0;
    ksnode_t* node = 0;
    ksindex_t* index = 0;
    uint32_t crcval = 0;
    uint16_t hashval = 0;
    int node_len = 0;
    char* ptr = 0;
    int len = 0;
    int i = 0;

    // 找到name对应的node
    crcval = kspack_crc32(name, nlen);
    hashval = (crcval&0xFFFF)^((crcval>>16)&0xFFFF);
    index = parent->hashtable[hashval];
    while(index){
        node = index->node;
        if(node->name_size==nlen && strncmp(node->name_value, name, nlen)==0){
            break;
        }
        index = index->next;
    }
    if(!index){
        return 0;
    }

    // 申请kspack结构实例
    pack = (kspack_t*)malloc(sizeof(kspack_t));
    if(!pack){
        return 0;
    }

    // 填写公共信息
    pack->data = node->name_value+node->name_size+1;
    pack->open_type = 'r';
    pack->head = (kshead_t*)(pack->data);
    if(sizeof(kshead_t)+pack->head->data_size!=node->value_size){
        free(pack);
        return 0;
    }
    pack->parent = parent;
    pack->child = 0;

    // 填写写入信息
    pack->mem = 0;
    pack->mem_size = 0;
    pack->mem_used = 0;
    pack->mem_own = 0;
    pack->mem_p = 0;
    pack->mem_size_p = 0;
    pack->mem_used_p = 0;
    pack->node = 0;

    // 初始化读取信息
    bzero(pack->hashtable, sizeof(ksindex_t*)*65536);
    pack->index_array = (ksindex_t*)malloc(sizeof(ksindex_t)*(pack->head->node_size));
    if(!pack->index_array){
        free(pack);
        return 0;
    }
    pack->index_now = 0;
    // 填写读取信息
    ptr = (char*)(pack->head+1);
    len = pack->head->data_size;
    for(i=0; i<pack->head->node_size; i++){
        node = (ksnode_t*)ptr;
        node_len = sizeof(ksnode_t)+node->name_size+1+node->value_size;
        if(node_len>len){
            free(pack->index_array);
            free(pack);
            return 0;
        }
        ptr += node_len;
        len -= node_len;
        pack->index_array[i].node = node;
        crcval = kspack_crc32(node->name_value, node->name_size);
        hashval = (crcval&0xFFFF)^((crcval>>16)&0xFFFF);
        pack->index_array[i].next = pack->hashtable[hashval];
        pack->hashtable[hashval] = &(pack->index_array[i]);
    }
    if(len!=0){
        free(pack->index_array);
        free(pack);
        return 0;
    }

    // 设置上层kspack的子kspack
    parent->child = pack;

    return pack;
}

/* 在一个kspack内部以只写方式打开一个子kspack */
static kspack_t* kspack_sub_open_w(kspack_t* parent, const char* name, const int nlen)
{
    kspack_t* pack = 0;

    // 如果上层kspack已经打开一个子kspack，先关闭它
    if(parent->child){
        if(kspack_done(parent->child)){
            return 0;
        }
    }

    // 申请一个kspack实例
    pack = (kspack_t*)malloc(sizeof(kspack_t));
    if(!pack){
        return 0;
    }

    // 如果现有缓冲区剩余空间不足，并且拥有缓冲区控制权则重新分配，否则返回错误
    while(*(parent->mem_size_p)-*(parent->mem_used_p)<sizeof(ksnode_t)+nlen+1+sizeof(kshead_t)){
        if(parent->mem_own){
            kspack_t* cur = 0;
            void* mem = malloc(*(parent->mem_size_p)*2);
            if(!mem){
                free(pack);
                return 0;
            }
            memcpy(mem, *(parent->mem_p), *(parent->mem_used_p));
            // 重写上层所有节点的相关信息
            cur = parent;
            while(cur){
                cur->data = mem+((char*)(cur->data)-(char*)*(cur->mem_p));
                cur->head = (kshead_t*)(cur->data);
                cur->node = mem+((char*)(cur->node)-(char*)*(cur->mem_p));
                cur = cur->parent;
            }
            free(*(parent->mem_p));
            *(parent->mem_p) = mem;
            *(parent->mem_size_p) *= 2;
        }
        else{
            free(pack);
            return 0;
        }
    }

    // 填写上层kspack的节点信息
    parent->node->name_size = nlen;
    parent->node->value_type = KS_PACK_VALUE;
    parent->node->value_size = 0;
    memcpy(parent->node->name_value, name, nlen);
    parent->node->name_value[nlen] = 0;
    
    // 填写公共信息
    pack->data = parent->node->name_value+nlen+1;
    pack->open_type = 'w';
    pack->head = (kshead_t*)(pack->data);
    pack->head->data_size = 0;
    pack->head->node_size = 0;
    pack->parent = parent;
    pack->child = 0;
    pack->node = (ksnode_t*)(pack->head+1);

    // 填写写入信息
    pack->mem_own = parent->mem_own;
    pack->mem_p = parent->mem_p;
    pack->mem_size_p = parent->mem_size_p;
    pack->mem_used_p = parent->mem_used_p;
    *(pack->mem_used_p) += sizeof(ksnode_t)+nlen+1+sizeof(kshead_t);

    // 填充读取信息
    pack->index_array = 0;

    // 设置上层kspack的子kspack
    parent->child = pack;

    return pack;
}

/* 关闭kspack实例 */
extern int kspack_close(kspack_t* pack, int dist)
{
    if(pack){
        if(kspack_done(pack)){
            return -1;
        }
        if(pack->mem && pack->mem_own && !pack->parent && dist){
            free(pack->mem);
        }
        if(pack->index_array){
            free(pack->index_array);
        }
        free(pack);
    }

    return 0;
}

/* kspack写入或读取结束 */
extern int kspack_done(kspack_t* pack)
{
    if(pack && (pack->open_type=='r' || pack->open_type=='w')){
        // 先结束下游各层子kspack
        if(pack->child){
            if(kspack_done(pack->child)){
                return -1;
            }
        }
        // 如果上层kspack以写入方式打开，重新计算各结构占用的空间
        if(pack->parent && pack->open_type=='w'){
            uint32_t node_len = sizeof(kshead_t)+pack->head->data_size;
            kspack_t* parent = pack->parent;
            if(node_len>VALUESIZEMAX){
                return -1;
            }
            parent->node->value_size = node_len;
            parent->head->data_size += sizeof(ksnode_t)+parent->node->name_size+1+parent->node->value_size;
            parent->head->node_size += 1;
            parent->node = (ksnode_t*)((char*)(parent->data)+sizeof(kshead_t)+parent->head->data_size);
            parent->child = 0;
        }
        pack->open_type = 0;
    }

    return 0;
}

/* 返回一个kspack实例对应的内存区域 */
extern void* kspack_pack(kspack_t* pack)
{
    if(pack && pack->open_type==0){
        return pack->data;
    }
    else{
        return 0;
    }
}

/* 返回一个kspack实例对应的内存区域长度 */
extern int kspack_size(kspack_t* pack)
{
    if(pack && pack->open_type==0){
        return (sizeof(kshead_t)+pack->head->data_size);
    }
    else{
        return -1;
    }
}

/* 向一个kspack实例中添加数据 */
extern int kspack_put(kspack_t* pack, const char* name, const int nlen, const void* value, const int vlen, const char vtype)
{
    int node_len = sizeof(ksnode_t)+nlen+1+vlen;

    if(!pack || !name || nlen<=0 || nlen>NAMESIZEMAX || vlen>VALUESIZEMAX || vtype>=KS_VALUE_SIZE){
        return -1;
    }
    if(!value || vlen<=0 || vtype==KS_NONE_VALUE){
        node_len = sizeof(ksnode_t)+nlen+1;
    }
    if(pack->open_type!='w'){
        return -1;
    }

    // 如果现有缓冲区剩余空间不足，并且拥有缓冲区控制权则重新分配，否则返回错
    while(*(pack->mem_size_p)-*(pack->mem_used_p)<node_len){
        if(pack->mem_own){
            kspack_t* cur = 0;
            void* mem = malloc(*(pack->mem_size_p)*2);
            if(!mem){
                return -1;
            }
            memcpy(mem, *(pack->mem_p), *(pack->mem_used_p));
            // 重写上层所有节点的相关信息
            cur = pack;
            while(cur){
                cur->data = mem+((char*)(cur->data)-(char*)*(cur->mem_p));
                cur->head = (kshead_t*)(cur->data);
                cur->node = mem+((char*)(cur->node)-(char*)*(cur->mem_p));
                cur = cur->parent;
            }
            free(*(pack->mem_p));
            *(pack->mem_p) = mem;
            *(pack->mem_size_p) *= 2;
        }
        else{
            return -1;
        }
    }

    // 填写node信息
    if(!value || vlen<=0 || vtype==KS_NONE_VALUE){
        pack->node->name_size = nlen;
        pack->node->value_type = KS_NONE_VALUE;
        pack->node->value_size = 0;
        memcpy(pack->node->name_value, name, nlen);
        pack->node->name_value[nlen] = 0;
    }
    else{
        pack->node->name_size = nlen;
        pack->node->value_type = vtype;
        pack->node->value_size = vlen;
        memcpy(pack->node->name_value, name, nlen);
        pack->node->name_value[nlen] = 0;
        memcpy(pack->node->name_value+nlen+1, value, vlen);
    }
    // 修改kspack头信息
    pack->head->data_size += node_len;
    pack->head->node_size += 1;

    // 修改写入信息
    *(pack->mem_used_p) += node_len;
    pack->node = (ksnode_t*)((char*)(pack->node)+node_len);

    return 0;
}

/* 从一个kspack实例中获取数据 */
extern int kspack_get(kspack_t* pack, const char* name, const int nlen, void** value, int* vlen, char* vtype)
{
    ksnode_t* node = 0;
    ksindex_t* index = 0;
    uint32_t crcval = 0;
    uint16_t hashval = 0;

    if(!pack || !name || nlen<=0 || nlen>NAMESIZEMAX || !value || !vlen || !vtype){
        return -1;
    }

    *value = 0;
    *vlen = 0;
    *vtype = KS_NONE_VALUE;

    if(pack->open_type!='r'){
        return -1;
    }

    // 查找name对应的节点
    crcval = kspack_crc32(name, nlen);
    hashval = (crcval&0xFFFF)^((crcval>>16)&0xFFFF);
    index = pack->hashtable[hashval];
    while(index){
        node = index->node;
        if(node->name_size==nlen && strncmp(node->name_value, name, nlen)==0){
            break;
        }
        index = index->next;
    }
    if(!index){
        return -1;
    }

    // 填充返回信息
    if(node->value_type==KS_NONE_VALUE){
        *vtype = KS_NONE_VALUE;
        *vlen = 0;
        *value = 0;
    }
    else{
        *vtype = node->value_type;
        *vlen = node->value_size;
        *value = node->name_value+node->name_size+1;
    }

    return 0;
}

/* 初始化轮询游 */
extern void kspack_first(kspack_t* pack)
{
    if(pack){
        pack->index_now = 0;
    }
}

/* 从kspack实例中获取下一条数据 */
extern int kspack_next(kspack_t* pack, char** name, int* nlen, void** value, int* vlen, char* vtype)
{
    ksnode_t* node = 0;

    if(!pack || !name || !nlen || !value || !vlen || !vtype){
        return -1;
    }

    // 先将返回值清零
    *name = 0;
    *nlen = 0;
    *value = 0;
    *vlen = 0;
    *vtype = KS_NONE_VALUE;

    // 检查kspack实例是否以读方式打开
    if(pack->open_type!='r'){
        return -1;
    }
    // 检查是否轮询完毕
    if(pack->index_now==pack->head->node_size){
        return -1;
    }

    // 填充返回值
    node = pack->index_array[pack->index_now++].node;
    *name = node->name_value;
    *nlen = node->name_size;
    if(node->value_type==KS_NONE_VALUE){
        *value = 0;
        *vlen = 0;
        *vtype = KS_NONE_VALUE;
    }
    else{
        *value = node->name_value+node->name_size+1;
        *vlen = node->value_size;
        *vtype = node->value_type;
    }

    return 0;
}

/* ncrc32码表 */
static uint32_t kspack_crc32_table[256] = { 
    0x00000000,0x77073096,0xee0e612c,0x990951ba,0x076dc419,0x706af48f,0xe963a535,0x9e6495a3,
    0x0edb8832,0x79dcb8a4,0xe0d5e91e,0x97d2d988,0x09b64c2b,0x7eb17cbd,0xe7b82d07,0x90bf1d91,
    0x1db71064,0x6ab020f2,0xf3b97148,0x84be41de,0x1adad47d,0x6ddde4eb,0xf4d4b551,0x83d385c7,
    0x136c9856,0x646ba8c0,0xfd62f97a,0x8a65c9ec,0x14015c4f,0x63066cd9,0xfa0f3d63,0x8d080df5,
    0x3b6e20c8,0x4c69105e,0xd56041e4,0xa2677172,0x3c03e4d1,0x4b04d447,0xd20d85fd,0xa50ab56b,
    0x35b5a8fa,0x42b2986c,0xdbbbc9d6,0xacbcf940,0x32d86ce3,0x45df5c75,0xdcd60dcf,0xabd13d59,
    0x26d930ac,0x51de003a,0xc8d75180,0xbfd06116,0x21b4f4b5,0x56b3c423,0xcfba9599,0xb8bda50f,
    0x2802b89e,0x5f058808,0xc60cd9b2,0xb10be924,0x2f6f7c87,0x58684c11,0xc1611dab,0xb6662d3d,
    0x76dc4190,0x01db7106,0x98d220bc,0xefd5102a,0x71b18589,0x06b6b51f,0x9fbfe4a5,0xe8b8d433,
    0x7807c9a2,0x0f00f934,0x9609a88e,0xe10e9818,0x7f6a0dbb,0x086d3d2d,0x91646c97,0xe6635c01,
    0x6b6b51f4,0x1c6c6162,0x856530d8,0xf262004e,0x6c0695ed,0x1b01a57b,0x8208f4c1,0xf50fc457,
    0x65b0d9c6,0x12b7e950,0x8bbeb8ea,0xfcb9887c,0x62dd1ddf,0x15da2d49,0x8cd37cf3,0xfbd44c65,
    0x4db26158,0x3ab551ce,0xa3bc0074,0xd4bb30e2,0x4adfa541,0x3dd895d7,0xa4d1c46d,0xd3d6f4fb,
    0x4369e96a,0x346ed9fc,0xad678846,0xda60b8d0,0x44042d73,0x33031de5,0xaa0a4c5f,0xdd0d7cc9,
    0x5005713c,0x270241aa,0xbe0b1010,0xc90c2086,0x5768b525,0x206f85b3,0xb966d409,0xce61e49f,
    0x5edef90e,0x29d9c998,0xb0d09822,0xc7d7a8b4,0x59b33d17,0x2eb40d81,0xb7bd5c3b,0xc0ba6cad,
    0xedb88320,0x9abfb3b6,0x03b6e20c,0x74b1d29a,0xead54739,0x9dd277af,0x04db2615,0x73dc1683,
    0xe3630b12,0x94643b84,0x0d6d6a3e,0x7a6a5aa8,0xe40ecf0b,0x9309ff9d,0x0a00ae27,0x7d079eb1,
    0xf00f9344,0x8708a3d2,0x1e01f268,0x6906c2fe,0xf762575d,0x806567cb,0x196c3671,0x6e6b06e7,
    0xfed41b76,0x89d32be0,0x10da7a5a,0x67dd4acc,0xf9b9df6f,0x8ebeeff9,0x17b7be43,0x60b08ed5,
    0xd6d6a3e8,0xa1d1937e,0x38d8c2c4,0x4fdff252,0xd1bb67f1,0xa6bc5767,0x3fb506dd,0x48b2364b,
    0xd80d2bda,0xaf0a1b4c,0x36034af6,0x41047a60,0xdf60efc3,0xa867df55,0x316e8eef,0x4669be79,
    0xcb61b38c,0xbc66831a,0x256fd2a0,0x5268e236,0xcc0c7795,0xbb0b4703,0x220216b9,0x5505262f,
    0xc5ba3bbe,0xb2bd0b28,0x2bb45a92,0x5cb36a04,0xc2d7ffa7,0xb5d0cf31,0x2cd99e8b,0x5bdeae1d,
    0x9b64c2b0,0xec63f226,0x756aa39c,0x026d930a,0x9c0906a9,0xeb0e363f,0x72076785,0x05005713,
    0x95bf4a82,0xe2b87a14,0x7bb12bae,0x0cb61b38,0x92d28e9b,0xe5d5be0d,0x7cdcefb7,0x0bdbdf21,
    0x86d3d2d4,0xf1d4e242,0x68ddb3f8,0x1fda836e,0x81be16cd,0xf6b9265b,0x6fb077e1,0x18b74777,
    0x88085ae6,0xff0f6a70,0x66063bca,0x11010b5c,0x8f659eff,0xf862ae69,0x616bffd3,0x166ccf45,
    0xa00ae278,0xd70dd2ee,0x4e048354,0x3903b3c2,0xa7672661,0xd06016f7,0x4969474d,0x3e6e77db,
    0xaed16a4a,0xd9d65adc,0x40df0b66,0x37d83bf0,0xa9bcae53,0xdebb9ec5,0x47b2cf7f,0x30b5ffe9,
    0xbdbdf21c,0xcabac28a,0x53b39330,0x24b4a3a6,0xbad03605,0xcdd70693,0x54de5729,0x23d967bf,
    0xb3667a2e,0xc4614ab8,0x5d681b02,0x2a6f2b94,0xb40bbe37,0xc30c8ea1,0x5a05df1b,0x2d02ef8d,
};

/* 计算crc32 hash值 */
static uint32_t kspack_crc32(const void* data, size_t len)
{
    uint32_t crc32 = 0;
    unsigned char* cur = 0;
    int i;

    crc32 ^= 0xffffffff;
    for(i=0,cur=(unsigned char*)data; i<len; i++,cur++){
        crc32 = kspack_crc32_table[(crc32^*cur)&0xff]^(crc32>>8);
    }   
    crc32 ^= 0xffffffff;

    return crc32;
}
