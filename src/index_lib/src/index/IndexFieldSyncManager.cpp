#include "IndexFieldSyncManager.h"
#include "index_struct.h"

INDEX_LIB_BEGIN

IndexFieldSyncManager::IndexFieldSyncManager(idx_dict_t * dict, const char * filePath, const char * fileName, uint32_t init)
{
    int fd = -1;
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", filePath, fileName);
    fd = ::open(path, O_CREAT|O_RDWR, 0644);
    if (fd < 0) {
        TERR("IndexFieldSyncManager open file:%s failed!", path);
    }
    _fd      = fd;
    _pDict   = dict;
    _usedNum = 0;
    IndexFieldSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < init; i++) {
        pInfo = new IndexFieldSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
    }
}

IndexFieldSyncManager::~IndexFieldSyncManager()
{
    for(uint32_t i = 0; i < _syncInfoVector.size(); i++) {
        if( _syncInfoVector[i] ) {
            delete _syncInfoVector[i];
        }
    }
    _syncInfoVector.clear();
    _usedNum = 0;
    _pDict   = NULL;

    if (_fd!= -1) {
        ::close(_fd);
        _fd = -1;
    }
}

int IndexFieldSyncManager::putSyncInfo(char * pMemAddr, size_t len, size_t pos)
{
    if (unlikely(pMemAddr == NULL || _fd == -1 )) {
        return -1;
    }

    IndexFieldSyncInfo * pInfo = NULL;
    if (unlikely(_usedNum >= _syncInfoVector.size())) {
        if (!expand()) {
            return -1;
        }
    }

    pInfo = _syncInfoVector[_usedNum];
    pInfo->pMemAddr = pMemAddr;
    pInfo->len      = len;
    pInfo->pos      = pos;

    ++_usedNum;
    return 0;
}

void IndexFieldSyncManager::syncToDisk()
{
    IndexFieldSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < _usedNum; i++) {
        pInfo = _syncInfoVector[i];
        if (lseek(_fd, pInfo->pos, SEEK_SET) == -1) {
            TERR("IndexFieldSyncManager lseek failed:%s", strerror(errno));
            continue;
        }

        if ((ssize_t)pInfo->len != write(_fd, pInfo->pMemAddr, pInfo->len)) {
            TERR("IndexFieldSyncManager write failed:%s", strerror(errno));
        }
    }
    _usedNum = 0;
}

void IndexFieldSyncManager::reset()
{
    _usedNum = 0;
}

bool IndexFieldSyncManager::expand(uint32_t step)
{
    IndexFieldSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < step; i++) {
        pInfo = new IndexFieldSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
        else {
            return false;
        }
    }
    return true;
}


int IndexFieldSyncManager::putIdxBlockPos ()
{
    return putSyncInfo((char *)&_pDict->block_pos, sizeof(unsigned int), sizeof(unsigned int));
}

int IndexFieldSyncManager::putIdxHashSize ()
{
    return putSyncInfo((char *)&_pDict->hashsize, sizeof(unsigned int), 0);
}

int IndexFieldSyncManager::putIdxHashTable()
{
    return putSyncInfo((char *)_pDict->hashtab, (sizeof(unsigned int) * _pDict->hashsize), (2 * sizeof(unsigned int)));
}

int IndexFieldSyncManager::putIdxAllBlock ()
{
    return putSyncInfo((char *)_pDict->block, (sizeof(idict_node_t) * _pDict->block_pos), (2 + _pDict->hashsize) * sizeof(unsigned int));
}

int IndexFieldSyncManager::putIdxDictNode (idict_node_t * node)
{
    size_t pos = (2 + _pDict->hashsize) * sizeof(unsigned int) + sizeof(idict_node_t) * (node - _pDict->block);
    return putSyncInfo((char *)node, sizeof(idict_node_t), pos);
}

int IndexFieldSyncManager::putIdxHashNode (uint32_t pos)
{
    size_t wpos = (2 + pos) * sizeof(unsigned int);
    return putSyncInfo((char *)(_pDict->hashtab + pos), sizeof(unsigned int), wpos);
}


INDEX_LIB_END
