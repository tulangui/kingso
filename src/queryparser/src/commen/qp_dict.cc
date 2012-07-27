#include "qp_dict.h"
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include "util/MemPool.h"
#include "util/strfunc.h"
#include "commdef/commdef.h"


/**
 * 在红黑树中查找hash值对应的节点
 *
 * @param root 红黑树的根
 * @param crcval crc32 hash值
 * @param parent 用于指向hash值对应节点的父节点
 *
 * @return !NULL hash值对应的节点
 *         NULL  查找失败
 */
static qp_dict_node_t* _qp_find_node(qp_dict_node_t* root, uint32_t crcval, qp_dict_node_t** parent);

/**
 * 计算crc32 hash值
 *
 * @param data 待计算的数据
 * @param len 待计算的数据长度
 *
 * @return crc32 hash值
 */
static uint32_t _qp_crc32(const void* data, size_t len);


/* 创建字典 */
extern qp_dict_t* qp_dict_create(MemPool* mempool)
{
	qp_dict_t* dict = 0;
	
	if(!mempool){
		TNOTE("qp_dict_create argument error, return!");
		return 0;
	}
	
	// 为词典分配空间，申请内存池空间，并初始化词典
	dict = NEW(mempool, qp_dict_t);
	if(dict){
		dict->root.rb_node = 0;
		dict->items = 0;
		dict->dnode = 0;
		dict->lnode = 0;
		dict->mempool = mempool;
	}
	else{
		TNOTE("NEW qp_dict_t error, return!");
	}
	
	return dict;
}

/* 销毁字典 */
extern void qp_dict_destroy(qp_dict_t* dict)
{
}

/* 清理字典 */
extern void qp_dict_clean(qp_dict_t* dict)
{
	if(dict){
		dict->root.rb_node = 0;
		dict->items = 0;
		dict->dnode = 0;
		dict->lnode = 0;
	}
}

/* 向字典中插入一个词条 */
extern int qp_dict_insert(qp_dict_t* dict, const char* name, int nlen, const void* value, int vlen)
{
	char real_name[MAX_FIELD_NAME_LEN+1] = {0};
	uint32_t crcval = 0;
	qp_dict_node_t* node = 0;
	qp_dict_node_t* parent = 0;
	qp_list_node_t* curr = 0;
	qp_list_node_t** curp = 0;
	MemPool* mempool = 0;
	
	if(!dict || !name || nlen<=0 || nlen>MAX_FIELD_NAME_LEN){
		TNOTE("qp_dict_insert argument error, return!");
		return -1;
	}
	
	mempool = dict->mempool;
	
	memcpy(real_name, name, nlen);
	real_name[nlen] = 0;
	str_lowercase(real_name);
	
	// 计算crc32 hash值，并根据该值查找索引项拉链，如果hash值对应的节点不存在则创建
	crcval = _qp_crc32(real_name, nlen);
	node = _qp_find_node((qp_dict_node_t*)(dict->root.rb_node), crcval, &parent);
	if(!node){
		node = NEW(mempool, qp_dict_node_t);
		if(!node){
			TNOTE("NEW qp_dict_node_t error, return!");
			return -1;
		}
		node->crcval = crcval;
		node->head = 0;
		if(!parent){
			rb_link_node((struct rb_node*)node, 0, &(dict->root.rb_node));
		}
		else if(parent->crcval>crcval){
			rb_link_node((struct rb_node*)node, (struct rb_node*)parent, &(parent->rb.rb_left));
		}
		else{
			rb_link_node((struct rb_node*)node, (struct rb_node*)parent, &(parent->rb.rb_right));
		}
		rb_insert_color((struct rb_node*)node, &(dict->root));
	}
	
	// 遍历索引项拉链，寻找是否存在重复词条
	curr = node->head;
	curp = &(node->head);
	while(curr){
		if(curr->nlen==nlen && strncmp(curr->name, real_name, nlen)==0){
			return 0;
		}
		curp = &(curr->next);
		curr = curr->next;
	}
	
	curr = NEW(mempool, qp_list_node_t);
	if(!curr){
		TNOTE("NEW qp_list_node_t error, return!");
		return -1;
	}
	curr->next = 0;
	curr->name = NEW_VEC(mempool, char, nlen+1);
	if(!curr->name){
		TNOTE("NEW_VEC name error, return!");
		return -1;
	}
	memcpy(curr->name, real_name, nlen);
	curr->name[nlen] = 0;
	curr->nlen = nlen;
	if(value && vlen>0){
		curr->value = NEW_VEC(mempool, char, vlen);
		if(!curr->value){
			TNOTE("NEW_VEC value error, return!");
			return -1;
		}
		memcpy(curr->value, value, vlen);
		curr->vlen = vlen;
	}
	else{
		curr->value = 0;
		curr->vlen = 0;
	}
	*curp = curr;
	
	dict->items ++;
	
	return 1;
}

/* 在字典中查找一个词条 */
extern int qp_dict_update(qp_dict_t* dict, const char* name, int nlen, const void* value, int vlen)
{
	char real_name[MAX_FIELD_NAME_LEN+1] = {0};
	uint32_t crcval = 0;
	qp_dict_node_t* node = 0;
	qp_dict_node_t* parent = 0;
	qp_list_node_t* curr = 0;
	qp_list_node_t** curp = 0;
	MemPool* mempool = 0;
	
	if(!dict || !name || nlen<=0 || nlen>MAX_FIELD_NAME_LEN){
		TNOTE("qp_dict_insert argument error, return!");
		return -1;
	}
	
	mempool = dict->mempool;
	
	memcpy(real_name, name, nlen);
	real_name[nlen] = 0;
	str_lowercase(real_name);
	
	// 计算crc32 hash值，并根据该值查找索引项拉链，如果hash值对应的节点不存在则创建
	crcval = _qp_crc32(real_name, nlen);
	node = _qp_find_node((qp_dict_node_t*)(dict->root.rb_node), crcval, &parent);
	if(!node){
		node = NEW(mempool, qp_dict_node_t);
		if(!node){
			TNOTE("NEW qp_dict_node_t error, return!");
			return -1;
		}
		node->crcval = crcval;
		node->head = 0;
		if(!parent){
			rb_link_node((struct rb_node*)node, 0, &(dict->root.rb_node));
		}
		else if(parent->crcval>crcval){
			rb_link_node((struct rb_node*)node, (struct rb_node*)parent, &(parent->rb.rb_left));
		}
		else{
			rb_link_node((struct rb_node*)node, (struct rb_node*)parent, &(parent->rb.rb_right));
		}
		rb_insert_color((struct rb_node*)node, &(dict->root));
	}
	
	// 遍历索引项拉链，寻找是否存在重复词条
	curr = node->head;
	curp = &(node->head);
	while(curr){
		if(curr->nlen==nlen && strncmp(curr->name, real_name, nlen)==0){
			break;
		}
		curp = &(curr->next);
		curr = curr->next;
	}
	if(!curr){
		curr = NEW(mempool, qp_list_node_t);
		if(!curr){
			TNOTE("NEW qp_list_node_t error, return!");
			return -1;
		}
		curr->next = 0;
		curr->name = NEW_VEC(mempool, char, nlen+1);
		if(!curr->name){
			TWARN("NEW_VEC name error, return!");
			return -1;
		}
		memcpy(curr->name, real_name, nlen);
		curr->name[nlen] = 0;
		curr->nlen = nlen;
		*curp = curr;
		dict->items ++;
	}
	
	if(value && vlen>0){
		curr->value = NEW_VEC(mempool, char, vlen);
		if(!curr->value){
			TNOTE("NEW_VEC value error, return!");
			curr->vlen = 0;
			return -1;
		}
		memcpy(curr->value, value, vlen);
		curr->vlen = vlen;
	}
	else{
		curr->value = 0;
		curr->vlen = 0;
	}
	
	return 1;
}

/* 在字典中查找一个词条 */
extern int qp_dict_select(qp_dict_t* dict, const char* name, int nlen, void** value, int* vlen)
{
	char real_name[MAX_FIELD_NAME_LEN+1] = {0};
	uint32_t crcval = 0;
	qp_dict_node_t* node = 0;
	qp_dict_node_t* parent = 0;
	qp_list_node_t* curr = 0;
	MemPool* mempool = 0;
	
	if(!dict || !name || nlen<=0 || nlen>MAX_FIELD_NAME_LEN){
		TNOTE("qp_dict_select argument error, return!");
		return -1;
	}
	
	mempool = dict->mempool;
	
	memcpy(real_name, name, nlen);
	real_name[nlen] = 0;
	str_lowercase(real_name);
	
	// 计算crc32 hash值，并根据该值查找索引项拉链
	crcval = _qp_crc32(real_name, nlen);
	node = _qp_find_node((qp_dict_node_t*)(dict->root.rb_node), crcval, &parent);
	if(!node){
		return 0;
	}
	
	// 遍历索引拉链，寻找对应词条
	curr = node->head;
	while(curr){
		if(curr->nlen==nlen && strncmp(curr->name, real_name, nlen)==0){
			break;
		}
		curr = curr->next;
	}
	if(!curr){
		return 0;
	}
	
	// 填充参数返回值
	if(value && vlen){
		*value = curr->value;
		*vlen = curr->vlen;
	}
	
	return 1;
}

/* 初始化字典遍历游标 */
extern void qp_dict_first(qp_dict_t* dict)
{
	if(!dict){
		return;
	}
	
	dict->dnode = 0;
	dict->lnode = 0;
}

/* 获取字典遍历的下一个节点 */
extern int qp_dict_next(qp_dict_t* dict, char** name, int* nlen, void** value, int* vlen)
{
	if(!dict || !name || !nlen){
		TNOTE("qp_dict_next argument error, return!");
		return -1;
	}
	
	// 如果dnode为空，表示是第一次调用，使dnode指向红黑树的第一个节点
	if(!dict->dnode){
		dict->dnode = (qp_dict_node_t*)rb_first(&(dict->root));
		if(!dict->dnode){
			return 0;
		}
		dict->lnode = dict->dnode->head;
	}
	// 否则指向当前hash值对应拉链的下一节点
	else{
		dict->lnode = dict->lnode->next;
	}
	// 只要lnode为空，就更换一个hash值对应的索引拉链
	while(!dict->lnode){
		dict->dnode = (qp_dict_node_t*)rb_next((struct rb_node*)(dict->dnode));
		if(!dict->dnode){
			return 0;
		}
		dict->lnode = dict->dnode->head;
	}
	
	// 填充参数返回值
	*name = dict->lnode->name;
	*nlen = dict->lnode->nlen;
	if(value && vlen){
		*value = dict->lnode->value;
		*vlen = dict->lnode->vlen;
	}
	
	return 1;
}

/* 获取字典中的词条总数 */
extern int qp_dict_get_count(qp_dict_t* dict)
{
	if(!dict){
		TNOTE("qp_dict_get_count argument error, return!");
		return -1;
	}
	
	return (dict->items);
}

/* 在红黑树中查找hash值对应的节点 */
static qp_dict_node_t* _qp_find_node(qp_dict_node_t* root, uint32_t crcval, qp_dict_node_t** parent)
{
	qp_dict_node_t* cur = root;
	
	*parent = 0;
	
	while(cur){
		// 如果当前节点hash值小于待查找hash值，则进入右子树查找
		if(cur->crcval<crcval){
			*parent = cur;
			cur = (qp_dict_node_t*)(cur->rb.rb_right);
		}
		// 如果当前节点hash值大于待查找hash值，则进入左子树查找
		else if(cur->crcval>crcval){
			*parent = cur;
			cur = (qp_dict_node_t*)(cur->rb.rb_left);
		}
		// 如果当前节点hash值等于待查找hash值，则返回当前节点
		else{
			break;
		}
	}
	
	return cur;
}

/* crc32码表 */
static uint32_t _qp_crc32_table[256] = {
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
static uint32_t _qp_crc32(const void* data, size_t len)
{
	uint32_t crc32 = 0;
	unsigned char* cur = 0;
	int i;
	
	crc32 ^= 0xffffffff;
	for(i=0,cur=(unsigned char*)data; i<len; i++,cur++){
		crc32 = _qp_crc32_table[(crc32^*cur)&0xff]^(crc32>>8);
	}
	crc32 ^= 0xffffffff;
	
	return crc32;
}

