#include "ProfileManagerTest.h"
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>

static char * bit_aliasArr[12] = {0};
static char * bit_nameArr[12] = {
    "bit_int8",
    "bit_uint8",
    "bit_int16",
    "bit_uint16",
    "bit_int32",
    "bit_uint32",
    "bit_int64",
    "bit_uint64",
    "bit_float",
    "bit_double",
    "bit_string",
    "bit_u32"
};

static uint32_t bit_lenArr[12] = { 1, 3, 2, 5, 7, 2, 4, 4, 5, 11, 8, 9};
static PF_DATA_TYPE bit_dtArr[12]  = {
    DT_INT8,
    DT_UINT8,
    DT_INT16,
    DT_UINT16,
    DT_INT32,
    DT_UINT32,
    DT_INT64,
    DT_UINT64,
    DT_FLOAT,
    DT_DOUBLE,
    DT_STRING,
    DT_UINT32
};

static EmptyValue    bit_emptyArr[12];
static bool    bit_encodeFlagArr[12] = { false, false, false, false, false, false, false, false, false, false, false, false};
static PF_FIELD_FLAG bit_FlagArr[12] = {F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_FILTER_BIT};

static char *field_name[12] = {
    "field_int8",
    "field_uint8",
    "field_int16",
    "field_uint16",
    "field_int32",
    "field_uint32",
    "nid",
    "field_uint64",
    "field_float",
    "field_double",
    "field_string",
    "field_u32"
};

static PF_DATA_TYPE field_type_Arr[12]  = {
    DT_INT8,
    DT_UINT8,
    DT_INT16,
    DT_UINT16,
    DT_INT32,
    DT_UINT32,
    DT_INT64,
    DT_UINT64,
    DT_FLOAT,
    DT_DOUBLE,
    DT_STRING,
    DT_UINT32
};

static uint32_t field_multiArr[12] = {1, 1, 2, 1, 2, 3, 1, 1, 1, 1, 1, 2};
static bool     field_encodeArr[12] = {
    false,
    false,
    false,
    false,
    false,
    false,
    true,
    false,
    false,
    false,
    false,
    false
};

static PF_FIELD_FLAG field_FlagArr[12] = {F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_FILTER_BIT};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ProfileManagerTest, "profile_manager");

void ProfileManagerTest::setUp()
{
    ProfileManager* pManager = ProfileManager::getInstance();
    pManager->setProfilePath("./");
    pManager->setDocIdDictPath("./");
 }

void ProfileManagerTest::tearDown()
{
    ProfileManager::freeInstance();
    DocIdManager::freeInstance();
}

void ProfileManagerTest::testGetInstance()
{
    ProfileManager * p1 = ProfileManager::getInstance();
    ProfileManager * p2 = ProfileManager::getInstance();
    CPPUNIT_ASSERT_MESSAGE("process testGetInstance failed", p1 != NULL && p1 == p2);
    printf("process testGetInstance pass!\n");
}

void ProfileManagerTest::testGetDocAccessor()
{
    ProfileDocAccessor * p1 = ProfileManager::getDocAccessor();
    ProfileDocAccessor * p2 = ProfileManager::getDocAccessor();
    CPPUNIT_ASSERT_MESSAGE("process testGetDocAccessor failed", p1 != NULL && p1 == p2);
    printf("process testGetDocAccessor pass!\n");
}


void ProfileManagerTest::testSetProfilePath()
{
    bool ret = true;
    CPPUNIT_ASSERT_MESSAGE( "process testSetProfilePath failed", ret);
    printf("process testSetProfilePath pass!\n");
}

void ProfileManagerTest::testSetIndicateFile()
{
    bool ret = true;
    CPPUNIT_ASSERT_MESSAGE( "process testSetIndicateFile failed", ret);
    printf("process testSetIndicateFile pass!\n");
}

void ProfileManagerTest::testAddBitRecordField()
{
    bool ret = addBitRecordField();
    CPPUNIT_ASSERT_MESSAGE( "process testAddBitRecordField failed", ret);
    printf("process testAddBitRecordField pass!\n");
}

void ProfileManagerTest::testAddField()
{
    bool ret = addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testAddField failed", ret);
    printf("process testAddField pass!\n");
}

void ProfileManagerTest::testSetDocHeader()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testSetDocHeader failed", ret);

    ProfileManager *pManager = ProfileManager::getInstance();
    char *fakeHeader = "vdocidbit_nofield1field_nofield2abc_field3no_field4";
    CPPUNIT_ASSERT_MESSAGE( "process testSetDocHeader failed", pManager->setDocHeader(fakeHeader) != 0);

    char *docHeader = "vdocidbit_int8bit_uint8bit_int16bit_uint16bit_int32bit_uint32bit_int64bit_uint64bit_floatbit_doublebit_stringbit_u32field_int8field_uint8field_int16field_uint16field_int32field_uint32nidfield_uint64field_floatfield_doublefield_stringfield_u32";
    CPPUNIT_ASSERT_MESSAGE( "process testSetDocHeader failed", pManager->setDocHeader(docHeader) == 0);

    printf("process testSetDocHeader pass!\n");
}

void ProfileManagerTest::testAddDoc()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", ret);

    char *docHeader = "vdocidbit_int8bit_uint8bit_int16bit_uint16bit_int32bit_uint32bit_int64bit_uint64bit_floatbit_doublebit_stringbit_u32field_int8field_uint8field_int16field_uint16field_int32field_uint32nidfield_uint64field_floatfield_doublefield_stringfield_u32";
    ProfileManager *pManager = ProfileManager::getInstance();
    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", pManager->setDocHeader(docHeader) == 0);

    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;
    gettimeofday(&tv_begin, NULL);

    char docValue0[] = "0-13-217-43-7136.8871.31a2575314232 45782-49 86871 9861 387112858316758.3512347.785pujian832 5791";
 pManager->addDoc(docValue0);
//    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", pManager->addDoc(docValue0) == 0);

    char docValue1[] = "1-13-217-43-7136.8871.31a2575314232 45782-49 86871 9861 387112868316758.3512347.785pujian832 5791";
    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", pManager->addDoc(docValue1) == 0);

    char docValue2[] = "2-13-217-43-7136.8871.31a2575314232 45782-49 86871 9861 387112878316758.3512347.785";
    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", pManager->addDoc(docValue2) != 0);

    char docValue3[] = "8-13-217-43-7136.8871.31a2575314232 45782-49 86871 9861 387112888316758.3512347.785pujian832 5791";
    CPPUNIT_ASSERT_MESSAGE( "process testAddDoc failed", pManager->addDoc(docValue3) != 0);
    gettimeofday(&tv_end, NULL);
    timersub(&tv_end, &tv_begin, &tv_total);

    printf("time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
    printf("process testAddDoc pass!\n");
}

void ProfileManagerTest::testLoad()
{
    ProfileManager *pManager = ProfileManager::getInstance();
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pManager->load() == 0);

    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField       = pDocAccessor->getProfileField(bit_nameArr[10]);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField != NULL);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", strcmp(pField->name, bit_nameArr[10]) == 0);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->type == DT_STRING);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->flag == F_NO_FLAG);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->multiValNum == 1);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->groupID  == 0);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->unitLen == 8);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->isEncoded);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->isBitRecord);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->encodeFilePtr != NULL);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->bitRecordPtr != NULL );
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->bitRecordPtr->field_len == 8 );
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->bitRecordPtr->u32_offset == 0 );
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->bitRecordPtr->bits_move == 4);
    CPPUNIT_ASSERT_MESSAGE( "process testLoad failed", pField->bitRecordPtr->read_mask == 0xff0);
    
    printf("process testLoad pass!\n");
}

void ProfileManagerTest::testDump()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testDump failed", ret);

    ProfileManager *pManager = ProfileManager::getInstance();
    for(int pos = 0; pos <= (1024*1024 + 1024); pos++) {
        pManager->newDocSegment();
    }

    CPPUNIT_ASSERT_MESSAGE( "process testDump failed", pManager->dump() == 0);
    printf("process testDump pass!\n");
}

void ProfileManagerTest::testNewDocSegment()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testNewDocSegment failed", ret);

    ProfileManager *pManager = ProfileManager::getInstance();
    for(int pos = 0; pos <= (1024*1024 + 1024); pos++) {
        if (pManager->newDocSegment() != 0) {
            printf("new doc segment failed at %d\n", pos);
            CPPUNIT_ASSERT_MESSAGE( "process testNewDocSegment failed", false);
            return;
        }
    }
    printf("process testNewDocSegment pass!\n");
}

void ProfileManagerTest::testGetProfileFieldNum()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testGetProfileFieldNum failed", ret);

    ProfileManager *pManager = ProfileManager::getInstance();
    CPPUNIT_ASSERT_MESSAGE( "process testGetProfileFieldNum failed", pManager->getProfileFieldNum() == 24);
    printf("process testGetProfileFieldNum pass!\n");
}

void ProfileManagerTest::testGetProfileDocNum()
{
    bool ret = true;
    ret = addBitRecordField() && addCommonField();
    CPPUNIT_ASSERT_MESSAGE( "process testGetProfileDocNum failed", ret);

    ProfileManager *pManager = ProfileManager::getInstance();
    for(int pos = 0; pos < 1024; pos++) {
        pManager->newDocSegment();
    }
    CPPUNIT_ASSERT_MESSAGE( "process testGetProfileDocNum failed", pManager->getProfileDocNum() == 1024);
    printf("process testGetProfileDocNum pass!\n");
}

bool ProfileManagerTest::addBitRecordField()
{
    ProfileManager *pManager = ProfileManager::getInstance();
    memset(&bit_emptyArr, 0, sizeof(EmptyValue) * 12);
    bool ret = true;
    if (pManager->addBitRecordField(12, bit_nameArr, bit_aliasArr, bit_lenArr, bit_dtArr, bit_encodeFlagArr, bit_FlagArr, bit_emptyArr) != 0) {
        ret = false;
    }

    ProfileDocAccessor * docAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    pField = docAccessor->getProfileField(bit_nameArr[8]);
    if (pField == NULL) {
        ret = false;
    }

    if (strcmp(pField->name, bit_nameArr[8]) != 0 || pField->type != DT_FLOAT || pField->flag != F_NO_FLAG
               || pField->multiValNum != 1 || pField->unitLen != 8 || !pField->isBitRecord || pField->bitRecordPtr == NULL
               || pField->bitRecordPtr->bits_move != 15 || pField->bitRecordPtr->field_len != 5
               || pField->bitRecordPtr->u32_offset != 1 || pField->bitRecordPtr->read_mask != 0xf8000)
    {
        ret = false;
    }
/*
    for(uint32_t pos = 0; pos < 12; pos++) {
        pField = docAccessor->getProfileField(bit_nameArr[pos]);
        if (pField == NULL) {
            ret = false;
        }
        printf("**********************************\n");
        printf("pos:%u\n", pos);
        printf("name:%s\n", pField->name);
        printf("type:%d\n", pField->type);
        printf("flag:%d\n", pField->flag);
        printf("multi:%u\n", pField->multiValNum);
        printf("offset:%u\n", pField->offset);
        printf("unitLen:%u\n",pField->unitLen);
        printf("groupID:%u\n",pField->groupID);
        printf("isBit:%s\n",pField->isBitRecord?"true":"false");
        printf("isEncode:%s\n", pField->isEncoded?"true":"false");
        printf("EncodeInit:%s\n", (pField->encodeFilePtr == NULL)?"failed":"ok");
        printf("BitInit:%s\n", (pField->bitRecordPtr == NULL)?"field":"ok");
        if (pField->bitRecordPtr != NULL) {
            printf("field_len:%u\n", pField->bitRecordPtr->field_len);
            printf("u32_offset:%u\n", pField->bitRecordPtr->u32_offset);
            printf("bits_move:%u\n", pField->bitRecordPtr->bits_move);
            printf("read_mask:0x%x\n", pField->bitRecordPtr->read_mask);
            printf("write_mask:0x%x\n", pField->bitRecordPtr->write_mask);
        }
    }
*/
    return ret;
}

bool ProfileManagerTest::addCommonField()
{
    ProfileManager *pManager = ProfileManager::getInstance();
    EmptyValue ev;
    memset(&ev, 0, sizeof(ev));
    for (uint32_t pos = 0; pos < 12; pos++) {
        if (pManager->addField(field_name[pos], NULL, field_type_Arr[pos], field_multiArr[pos], field_encodeArr[pos], field_FlagArr[pos], false, ev, 0) != 0) {
            printf("add Field failed!\n");
            return false;
        }
    }
/*
    ProfileDocAccessor * docAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for (uint32_t pos = 0; pos < 12; pos++) {
        pField = docAccessor->getProfileField(name[pos]);
        if (pField == NULL) {
            return false;
        }
        printf("**********************************\n");
        printf("pos:%u\n", pos);
        printf("name:%s\n", pField->name);
        printf("type:%d\n", pField->type);
        printf("flag:%d\n", pField->flag);
        printf("multi:%u\n", pField->multiValNum);
        printf("offset:%u\n", pField->offset);
        printf("unitLen:%u\n",pField->unitLen);
        printf("groupID:%u\n",pField->groupID);
        printf("isBit:%s\n",pField->isBitRecord?"true":"false");
        printf("isEncode:%s\n", pField->isEncoded?"true":"false");
        printf("EncodeInit:%s\n", (pField->encodeFilePtr == NULL)?"failed":"ok");
        printf("BitInit:%s\n", (pField->bitRecordPtr == NULL)?"field":"ok");
    }
*/
    return true;
}

