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

class testTaskQueue : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(testTaskQueue);
    CPPUNIT_TEST(testPushPop);
    CPPUNIT_TEST_SUITE_END();
public:
    testTaskQueue();
    ~testTaskQueue();
    void setUp() {
    }
    void tearDown() {
    }

public:
    void testPushPop();

private:
    char _message[10240];
};


#endif //INDEX_TEST_TEMPLATE_H_
