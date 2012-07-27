#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test.h"
#include "testIndexTerm.h"
#include "IndexTermActualize.h"
#include "IndexFieldReader.h"

using namespace index_lib;

testIndexTerm::testIndexTerm()
{
  IndexFieldReader read(0, 0);
}

testIndexTerm::~testIndexTerm()
{
}

void testIndexTerm::setUp()
{
}

void testIndexTerm::tearDown()
{
}

void testIndexTerm::testBitmap()
{
#define BIT_MAP_LEN 32
#define DOC_ID_NUM (BIT_MAP_LEN * 64)
  uint64_t bitmap[BIT_MAP_LEN];

  {// 边界
    uint32_t docArray[DOC_ID_NUM] = {0, 32, 64, DOC_ID_NUM-1, INVALID_DOCID};
    testBitmap(docArray, 4, bitmap);
  }

  {// 连续
    uint32_t docArray[DOC_ID_NUM] = {0, 1, 2, 3, 4, 63, 64, 65, DOC_ID_NUM-1, INVALID_DOCID};
    testBitmap(docArray, 9, bitmap);
  }
}

void testIndexTerm::testBitmap(uint32_t* array, uint32_t num, uint64_t* bitmap)
{
  memset(bitmap, 0, BIT_MAP_LEN * 8);
  for(uint32_t i = 0; i < num; i++) {
    bitmap[array[i] / 64] |= bit_mask_tab[array[i] % 64];
  }
  uint32_t maxDocId = array[num-1];

  IndexBitmapTerm cTerm;
  cTerm.setBitMap(bitmap);
  CPPUNIT_ASSERT(0 == cTerm.init(num, DOC_ID_NUM, maxDocId));

  // next 
  for(uint32_t i = 0; i < num; i++) {
    uint32_t ret = cTerm.next();
    sprintf(_message, "i=%u, get=%u, value=%u", i, ret, array[i]);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == array[i]);
  }
  CPPUNIT_ASSERT(cTerm.next() == INVALID_DOCID);

  // seek
  for(uint32_t i = 0; i < num; i++) {
    CPPUNIT_ASSERT(cTerm.seek(array[i]) == array[i]);
    if(array[i] <= 0) {
      continue;
    }
    uint32_t ret = cTerm.seek(array[i]-1);
    if(i > 0 && array[i]-1 == array[i-1]) {
      CPPUNIT_ASSERT(ret == array[i-1]);
    } else {
      sprintf(_message, "i=%u, seek=%u, ret=%u", i, array[i]-1, ret);
      CPPUNIT_ASSERT_MESSAGE(_message, ret == array[i]);
    }
  }

  uint32_t* p = array;
  for(uint32_t i = 0; i < DOC_ID_NUM; i++) {
    if(*p < i) p++;
    CPPUNIT_ASSERT(*p == cTerm.seek(i));
  }
}

