#include "ProfileMMapFileTest.h"
#include <stdio.h>
#include <unistd.h>

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ProfileMMapFileTest, "mmap_file");

void ProfileMMapFileTest::setUp()
{
    _mwFile = new ProfileMMapFile("test.mmap");
    _mrFile = new ProfileMMapFile("test.mmap");
}

void ProfileMMapFileTest::tearDown()
{
    if (_mwFile != NULL) {
        delete _mwFile;
        _mwFile = NULL;
    }

    if (_mrFile != NULL) {
        delete _mrFile;
        _mrFile = NULL;
    }
    unlink("test.mmap");
}

void ProfileMMapFileTest::testOpen()
{
    bool ret = _mwFile->open(false, false, 5000);
    CPPUNIT_ASSERT_MESSAGE( "process testOpen failed", ret);

    ret = _mrFile->open(true, false, 0);
    CPPUNIT_ASSERT_MESSAGE( "process testOpen failed", ret);
    printf("process testOpen pass!\n");
}

void ProfileMMapFileTest::testClose()
{
    testOpen();
    bool ret = _mwFile->close();
    CPPUNIT_ASSERT_MESSAGE( "process testClose failed", ret);

    ret = _mrFile->close();
    CPPUNIT_ASSERT_MESSAGE( "process testClose failed", ret);
    printf("process testClose pass!\n");
}

void ProfileMMapFileTest::testFlush()
{
    testOpen();
    bool ret = _mwFile->flush(true);
    CPPUNIT_ASSERT_MESSAGE( "process testFlush failed", ret);

    ret = _mrFile->flush(true);
    CPPUNIT_ASSERT_MESSAGE( "process testFlush failed", !ret);
    printf("process testFlush pass!\n");
}


void ProfileMMapFileTest::testMakeNewPage()
{
    testOpen();
    size_t newlen = _mwFile->makeNewPage(1000);
    CPPUNIT_ASSERT_MESSAGE( "process testMakeNewPage failed", newlen == 6000);
    
    printf("process testMakeNewPage pass!\n");
}

void ProfileMMapFileTest::testOffset2Addr()
{
    testOpen();
    char * pBase = _mwFile->offset2Addr(0);
    char * pEnd  = _mwFile->offset2Addr(4999);

    CPPUNIT_ASSERT_MESSAGE( "process testOffset2Addr failed", (pEnd - pBase) == 4999);
    printf("process testOffset2Addr pass!\n");
}


void ProfileMMapFileTest::testGetMMapStat()
{
    CPPUNIT_ASSERT_MESSAGE( "process testGetMMapStat failed", !_mwFile->getMMapStat());
    CPPUNIT_ASSERT_MESSAGE( "process testGetMMapStat failed", !_mrFile->getMMapStat());
    testOpen();
    CPPUNIT_ASSERT_MESSAGE( "process testGetMMapStat failed", _mwFile->getMMapStat());
    CPPUNIT_ASSERT_MESSAGE( "process testGetMMapStat failed", _mrFile->getMMapStat());
    printf("process testGetMMapStat pass!\n");
}

void ProfileMMapFileTest::testGetLength()
{
    testOpen();
    CPPUNIT_ASSERT_MESSAGE( "process testGetLength failed", _mwFile->getLength() == 5000);
    CPPUNIT_ASSERT_MESSAGE( "process testGetLength failed", _mrFile->getLength() == 5000);
    printf("process testGetLength pass!\n");
}

void ProfileMMapFileTest::testWrite()
{
    testOpen();
    uint32_t value = 12834;
    CPPUNIT_ASSERT_MESSAGE( "process testWrite failed",  _mwFile->write(500, &value, sizeof(value)) == sizeof(value));
    printf("process testWrite pass!\n");
}

void ProfileMMapFileTest::testRead()
{
    testWrite();
    uint32_t value;
    CPPUNIT_ASSERT_MESSAGE( "process testRead failed",  _mwFile->read(500, &value, sizeof(value)) == sizeof(value) && value == 12834);

    CPPUNIT_ASSERT_MESSAGE( "process testRead failed",  _mrFile->read(500, &value, sizeof(value)) == sizeof(value) && value == 12834);
    
    printf("process testRead pass!\n");
}

