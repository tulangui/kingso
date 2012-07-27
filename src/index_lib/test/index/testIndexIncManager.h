#ifndef INDEX_TESTINDEXINCMANAGER_H_
#define INDEX_TESTINDEXINCMANAGER_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class testIndexIncManager : public CppUnit::TestFixture
{ 
  CPPUNIT_TEST_SUITE(testIndexIncManager);
  CPPUNIT_TEST(testAddTerm);
  CPPUNIT_TEST_SUITE_END();
public:
  testIndexIncManager();
  ~testIndexIncManager();
  void setUp(); 
  void tearDown();

public:
  void testAddTerm();

  // 
  void testAddTerm(uint32_t maxOccNum);

  // setpos 后执行next操作
  void testSetposNext(uint64_t sign);

  // setpos 后执行seek操作
  void testSetposSeek(uint64_t sign);

private:
  char _message[10240];
};

 
#endif //INDEX_TESTINDEXINCMANAGER_H_
