/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexIncMemManager.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_INDEXINCMEMMANAGER_H_
#define INDEX_INDEXINCMEMMANAGER_H_

#include <limits.h>

#include "Common.h"
#include "MMapFile.h"
#include "mempool/ShareMemPool.h"
#include "util/HashMap.hpp"

INDEX_LIB_BEGIN

#define MAX_BLOCK_NUM  (1<<5)             // 最多分配块儿数
#define MAX_BLOCK_SIZE (1<<27)            // 每块儿内存大小
#define UNVALID_OFFSET (0xFFFFFFFF)       // 最大支持4G
#define GET_BLOCK_NO(off) (off>>27)       // 获取偏移对应的块儿号
#define GET_BLOCK_OFF(off) ((off<<5)>>5)  // 获取块儿内偏移

class IndexIncMemManager
{ 
public:
    IndexIncMemManager(const char* fileNameTemplet, index_mem::ShareMemPool& pool);
    ~IndexIncMemManager();

    int32_t makeNewPage(int32_t pageSize, uint32_t& offset);
    int32_t freeOldPage(int32_t pageSize, uint32_t offset);

    inline char *offset2Addr(uint32_t nOffset = 0) {
        char* pp = (char*)_pool.addr(nOffset);
        return pp;

        int32_t no = GET_BLOCK_NO(nOffset);
        int32_t off = GET_BLOCK_OFF(nOffset);
        return _cMapFileArray[no].offset2Addr(off);
    }

    inline int size(uint32_t key) {
        return _pool.size(key) * sizeof(int);
    }

    int32_t close();
private:
    int32_t makeNewBlock();

private:
    
    index_mem::ShareMemPool& _pool;

    uint64_t _nMakeNum;
    uint64_t _nAllocSize;   // 分配出去的内存总量
    uint64_t _nDirtySize;   // 释放掉的内存，暂时都没有回收使用  
    uint64_t _nChipSize;    // 碎片大小，不满足分配条件导致

    uint32_t _nOffBegin;    // 可分配内存的起始位置
    uint32_t _nOffEnd;      // 可分配内存的结束位置

    int32_t _nMapFileNum;   // MMapFile 当前使用数量
    char _fileNameTemplet[PATH_MAX];                // 对应磁盘文件名模板(实际文件名会加后缀)
    store::CMMapFile _cMapFileArray[MAX_BLOCK_NUM]; // MMapFile 句柄数组
};

INDEX_LIB_END

 
#endif //INDEX_INDEXINCMEMMANAGER_H_
