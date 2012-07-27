#ifndef _CPPUNIT_PROFILE_DOCACCESSOR_TEST
#define _CPPUNIT_PROFILE_DOCACCESSOR_TEST

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"

using namespace index_lib;

class ProfileDocAccessorTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProfileDocAccessorTest);
    CPPUNIT_TEST(testAddProfileField);
    CPPUNIT_TEST(testGetProfileField);
    CPPUNIT_TEST(testGetFieldMultiValNum);
    CPPUNIT_TEST(testGetFieldDataType);
    CPPUNIT_TEST(testGetFieldFlag);
    CPPUNIT_TEST(testGetFieldName);
    CPPUNIT_TEST(testSet_GetInt);
    CPPUNIT_TEST(testSet_GetUInt);
    CPPUNIT_TEST(testSet_GetFloat);
    CPPUNIT_TEST(testSet_GetDouble);
    CPPUNIT_TEST(testSet_GetString);
    CPPUNIT_TEST(testSet_GetInt8);
    CPPUNIT_TEST(testSet_GetInt16);
    CPPUNIT_TEST(testSet_GetInt32);
    CPPUNIT_TEST(testSet_GetInt64);
    CPPUNIT_TEST(testSet_GetUInt8);
    CPPUNIT_TEST(testSet_GetUInt16);
    CPPUNIT_TEST(testSet_GetUInt32);
    CPPUNIT_TEST(testSet_GetUInt64);
    CPPUNIT_TEST(testSet_GetRepeatedInt8);
    CPPUNIT_TEST(testSet_GetRepeatedInt16);
    CPPUNIT_TEST(testSet_GetRepeatedInt32);
    CPPUNIT_TEST(testSet_GetRepeatedInt64);
    CPPUNIT_TEST(testSet_GetRepeatedUInt8);
    CPPUNIT_TEST(testSet_GetRepeatedUInt16);
    CPPUNIT_TEST(testSet_GetRepeatedUInt32);
    CPPUNIT_TEST(testSet_GetRepeatedUInt64);
    CPPUNIT_TEST(testSet_GetRepeatedVarint);
    CPPUNIT_TEST(testSet_GetRepeatedFloat);
    CPPUNIT_TEST(testSet_GetRepeatedDouble);
    CPPUNIT_TEST(testSet_GetRepeatedString);
    CPPUNIT_TEST_SUITE_END();

public:
    ProfileDocAccessorTest();
    ~ProfileDocAccessorTest();

public:
    void setUp();
    void tearDown();

    void testAddProfileField();
    void testGetProfileField();
    void testGetFieldMultiValNum();
    void testGetEmptyValue();
    void testGetFieldDataType();
    void testGetFieldFlag();
    void testGetFieldName();
    void testSet_GetInt();
    void testSet_GetUInt();
    void testSet_GetFloat();
    void testSet_GetDouble();
    void testSet_GetString();
    void testSet_GetRepeatedVarint();
    void testSet_GetRepeatedFloat();
    void testSet_GetRepeatedDouble();
    void testSet_GetRepeatedString();
    void testSet_GetInt8();
    void testSet_GetInt16();
    void testSet_GetInt32();
    void testSet_GetInt64();
    void testSet_GetUInt8();
    void testSet_GetUInt16();
    void testSet_GetUInt32();
    void testSet_GetUInt64();
    void testSet_GetRepeatedInt8();
    void testSet_GetRepeatedInt16();
    void testSet_GetRepeatedInt32();
    void testSet_GetRepeatedInt64();
    void testSet_GetRepeatedUInt8();
    void testSet_GetRepeatedUInt16();
    void testSet_GetRepeatedUInt32();
    void testSet_GetRepeatedUInt64();
};


#endif //_CPPUNIT_PROFILE_MANAGER_TEST
