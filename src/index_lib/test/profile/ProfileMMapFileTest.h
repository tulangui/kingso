#ifndef _CPPUNIT_PROFILE_MMAPFILE_TEST
#define _CPPUNIT_PROFILE_MMAPFILE_TEST

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include "index_lib/ProfileMMapFile.h"

using namespace index_lib;

class ProfileMMapFileTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProfileMMapFileTest);
    CPPUNIT_TEST(testOpen);
    CPPUNIT_TEST(testClose);
    CPPUNIT_TEST(testFlush);
    CPPUNIT_TEST(testMakeNewPage);
    CPPUNIT_TEST(testOffset2Addr);
    CPPUNIT_TEST(testGetMMapStat);
    CPPUNIT_TEST(testGetLength);
    CPPUNIT_TEST(testWrite);
    CPPUNIT_TEST(testRead);
    CPPUNIT_TEST_SUITE_END();

public:
    ProfileMMapFileTest():_mwFile(NULL), _mrFile(NULL){}
    ~ProfileMMapFileTest() {
        if (_mwFile != NULL) {
            delete _mwFile;
            _mwFile = NULL;
        }

        if (_mrFile != NULL) {
            delete _mrFile;
            _mrFile = NULL;
        }
    }

public:
    void setUp();
    void tearDown();

    void testOpen();
    void testClose();
    void testFlush();
    void testMakeNewPage();
    void testOffset2Addr();
    void testGetMMapStat();
    void testGetLength();
    void testWrite();
    void testRead();

private:
    ProfileMMapFile *_mwFile;
    ProfileMMapFile *_mrFile;
};


#endif //_CPPUNIT_PROFILE_MMAP_TEST
