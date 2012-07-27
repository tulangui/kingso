#include "MetaDataManager.h"

namespace index_mem
{


static const char * DUMPFILE = "mempool.meta";

int
MetaDataManager::create(const char * path, int maxLevelNum, int qsize, int maxbnum)
{   
    size_t size = sizeof(int) * 3
                 + sizeof(struct MemPoolMeta)
                 + maxLevelNum * sizeof(struct FixMemMeta)
                 + maxLevelNum * sizeof(u_int)
                 + sizeof(struct queue_t)
                 + qsize * sizeof(struct delay_t)
                 + sizeof(struct MemSpaceMeta)
                 + maxbnum * sizeof(int);

    if (_shmSeg.createShareMem(path, 0, size) != 0) {
        return -1;
    }

    void * addr = _shmSeg.addr();
    memset(addr, 0, size);

    ((int *)addr)[0] = maxLevelNum;
    ((int *)addr)[1] = qsize;
    ((int *)addr)[2] = maxbnum;

    _addr        = addr;
    _size        = size;
    _maxLevelNum = maxLevelNum;
    _queueSize   = qsize;
    _maxbnum     = maxbnum;

    return 0;
}

int
MetaDataManager::loadMem(const char * path)
{
    if (_shmSeg.loadShareMem(path, 0) != 0) {
        return -1;
    }

    void * addr     = _shmSeg.addr();
    int maxLevelNum = ((int *)addr)[0];
    int qsize       = ((int *)addr)[1];
    int maxbnum     = ((int *)addr)[2];

    size_t size = sizeof(int) * 3
                 + sizeof(struct MemPoolMeta)
                 + maxLevelNum * sizeof(struct FixMemMeta)
                 + maxLevelNum * sizeof(u_int)
                 + sizeof(struct queue_t)
                 + qsize * sizeof(struct delay_t)
                 + sizeof(struct MemSpaceMeta)
                 + maxbnum * sizeof(int);

    if (_shmSeg.size() < size) {
        return -1;
    }

    _addr        = addr;
    _size        = size;
    _maxLevelNum = maxLevelNum;
    _queueSize   = qsize;
    _maxbnum     = maxbnum;

    return 0;
}

int
MetaDataManager::loadData(const char * path)
{
    char fileName[512];
    snprintf(fileName, 512, "%s/%s", path, DUMPFILE);

    FILE * fp = NULL;
    fp = fopen(fileName, "r");
    if (fp == NULL) {
        return -1;
    }

    int maxlevel = 0;
    int qsize    = 0;
    int maxbnum  = 0;

    if (fread(&maxlevel, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    if (fread(&qsize, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    if (fread(&maxbnum, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return -1;
    }

    int ret = create(path, maxlevel, qsize, maxbnum);
    if (ret != 0) {
        fclose(fp);
        return -1;
    }

    size_t size     = sizeof(struct MemPoolMeta)
                      + maxlevel * sizeof(struct FixMemMeta)
                      + maxlevel * sizeof(u_int)
                      + sizeof(struct queue_t)
                      + qsize * sizeof(struct delay_t)
                      + sizeof(struct MemSpaceMeta)
                      + maxbnum * sizeof(int);

    void * targetAddr = (char *)_addr + sizeof(int) * 3;
    if (fread(targetAddr, 1, size, fp) != size) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int
MetaDataManager::dump(const char * path)
{
    if (_addr == NULL || _size == 0) {
        return -1;
    }

    char fileName[512];
    snprintf(fileName, 512, "%s/%s", path, DUMPFILE);

    FILE * fp = NULL;
    fp = fopen(fileName, "w");
    if (fp == NULL) {
        return -1;
    }

    if (fwrite(_addr, 1, _size, fp) != _size) {
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

int
MetaDataManager::detach()
{
    return _shmSeg.detachShareMem();
}

int
MetaDataManager::destroy()
{
    return _shmSeg.freeShareMem();
}


struct MemPoolMeta *
MetaDataManager::getMemPoolMeta()
{
    size_t pos = sizeof(int) * 3;

    if (_addr == NULL) {
        return NULL;
    }
    return (struct MemPoolMeta *)((char *)_addr + pos);
}

struct FixMemMeta *
MetaDataManager::getFixMemMeta()
{
    size_t pos = sizeof(int) * 3 + sizeof(struct MemPoolMeta);

    if (_addr == NULL) {
        return NULL;
    }
    return (struct FixMemMeta *)((char *)_addr + pos);
}

u_int *
MetaDataManager::getFreeListArray()
{
    size_t pos = sizeof(int) * 3
                 + sizeof(struct MemPoolMeta)
                 + _maxLevelNum * sizeof(struct FixMemMeta);

    if (_addr == NULL) {
        return NULL;
    }
    return (u_int *)((char *)_addr + pos);
}

struct queue_t *
MetaDataManager::getQueueAddress()
{
    size_t pos = sizeof(int) * 3
                 + sizeof(struct MemPoolMeta)
                 + _maxLevelNum * sizeof(struct FixMemMeta)
                 + _maxLevelNum * sizeof(u_int);

    if (_addr == NULL) {
        return NULL;
    }
    return (struct queue_t *)((char *)_addr + pos);
}

struct MemSpaceMeta *
MetaDataManager::getMemSpaceMeta()
{
    size_t pos = sizeof(int) * 3
                 + sizeof(struct MemPoolMeta)
                 + _maxLevelNum * sizeof(struct FixMemMeta)
                 + _maxLevelNum * sizeof(u_int)
                 + sizeof(struct queue_t)
                 + _queueSize * sizeof(struct delay_t);

    if (_addr == NULL) {
        return NULL;
    }
    return (struct MemSpaceMeta *)((char *)_addr + pos);
}

size_t
MetaDataManager::getMetaSpaceSize()
{
    return _size;
}

void *
MetaDataManager::getMetaBaseAddr()
{
    return _addr;
}

int
MetaDataManager::getMaxLevelNum()
{
    return _maxLevelNum;
}

int
MetaDataManager::getQueueSize()
{
    return _queueSize;
}

int
MetaDataManager::getMaxBlockNum()
{
    return _maxbnum;
}

}
