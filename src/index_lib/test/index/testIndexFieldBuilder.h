#ifndef INDEX_TEST_INDEX_FIELD_BUILDER_H_
#define INDEX_TEST_INDEX_FIELD_BUILDER_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

#include "IndexFieldBuilder.h"

class testIndexFieldBuilder : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(testIndexFieldBuilder);
    CPPUNIT_TEST(testOpen);
    //  CPPUNIT_TEST(testAbandon);
    CPPUNIT_TEST(testAddNullOcc);
    CPPUNIT_TEST(testAddOneOcc);
    CPPUNIT_TEST(testAddMulOcc);
    CPPUNIT_TEST(testBuildIndex);
    CPPUNIT_TEST_SUITE_END();

public:
    testIndexFieldBuilder();
    ~testIndexFieldBuilder();
    void setUp(); 
    void tearDown();

public:
    void testOpen();

    void testAddNullOcc();
    void testAddOneOcc();
    void testAddMulOcc();

    // 测试build稳定性 
    void testBuildIndex();

    // 测试docnum超长截断
    void testAbandon();

private:
    // 造一批docid数据
    int createDocInfo(int termNum, int docNum);

private:
    char _message[10240]; // output assert message

    int32_t _termNum;
    uint64_t* _pTermSign;

    int32_t* _pDocNumArray; 
    int32_t** _ppDocArray;
};

 
#endif //INDEX_TEST_INDEXBUILDER_H_
