#include "ProfileHashTableTest.h"
#include <stdio.h>
#include <unistd.h>

char* strArr[10] = {
    "pujian",
    "zhuangyuan",
    "danyuan",
    "santai",
    "",
    "abc",
    "shituo",
    "yinnong",
    "qiaoyin",
    "guijiaoqi"
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ProfileHashTableTest, "hash_table");

void ProfileHashTableTest::setUp()
{
    _hashTable = new ProfileHashTable(2);
}

void ProfileHashTableTest::tearDown()
{
    if (_hashTable != NULL) {
        delete _hashTable;
        _hashTable = NULL;
    }
}

void ProfileHashTableTest::testInitHashTable()
{
    CPPUNIT_ASSERT_MESSAGE( "process testInitHashTable failed", _hashTable->initHashTable(51, sizeof(uint32_t)));
    printf("process testInitHashTable pass!\n");
}

void ProfileHashTableTest::testDumpHashTable()
{
    testAddValuePtr();
    CPPUNIT_ASSERT_MESSAGE( "process testDumpHashTable failed", _hashTable->dumpHashTable(".", "test.idx"));
    printf("process testDumpHashTable pass!\n");
}

void ProfileHashTableTest::testLoadHashTable()
{
    CPPUNIT_ASSERT_MESSAGE( "process testLoadHashTable failed", _hashTable->loadHashTable(".", "test.idx"));
    for(uint32_t pos = 0; pos < 10; pos++) {
        uint32_t *pValue =  (uint32_t *)_hashTable->findValuePtr(strArr[pos], strlen(strArr[pos]));
        CPPUNIT_ASSERT_MESSAGE( "process testLoadHashTable failed", pValue != NULL && *pValue == pos);
    }
    printf("process testLoadHashTable pass!\n");
}

void ProfileHashTableTest::testResetHashTable()
{
    testAddValuePtr();
    _hashTable->resetHashTable();
    for (uint32_t pos = 0; pos < 10; pos++) {
        CPPUNIT_ASSERT_MESSAGE( "process testResetHashTable failed",
                                 _hashTable->findValuePtr(strArr[pos], strlen(strArr[pos])) == NULL);
    }
    
    printf("process testResetHashTable pass!\n");
}

void ProfileHashTableTest::testAddValuePtr()
{
    testInitHashTable();
    for(uint32_t pos = 0; pos < 10; pos++) {
        CPPUNIT_ASSERT_MESSAGE( "process testAddValuePtr failed",
                                _hashTable->addValuePtr(strArr[pos], strlen(strArr[pos]), &pos) == 0);
    }

    for(uint32_t pos = 0; pos < 10; pos++) {
        CPPUNIT_ASSERT_MESSAGE( "process testAddValuePtr failed",
                                _hashTable->addValuePtr(strArr[pos], strlen(strArr[pos]), &pos) == 1);
    }
    printf("process testAddValuePtr pass!\n");
}

void ProfileHashTableTest::testFindValuePtr()
{
    testAddValuePtr();
    for (uint32_t pos = 0; pos < 10; pos++) {
        uint32_t *pValue = (uint32_t *)_hashTable->findValuePtr(strArr[pos], strlen(strArr[pos]));
        CPPUNIT_ASSERT_MESSAGE( "process testFindValuePtr failed", pValue != NULL && *pValue == pos);
    }
    printf("process testFindValuePtr pass!\n");
}

void ProfileHashTableTest::testWalkNode()
{
    testAddValuePtr();
    uint32_t num = 0;
    uint32_t pos = 0;
    ProfileHashNode *pNode = NULL;
    pNode = _hashTable->firstNode(pos);
    CPPUNIT_ASSERT_MESSAGE( "process testWalkNode failed", pNode != NULL);

    do {
        num++;
        pNode = _hashTable->nextNode(pos);
    }while(pNode != NULL);
    CPPUNIT_ASSERT_MESSAGE( "process testWalkNode failed", num == 10);
    printf("process testWalkNode pass!\n");
}

void ProfileHashTableTest::testGetKeyValueNum()
{
    testAddValuePtr();
    CPPUNIT_ASSERT_MESSAGE( "process testGetKeyValueNum failed", _hashTable->getKeyValueNum() == 10);
    printf("process testGetBlockNum pass!\n");
}

void ProfileHashTableTest::testGetBlockSize()
{
    testInitHashTable();
    CPPUNIT_ASSERT_MESSAGE( "process testGetBlockSize failed", 
                             _hashTable->getBlockSize() == (sizeof(ProfileHashNode) + sizeof(uint32_t)));
    printf("process testGetBlockSize pass!\n");
}
void ProfileHashTableTest::testGetPayloadSize()
{
    testInitHashTable();
    CPPUNIT_ASSERT_MESSAGE( "process testGetPayloadSize failed", 
                             _hashTable->getPayloadSize() == sizeof(uint32_t));
    printf("process testGetPayloadSize pass!\n");
}
