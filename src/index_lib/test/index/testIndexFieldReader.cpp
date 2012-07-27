#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test.h"
#include "util/HashMap.hpp"
#include "testIndexFieldReader.h"
#include "IndexFieldBuilder.h"
#include "IndexFieldReader.h"


using namespace index_lib;

testIndexFieldReader::testIndexFieldReader()
{
  _termNum = 0;
  _pTermSign = NULL;

  _pDocNumArray = NULL; 
  _ppDocArray = NULL;
  createDocInfo(MAX_TERM_NUM, 0);
}

testIndexFieldReader::~testIndexFieldReader()
{
  if (_pTermSign) delete _pTermSign;
  if (_pDocNumArray) delete _pDocNumArray;
  if (_ppDocArray) {
    for(uint32_t i = 0; i < _termNum; i++) {
      if(_ppDocArray[i]) delete[] _ppDocArray[i];
    }
    delete _ppDocArray;
  }
}

void testIndexFieldReader::setUp()
{
  remove(ENCODEFILE);
  test::mkdir(NORMAL_PATH);
}

void testIndexFieldReader::tearDown()
{
  remove(ENCODEFILE);
  test::rmdir(NORMAL_PATH);
}

void testIndexFieldReader::testOpen()
{
  char message[1024];
  {// 正常返回
    index_lib::IndexFieldBuilder cBuilder(FIELDNAME, 1, _termNum, NULL);
    int ret = cBuilder.open(NORMAL_PATH);
    snprintf(message, sizeof(message), "ret=%d %s", ret, NORMAL_PATH);
    CPPUNIT_ASSERT_MESSAGE(message, 0 == ret);
  }

  {// 路径不存在， 或者没有权限
    index_lib::IndexFieldBuilder cBuilder(FIELDNAME, 1, _termNum, NULL);
    int ret = cBuilder.open("/xxxxxxxx/");
    snprintf(message, sizeof(message), "ret=%d %s", ret, NORMAL_PATH);
    CPPUNIT_ASSERT_MESSAGE(message, 0 > ret);
  }
}

void testIndexFieldReader::testNextDoc()
{
  CPPUNIT_ASSERT(buildAddTerm () == 0); 

  IndexFieldReader cReader(0, MAX_DOC_NUM);
  CPPUNIT_ASSERT(cReader.open(NORMAL_PATH, FIELDNAME) == 0);
  MemPool cMemPool;

  for(uint32_t i = 0; i < _termNum; i++) {
    IndexTerm* pTerm = cReader.getTerm(&cMemPool, _pTermSign[i]);
    sprintf(_message, "%d %lu", i, _pTermSign[i]);
    CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);

    uint32_t docNum = pTerm->getDocNum();
    sprintf(_message, "true=%d, docNum=%d", _pDocNumArray[i], docNum);
    CPPUNIT_ASSERT_MESSAGE(_message, docNum == _pDocNumArray[i]);
    for(uint32_t j = 0; j < docNum; j++) {
      uint32_t doc = pTerm->next();
      sprintf(_message, "err=(%d)%d, true=%d, doc=%d", i, j, _pDocNumArray[i], doc);
      CPPUNIT_ASSERT_MESSAGE(_message, _ppDocArray[i][j] == doc);
    }
  }
  cReader.close();
}

void testIndexFieldReader::testSeekDoc()
{
}

void testIndexFieldReader::testReadDoc()
{
  CPPUNIT_ASSERT(buildAddTerm () == 0); 

  IndexFieldReader cReader(1, MAX_DOC_NUM);
  CPPUNIT_ASSERT(cReader.open(NORMAL_PATH, FIELDNAME) == 0);
  MemPool cMemPool;

  for(uint32_t i = 0; i < _termNum; i++) {
    // next doc
    IndexTerm* pTerm = cReader.getTerm(&cMemPool, _pTermSign[i]);
    sprintf(_message, "%d %lu", i, _pTermSign[i]);
    CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);

    uint32_t docNum = pTerm->getDocNum();
    sprintf(_message, "true=%d, docNum=%d", _pDocNumArray[i], docNum);
    CPPUNIT_ASSERT_MESSAGE(_message, docNum == _pDocNumArray[i]);
    for(uint32_t j = 0; j < docNum; j++) {
      uint32_t doc = pTerm->next();
      sprintf(_message, "err=(%d)%d, true=%d, doc=%d", i, j, _pDocNumArray[i], doc);
      CPPUNIT_ASSERT_MESSAGE(_message, _ppDocArray[i][j] == doc);
    }

    const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
    CPPUNIT_ASSERT(NULL != pTermInfo);
    const uint64_t* bitmap = pTerm->getBitMap();
    if (pTermInfo->bitmapFlag == 1) {
      CPPUNIT_ASSERT(NULL != bitmap);
    } else {
      CPPUNIT_ASSERT(NULL == bitmap);
    }

    if (bitmap) {
      sprintf(_message, "err bitmapFlag=(%d)", i);
      CPPUNIT_ASSERT_MESSAGE(_message, 1 == pTermInfo->bitmapFlag);
      CPPUNIT_ASSERT(((MAX_DOC_NUM + 63) / 64) * 8 == pTermInfo->bitmapLen);
      uint32_t* pDoc = _ppDocArray[i];
      for(uint32_t j = 0; j < MAX_DOC_NUM; j++) {
        if (j < *pDoc) {
          sprintf(_message, "err=(%d)%d, doc=%d", i, j, *pDoc);
          CPPUNIT_ASSERT_MESSAGE(_message, 0 == (bitmap[j / 64]&bit_mask_tab[j % 64]));
        } else {
          CPPUNIT_ASSERT(0 != (bitmap[j / 64]&bit_mask_tab[j % 64]));
          pDoc++;
        }
      }
    } else {
      CPPUNIT_ASSERT(0 == pTermInfo->bitmapFlag);
    }
    cMemPool.reset();

    // seek doc
    uint32_t* pDoc = _ppDocArray[i];
    for(uint32_t j = 0; j < MAX_DOC_NUM; j++) {
      pTerm = cReader.getTerm(&cMemPool, _pTermSign[i]);
      sprintf(_message, "%d %lu", i, _pTermSign[i]);
      CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);

      uint32_t doc = pTerm->seek(j);
      sprintf(_message, "err=(%d), true=%d, doc=%d", i, _ppDocArray[i][j], doc);
      if (j == *pDoc) {
        CPPUNIT_ASSERT_MESSAGE(_message, j == doc);
        pDoc++;
      } else {
        CPPUNIT_ASSERT_MESSAGE(_message, j < doc);
      }
      cMemPool.reset();
    }
  }
  cReader.close();
}

int testIndexFieldReader::buildAddTerm()
{
  char message[1024];
  index_lib::IndexFieldBuilder cBuilder(FIELDNAME, 0, MAX_DOC_NUM, NULL);
  int ret = cBuilder.open(NORMAL_PATH);
  snprintf(message, sizeof(message), "%s ret=%d", NORMAL_PATH, ret);
  CPPUNIT_ASSERT_MESSAGE(message, ret == 0);

  for(uint32_t i = 0; i < _termNum; i++) {
    ret = cBuilder.addTerm(_pTermSign[i], _ppDocArray[i], _pDocNumArray[i]);
    snprintf(message, sizeof(message), "i = %d ret=%d", i, ret);
    CPPUNIT_ASSERT_MESSAGE(message, ret >= 0 && (uint32_t)ret == _pDocNumArray[i]);
  }
  CPPUNIT_ASSERT(cBuilder.dump() == 0);
  CPPUNIT_ASSERT(cBuilder.close() == 0);
  
  return 0;
}

int testIndexFieldReader::createDocInfo(int termNum, int docNum)
{
  _termNum = termNum;
  _pTermSign = new uint64_t[_termNum];
  _pDocNumArray = new uint32_t[_termNum];
  _ppDocArray = new uint32_t*[_termNum];

  srand(time(NULL));
  uint64_t sign = rand() - _termNum;
  util::HashMap<uint32_t, int32_t> cHash;

  for(uint32_t i = 0; i < _termNum; i++) {
    _pTermSign[i] = sign + i;
    
    docNum = rand() % MAX_DOC_NUM;
    if(docNum <= 0) docNum++;
    
    cHash.clear();
    for(int j = 0; j < docNum; j++) {
      uint32_t doc = rand() % MAX_DOC_NUM;

      util::HashMap<uint32_t, int32_t>::iterator pNode = cHash.find(doc);
      if (pNode != cHash.end()) {
        pNode->value++;
      } else {
        int32_t value = 1;
        cHash.insert(doc, value);
      }
    }

    int32_t value;
    _pDocNumArray[i] = cHash.size();
    _ppDocArray[i] = new uint32_t[_pDocNumArray[i] + 1];
    cHash.itReset();
    std::vector < int > vect;
    for(uint32_t j = 0; j < cHash.size(); j++) {
      cHash.itNext(_ppDocArray[i][j], value);
      vect.push_back(_ppDocArray[i][j]);
    }
    std::sort(vect.begin(), vect.end());
    for(uint32_t j = 0; j < cHash.size(); j++) {
      _ppDocArray[i][j] = vect[j];
    }
    _ppDocArray[i][cHash.size()] = INVALID_DOCID; 
  }
  return 0;
}

