#include "IndexFieldReader.h"

#include "Common.h"
#include "idx_dict.h"
#include "IndexTermActualize.h"
#include "MMapFile.h"
#include "util/errcode.h"

INDEX_LIB_BEGIN

IndexFieldReader::IndexFieldReader(int32_t maxOccNum,
          int32_t maxDocNum) : _indexTermFactory(maxOccNum)
{
  _invertedDict = NULL;               // 1级倒排索引hash表, idx_dict_t
  _bitmapDict = NULL;                 // 1级bitmap索引hash表, idx_dict_t

  _invertedIdx2Num = 0;
  _bitmapIdx2 = NULL;

  _maxOccNum = maxOccNum;             // 最大occ数量
  _maxDocNum = maxDocNum;             // 最大doc数量
  _bitMapLen = (maxDocNum + 63)>>6;

  _travelPos  = 0;
  _travelDict = NULL;
}

IndexFieldReader::~IndexFieldReader()
{
   store::CMMapFile* idx2 = (store::CMMapFile*)_bitmapIdx2;
   if (idx2) delete idx2;

   for(uint32_t i = 0; i < _invertedIdx2Num; i++) {
     idx2 = (store::CMMapFile*)_invertedIdx2[i];
     if(idx2) delete idx2;
   }
}

int IndexFieldReader::open(const char * path, const char * fieldName, bool bPreLoad)
{
    if (NULL == path || NULL == fieldName) {
        return KS_EINVAL;
    }
    int pathLen = strlen(path);
    int fieldLen = strlen(fieldName);
    if (pathLen <= 0 || pathLen >= PATH_MAX || fieldLen <= 0 || fieldLen >=PATH_MAX) {
        return KS_EINVAL;
    }
    strcpy(_path, path);
    strcpy(_fieldName, fieldName);

    char             filename[PATH_MAX];
    struct stat      st;

    // load inverted index
    snprintf(filename, sizeof(filename), "%s.%s", _fieldName, INVERTED_IDX1_INFIX);
    _invertedDict = idx_dict_load(path, filename);
    if (NULL == _invertedDict) {
        TERR("load idx1 dict from %s/%s failed,", path, filename);
        return KS_EFAILED;
    }

    store::CMMapFile* mapFile = NULL;
    for (uint32_t i=0; i < MAX_IDX2_FD; i++) {
        snprintf(filename, sizeof(filename), "%s/%s.%s.%u", path,
                _fieldName, INVERTED_IDX2_INFIX, i);
        if (-1 == stat(filename, &st) || 0 == st.st_size) {
            break;
        }

        mapFile = new (std::nothrow) store::CMMapFile;
        if (NULL == mapFile) {
            return KS_ENOMEM;
        }
        _invertedIdx2Num++;
        if (false == mapFile->open(filename)) {
            TLOG("open %s failed", filename);
            return KS_EIO;
        }
        if(bPreLoad) {
            mapFile->preLoad(filename);
        }
        _invertedIdx2[i] = mapFile;
    }

    // load bitmap index
    snprintf(filename, sizeof(filename), "%s/%s.%s", path,
            _fieldName, BITMAP_IDX2_INFIX);
    if (-1 == stat(filename, &st)) {
        TLOG("no bitmap index of field %s", _fieldName);
        return KS_EFAILED;
    }
    if(st.st_size > 0) {
        mapFile = new (std::nothrow) store::CMMapFile;
        if (NULL == mapFile) {
            return KS_ENOMEM;
        }
        if (false == mapFile->open(filename)) {
            TLOG("open %s failed", filename);
            return KS_EIO;
        }
        _bitmapIdx2 = mapFile;
        if(bPreLoad) {
            mapFile->preLoad(filename);
        }
    }

    snprintf(filename, sizeof(filename), "%s.%s", _fieldName, BITMAP_IDX1_INFIX);
    _bitmapDict = idx_dict_load(path, filename);
    if (NULL == _bitmapDict) {
        TERR("load idx1 dict from %s/%s failed,", path, filename);
        return KS_EFAILED;
    }

    return 0;
}

void IndexFieldReader::close()
{
  if (_invertedDict) {
    idx_dict_free((idx_dict_t*)_invertedDict);
    _invertedDict = NULL;
  }
  if (_bitmapDict) {
    idx_dict_free((idx_dict_t*)_bitmapDict);
    _bitmapDict = NULL;
  }

  for (uint32_t i = 0; i < _invertedIdx2Num; i++) {
    store::CMMapFile* mmapFile = (store::CMMapFile*)_invertedIdx2[i];
    if (NULL == mmapFile) {
      continue;
    }
    mmapFile->close();
  }

  store::CMMapFile* mmapFile= (store::CMMapFile*)_bitmapIdx2;
  if (mmapFile) {
    mmapFile->close();
  }
}

IndexTerm * IndexFieldReader::getTerm(MemPool * pMemPool, uint64_t termSign)
{
  full_idx1_unit_t* pIdxNode = (full_idx1_unit_t*)idx_dict_find(_invertedDict, termSign);
  bitmap_idx1_unit_t* pBitmapNode = (bitmap_idx1_unit_t*)idx_dict_find(_bitmapDict, termSign);
  if (NULL == pIdxNode && NULL == pBitmapNode) {
    return NEW(pMemPool, IndexTerm);
  }

  uint64_t* bitmap = NULL;
  if(pBitmapNode) {
    // 预读bitmap索引
    // _bitmapIdx2->readahead(pBitmapNode->pos, _bitMapLen<<3);
    bitmap = (uint64_t*)_bitmapIdx2->offset2Addr(pBitmapNode->pos);
  }

  char* invertList = NULL;
  if(pIdxNode) {
    // 预读倒排索引
    store::CMMapFile* idx2 = _invertedIdx2[pIdxNode->file_num];
    //  idx2->readahead(pIdxNode->pos, pIdxNode->len);
    invertList = idx2->offset2Addr(pIdxNode->pos);

     // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩 3: p4delta压缩
    if(pIdxNode->zip_flag == 2) {
      return _indexTermFactory.make(pMemPool, 3,
          bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
    } else if(pIdxNode->zip_flag == 1) {
      return _indexTermFactory.make(pMemPool, 2,
          bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
    } else {
      return _indexTermFactory.make(pMemPool, 1,
          bitmap, invertList, pIdxNode->doc_count, _maxDocNum, 0 , pIdxNode->not_orig_flag);
    }
  }

  // 只有bitmap索引
  return _indexTermFactory.make(pMemPool, 0, bitmap, invertList, pBitmapNode->doc_count,
                                  _maxDocNum, pBitmapNode->max_docId, pBitmapNode->not_orig_flag);

}

/**
 * * 输出index部分的信息，包括有哪些字段、文档数量、term数量等等
 * *
 * * @param info   输出的index信息内存
 * * @param maxlen 输出信息的最大长度
 * *
 * * @return < 0 失败， >=0 信息长度
 * */
int IndexFieldReader::getIndexInfo(char* info, int maxlen)
{
    int baklen = maxlen;
    int32_t bitmapBlockPos = 0;
    int32_t bitmapHashSize = 0;
    if(_bitmapDict) {
        bitmapBlockPos = _bitmapDict->block_pos;
        bitmapHashSize = _bitmapDict->hashsize;
    }

    unsigned int travelPos = -1;
    idx_dict_t* pdict =  _invertedDict;

    uint32_t maxPos = 1<<13;
    uint32_t statNum[maxPos];
    memset(statNum, 0, sizeof(uint32_t) * maxPos);

    // 获取一级索引的第一个节点
    full_idx1_unit_t* pIdxNode = (full_idx1_unit_t*)idx_dict_first(pdict, &travelPos);
    while(pIdxNode) {
        uint32_t pos = pIdxNode->doc_count / 10000;
        if(pos >= maxPos) pos = maxPos - 1;
        statNum[pos]++;

        pIdxNode = (full_idx1_unit_t*)idx_dict_next(pdict, &travelPos);
    }

    char* p = info;
    int ret = snprintf(p, maxlen, "%s termNum=%d, hash=%d, bitmapNum=%d, hash=%d, ",
            _fieldName, _invertedDict->block_pos, _invertedDict->hashsize, bitmapBlockPos, bitmapHashSize);
    if(ret >= maxlen) {
        return -1;
    }
    maxlen -= ret;
    p += ret;

    for(uint32_t i = 0; i < maxPos; i++) {
        if(statNum[i] <= 0) continue;
        ret = snprintf(p, maxlen, "%u=%u, ", i * 10000, statNum[i]);
        if(ret >= maxlen) return -1;
        maxlen -= ret;
        p += ret;
    }
    ret = snprintf(p, maxlen, "\n");
    if(ret >= maxlen) return -1;
    maxlen -= ret;
    p += ret;

    return baklen - maxlen;
}


/**
 * 遍历当前字段的所有term
 *
 * @param termSign    返回的当前term的签名
 *
 * @return 0：成功;   -1：失败
 */
int IndexFieldReader::getFirstTerm( uint64_t &termSign )
{
    idict_node_t * pNode = NULL;

    _travelDict = _invertedDict;

    pNode = idx_dict_first( _travelDict, &_travelPos );

    if ( pNode == NULL )
    {
        _travelDict = _bitmapDict;

        pNode = idx_dict_first( _travelDict, &_travelPos );

        if ( pNode == NULL ) return -1;          // 没有找到任何term
    }

    termSign = pNode->sign;
    return 0;
}


int IndexFieldReader::getNextTerm( uint64_t &termSign )
{
    idict_node_t * pNode = NULL;

    pNode = idx_dict_next( _travelDict, &_travelPos );

    if ( _travelDict == _invertedDict )
    {
        if ( pNode != NULL)
        {
            termSign = pNode->sign;
            return 0;
        }

        _travelDict = _bitmapDict;
        pNode = idx_dict_first( _travelDict, &_travelPos );
    }

    while ( pNode != NULL && idx_dict_find( _invertedDict, pNode->sign ) != NULL )
    {
        pNode = idx_dict_next( _travelDict, &_travelPos );
    }

    if ( pNode == NULL )  return -1;

    termSign = pNode->sign;
    return 0;
}

INDEX_LIB_END

