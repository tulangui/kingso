#include "test.h"
#include "idx_dict.h"
#include "idx_dict.cpp"
#include "index_lib/IndexLib.h"
#include "testIdxDict.h"

extern int32_t szHashMap_StepTable[25];

testIdxDict::testIdxDict()
{
}

testIdxDict::~testIdxDict()
{
}

void testIdxDict::setUp()
{
}

void testIdxDict::tearDown()
{
}

void testIdxDict::testOptimize()
{
  idx_dict_t* pdict = idx_dict_create(DICT_HASH_SIZE, DICT_NODE_SIZE);
  CPPUNIT_ASSERT(NULL != pdict);

  srand(time(NULL));
  idict_node_t node;

  printf("dictNodeSize=%lu\n", sizeof(idict_node_t));

  uint64_t* pSign = new uint64_t[DICT_NODE_NUM];

  for(uint32_t i = 0; i < DICT_NODE_NUM; i++) {
    node.sign = rand() * rand() * rand();
    node.cuint1 = i;
    node.cuint2 = i + 1;
    node.cuint3 = i + 2;

    pSign[i] = node.sign;
    int ret = idx_dict_add(pdict, &node);
    CPPUNIT_ASSERT(ret >= 0);
  }

  idx_dict_t* popt_dict = idx_dict_optimize(pdict);
  for(uint32_t i = 0; i < DICT_NODE_NUM; i++) {
    idict_node_t* node1 = idx_dict_find(pdict, pSign[i]);
    idict_node_t* node2 = idx_dict_find(popt_dict, pSign[i]);

    sprintf(_message, "find i=%d, key=%lu", i, pSign[i]);
    CPPUNIT_ASSERT_MESSAGE(_message, NULL != node1 && NULL != node2);
    CPPUNIT_ASSERT_MESSAGE(_message, node1->cuint1 == node2->cuint1
        && node1->cuint2 == node2->cuint2 && node1->cuint3 == node2->cuint3);
  }
  
  int32_t collisionNum1 = 0;
  int32_t maxCollision1 = idx_get_max_collision(pdict, collisionNum1);

  int32_t collisionNum2 = 0;
  int32_t maxCollision2 = idx_get_max_collision(popt_dict, collisionNum2);
  sprintf(_message, "maxCollision1=%d,maxCollision2=%d,collisionNum1=%d, collisionNum2=%d",
      maxCollision1, maxCollision2, collisionNum1, collisionNum2);
  printf("%s\n", _message);

  CPPUNIT_ASSERT_MESSAGE(_message, maxCollision2 < collisionNum1 || maxCollision2 < maxCollision1);

  delete[] pSign;
}
  
// 测试范围查询
void testIdxDict::testBoundFind()
{
  for(int32_t i = 0; i < 25; i++) {
    int32_t ret = idx_find_value(szHashMap_StepTable, 25, szHashMap_StepTable[i] - 2);
    sprintf(_message, "find i=%d, value=%d", i, szHashMap_StepTable[i] - 2);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == i);

    ret = idx_find_value(szHashMap_StepTable, 25, szHashMap_StepTable[i] - 1);
    sprintf(_message, "find i=%d, value=%d", i, szHashMap_StepTable[i] - 1);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == i);

    ret = idx_find_value(szHashMap_StepTable, 25, szHashMap_StepTable[i]);
    sprintf(_message, "find i=%d, value=%d", i, szHashMap_StepTable[i]);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == i);

    ret = idx_find_value(szHashMap_StepTable, 25, szHashMap_StepTable[i] + 1);
    sprintf(_message, "find i=%d, value=%d", i, szHashMap_StepTable[i] + 1);
    if(i == 24) {
      CPPUNIT_ASSERT_MESSAGE(_message, ret == i);
    } else {
      CPPUNIT_ASSERT_MESSAGE(_message, ret == i + 1);
    }
  }
}

// 测试冲突统计
void testIdxDict::testCollision()
{
  idx_dict_t* pdict = idx_dict_create(DICT_COLLISION_HASH_SIZE, DICT_COLLISION_NODE_SIZE);
  CPPUNIT_ASSERT(NULL != pdict);

  idict_node_t node;

  for(uint32_t i = 0; i < DICT_COLLISION_NODE_NUM; i++) {
    node.sign = i;
    node.cuint1 = i;
    node.cuint2 = i + 1;
    node.cuint3 = i + 2;

    int ret = idx_dict_add(pdict, &node);
    CPPUNIT_ASSERT(ret == 0);
  }

  idx_dict_t* popt_dict = idx_dict_optimize(pdict);
  CPPUNIT_ASSERT(NULL != popt_dict);

  int32_t collisionNum1 = 0;
  int32_t maxCollision1 = idx_get_max_collision(pdict, collisionNum1);

  int32_t collisionNum2 = 0;
  int32_t maxCollision2 = idx_get_max_collision(popt_dict, collisionNum2);
  sprintf(_message, "maxCollision1=%d,maxCollision2=%d,collisionNum1=%d, collisionNum2=%d",
      maxCollision1, maxCollision2, collisionNum1, collisionNum2);
  printf("%s\n", _message);

  CPPUNIT_ASSERT_MESSAGE(_message, maxCollision2 < collisionNum1 || maxCollision2 < maxCollision1);
}

