#ifndef INDEX_TEST_IDXDICT_H_
#define INDEX_TEST_IDXDICT_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIdxDict : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testIdxDict);
  CPPUNIT_TEST(testOptimize);
  CPPUNIT_TEST(testBoundFind);
  CPPUNIT_TEST(testCollision);
  CPPUNIT_TEST_SUITE_END();

public:
  testIdxDict();
  ~testIdxDict();
  void setUp(); 
  void tearDown();

public:
  // 测试字典优化，优化后和优化前查询结果应相同
  void testOptimize();
  
  // 测试范围查询
  void testBoundFind();

  // 测试冲突统计
  void testCollision();

private:
  char _message[10240];
};

 
#endif //INDEX_TEST_INDEXBUILDER_H_
