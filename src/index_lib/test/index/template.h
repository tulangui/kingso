/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: template.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_TEST_TEMPLATE_H_
#define INDEX_TEST_TEMPLATE_H_

#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TextTestRunner.h>
#include <cppunit/extensions/HelperMacros.h>

class templateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(templateTest);
    CPPUNIT_TEST(testBitflow);
    CPPUNIT_TEST(testBSearch);
    //  CPPUNIT_TEST(testOverflow);
    //  CPPUNIT_TEST(testOperatorBit);
    CPPUNIT_TEST_SUITE_END();
public:
    templateTest();
    ~templateTest();
    void setUp() {
    }
    void tearDown() {
    }

public:
    // 测试bit使用溢出
    void testBitflow();

    // 立即数移位溢出错误
    void testOverflow();
    // 位运算速度测试
    void testOperatorBit();
    
    // 测试二分查找
    void testBSearch();

private:
    uint64_t _bitRul[64];
};


#endif //INDEX_TEST_TEMPLATE_H_
