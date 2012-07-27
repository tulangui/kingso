#include "testIndexTermParse.h" 
#include "IndexTermParse.h"

#define TEST_DOC_NUM 3

using namespace index_lib;

testIndexTermParse::testIndexTermParse()
{
}

testIndexTermParse::~testIndexTermParse()
{
}
void testIndexTermParse::setUp()
{
}

void testIndexTermParse::tearDown()
{
}

// 测试没有occ情况
void testIndexTermParse::testNoneOccParse()
{
  { // 正常解析
    IndexNoneOccParse cParse;
    uint64_t sign = 12;
    uint32_t doc[TEST_DOC_NUM] = {12, 12, 13};
    uint8_t occ[TEST_DOC_NUM] = {1, 2, 3};

    char line[1024];
    char* p = line + sprintf(line, "%lu ", sign);
    
    for(int32_t i = 0; i < TEST_DOC_NUM; i++) {
      p += sprintf(p, "%u ", doc[i]);
      p += sprintf(p, "%u ", occ[i]);
    }
    *p = 0;

    int ret = cParse.init(line, strlen(line));
    sprintf(_message, "init ret = %d", ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);
   
    uint64_t s = cParse.getSign();
    CPPUNIT_ASSERT(sign == s);

    int32_t len = 0;
    uint32_t docId;
    char* outBuf;
    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == sizeof(uint32_t) && docId == 12);
    
    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == sizeof(uint32_t) && docId == 13);
      
    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == 0);
  }
}

// 测试一个occ情况
void testIndexTermParse::testOneOccParse()
{
  { // 正常解析
    IndexOneOccParse cParse;
    uint64_t sign = 12;
    uint32_t doc[TEST_DOC_NUM] = {12, 12, 13};
    uint8_t occ[TEST_DOC_NUM] = {1, 2, 3};

    char line[1024];
    char* p = line + sprintf(line, "%lu ", sign);
    
    for(int32_t i = 0; i < TEST_DOC_NUM; i++) {
      p += sprintf(p, "%u ", doc[i]);
      p += sprintf(p, "%u ", occ[i]);
    }
    *(--p) = 0;

    int ret = cParse.init(line, strlen(line));
    sprintf(_message, "init ret = %d", ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);
   
    uint64_t s = cParse.getSign();
    CPPUNIT_ASSERT(sign == s);

    int32_t len = 0;
    uint32_t docId;
    char* outBuf;
    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == sizeof(idx2_nzip_unit_t) && docId == 12);
    idx2_nzip_unit_t* node = (idx2_nzip_unit_t*)outBuf;
    CPPUNIT_ASSERT(node->doc_id == docId && node->occ == 1);

    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == sizeof(idx2_nzip_unit_t) && docId == 13);
    node = (idx2_nzip_unit_t*)outBuf;
    CPPUNIT_ASSERT(node->doc_id == docId && node->occ == 3);
      
    len = cParse.next(outBuf, docId);
    CPPUNIT_ASSERT(len == 0);
  }
}

