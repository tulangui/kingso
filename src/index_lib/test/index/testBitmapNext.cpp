#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "test.h"
#include "testBitmapNext.h"
#include "util/idx_sign.h"
#include "util/MemPool.h"
#include "util/Log.h"
#include "index_struct.h"
#include "index_lib/IndexTerm.h"
#include "IndexTermActualize.h"


namespace index_lib
{

testBitmapNext::testBitmapNext() : _cReader(1, 52000000)
{
  _nodeNum = 0;
  _pBitmapNode = NULL;
  _pInvertNode = NULL;
  _ppDocList = NULL;
}

testBitmapNext::~testBitmapNext()
{
  if(_pBitmapNode) delete[] _pBitmapNode;
  if(_pInvertNode) delete[] _pInvertNode;
  
  if(_ppDocList) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      if(_ppDocList[i]) delete[] _ppDocList[i];
    }
    delete[] _ppDocList;
  }
  _cReader.close();
}

int testBitmapNext::init(const char* idxPath)
{
  if(_cReader.open(idxPath, "title", false) < 0) {
    printf("open indexlib %s error\n", idxPath);
    return -1;
  }

  _nodeNum = _cReader._bitmapDict->block_pos;
  _pBitmapNode = new idict_node_t[_nodeNum];
  _pInvertNode = new idict_node_t[_nodeNum];
  _ppDocList = new uint32_t*[_nodeNum];
  
  MemPool cPool;
  uint32_t docId1, docId2;
  memset(_ppDocList, 0, sizeof(uint32_t*) * _nodeNum);

  IndexBitmapTerm term1;
  IndexZipOneOccTerm term2;
  IndexZipMulOccTerm term3;

  _nodeNum = 0;
  unsigned int pos = 0;
  idict_node_t* pNode = idx_dict_first(_cReader._bitmapDict, &pos);
  while(pNode) {
    _pBitmapNode[_nodeNum] = *pNode;
    pNode = idx_dict_find(_cReader._invertedDict, pNode->sign);
    if(NULL == pNode) {
      printf("error, not find %lu\n", _pBitmapNode[_nodeNum].sign);
      return -1;
    }
    _pInvertNode[_nodeNum] = *pNode;
  
    full_idx1_unit_t* pIdxNode = (full_idx1_unit_t*)(_pInvertNode+_nodeNum);
    bitmap_idx1_unit_t* pBitmapNode = (bitmap_idx1_unit_t*)(_pBitmapNode+_nodeNum);

    term1.setBitMap((uint64_t*)_cReader._bitmapIdx2->offset2Addr(pBitmapNode->pos));
    term1.init(pBitmapNode->doc_count, _cReader._maxDocNum, pBitmapNode->max_docId);
    term2.setInvertList(_cReader._invertedIdx2[pIdxNode->file_num]->offset2Addr(pIdxNode->pos));
    term2.init(pIdxNode->doc_count, _cReader._maxDocNum, _cReader._maxOccNum);

    _ppDocList[_nodeNum] = new uint32_t[pIdxNode->doc_count+1];
    int32_t docNum = 0;
    while((docId1 = term1.next()) != INVALID_DOCID) {
      docId2 = term2.next();
      if(docId1 != docId2) {
        printf("term %lu 1=%u, 2=%u, error\n", pNode->sign, docId1, docId2);
        return -1;
      }
      _ppDocList[_nodeNum][docNum++] = docId1;
    }
    if(term2.next() != INVALID_DOCID) {
      printf("term %lu last error\n", pNode->sign);
      return -1;
    }
    _ppDocList[_nodeNum][docNum] = docId1;

    _nodeNum++;
    pNode = idx_dict_next(_cReader._bitmapDict, &pos);
  }

  return _nodeNum;
}

int testBitmapNext::bitmapNext()
{
  IndexBitmapTerm cTerm;
  uint32_t docId, docNum;
  bitmap_idx1_unit_t* pBitmapNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pBitmapNode = (bitmap_idx1_unit_t*)(_pBitmapNode + i);
      cTerm.setBitMap((uint64_t*)_cReader._bitmapIdx2->offset2Addr(pBitmapNode->pos));
      cTerm.init(pBitmapNode->doc_count, _cReader._maxDocNum, pBitmapNode->max_docId);

      docNum = 0;
      while((docId = cTerm.next()) != INVALID_DOCID) {
        docNum++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::inverteNext()
{
  MemPool cPool;
  IndexZipOneOccTerm cTerm;
  uint32_t docId, docNum;
  full_idx1_unit_t* pIdxNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pIdxNode = (full_idx1_unit_t*)(_pInvertNode + i);
      cTerm.setInvertList(_cReader._invertedIdx2[pIdxNode->file_num]->offset2Addr(pIdxNode->pos));
      cTerm.init(pIdxNode->doc_count, _cReader._maxDocNum, _cReader._maxOccNum);

      docNum = 0;
      while((docId = cTerm.next()) != INVALID_DOCID) {
        docNum++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::bitmapSeek()
{
  IndexBitmapTerm cTerm;
  uint32_t docId, docNum;
  bitmap_idx1_unit_t* pBitmapNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pBitmapNode = (bitmap_idx1_unit_t*)(_pBitmapNode + i);
      cTerm.setBitMap((uint64_t*)_cReader._bitmapIdx2->offset2Addr(pBitmapNode->pos));
      cTerm.init(pBitmapNode->doc_count, _cReader._maxDocNum, pBitmapNode->max_docId);

      docNum = 0;
      docId = 0xffffffff;
      while((docId = cTerm.seek(++docId)) != INVALID_DOCID) {
        docNum++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::bitmapBestSeek()
{
  IndexBitmapTerm cTerm;
  uint32_t docId, docNum;
  bitmap_idx1_unit_t* pBitmapNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pBitmapNode = (bitmap_idx1_unit_t*)(_pBitmapNode + i);
      cTerm.setBitMap((uint64_t*)_cReader._bitmapIdx2->offset2Addr(pBitmapNode->pos));
      cTerm.init(pBitmapNode->doc_count, _cReader._maxDocNum, pBitmapNode->max_docId);

      docNum = 0;
      docId = _ppDocList[i][docNum];
      while((docId = cTerm.seek(docId)) != INVALID_DOCID) {
        docId = _ppDocList[i][++docNum];
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::bitmapNormalSeek(int32_t step)
{
  IndexBitmapTerm cTerm;
  uint32_t docId, docNum;
  bitmap_idx1_unit_t* pBitmapNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pBitmapNode = (bitmap_idx1_unit_t*)(_pBitmapNode + i);
      cTerm.setBitMap((uint64_t*)_cReader._bitmapIdx2->offset2Addr(pBitmapNode->pos));
      cTerm.init(pBitmapNode->doc_count, _cReader._maxDocNum, pBitmapNode->max_docId);

      docNum = 0;
      docId = _ppDocList[i][docNum];
      while((docId = cTerm.seek(docId)) != INVALID_DOCID) {
        docId = _ppDocList[i][++docNum];
        if(docNum % step == 0) docId++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::inverteNormalSeek(int32_t step)
{
  MemPool cPool;
  IndexZipOneOccTerm cTerm;
  uint32_t docId, docNum;
  full_idx1_unit_t* pIdxNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pIdxNode = (full_idx1_unit_t*)(_pInvertNode + i);
      cTerm.setInvertList(_cReader._invertedIdx2[pIdxNode->file_num]->offset2Addr(pIdxNode->pos));
      cTerm.init(pIdxNode->doc_count, _cReader._maxDocNum, _cReader._maxOccNum);

      docNum = 0;
      docId = _ppDocList[i][docNum];
      while((docId = cTerm.seek(docId)) != INVALID_DOCID) {
        docId = _ppDocList[i][++docNum];
        if(docNum % step == 0) docId++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

int testBitmapNext::inverteSeek()
{
  MemPool cPool;
  IndexZipOneOccTerm cTerm;
  uint32_t docId, docNum;
  full_idx1_unit_t* pIdxNode = NULL;

  time_t begin = time(NULL);

  for(int k = 0; k < RUN_NUM; k++) {
    for(int32_t i = 0; i < _nodeNum; i++) {
      pIdxNode = (full_idx1_unit_t*)(_pInvertNode + i);
      cTerm.setInvertList(_cReader._invertedIdx2[pIdxNode->file_num]->offset2Addr(pIdxNode->pos));
      cTerm.init(pIdxNode->doc_count, _cReader._maxDocNum, _cReader._maxOccNum);

      docNum = 0;
      docId = 0xffffffff;
      while((docId = cTerm.seek(++docId)) != INVALID_DOCID) {
        docNum++;
      }
    }
  }

  begin = time(NULL) - begin;
  return begin;
}

}

const char* idxPath = "/home/nfs/xiaojie.lgx/index_title_oneocc";
int main(int argc, char** argv)
{
  index_lib::testBitmapNext cTest;

  if(cTest.init(idxPath) < 0) {
    return -1;
  }
  printf("init success\n");

  int time1 = cTest.inverteNext();
  int time2 = cTest.bitmapNext();
  printf("next invert use=%d, bitmap use=%d\n", time1, time2);
 
  for(int32_t i = 1; i < 4; i++) {
    int time1 = cTest.inverteNormalSeek(i);
    int time2 = cTest.bitmapNormalSeek(i);
    printf("normal seek invert use=%d, bitmap use=%d\n", time1, time2);
  }

  time1 = cTest.inverteSeek();
  time2 = cTest.bitmapSeek();
  int time3 = cTest.bitmapBestSeek();
  printf("seek invert use=%d, bitmap use=(best:%d,%d)\n", time1, time3, time2);
  
  return 0;
}


