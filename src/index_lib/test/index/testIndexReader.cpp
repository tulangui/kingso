#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test.h"
#include "util/HashMap.hpp"
#include "testIndexReader.h"
#include "IndexFieldBuilder.h"
#include "IndexFieldReader.h"
#include "util/idx_sign.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexReader.h"
#include "index_lib/IndexBuilder.h"

#define FIELD_NUM 4 

using namespace index_lib;

const char* TEST_DATA_CFG = "index_lib_cfg.xml";
const char* FIELD_ARRAY[FIELD_NUM] = {"title", "title1", "cat_id_path", "spuid"};
const uint32_t MAX_OCC_ARRAY[FIELD_NUM] = {1, 256, 0, 0};

testIndexReader::testIndexReader()
{
  strcpy(_binIdxPath, BIN_IDX_PATH);
  strcpy(_txtIdxPath, TXT_IDX_PATH);
}

testIndexReader::~testIndexReader()
{
}

void testIndexReader::setUp()
{
  test::mkdir(NORMAL_PATH);
}

void testIndexReader::tearDown()
{
  test::rmdir(NORMAL_PATH);
}

void testIndexReader::testGetNotExistTerm()
{
  IndexBuilder cBuilder(10);
  const char** field = FIELD_ARRAY;
  const uint32_t* maxOccNum = MAX_OCC_ARRAY;
  for(int32_t i = 0; i < FIELD_NUM; i++) {
    cBuilder.addField(field[i], maxOccNum[i]);
  }
  CPPUNIT_ASSERT(0 == cBuilder.open(NORMAL_PATH));
  CPPUNIT_ASSERT(0 == cBuilder.dump());
  CPPUNIT_ASSERT(0 == cBuilder.close());

  IndexReader* pReader = IndexReader::getInstance();
  CPPUNIT_ASSERT(0 == pReader->open(NORMAL_PATH));

  MemPool cPool;
  IndexTerm* pTerm = pReader->getTerm(&cPool, "aaaa", "abcde"); 
  CPPUNIT_ASSERT(NULL == pTerm);

  for(int32_t i = 0; i < FIELD_NUM; i++) {
    IndexTerm* pTerm = pReader->getTerm(&cPool, field[i], "abcde"); 
    CPPUNIT_ASSERT(NULL != pTerm);

    const IndexTermInfo* pInfo = pTerm->getTermInfo();
    CPPUNIT_ASSERT(pInfo->docNum == 0);
    cPool.reset();
  }
  pReader->close();
}

void testIndexReader::testInit()
{
  // 解析配置文件
  FILE* fp = fopen(TEST_DATA_CFG, "r");
  if(fp == NULL) {
    return;
  }

  mxml_node_t* pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
  CPPUNIT_ASSERT(pXMLTree != NULL);
  fclose(fp);
  
  int ret = index_lib::init(pXMLTree);
  CPPUNIT_ASSERT(ret == 0);
  fprintf(stderr, "init test ok\n");
  mxmlDelete(pXMLTree);

  IndexReader::getInstance()->close();
}

void testIndexReader::testGetDoc()
{
  initIndexLib();

  {
    MemPool cMemPool;
    IndexReader* pReader = IndexReader::getInstance();
    IndexTerm* pTerm = pReader->getTerm(&cMemPool, 54311681026420L, 210813765861L);
    uint32_t docId;
    while((docId = pTerm->next()) != INVALID_DOCID) {
      int32_t count;
      uint8_t* occ = pTerm->getOcc(count);
      printf("%u %u\n", docId, *occ);
    }
  }


  FILE* fpArray[FIELD_NUM];
  uint64_t fieldSign[FIELD_NUM];
  const char** field = FIELD_ARRAY;

  uint32_t* pDoc = new uint32_t[50<<20];
  uint32_t* pOff = new uint32_t[50<<20];
  uint8_t* pOcc = new uint8_t[50<<20];

  char fileName[PATH_MAX];
  for (int32_t i = 0; i < FIELD_NUM; i++) {
    sprintf(fileName, "%s/%s.idx.txt", _txtIdxPath, field[i]);
    fpArray[i] = fopen(fileName, "rb");
    CPPUNIT_ASSERT_MESSAGE(fileName, NULL != fpArray[i]);

    IndexReader* pReader = IndexReader::getInstance();
    CPPUNIT_ASSERT_MESSAGE(field[i], true == pReader->hasField(field[i]));

    fieldSign[i] = idx_sign64(field[i], strlen(field[i])); 
  }

  char * line = NULL;
  size_t len = 0;
  ssize_t read;
  MemPool cMemPool;

  time_t begin = time(NULL);
  time_t per = begin;

  for(int32_t i = 0; i < FIELD_NUM; i++) {
    int32_t lineNo = 0;
    uint32_t maxDocNum = 0;

    while ((read = getline(&line, &len, fpArray[i])) != -1) {
      const char * pre = line;
      char * cur = NULL;
      uint64_t sign = strtoull(pre, &cur, 10);
      uint32_t occPos = 0;
      uint32_t realDocNum = 0;
      uint32_t lasDoc = INVALID_DOCID;
      while(pre != cur) {
        pre = cur;
        pDoc[realDocNum] = strtoul(pre, &cur, 10);
        if (pre == cur) break;

        pre = cur;
        pOcc[occPos++] = strtoul(pre, &cur, 10);
        if (lasDoc != pDoc[realDocNum]) {
          lasDoc = pDoc[realDocNum];
          pOff[realDocNum] = occPos;
          realDocNum++;
        } else {
          pOff[realDocNum-1]++;
        }
      }
      pOff[realDocNum] = occPos;
      pDoc[realDocNum] = INVALID_DOCID;

      lineNo++;
//if(lineNo != 133386) continue;
      IndexReader* pReader = IndexReader::getInstance();

      // next doc
      IndexTerm* pTerm = pReader->getTerm(&cMemPool, fieldSign[i], sign);
      sprintf(_message, "%d %lu", lineNo, sign);
      CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);

      uint32_t docNum = pTerm->getDocNum();
      if(MAX_OCC_ARRAY[i] <= 1) {
        sprintf(_message, "line=%d, true=%d, docNum=%d", lineNo, realDocNum, docNum);
        CPPUNIT_ASSERT_MESSAGE(_message, docNum == realDocNum);
      } else {
        sprintf(_message, "line=%d, true=%d, docNum=%d, occ=%d", lineNo, realDocNum, docNum, occPos);
        CPPUNIT_ASSERT_MESSAGE(_message, docNum >= realDocNum && docNum == occPos);
      }
      
      uint32_t off = 0;
      int32_t count = 0;
      for(uint32_t j = 0; j < docNum; j++) {
        uint32_t doc = pTerm->next();
        if(doc == INVALID_DOCID && MAX_OCC_ARRAY[i] > 1) {
          break;
        }
        sprintf(_message, "err=(%d)%d, true=%u, doc=%u, field=%lu, sign=%lu",
            lineNo, j, pDoc[j], doc, fieldSign[i], sign);
        CPPUNIT_ASSERT_MESSAGE(_message, pDoc[j] == doc);

        if(MAX_OCC_ARRAY[i] > 1) {
          uint8_t* occ = pTerm->getOcc(count);
          sprintf(_message, "err=(%d)%d, true=%u, doc=%u, field=%lu, sign=%lu, count=(%d,%d)",
              lineNo, j, pDoc[j], doc, fieldSign[i], sign, count, pOff[j] - off);
          CPPUNIT_ASSERT_MESSAGE(_message, count == (int32_t)(pOff[j] - off));
          CPPUNIT_ASSERT_MESSAGE(_message, memcmp(pOcc+off, occ, count) == 0);
        } else if(MAX_OCC_ARRAY[i] == 1) {
          uint8_t* occ = pTerm->getOcc(count);
          sprintf(_message, "field=%lu, sign=%lu, doc=%u, occ=%u, true=%u", fieldSign[i], sign, doc, *occ, pOcc[off]);  
          CPPUNIT_ASSERT_MESSAGE(_message, count == 1);
          CPPUNIT_ASSERT_MESSAGE(_message, memcmp(pOcc+off, occ, count) == 0);
        }
        off = pOff[j];
      }

      const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
      CPPUNIT_ASSERT(NULL != pTermInfo);
      const uint64_t* bitmap = pTerm->getBitMap();
      if (pTermInfo->bitmapFlag == 1) {
        CPPUNIT_ASSERT(NULL != bitmap);
      } else {
        CPPUNIT_ASSERT(NULL == bitmap);
      }
      if (maxDocNum == 0) {
        maxDocNum = pTermInfo->maxDocNum;
        CPPUNIT_ASSERT(maxDocNum > (1<<20));
      } else {
        CPPUNIT_ASSERT(maxDocNum == pTermInfo->maxDocNum);
      }
      if(lineNo % 1000 == 0) {
        fprintf(stderr, "finish next, use time=%ld\n", time(NULL) - per);
      }

      if (bitmap) {
        sprintf(_message, "err bitmapFlag=(%d)", lineNo);
        CPPUNIT_ASSERT_MESSAGE(_message, 1 == pTermInfo->bitmapFlag);
        CPPUNIT_ASSERT(((pTermInfo->maxDocNum+ 63) / 64) * 8 <= pTermInfo->bitmapLen);
        uint32_t* p = pDoc;
        for(uint32_t j = 0; j < pTermInfo->maxDocNum; j++) {
          if (j < *p) {
            sprintf(_message, "err=(%d)%d, doc=%u", lineNo, j, *p);
            CPPUNIT_ASSERT_MESSAGE(_message, 0 == (bitmap[j / 64]&bit_mask_tab[j % 64]));
          } else {
            sprintf(_message, "err=(%d)%d, doc=%u", lineNo, j, *p);
            CPPUNIT_ASSERT_MESSAGE(_message, 0 != (bitmap[j / 64]&bit_mask_tab[j % 64]));
            p++;
          }
        }
        fprintf(stderr, "finish bitmap, use time=%ld\n", time(NULL) - per);
      } else {
        CPPUNIT_ASSERT(0 == pTermInfo->bitmapFlag);
      }
      cMemPool.reset();

      // seek doc
      uint32_t* p = pDoc;
      for(uint32_t j = 0; j < maxDocNum; ) {
        pTerm = pReader->getTerm(&cMemPool, fieldSign[i], sign);
        sprintf(_message, "%d %lu", lineNo, sign);
        CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);

        uint32_t doc = pTerm->seek(j);
        sprintf(_message, "err=(%d), field=%lu, sign=%lu, true=%u, doc=%u",
            j, fieldSign[i], sign, *p, doc);
        if (j == *p) {
          CPPUNIT_ASSERT_MESSAGE(_message, j == doc);
          p++;
        } else {
          CPPUNIT_ASSERT_MESSAGE(_message, j < doc);
        }
        if (j + 3 < *p) {
          uint32_t n = j + 1;
          j = *p - 3;
          if(j < n) 
            printf("error\n");
        } else {
          j++;
        }
        cMemPool.reset();
        if (j % 100000 == 0) {
          fprintf(stderr, "finish seek(%d / %d), use time=%ld\n", j, maxDocNum, time(NULL) - per);
        }
      }

      // 连续seek
      p = pDoc;
      pTerm = pReader->getTerm(&cMemPool, fieldSign[i], sign);
      sprintf(_message, "%d %lu", lineNo, sign);
      CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTerm);
      
      uint32_t doc = 0, tmpDoc, no = 0;
      while (1) {
        no++;
        tmpDoc = pTerm->seek(doc);
        if (tmpDoc == INVALID_DOCID) {
          break;
        }
        sprintf(_message, "err=(%d,%d), true=%u, doc=%u", lineNo, no, *p, tmpDoc);
        CPPUNIT_ASSERT_MESSAGE(_message, tmpDoc == *p);
        p++;
        doc = tmpDoc + 1;
      }
      sprintf(_message, "last err=(%d), true=%u, doc=%u", lineNo, *p, tmpDoc);
      CPPUNIT_ASSERT_MESSAGE(_message, INVALID_DOCID == *p);
      cMemPool.reset();
    }
  }

  delete[] pDoc;
  delete[] pOcc;
  delete[] pOff;
  IndexReader::getInstance()->close();
}

void testIndexReader::initIndexLib()
{
  IndexReader* pReader = IndexReader::getInstance();
  CPPUNIT_ASSERT(0 == pReader->open(_binIdxPath));
}

void testIndexReader::trimString(char* str)
{
  int len = strlen(str) - 1;
  while(len >= 0 && (str[len] == 0x0a || str[len] == 0x0d || str[len] == ' ')) {
    len--;
  }
  len++;
  str[len] = 0;
}
