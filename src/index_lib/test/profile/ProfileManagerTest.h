#ifndef _CPPUNIT_PROFILE_MANAGER_TEST
#define _CPPUNIT_PROFILE_MANAGER_TEST

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"

using namespace index_lib;

class ProfileManagerTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProfileManagerTest);
    CPPUNIT_TEST(testGetInstance);
    CPPUNIT_TEST(testGetDocAccessor);
    CPPUNIT_TEST(testSetProfilePath);
    CPPUNIT_TEST(testSetIndicateFile);
    CPPUNIT_TEST(testAddBitRecordField);
    CPPUNIT_TEST(testAddField);
    CPPUNIT_TEST(testSetDocHeader);
    CPPUNIT_TEST(testAddDoc);
    CPPUNIT_TEST(testDump);
    CPPUNIT_TEST(testLoad);
    CPPUNIT_TEST(testNewDocSegment);
    CPPUNIT_TEST(testGetProfileFieldNum);
    CPPUNIT_TEST(testGetProfileDocNum);
    CPPUNIT_TEST_SUITE_END();

public:
    ProfileManagerTest(){}
    ~ProfileManagerTest(){}

private:
    bool addBitRecordField();
    bool addCommonField();

public:
    void setUp();
    void tearDown();

    void testGetInstance();
    void testGetDocAccessor();
    void testSetProfilePath();
    void testSetIndicateFile();
    void testAddBitRecordField();
    void testAddField();
    void testSetDocHeader();
    void testAddDoc();
    void testLoad();
    void testDump();
    void testNewDocSegment();
    void testGetProfileFieldNum();
    void testGetProfileDocNum();
};


#endif //_CPPUNIT_PROFILE_MANAGER_TEST
