#ifndef _TEST_INDEX_READER_H_
#define _TEST_INDEX_READER_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexReader : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testIndexReader);
//  CPPUNIT_TEST(testInit);
  CPPUNIT_TEST(testGetDoc);
  CPPUNIT_TEST(testGetNotExistTerm);
  CPPUNIT_TEST_SUITE_END();

public:
  testIndexReader();
  ~testIndexReader();
  void setUp(); 
  void tearDown();

public:
  // 包括next 和 seek操作 
  void testGetDoc();

  // 初始化
  void testInit();

  // 获取不存在的term
  void testGetNotExistTerm();

private:
  // 初始化indexLib，包括 open 
  void initIndexLib();
  
  void trimString(char* str);

private:
  char _message[10240]; // output assert message
  char _binIdxPath[PATH_MAX];
  char _txtIdxPath[PATH_MAX];
};

 
#endif //INDEX_TEST_INDEXBUILDER_H_
