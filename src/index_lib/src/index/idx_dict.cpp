/***********************************************************************************
 * Describe : a simple hash table, interger key, interger value
 * 
 * Author   : Paul Yang, zhenahoji@gmail.com
 * 
 * Create   : 2008-10-15
 * 
 * Modify   : 2010-10-10 by shituo
 **********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>

#include "idx_dict.h"
#include "util/common.h"
#include "util/Log.h"
#include "IndexFieldSyncManager.h"

// 用于表示节点无效的游标值
static const unsigned int INODE_COMMON_NULL = 0xFFFFFFFF;
// 节点不够重新分配内存是增加的步长
static const int          IDICT_BLOCK_STEP  = 50000;
// 双buffer结点的删除延长时间
static const int          DELAY_TIME        = 30;

// hash 表长度取值范围
#define HASH_STEPTABLE_SIZE 25
static int32_t szHashMap_StepTable[25] = {
  53,97,193,389,769,
  1543,3079,6151,12289,24593,
  49157,98317,196613,393241,786433,
  1572869,3145739,6291469,12582917,25165843,
  50331653,100663319,201326611,402653189,805306457
};

#define IDICT_GET_HASH(sign, hashsize) \
	((((sign)&0x00000000FFFFFFFFL) + ((sign)>>32)) % (hashsize))

/*
 * func : create an idx_dict_t struct
 *
 * args : hashsize, the hash table size
 *      : nodesize, the node array size
 *
 * ret  : NULL, error;
 *      : else, pointer the the idx_dict_t struct
 */ 
idx_dict_t*  idx_dict_create(const int hashsize, const int nodesize)
{
	if ( unlikely( hashsize <= 0 || nodesize < 0 ) ) {
		TERR("parameter error: hashsize %d, nodesize %d", hashsize, nodesize);
		return NULL;
	}
	
	idx_dict_t      *pdict   = NULL;
	idict_node_t    *block   = NULL;
	unsigned int    *hashtab = NULL;

	// create hash table
	hashtab = (unsigned int*)calloc(hashsize, sizeof(unsigned int));
	if (!hashtab) {
		TERR("alloc hashtab size %d failed", hashsize);
		goto failed;
	}
	for (int i=0;i<hashsize;i++) {
		hashtab[i] = INODE_COMMON_NULL;
	}

	block = (idict_node_t*)calloc(nodesize, sizeof(idict_node_t));
	if (!block) {
		TERR("alloc block size %d failed", nodesize);
		goto failed;
	}
	for (int i=0;i<nodesize;i++){
		block[i].next = INODE_COMMON_NULL;
	}

	// create the struct
	pdict = (idx_dict_t*)calloc(1, sizeof(idx_dict_t));
	if (!pdict) {
		TERR("alloc struct idx_dict_t failed");
		goto failed;
	}
	pdict->hashtab      = hashtab;
	pdict->hashsize     = hashsize;
	pdict->block        = block;
	pdict->block_size   = nodesize;
	pdict->block_pos    = 0;
    pdict->syncMgr      = NULL;
    pdict->old_block    = NULL;
    pdict->canFreeTime  = 0;

	return pdict;

failed:
	if (pdict) {
		free(pdict);
		pdict = NULL;
	}
	if (hashtab) {
		free(hashtab);
		hashtab = NULL;
	}
	if (block) {
		free(block);
		block = NULL;
	}
	return NULL;
}

/*
 * func : free a idx_dict_t struct
 */
void idx_dict_free(idx_dict_t *pdict)
{
	if (!pdict) {
		return;
	}
	
	if (pdict->hashtab) {
		free(pdict->hashtab);
		pdict->hashtab = NULL;
	}
	if (pdict->block) {
		free(pdict->block);
		pdict->block = NULL;
	}
    if (pdict->old_block) {
        free(pdict->old_block);
        pdict->old_block = NULL;
    }

	free(pdict);
	pdict = NULL;
}

/*
 * func : add a node to the hash table;
 * 
 * args : pdict, pointer to idx_dict_t 
 *      : node , pointer to input node
 *
 * ret  : 1,  find a same key, value changed;
 *      : 0,  find NO same key, new node added,
 *      : -1, error;
 */
int idx_dict_add(idx_dict_t *pdict, idict_node_t *node) 
{
	if ( unlikely( NULL == pdict || NULL == node ) ) {
		TERR("parameter error: pdict %p, node %p", pdict, node);
		return -1;
	}

	idict_node_t  *pnode = idx_dict_find(pdict, node->sign);
	if (!pnode) { // not found
		unsigned int  *hashtab    = pdict->hashtab;
		unsigned int   pos        = IDICT_GET_HASH(node->sign, pdict->hashsize);

		if (pdict->block_pos == pdict->block_size) { 
			// if block array is full, realloc block array
            idict_node_t *block = (idict_node_t *)calloc((pdict->block_size + IDICT_BLOCK_STEP), sizeof(idict_node_t));
            if (!block) {
                TERR("idx_dict malloc failed!");
                return -1;
            }

            memcpy(block, pdict->block, sizeof(idict_node_t) * pdict->block_size);
            pdict->canFreeTime = time(NULL) + DELAY_TIME;
            if (pdict->old_block != NULL) {
                free(pdict->old_block);
                pdict->old_block = NULL;
            }

            pdict->old_block   = pdict->block;
            pdict->block       = block;
			pdict->block_size += IDICT_BLOCK_STEP;
		}

        pnode = pdict->block + pdict->block_pos;
		node->next = hashtab[pos]; // insert to the head of the list
		memcpy(pnode, node, sizeof(idict_node_t));
		
		hashtab[pos] = pdict->block_pos; 
		(pdict->block_pos)++;

        if (pdict->syncMgr) {
            index_lib::IndexFieldSyncManager * pSyncMgr = (index_lib::IndexFieldSyncManager *)pdict->syncMgr;
            pSyncMgr->putIdxDictNode(pnode);
            pSyncMgr->putIdxHashNode(pos);
            pSyncMgr->putIdxBlockPos();
            pSyncMgr->syncToDisk();
        }

		return 0;
		
	} else { // found
		pnode->cuint1 = node->cuint1;
		pnode->cuint2 = node->cuint2;
		pnode->cuint3 = node->cuint3;

        if (pdict->syncMgr) {
            index_lib::IndexFieldSyncManager * pSyncMgr = (index_lib::IndexFieldSyncManager *)pdict->syncMgr;
            pSyncMgr->putIdxDictNode(pnode);
            pSyncMgr->syncToDisk();
        }
		return 1;
	}

}

/*
 * func : reset the hash table;
 */
void idx_dict_reset(idx_dict_t *pdict)
{
	if (pdict) {
		for (unsigned int i=0;i<pdict->block_pos;i++){
			pdict->block[i].next = INODE_COMMON_NULL;
		}
		pdict->block_pos = 0;

		for(unsigned int i=0;i<pdict->hashsize;i++){
			pdict->hashtab[i] = INODE_COMMON_NULL;
		}
	} 
}

/*
 * func : get the first node in hash table
 *
 * args : pos, return the position of the first node
 *
 * ret  : NULL, can NOT get first node, hash table empty
 *      : else, pointer to the first node
 */
idict_node_t* idx_dict_first(idx_dict_t *pdict, unsigned int *pos)
{
	if (pdict->block_pos > 0)	{
		*pos = 0;
		return pdict->block;
	}
	return NULL;
}

/*
 * func : get next node from the hash table
 *
 * args : pos, the start position, get node after pos, 
 *
 * ret  : NULL, can NOT get next NODE, reach the end.
 *      : else, pointer to the next NODE
 */
idict_node_t* idx_dict_next(idx_dict_t *pdict, unsigned int *pos)
{
	if (pdict->block_pos > 0 && *pos < pdict->block_pos - 1)
	{
		(*pos)++;
		return pdict->block + *pos;
	}
	return NULL;
}

/*
 * func : find node in hash table by signature
 *
 * args : pdict, pointer to hash table
 *      : key,keylen, string and it's length
 *
 * ret  : NULL, not found
 *      : else, pointer to the founded node
 */
idict_node_t* idx_dict_find(idx_dict_t *pdict, const char *key, const int keylen)
{
	idict_node_t  *pnode = NULL;
	uint64_t       sign  = 0;

	sign  = idx_sign64(key, keylen);
	pnode = idx_dict_find(pdict, sign);

	return pnode;
}

/*
 * func : find node in hash table by signature
 *
 * args : pdict, pointer to hash table
 *      : sign,  64 bit string signature
 *
 * ret  : NULL, not found
 *      : else, pointer to the founded node
 */
idict_node_t* idx_dict_find(idx_dict_t *pdict, uint64_t sign)
{
	if ( unlikely(NULL == pdict) ) {
		TERR("parameter error: pdict NULL");
		return NULL;
	}

    /*
    if (unlikely(NULL != pdict->old_block && (time(NULL) >= pdict->canFreeTime))){
        free(pdict->old_block);
        pdict->old_block   = NULL;
        pdict->canFreeTime = 0;
    }
    */

	unsigned int  *hashtab    = pdict->hashtab;
	idict_node_t  *block      = pdict->block;
	unsigned int   pos        = IDICT_GET_HASH(sign, pdict->hashsize);
	unsigned int   ind        = hashtab[pos];

	while (INODE_COMMON_NULL != ind) {
		if (block[ind].sign == sign) {
			return block + ind;
		}
		ind = block[ind].next;
	}
	return NULL;
}

/*
 * func : save idx_dict_t to disk file
 *
 * args : pdict, the idx_dict_t pointer 
 *      : path, file, dest path and file
 *
 * ret  : 0, succeed; 
 *        -1, error.
 */
int idx_dict_save(idx_dict_t *pdict, const char *path, const char *file)
{
	if ( unlikely(NULL == pdict || NULL == path || NULL == file) ) {
		TERR("parameter error: pdict %p, path %s, file %s", pdict, path, file);
		return -1;
	}
  // 进行哈希优化,save 完成后要销毁新生成的pdict
  pdict = idx_dict_optimize(pdict);
  if(NULL == pdict) {
		TERR("dict optimize failed");
    return -1;
  }


	FILE         *fp         = NULL;
	unsigned int  hashsize   = 0;
	unsigned int  block_pos  = 0;
	char          fullpath[PATH_MAX];

	hashsize   = pdict->hashsize;
	block_pos  = pdict->block_pos;

	snprintf(fullpath, sizeof(fullpath), "%s/%s", path, file);
	fp = fopen(fullpath, "wb");
	if (!fp) {
		TERR("open file %s for idict save failed. errno %d", 
				fullpath, errno);
		goto failed;
	}
	
	// save integer values of idx_dict_t
	if (fwrite(&hashsize, sizeof(unsigned int), 1, fp) != 1) {
		TERR("write hashsize to file %s failed. errno %d", 
				fullpath, errno);
		goto failed;
	}
	if (fwrite(&block_pos, sizeof(unsigned int), 1, fp) != 1) {
		TERR("write block_pos to file %s failed. errno %d", 
				fullpath, errno);
		goto failed;
	}

	// save hashtable
	if (fwrite(pdict->hashtab, sizeof(unsigned int), hashsize, fp) != hashsize) {
		TERR("write hashtab size %u to file %s failed. errno %d", 
				hashsize, fullpath, errno);
		goto failed;
	}
	
	// save blocks
	if (fwrite(pdict->block, sizeof(idict_node_t), block_pos, fp) != block_pos) {
		TERR("write block size %u to file %s failed. errno %d", 
				block_pos, fullpath, errno);
		goto failed;
	}

	fclose(fp);
  idx_dict_free(pdict);
	return 0;

failed:
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
  idx_dict_free(pdict);
	return -1;
}

/*
 * func : load idx_dict_t from disk file
 *
 * args : path, file
 *
 * ret  : NULL, error
 *      : else, pointer to idx_dict_t struct
 */
idx_dict_t* idx_dict_load(const char *path, const char *file)
{
	if ( unlikely(NULL == path || NULL == file) ) {
		TERR("parameter error: path %s, file %s", path, file);
		return NULL;
	}

	unsigned int  hashsize   = 0;
	unsigned int  block_pos  = 0;
  unsigned int  nodeSize;
	FILE         *fp         = NULL;
	idx_dict_t   *pdict      = NULL;
	char          fullpath[PATH_MAX];

	// open dict file
	snprintf(fullpath, sizeof(fullpath), "%s/%s", path, file);
	fp = fopen(fullpath, "rb");
	if (!fp) {
		TLOG("open file %s: %s", fullpath, strerror(errno));
		goto failed;
	}

	// load integer values of idx_dict_t
	if (fread(&hashsize, sizeof(unsigned int), 1, fp) != 1) {
		TERR("read hashsize from file %s failed. errno %d",
				fullpath, errno);
		goto failed;
	}
	if (fread(&block_pos, sizeof(unsigned int), 1, fp) != 1) {
		TERR("read block_pos from file %s failed. errno %d",
				fullpath, errno);
		goto failed;
	}

  nodeSize = block_pos;
  if(nodeSize <= 0) nodeSize = 1;
	// create idx_dict_t struct
	pdict = idx_dict_create(hashsize, nodeSize);
	if (!pdict) {
		TERR("create idx_dict failed. hashsize %u, nodesize %u", 
				hashsize, nodeSize);
		goto failed;
	}
	
	// load hash tabel
	if (fread(pdict->hashtab, sizeof(unsigned int), hashsize, fp) != hashsize) {
		TERR("read hashtab size %u from file %s failed. errno %d",
				hashsize, fullpath, errno);
		goto failed;
	}

	// load blocks
	if (fread(pdict->block, sizeof(idict_node_t), block_pos, fp) != block_pos) {
		TERR("read block size %u from file %s failed. errno %d",
				block_pos, fullpath, errno);
		goto failed;
	}

	pdict->block_pos = block_pos;
	pdict->hashsize  = hashsize;
    pdict->syncMgr   = NULL;

	fclose(fp);
	return pdict;

failed:
	if (fp) {
		fclose(fp);
		fp = NULL;
	}
	if (pdict) {
		idx_dict_free(pdict);
		pdict = NULL;
	}
	return NULL;
}

// dict optimize
idx_dict_t* idx_dict_optimize(idx_dict_t *pdict)
{
	if ( unlikely(NULL == pdict) ) {
		TERR("parameter error: pdict %p", pdict);
		return NULL;
	}

	// create the struct
  idx_dict_t*	opt_dict = (idx_dict_t*)calloc(1, sizeof(idx_dict_t));
	if (NULL == opt_dict) {
		TERR("alloc struct idx_dict_t failed");
		return NULL;
	}
  memset(opt_dict, 0, sizeof(idx_dict_t));

  // 拷贝hash节点
  uint32_t block_pos = pdict->block_pos;
  if(block_pos <= 0) {
    block_pos = 1;
  }
	opt_dict->block = (idict_node_t*)calloc(block_pos, sizeof(idict_node_t));
  if(NULL == opt_dict->block) {
		TERR("alloc block idict_node_t failed, num=%d", block_pos);
    idx_dict_free(opt_dict);
    return NULL;
  }
  memcpy(opt_dict->block, pdict->block, pdict->block_pos * sizeof(idict_node_t));
  opt_dict->block_pos = pdict->block_pos;
  opt_dict->block_size = block_pos;

  // 优化哈希表
  int32_t ret = idx_find_value(szHashMap_StepTable, HASH_STEPTABLE_SIZE, pdict->block_pos);
  if(ret < 0) {
    TERR("寻找合适的哈希值失败, ret=%d", ret);
    idx_dict_free(opt_dict);
    return NULL;
  }
  uint32_t hash_size = szHashMap_StepTable[ret];
	// create hash table
	opt_dict->hashtab = (unsigned int*)calloc(hash_size, sizeof(unsigned int));
	if (NULL == opt_dict->hashtab) {
		TERR("alloc hashtab size %d failed", hash_size);
    idx_dict_free(opt_dict);
    return NULL;
	}
  opt_dict->hashsize = hash_size;
	for (uint32_t i=0;i<hash_size;i++) {
		opt_dict->hashtab[i] = INODE_COMMON_NULL;
	}
  
  // create hash struct
  unsigned int   pos;
  unsigned int  *hashtab = opt_dict->hashtab;
  idict_node_t* block = opt_dict->block;
  for(uint32_t i = 0; i < pdict->block_pos; i++) {
		pos = IDICT_GET_HASH(block[i].sign, hash_size);
		block[i].next = hashtab[pos]; // insert to the head of the list
		hashtab[pos] = i; 
  }

	return opt_dict;
}

// 查询value在数组中的上限区间
int32_t idx_find_value(int32_t* array, int32_t num, int32_t value)
{
  int32_t* begin = array;
  int32_t* end = array + num;
  int32_t* mid = begin;
  int32_t midPos = num>>1;

  while(begin < end) { 
    mid = begin + midPos;
    if(*mid < value) {
      begin = mid + 1;
    } else if(*mid > value) {
      end = mid;
    } else {
      return mid - array;
    }
    midPos = (end - begin)>>1;
  }

  midPos = mid - array;
  if(*mid < value) {
    midPos++;
    if(midPos >= num) {
      midPos--;
    }
  }
  return midPos;
}

// 获取字典中最大冲突数
int32_t idx_get_max_collision(idx_dict_t *pdict, int32_t& collisionNum)
{
  collisionNum = 0;

  int32_t maxCollision = 0;
  int32_t collision = 0; 

  // create hash struct
  unsigned int  *hashtab = pdict->hashtab;

  for(uint32_t i = 0; i < pdict->hashsize; i++) {
    if(hashtab[i] == INODE_COMMON_NULL) continue;

    collision = 0;
    idict_node_t* block = pdict->block + hashtab[i];
    while(block->next != INODE_COMMON_NULL) {
      collision++;
      block = pdict->block + block->next;
    }
    if(collision > maxCollision) {
      maxCollision = collision;
    }
    if(collision > 0) {
      collisionNum++;
    }
  }
  return maxCollision + 1;
}

/*
 * func : 检查并释放可以回收的旧block空间
 * args : pdict 字典
 */
void    idx_dict_free_oldblock(idx_dict_t *pdict)
{
    if (unlikely(NULL == pdict)) {
        return;
    }

    if (unlikely(NULL != pdict->old_block && (time(NULL) >= pdict->canFreeTime))){
        free(pdict->old_block);
        pdict->old_block   = NULL;
        pdict->canFreeTime = 0;
    }
}

