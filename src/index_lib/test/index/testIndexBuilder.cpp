#include "testIndexBuilder.h"
#include "test.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexBuilder.h"

testIndexBuilder::testIndexBuilder()
{
}

testIndexBuilder::~testIndexBuilder()
{
}

void testIndexBuilder::setUp()
{
  remove(ENCODEFILE);
  test::mkdir(NORMAL_PATH);
}

void testIndexBuilder::tearDown()
{
  remove(ENCODEFILE);
  test::rmdir(NORMAL_PATH);
}

void testIndexBuilder::testAddField()
{
  char fieldName[MAX_FIELD_NAME_LEN+1];

  { // fieldName 超长出错
    index_lib::IndexBuilder cBuilder(10);
    memset(fieldName, 'a', MAX_FIELD_NAME_LEN);
    fieldName[MAX_FIELD_NAME_LEN] = 0;
    CPPUNIT_ASSERT(0 > cBuilder.addField(fieldName, 0));
  }

  { // fieldName 名称重复出错
    index_lib::IndexBuilder cBuilder(10);
    memset(fieldName, 'a', MAX_FIELD_NAME_LEN);
    fieldName[MAX_FIELD_NAME_LEN-1] = 0;
    CPPUNIT_ASSERT(0 == cBuilder.addField(fieldName, 1));
    CPPUNIT_ASSERT(0 > cBuilder.addField(fieldName, 1));
  }

  { // field 超过个数出错
    index_lib::IndexBuilder cBuilder(10);
    int i = 0;
    for(i = 0; i < MAX_INDEX_FIELD_NUM; i++) {
      snprintf(fieldName, MAX_FIELD_NAME_LEN, "%d_field", i);
      CPPUNIT_ASSERT(0 == cBuilder.addField(fieldName, i));
    }
    snprintf(fieldName, MAX_FIELD_NAME_LEN, "%d_field", i);
    int ret = cBuilder.addField(fieldName, i);
    char message[1024];
    snprintf(message, sizeof(message), "%s %d", fieldName, ret);
    CPPUNIT_ASSERT_MESSAGE(message, ret < 0);
  }

  { // 字段排重的field, 重复对照表必须存在 
    index_lib::IndexBuilder cBuilder(10);
    memset(fieldName, 'a', MAX_FIELD_NAME_LEN);
    fieldName[MAX_FIELD_NAME_LEN-1] = 0;
    CPPUNIT_ASSERT(0 > cBuilder.addField(fieldName, 0, ENCODEFILE));

    FILE* fp = fopen(ENCODEFILE, "w");
    fclose(fp);
    int ret = cBuilder.addField(fieldName, 0, ENCODEFILE);
    char message[1024];
    snprintf(message, sizeof(message), "%s %d", fieldName, ret);
    CPPUNIT_ASSERT_MESSAGE(message, 0 == ret);
  }
}

void testIndexBuilder::testOpen()
{
  char fieldName[MAX_FIELD_NAME_LEN];
  index_lib::IndexBuilder cBuilder(1000);
  for(int32_t i = 0; i < 16; i++) {
    snprintf(fieldName, MAX_FIELD_NAME_LEN, "%d_field", i);
    CPPUNIT_ASSERT(0 == cBuilder.addField(fieldName, i));
  }

  char message[1024];
  int ret = cBuilder.open(NORMAL_PATH);
  snprintf(message, sizeof(message), "index builder open %s %d", NORMAL_PATH, ret);
  CPPUNIT_ASSERT_MESSAGE(message, 0 == ret);

  ret = cBuilder.close();
  snprintf(message, sizeof(message), "index builder close %s %d", NORMAL_PATH, ret);
  CPPUNIT_ASSERT_MESSAGE(message, 0 == ret);
}

