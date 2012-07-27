#ifndef _CPPUNIT_PROFILE_ENCODEFILE_TEST
#define _CPPUNIT_PROFILE_ENCODEFILE_TEST

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include "index_lib/ProfileEncodeFile.h"

using namespace index_lib;

class ProfileEncodeFileTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProfileEncodeFileTest);
    CPPUNIT_TEST(testCreateFile);
    CPPUNIT_TEST(testDumpFile);
    CPPUNIT_TEST(testLoadFile);
    CPPUNIT_TEST(testGetDataType);
    CPPUNIT_TEST(testGetUnitSize);
    CPPUNIT_TEST(testAddEncode);
    CPPUNIT_TEST(testGetEncodeIdx);
    CPPUNIT_TEST(testGetEncodeValueNum);
    CPPUNIT_TEST_SUITE_END();

public:
    ProfileEncodeFileTest():_stringFile(NULL),_baseValueFile(NULL),_singleStringFile(NULL),_singleBaseValueFile(NULL){}
    ~ProfileEncodeFileTest() {
        if (_stringFile != NULL) {
            delete _stringFile;
        }
        if (_baseValueFile != NULL) {
            delete _baseValueFile;
        }
        if (_singleStringFile != NULL) {
            delete _singleStringFile;
        }
        if (_singleBaseValueFile != NULL) {
            delete _singleBaseValueFile;
        }
    }

public:
    void setUp();
    void tearDown();

    void testCreateFile();
    void testLoadFile();
    void testDumpFile();
    void testGetDataType();
    void testGetUnitSize();
    void testAddEncode();
    void testGetEncodeIdx();
    void testGetEncodeValueNum();

private:
    ProfileEncodeFile *_stringFile;
    ProfileEncodeFile *_baseValueFile;
    ProfileEncodeFile *_singleStringFile;
    ProfileEncodeFile *_singleBaseValueFile;

    uint32_t  offset1[10];
    uint32_t  offset2[10];
    uint32_t  offset3[10];
    uint32_t  offset4[10];
};


#endif //_CPPUNIT_ENCODEFILE_TEST
