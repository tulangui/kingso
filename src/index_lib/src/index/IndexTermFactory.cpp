
#include "IndexTermFactory.h"

INDEX_LIB_BEGIN


IndexTermFactory::IndexTermFactory(uint32_t maxOccNum)
{
  _maxOccNum = maxOccNum;
}
IndexTermFactory::~IndexTermFactory()
{

}


IndexTerm * IndexTermFactory::make( MemPool        * pMemPool,
                                    uint32_t         zipFlag,
                                    const uint64_t * bitmap,
                                    const char     * invertList,
                                    uint32_t         docCount,
                                    uint32_t         maxDocNum,
                                    uint32_t         maxDocId,
                                    uint32_t         not_orig_flag)
{
  // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩 3: p4delta压缩
  if (3 == zipFlag) { // p4delta压缩
    switch (_maxOccNum) {
      case 0:
        {// null occ
          IndexP4DNullOccTerm * pTerm = NEW(pMemPool, IndexP4DNullOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      case 1:
        { // one occ
          IndexP4DOneOccTerm * pTerm = NEW(pMemPool, IndexP4DOneOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      default:
        { // mul occ
          IndexP4DMulOccTerm * pTerm = NEW(pMemPool, IndexP4DMulOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
    }
  } else if (2 == zipFlag) { // 压缩
    switch (_maxOccNum) {
      case 0:
        {// null occ
          IndexZipNullOccTerm * pTerm = NEW(pMemPool, IndexZipNullOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      case 1:
        { // one occ
          IndexZipOneOccTerm * pTerm = NEW(pMemPool, IndexZipOneOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      default:
        { // mul occ
          IndexZipMulOccTerm * pTerm = NEW(pMemPool, IndexZipMulOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(bitmap);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, pMemPool, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
    }
  } else if (1 == zipFlag) { // 非压缩
    switch (_maxOccNum) {
      case 0:
        {// null occ
          IndexUnZipNullOccTermOpt* pTerm = NEW(pMemPool, IndexUnZipNullOccTermOpt);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(NULL);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      case 1:
        {// one occ
          IndexUnZipOneOccTermOpt* pTerm = NEW(pMemPool, IndexUnZipOneOccTermOpt);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(NULL);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
      default:
        { // mul occ
          IndexUnZipMulOccTerm* pTerm = NEW(pMemPool, IndexUnZipMulOccTerm);
          if (NULL == pTerm) {
            return NULL;
          }
          pTerm->setBitMap(NULL);
          pTerm->setInvertList(invertList);
          if(pTerm->init(docCount, maxDocNum, _maxOccNum, pMemPool, not_orig_flag) < 0) {
            return NEW(pMemPool, IndexTerm);
          }
          return pTerm;
        }
    }
  } else { // 只有bitmap索引
    IndexBitmapTerm* pTerm = NEW(pMemPool, IndexBitmapTerm);
    if(NULL == pTerm) {
      return NULL;
    }
    pTerm->setBitMap(bitmap);
    pTerm->setInvertList(NULL);
    if(pTerm->init(docCount, maxDocNum, maxDocId, not_orig_flag) < 0) {
      return NEW(pMemPool, IndexTerm);
    }
    return pTerm;
  }

  return NULL;
}

INDEX_LIB_END


