#ifndef _CPPUNIT_PROFILE_HASHTABLE_TEST
#define _CPPUNIT_PROFILE_HASHTABLE_TEST

#include <stdint.h>
#include <cppunit/extensions/HelperMacros.h>
#include "index_lib/ProfileHashTable.h"

using namespace index_lib;

class ProfileHashTableTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE(ProfileHashTableTest);
    CPPUNIT_TEST(testInitHashTable);
    CPPUNIT_TEST(testDumpHashTable);
    CPPUNIT_TEST(testLoadHashTable);
    CPPUNIT_TEST(testResetHashTable);
    CPPUNIT_TEST(testAddValuePtr);
    CPPUNIT_TEST(testFindValuePtr);
    CPPUNIT_TEST(testWalkNode);
    CPPUNIT_TEST(testGetKeyValueNum);
    CPPUNIT_TEST(testGetBlockSize);
    CPPUNIT_TEST(testGetPayloadSize);
    CPPUNIT_TEST_SUITE_END();

public:
    ProfileHashTableTest():_hashTable(NULL){}
    ~ProfileHashTableTest() {
        if (_hashTable != NULL) {
            delete _hashTable;
            _hashTable = NULL;
        }
    }

public:
    void setUp();
    void tearDown();

    void testInitHashTable();
    void testDumpHashTable();
    void testLoadHashTable();
    void testResetHashTable();
    void testAddValuePtr();
    void testFindValuePtr();
    void testWalkNode();
    void testGetKeyValueNum();
    void testGetBlockSize();
    void testGetPayloadSize();

private:
    ProfileHashTable *_hashTable;
};


#endif //_CPPUNIT_PROFILE_HASHTABLE_TEST
