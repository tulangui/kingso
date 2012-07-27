#ifndef _TEST_INDEX_TERM_H_ 
#define _TEST_INDEX_TERM_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexTerm : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE(testIndexTerm);
  CPPUNIT_TEST(testBitmap);
  CPPUNIT_TEST_SUITE_END();

public:
  testIndexTerm();
  ~testIndexTerm();
  void setUp(); 
  void tearDown();

public:
  // 包括next 和 seek操作 
  void testBitmap();

private:
  void testBitmap(uint32_t* array, uint32_t num, uint64_t* bitmap);

  char _message[10240]; // output assert message
};

 
#endif // _TEST_INDEX_TERM_H_

