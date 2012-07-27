#ifndef INDEX_TESTINDEXTERMPARSE_H_
#define INDEX_TESTINDEXTERMPARSE_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexTermParse : public CppUnit::TestFixture
{ 
  CPPUNIT_TEST_SUITE(testIndexTermParse);
  CPPUNIT_TEST(testNoneOccParse);
  CPPUNIT_TEST(testOneOccParse);
  CPPUNIT_TEST_SUITE_END();

public:
  testIndexTermParse();
  ~testIndexTermParse();
  void setUp(); 
  void tearDown();

public:
  // 测试没有occ情况
  void testNoneOccParse();

  // 测试一个occ情况
  void testOneOccParse();

private:
  char _message[10240];
};

#endif //INDEX_TESTINDEXTERMPARSE_H_
