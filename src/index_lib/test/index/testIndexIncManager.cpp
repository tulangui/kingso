#include "test.h"
#include "testIndexIncManager.h" 
#include "util/idx_sign.h"
#include "index_lib/IndexBuilder.h"
#include "index_lib/IndexIncManager.h"

using namespace index_lib;

#define FIELD_NUM 3
const char* field[FIELD_NUM] = {"catmap", "spuid", "title"};
uint64_t fieldSign[FIELD_NUM];

testIndexIncManager::testIndexIncManager()
{
  for(int32_t i = 0; i < FIELD_NUM; i++) {
    fieldSign[i] = idx_sign64(field[i], strlen(field[i]));
  }
}

testIndexIncManager::~testIndexIncManager()
{
}

void testIndexIncManager::setUp()
{
  test::mkdir(NORMAL_PATH);
}

void testIndexIncManager::tearDown()
{
  test::rmdir(NORMAL_PATH);
}

void testIndexIncManager::testAddTerm()
{
    testAddTerm(0);
    testAddTerm(1);
    testAddTerm(255);
}

void testIndexIncManager::testAddTerm(uint32_t maxOccNum)
{
  IndexBuilder cBuilder(INC_MAX_DOC_NUM);
  for(int32_t i = 0; i < FIELD_NUM; i++) {
     CPPUNIT_ASSERT(0 == cBuilder.addField(field[i], maxOccNum));
  }
  CPPUNIT_ASSERT(0 == cBuilder.open(NORMAL_PATH));
  CPPUNIT_ASSERT(0 == cBuilder.dump());
  CPPUNIT_ASSERT(0 == cBuilder.close());

  IndexIncManager* pInc = IndexIncManager::getInstance();
  
  int ret = pInc->open(NORMAL_PATH);
  sprintf(_message, "inc open %s, code=%d", NORMAL_PATH, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret >= 0);

  char line[1024];
//  int32_t len;
  srand(time(NULL));
  MemPool cMemPool;

  /*
  len = sprintf(line, "149198147206885 2997894 0");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);

  len = sprintf(line, "162443793706725 2997894 2");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);


  len = sprintf(line, "149198147206885 2997895 0");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);

  len = sprintf(line, "162443793706725 2997895 2");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);

  len = sprintf(line, "149198147206885 2997896 0");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);

  len = sprintf(line, "162443793706725 2997896 2");
  ret = pInc->addTerm("title", line, len);
  sprintf(_message, "inc addTerm %s, code=%d", line, ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret > 0);
*/

  for(uint64_t sign = 100; sign < 100 + INC_TERM_NUM; sign++) {  
    time_t begin = time(NULL);
    for(uint32_t i = 0; i < INC_DOC_NUM; i++) {
      for(uint32_t j = 0; j < FIELD_NUM; j++) {
        char* p = line;
        p += sprintf(line, "%lu ", sign);

        int32_t occNum = rand() % OCC_NUM;
        if(occNum <= 0) occNum = 1;
        for(int32_t k = 0; k < occNum; k++) {
          p += sprintf(p, "%d %d ", i, rand() % MAX_OCC_NUM);
        }

        ret = pInc->addTerm(field[j], j, sign, i, rand() % MAX_OCC_NUM);
        sprintf(_message, "inc addTerm %s, code=%d", line, ret);
        CPPUNIT_ASSERT_MESSAGE(_message, ret >= 0);
      }

      // 检测增加是否成功
      for(uint32_t j = 0; j < FIELD_NUM; j++) {
        IndexTerm* pTerm = pInc->getTerm(&cMemPool, fieldSign[j], sign);
        sprintf(_message, "inc getTerm %s error", field[j]);
        CPPUNIT_ASSERT_MESSAGE(_message, pTerm != NULL);

        const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
        sprintf(_message, "%s sign=%lu j=%d", field[j], sign, j);
        CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTermInfo);
        sprintf(_message, "%s ret=%d need=%d", field[j], pTermInfo->docNum, i + 1);
        CPPUNIT_ASSERT_MESSAGE(_message, i + 1 == pTermInfo->docNum);

        uint32_t doc = pTerm->seek(i);
        sprintf(_message, "inc getTerm %s, true=%u, need=%u", field[j], doc, i);
        CPPUNIT_ASSERT_MESSAGE(_message, doc == i);
      }
     
      cMemPool.reset();
      testSetposNext(sign);
      testSetposSeek(sign);
    }
    printf("%lu use=%ld\n", sign, time(NULL) - begin);
  }
  // dump
  ret = pInc->dump();
  sprintf(_message, "inc dump, code=%d", ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret >= 0);
  // close
  ret = pInc->close();
  sprintf(_message, "inc close, code=%d", ret);
  CPPUNIT_ASSERT_MESSAGE(_message, ret >= 0);

  fprintf(stderr, "test inc add term success\n");
}

void testIndexIncManager::testSetposNext(uint64_t sign)
{
    MemPool cMemPool;
    IndexIncManager* pInc = IndexIncManager::getInstance();

    for(uint32_t j = 0; j < FIELD_NUM; j++) {
        IndexTerm* pTerm = pInc->getTerm(&cMemPool, fieldSign[j], sign);
        sprintf(_message, "inc getTerm %s error", field[j]);
        CPPUNIT_ASSERT_MESSAGE(_message, pTerm != NULL);

        const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
        sprintf(_message, "%s sign=%lu j=%d", field[j], sign, j);
        CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTermInfo);

        uint32_t maxNum = pTermInfo->docNum;
        uint32_t * array = NEW_VEC(&cMemPool, uint32_t, maxNum + 1);
        for(uint32_t k = 0; k < maxNum + 1; k++) {
            array[k] = pTerm->next();
        }

        uint32_t pos = 0, dest = 0, doc;
        do {
            pTerm->setpos(pos);
            doc = pTerm->next();
            if(pos <= array[dest]) {
                sprintf(_message, "inc getTerm %s, sign=%lu, true=%u, need=%u, max=%u", field[j], sign, doc, array[dest], maxNum);
            } else {
                dest++;
                sprintf(_message, "inc getTerm %s, sign=%lu, true=%u, need=%u, max=%u", field[j], sign, doc, array[dest], maxNum);
            }
            CPPUNIT_ASSERT_MESSAGE(_message, doc == array[dest]);
            pos++;
        } while(doc < INVALID_DOCID);
    }
    cMemPool.reset();
}

void testIndexIncManager::testSetposSeek(uint64_t sign)
{
    MemPool cMemPool;
    IndexIncManager* pInc = IndexIncManager::getInstance();

    for(uint32_t j = 0; j < FIELD_NUM; j++) {
        IndexTerm* pTerm = pInc->getTerm(&cMemPool, fieldSign[j], sign);
        sprintf(_message, "inc getTerm %s error", field[j]);
        CPPUNIT_ASSERT_MESSAGE(_message, pTerm != NULL);

        const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
        sprintf(_message, "%s sign=%lu j=%d", field[j], sign, j);
        CPPUNIT_ASSERT_MESSAGE(_message, NULL != pTermInfo);

        uint32_t maxNum = pTermInfo->docNum;
        uint32_t * array = NEW_VEC(&cMemPool, uint32_t, maxNum + 1);
        for(uint32_t k = 0; k < maxNum + 1; k++) {
            array[k] = pTerm->next();
        }

        uint32_t pos = 0, dest = 0, doc;
        do {
            pTerm->setpos(pos);
            doc = pTerm->seek(pos);
            if(pos <= array[dest]) {
                sprintf(_message, "inc getTerm %s, sign=%lu, true=%u, need=%u", field[j], sign, doc, array[dest]);
            } else {
                dest++;
                sprintf(_message, "inc getTerm %s, sign=%lu, true=%u, need=%u", field[j], sign, doc, array[dest]);
            }
            CPPUNIT_ASSERT_MESSAGE(_message, doc == array[dest]);
            pos++;
        } while(doc < INVALID_DOCID);
    }
    cMemPool.reset();
}

