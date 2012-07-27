/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexFieldSyncManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef KINGSO_INDEX_INDEXFIELDSYNCMANAGER_H_
#define KINGSO_INDEX_INDEXFIELDSYNCMANAGER_H_

#include "Common.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <vector>

#include "idx_dict.h"

using namespace std;

INDEX_LIB_BEGIN

struct IndexFieldSyncInfo
{
    public:
        char * pMemAddr;         // value address in memory
        size_t len;              // value length
        size_t pos;              // value pos in file

    public:
        explicit IndexFieldSyncInfo():pMemAddr(NULL),len(0),pos(0){ }
};

/**
 * 该类用于持久化增量一级索引(哈希表)结构，内部部分方法与哈希表存储结构紧密相关
 * 如果idx_dict结构变更，则内部相关方法需要同步变更
 */
class IndexFieldSyncManager
{ 
    public:
        IndexFieldSyncManager(idx_dict_t * dict, const char * filePath, const char * fileName, uint32_t init = 20);
        ~IndexFieldSyncManager();

        void syncToDisk();
        void reset();

        /**
         * 下面的put系列方法与idx_dict内部结构相关，如idx_dict内部结构变更需要同步修改
         */
        int putIdxBlockPos ();
        int putIdxHashSize ();
        int putIdxHashTable();
        int putIdxAllBlock ();
        int putIdxDictNode (idict_node_t * node);
        int putIdxHashNode (uint32_t pos);

    protected:
        bool expand(uint32_t step = 10);
        int  putSyncInfo(char * pMemAddr, size_t len, size_t pos);

    private:
        vector <IndexFieldSyncInfo *>   _syncInfoVector;   // SyncInfo
        uint32_t                        _usedNum;          // SyncInfo vector中的个数
        int                             _fd;               // 持久化对应的fd

        idx_dict_t                    * _pDict;            //存储对应的哈希表
};


INDEX_LIB_END


#endif //INDEX_INDEXFIELDSYNCMANAGER_H_
