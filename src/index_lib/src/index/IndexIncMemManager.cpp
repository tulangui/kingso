#include "IndexIncMemManager.h" 

INDEX_LIB_BEGIN

IndexIncMemManager::IndexIncMemManager(const char* fileNameTemplet, index_mem::ShareMemPool& pool) : _pool(pool)
{
    _nMapFileNum = 0; // MMapFile 当前使用数量 
    strcpy(_fileNameTemplet, fileNameTemplet);

    _nOffBegin = 0;   // 可分配内存的起始位置
    _nOffEnd = 0;     // 可分配内存的结束位置

    _nAllocSize = 0;   // 分配出去的内存总量
    _nDirtySize = 0;   // 释放掉的内存，暂时都没有回收使用
    _nChipSize = 0;    // 碎片大小，不满足分配条件导致
    _nMakeNum = 0;
}

IndexIncMemManager::~IndexIncMemManager()
{
}

int32_t IndexIncMemManager::makeNewPage(int32_t pageSize, uint32_t& offset)
{
    offset = _pool.alloc(pageSize);
    if(offset == 0xFFFFFFFF) {
        return -1;
    }
    int32_t true_size = size(offset);
    pageSize = true_size;
/*
    if(_nOffBegin + pageSize > _nOffEnd) {
        _nChipSize += _nOffEnd - _nOffBegin;
        if(pageSize > MAX_BLOCK_SIZE || makeNewBlock() < 0) {
            return -1;
        }
    }
    offset = _nOffBegin;
    _nOffBegin += pageSize;
*/
    _nAllocSize += pageSize;
    _nMakeNum++;
/*
    if(_nMakeNum % 1000 == 0) {
        TLOG("Inc size=%lu, alloc=%lu, dirty=%lu, chip=%lu, file=%s",
                _nMakeNum, _nAllocSize, _nDirtySize, _nChipSize, _fileNameTemplet);
    }
*/
    return 0;
}

int32_t IndexIncMemManager::freeOldPage(int32_t pageSize, uint32_t offset)
{
    _pool.dealloc(offset);
    _nDirtySize += pageSize;

    return 0;
}

int32_t IndexIncMemManager::makeNewBlock()
{
  if(_nMapFileNum >= MAX_BLOCK_NUM) {
    return -1;
  }
  char fileName[PATH_MAX];
  snprintf(fileName, PATH_MAX, "%s.%d", _fileNameTemplet, _nMapFileNum);
  if(false == _cMapFileArray[_nMapFileNum].open(fileName, FILE_OPEN_WRITE, MAX_BLOCK_SIZE)) {
    return -1;
  }
  _nOffBegin = MAX_BLOCK_SIZE * _nMapFileNum;
  _nOffEnd = _nOffBegin + MAX_BLOCK_SIZE;
  _nMapFileNum++;

  return 0;
}

int32_t IndexIncMemManager::close()
{
  for(int32_t i = 0; i < _nMapFileNum; i++) {
    _cMapFileArray[i].close();
  }
  return 0;
}

INDEX_LIB_END
 

