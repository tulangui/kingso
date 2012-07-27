#ifndef INDEX_TEST_INDEX_FIELD_READER_H_
#define INDEX_TEST_INDEX_FIELD_READER_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexFieldReader : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testIndexFieldReader);
//  CPPUNIT_TEST(testOpen);
  CPPUNIT_TEST(testNextDoc);
//  CPPUNIT_TEST(testSeekDoc);
//  CPPUNIT_TEST(testReadDoc);
  CPPUNIT_TEST_SUITE_END();

public:
  testIndexFieldReader();
  ~testIndexFieldReader();
  void setUp(); 
  void tearDown();

public:
  void testOpen();
  
  // 顺序读取docid, 需增加测试点
  void testNextDoc();

  // Seek读取docid，与增加测试点
  void testSeekDoc();

  // 半集成测试
  void testReadDoc();

private:

  // 造一批docid数据
  int createDocInfo(int termNum, int docNum);

  int buildOpen();
  int buildAddTerm();
  
private:
  char _message[10240]; // output assert message

  uint32_t _termNum;
  uint64_t* _pTermSign;

  uint32_t* _pDocNumArray; 
  uint32_t** _ppDocArray;
};

 
#endif //INDEX_TEST_INDEXBUILDER_H_
