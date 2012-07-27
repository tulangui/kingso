#ifndef INDEX_TEST_INDEXBUILDER_H_
#define INDEX_TEST_INDEXBUILDER_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexBuilder : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testIndexBuilder);
  CPPUNIT_TEST(testOpen);
  CPPUNIT_TEST(testAddField);
  CPPUNIT_TEST_SUITE_END();

public:
  testIndexBuilder();
  ~testIndexBuilder();
  void setUp(); 
  void tearDown();

public:
  // 测试增加字段
  void testAddField();
  
  // 测试打开索引文件
  void testOpen();
};

 
#endif //INDEX_TEST_INDEXBUILDER_H_
