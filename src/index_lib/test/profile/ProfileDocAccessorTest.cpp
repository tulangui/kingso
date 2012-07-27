#include "ProfileDocAccessorTest.h"
#include <stdio.h>
#include <unistd.h>

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
PF_DATA_TYPE bit_dtArr[12]  = {
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

EmptyValue  bit_emptyValueArr[12];

static bool    bit_encodeFlagArr[12] = { false, false, false, false, false, false, false, false, false, false, false, false};
PF_FIELD_FLAG bit_FlagArr[12] = {F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_NO_FLAG, F_FILTER_BIT};

static char *field_name[12] = {
    "field_int8",
    "field_uint8",
    "field_int16",
    "field_uint16",
    "field_int32",
    "field_uint32",
    "field_int64",
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

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ProfileDocAccessorTest, "doc_accessor");

ProfileDocAccessorTest::ProfileDocAccessorTest()
{
}

ProfileDocAccessorTest::~ProfileDocAccessorTest()
{
}

void ProfileDocAccessorTest::setUp()
{
    ProfileManager* pManager = ProfileManager::getInstance();
    pManager->setProfilePath(".");

    EmptyValue ev;
    memset(&ev, 0, sizeof(EmptyValue));
    memset(bit_emptyValueArr, 0, 12 * sizeof(EmptyValue));

    if (pManager->addBitRecordField(12, bit_nameArr, bit_aliasArr, bit_lenArr, bit_dtArr, bit_encodeFlagArr, bit_FlagArr, bit_emptyValueArr) != 0) {
        printf("add bitRecord Field failed!\n");
    }
    for (uint32_t pos = 0; pos < 12; pos++) {
        if (pManager->addField(field_name[pos], NULL, field_type_Arr[pos], field_multiArr[pos], field_encodeArr[pos], field_FlagArr[pos], false, ev, 0) != 0) {
            printf("add Field failed!\n");
        }
    }

    pManager->addField("repeated_float", NULL, DT_FLOAT, 0, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("repeated_double", NULL, DT_DOUBLE, 0, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("repeated_String", NULL, DT_STRING, 0, false, F_NO_FLAG, false, ev, 0);

    pManager->addField("single_int16", NULL, DT_INT16, 1, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("single_int32", NULL, DT_INT32, 1, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("single_uint32", NULL, DT_UINT32, 1, false, F_NO_FLAG, false, ev, 0);

    pManager->addField("repeated_int8", NULL, DT_INT8, 0, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("repeated_int64", NULL, DT_INT64, 0, false, F_NO_FLAG, false, ev, 0, false);
    pManager->addField("repeated_uint8", NULL, DT_UINT8, 0, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("repeated_uint16", NULL, DT_UINT16, 0, false, F_NO_FLAG, false, ev, 0);
    pManager->addField("repeated_uint64", NULL, DT_UINT64, 0, false, F_NO_FLAG, false, ev, 0);


    pManager->addField("varint_int32", NULL, DT_INT32,  0, false, F_NO_FLAG, true, ev, 0);
    pManager->addField("varint_int64", NULL, DT_INT64,  0, false, F_NO_FLAG, true, ev, 0);
    pManager->addField("varint_uint32", NULL, DT_UINT32, 0, false, F_NO_FLAG, true, ev, 0);
    pManager->addField("varint_uint64", NULL, DT_UINT64, 0, false, F_NO_FLAG, true, ev, 0);

    for(uint32_t pos = 0; pos < 10; pos++) {
        if (pManager->newDocSegment(false) != 0) {
            printf("new doc segment failed!\n");
        }
    }
}

void ProfileDocAccessorTest::tearDown()
{
    ProfileManager::freeInstance();
}

void ProfileDocAccessorTest::testAddProfileField()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    ProfileField fieldStruct;
    snprintf(fieldStruct.name, MAX_FIELD_NAME_LEN, "%s", "test_field");

    CPPUNIT_ASSERT_MESSAGE("process testAddProfileField failed", pDocAccessor->addProfileField(&fieldStruct) == 0);

    snprintf(fieldStruct.name, MAX_FIELD_NAME_LEN, "%s", bit_nameArr[10]);
    CPPUNIT_ASSERT_MESSAGE("process testAddProfileField failed", pDocAccessor->addProfileField(&fieldStruct) != 0);

    printf("process testAddProfileField pass!\n");
}

void ProfileDocAccessorTest::testGetProfileField()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetProfileField failed", pField != NULL && strcmp(bit_nameArr[pos], pField->name) == 0);
    }

    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(field_name[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetProfileField failed", pField != NULL && strcmp(field_name[pos], pField->name) == 0);
    }
    printf("process testGetProfileField pass!\n");
}

void ProfileDocAccessorTest::testGetEmptyValue()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldMultiValNum failed", pField != NULL && pDocAccessor->getFieldEmptyValue(pField).EV_INT64 == 0);
    }

    printf("process testGetEmptyValue pass!\n");
}

void ProfileDocAccessorTest::testGetFieldMultiValNum()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldMultiValNum failed", pField != NULL && pDocAccessor->getFieldMultiValNum(pField) == 1);
    }

    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(field_name[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldMultiValNum failed", pField != NULL && pDocAccessor->getFieldMultiValNum(pField) == field_multiArr[pos]);
    }
    printf("process testGetFieldMultiValNum pass!\n");
}

void ProfileDocAccessorTest::testGetFieldDataType()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldDataType failed", pField != NULL && pDocAccessor->getFieldDataType(pField) == bit_dtArr[pos]);
    }

    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(field_name[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldDataType failed", pField != NULL && pDocAccessor->getFieldDataType(pField) == field_type_Arr[pos]);
    }
    printf("process testGetFieldDataType pass!\n");
}

void ProfileDocAccessorTest::testGetFieldFlag()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldFlag failed", pField != NULL && pDocAccessor->getFieldFlag(pField) == bit_FlagArr[pos]);
    }

    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(field_name[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldFlag failed", pField != NULL && pDocAccessor->getFieldFlag(pField) == field_FlagArr[pos]);
    }
    printf("process testGetFieldFlag pass!\n");
}

void ProfileDocAccessorTest::testGetFieldName()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(bit_nameArr[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldName failed", pField != NULL && strcmp(bit_nameArr[pos], pDocAccessor->getFieldName(pField)) == 0);
    }

    for(int pos = 0; pos < 12; pos++) {
        pField = pDocAccessor->getProfileField(field_name[pos]);
        CPPUNIT_ASSERT_MESSAGE("process testGetFieldName failed", pField != NULL && strcmp(field_name[pos], pDocAccessor->getFieldName(pField)) == 0);
    }
    printf("process testGetFieldName pass!\n");
}

void ProfileDocAccessorTest::testSet_GetInt()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    // bit字段/编码表字段
    pField = pDocAccessor->getProfileField("bit_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pDocAccessor->setFieldValue(0, pField, "2001") == 0);

    // bit字段/编码表字段
    pField = pDocAccessor->getProfileField("bit_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pDocAccessor->setFieldValue(1, pField, "3") == 0 );

    // 非bit字段/编码表字段
    pField = pDocAccessor->getProfileField("field_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pDocAccessor->setFieldValue(2, pField, "381757987") == 0);

    // 非bit字段/非编码表字段
    pField = pDocAccessor->getProfileField("field_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pDocAccessor->setFieldValue(4, pField, "757987") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();
    
    // bit字段/编码表字段
    pField = pLoadDocAccessor->getProfileField("bit_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pLoadDocAccessor->getInt(0, pField) == 2001);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pLoadDocAccessor->setFieldValue(0, pField, "2002") == 0 && pLoadDocAccessor->getInt(0, pField) == 2002);

    // bit字段/编码表字段
    pField = pLoadDocAccessor->getProfileField("bit_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed",  pLoadDocAccessor->setFieldValue(1, pField, "2") == 0  && pLoadDocAccessor->getInt(1, pField) == 2);

    // 非bit字段/编码表字段
    pField = pLoadDocAccessor->getProfileField("field_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed", pLoadDocAccessor->setFieldValue(3, pField, "381757985") == 0 && pLoadDocAccessor->getInt(3, pField) == 381757985);

    // 非bit字段/非编码表字段
    pField = pLoadDocAccessor->getProfileField("single_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt failed", pLoadDocAccessor->setFieldValue(4, pField, "757985") == 0 && pLoadDocAccessor->getInt(4, pField) == 757985);

    printf("process testSet_GetInt pass!\n");
}

void ProfileDocAccessorTest::testSet_GetUInt()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    // bit字段,超过字段范围
    pField = pDocAccessor->getProfileField("bit_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pDocAccessor->setFieldValue(0, pField, "7") != 0);

    // bit字段，未超过字段范围
    pField = pDocAccessor->getProfileField("bit_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pDocAccessor->setFieldValue(1, pField, "3") == 0);

    // 非bit字段/非编码表字段
    pField = pDocAccessor->getProfileField("field_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pDocAccessor->setFieldValue(4, pField, "7587") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pDocAccessor->setFieldValue(10, pField, "7587") != 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    // bit字段，未超过字段范围
    pField = pLoadDocAccessor->getProfileField("bit_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pLoadDocAccessor->getUInt(1, pField) == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pLoadDocAccessor->setFieldValue(1, pField, "2") == 0 && pLoadDocAccessor->getUInt(1, pField) == 2);

    // 非bit字段/非编码表字段
    pField = pLoadDocAccessor->getProfileField("field_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pLoadDocAccessor->getUInt(4, pField) == 7587);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt failed",  pLoadDocAccessor->setFieldValue(4, pField, "841") == 0 && pLoadDocAccessor->getUInt(4, pField) == 841);
    printf("process testSet_GetUInt pass!\n");
}

void ProfileDocAccessorTest::testSet_GetFloat()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    pField = pDocAccessor->getProfileField("bit_float");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pDocAccessor->setFieldValue(2, pField, "46871.6") == 0);

    pField = pDocAccessor->getProfileField("field_float");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pDocAccessor->setFieldValue(4, pField, "4871.6") == 0);
    
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_float");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pLoadDocAccessor->getFloat(2, pField) == 46871.6f );
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pLoadDocAccessor->setFieldValue(2, pField, "46831.6") == 0 && pLoadDocAccessor->getFloat(2, pField) == 46831.6f );

    pField = pLoadDocAccessor->getProfileField("field_float");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pLoadDocAccessor->getFloat(4, pField) == 4871.6f );
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetFloat failed", pLoadDocAccessor->setFieldValue(4, pField, "4271.6") == 0 && pLoadDocAccessor->getFloat(4, pField) == 4271.6f );
    printf("process testSet_GetFloat pass!\n");
}

void ProfileDocAccessorTest::testSet_GetDouble()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    pField = pDocAccessor->getProfileField("bit_double");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pDocAccessor->setFieldValue(2, pField, "46871.613467") == 0);

    pField = pDocAccessor->getProfileField("field_double");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pDocAccessor->setFieldValue(4, pField, "4871135.6") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_double");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pLoadDocAccessor->getDouble(2, pField) == 46871.613467 );
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pLoadDocAccessor->setFieldValue(2, pField, "46271.613467") == 0 && pLoadDocAccessor->getDouble(2, pField) == 46271.613467 );

    pField = pLoadDocAccessor->getProfileField("field_double");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pLoadDocAccessor->getDouble(4, pField) == 4871135.6 );
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetDouble failed", pLoadDocAccessor->setFieldValue(4, pField, "471135.6") == 0 && pLoadDocAccessor->getDouble(4, pField) == 471135.6 );
    printf("process testSet_GetDouble pass!\n");
}

void ProfileDocAccessorTest::testSet_GetString()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    pField = pDocAccessor->getProfileField("bit_string");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pDocAccessor->setFieldValue(0, pField, "pujian") == 0);

    pField = pDocAccessor->getProfileField("field_string");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pDocAccessor->setFieldValue(8, pField, "zhangyongwang") == 0);

    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pDocAccessor->setFieldValue(9, pField, "") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_string");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", strcmp(pLoadDocAccessor->getString(0, pField), "pujian") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pLoadDocAccessor->setFieldValue(0, pField, "pujian_bak") == 0 && strcmp(pLoadDocAccessor->getString(0, pField), "pujian_bak") == 0);

    pField = pLoadDocAccessor->getProfileField("field_string");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", strcmp(pLoadDocAccessor->getString(8, pField), "zhangyongwang") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pLoadDocAccessor->setFieldValue(8, pField, "zhangyongwang_bak") == 0 && strcmp(pLoadDocAccessor->getString(8, pField), "zhangyongwang_bak") == 0);

    CPPUNIT_ASSERT_MESSAGE("process testSet_GetString failed", pLoadDocAccessor->getString(9, pField) == NULL);
    printf("process testSet_GetString pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedFloat()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;

    pField = pDocAccessor->getProfileField("repeated_float");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedFloat failed", pDocAccessor->setFieldValue(9, pField, "4726.8 8472.2") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_float");
    pLoadDocAccessor->getRepeatedValue(9, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedFloat failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedFloat failed", iterator.nextFloat() == 4726.8f);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedFloat failed", iterator.nextFloat() == 8472.2f);
    printf("process testSet_GetRepeatedFloat pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedDouble()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;

    pField = pDocAccessor->getProfileField("repeated_double");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedDouble failed", pDocAccessor->setFieldValue(7, pField, "426.168 168472.2 8917.72") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_double");
    pLoadDocAccessor->getRepeatedValue(7, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedDouble failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedDouble failed", iterator.nextDouble() == 426.168);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedDouble failed", iterator.nextDouble() == 168472.2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedDouble failed", iterator.nextDouble() == 8917.72);
    printf("process testSet_GetRepeatedDouble pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedString()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;

    pField = pDocAccessor->getProfileField("repeated_String");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedString failed", pDocAccessor->setFieldValue(5, pField, "pujian zhangyongwang nihao") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_String");
    pLoadDocAccessor->getRepeatedValue(5, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedString failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedString failed", strcmp("pujian", iterator.nextString()) == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedString failed", strcmp("zhangyongwang", iterator.nextString()) == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedString failed", strcmp("nihao", iterator.nextString()) == 0);
    printf("process testSet_GetRepeatedString pass!\n");
}

void ProfileDocAccessorTest::testSet_GetInt8()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pDocAccessor->setFieldValue(8, pField, "74") == 0);

    pField = pDocAccessor->getProfileField("field_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pDocAccessor->setFieldValue(8, pField, "45") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->getInt8(8, pField) == 74);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->setFieldValue(8, pField, "71") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->getInt8(8, pField) == 71);

    pField = pLoadDocAccessor->getProfileField("field_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->getInt8(8, pField) == 45);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->setFieldValue(8, pField, "43") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt8 failed", pLoadDocAccessor->getInt8(8, pField) == 43);
    printf("process testSet_GetInt8 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetInt16()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_int16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pDocAccessor->setFieldValue(7, pField, "834") == 0);

    pField = pDocAccessor->getProfileField("single_int16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pDocAccessor->setFieldValue(6, pField, "176") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_int16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->getInt16(7, pField) == 834);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->setFieldValue(7, pField, "84") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->getInt16(7, pField) == 84);

    pField = pLoadDocAccessor->getProfileField("single_int16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->getInt16(6, pField) == 176);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->setFieldValue(6, pField, "16") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt16 failed", pLoadDocAccessor->getInt16(6, pField) == 16);
    printf("process testSet_GetInt16 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetInt32()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pDocAccessor->setFieldValue(5, pField, "26345") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pDocAccessor->setFieldValue(2, pField, "6345") == 0);

    pField = pDocAccessor->getProfileField("single_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pDocAccessor->setFieldValue(5, pField, "28692") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(5, pField) ==  26345);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(2, pField) ==  6345);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->setFieldValue(5, pField, "2345") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->setFieldValue(2, pField, "645") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(5, pField) ==  2345);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(2, pField) ==  645);

    pField = pLoadDocAccessor->getProfileField("single_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(5, pField) == 28692);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->setFieldValue(5, pField, "2862") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt32 failed", pLoadDocAccessor->getInt32(5, pField) == 2862);
    printf("process testSet_GetInt32 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetInt64()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pDocAccessor->setFieldValue(2, pField, "4796166") == 0);

    pField = pDocAccessor->getProfileField("field_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pDocAccessor->setFieldValue(0, pField, "7419") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->getInt64(2, pField) == 4796166);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->setFieldValue(2, pField, "796166") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->getInt64(2, pField) == 796166);

    pField = pLoadDocAccessor->getProfileField("field_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->getInt64(0, pField) == 7419);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->setFieldValue(0, pField, "741") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetInt64 failed", pLoadDocAccessor->getInt64(0, pField) == 741);
    printf("process testSet_GetInt64 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetUInt8()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pDocAccessor->setFieldValue(6, pField, "7") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pDocAccessor->setFieldValue(8, pField, "11") != 0);

    pField = pDocAccessor->getProfileField("field_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pDocAccessor->setFieldValue(1, pField, "96") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->getUInt8(6, pField) == 7);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->setFieldValue(6, pField, "4") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->getUInt8(6, pField) == 4);

    pField = pLoadDocAccessor->getProfileField("field_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->getUInt8(1, pField) == 96);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->setFieldValue(1, pField, "6") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt8 failed", pLoadDocAccessor->getUInt8(1, pField) == 6);
    printf("process testSet_GetUInt8 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetUInt16()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pDocAccessor->setFieldValue(2, pField, "30") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pDocAccessor->setFieldValue(4, pField, "32") != 0);

    pField = pDocAccessor->getProfileField("field_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pDocAccessor->setFieldValue(3, pField, "37") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();

    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();
    pField = pLoadDocAccessor->getProfileField("bit_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->getUInt16(2, pField) == 30);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->setFieldValue(2, pField, "20") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->getUInt16(2, pField) == 20);

    pField = pLoadDocAccessor->getProfileField("field_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->getUInt16(3, pField) == 37);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->setFieldValue(3, pField, "35") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt16 failed", pLoadDocAccessor->getUInt16(3, pField) == 35);
    printf("process testSet_GetUInt16 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetUInt32()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pDocAccessor->setFieldValue(7, pField, "1") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pDocAccessor->setFieldValue(2, pField, "4") != 0);

    pField = pDocAccessor->getProfileField("single_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pDocAccessor->setFieldValue(8, pField,"12368") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->getUInt32(7, pField) == 1);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->setFieldValue(7, pField, "0") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->getUInt32(7, pField) == 0);

    pField = pLoadDocAccessor->getProfileField("single_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->getUInt32(8, pField) == 12368);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->setFieldValue(8, pField,"1238") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt32 failed", pLoadDocAccessor->getUInt32(8, pField) == 1238);
    printf("process testSet_GetUInt32 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetUInt64()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    
    pField = pDocAccessor->getProfileField("bit_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pDocAccessor->setFieldValue(2, pField, "14") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pDocAccessor->setFieldValue(5, pField, "16") != 0);

    pField = pDocAccessor->getProfileField("field_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pDocAccessor->setFieldValue(7, pField, "171852782") == 0);
    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("bit_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->getUInt64(2, pField) == 14);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->setFieldValue(2, pField, "12") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->getUInt64(2, pField) == 12);

    pField = pLoadDocAccessor->getProfileField("field_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->getUInt64(7, pField) ==  171852782);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->setFieldValue(7, pField, "11852782") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetUInt64 failed", pLoadDocAccessor->getUInt64(7, pField) ==  11852782);
    printf("process testSet_GetUInt64 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedInt8()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("repeated_int8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", pDocAccessor->setFieldValue(2, pField, "7 57 71") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_int8");
    pLoadDocAccessor->getRepeatedValue(2, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", iterator.nextInt8() == 7);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", iterator.nextInt8() == 57);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", iterator.nextInt8() == 71);

    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", pLoadDocAccessor->setFieldValue(3, pField, "") == 0);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt8 failed", pLoadDocAccessor->getValueNum(3, pField) == 0);
    printf("process testSet_GetRepeatedInt8 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedInt16()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("field_int16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt16 failed", pDocAccessor->setFieldValue(4, pField, "481 651 7181") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("field_int16");
    pLoadDocAccessor->getRepeatedValue(4, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt16 failed", iterator.nextInt16() == 481);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt16 failed", iterator.nextInt16() == 651);
    printf("process testSet_GetRepeatedInt16 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedInt32()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("field_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt32 failed", pDocAccessor->setFieldValue(1, pField, "6719 176 -146") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("field_int32");
    pLoadDocAccessor->getRepeatedValue(1, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt32 failed", iterator.nextInt32() == 6719);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt32 failed", iterator.nextInt32() == 176);
    printf("process testSet_GetRepeatedInt32 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedInt64()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("repeated_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", pDocAccessor->setFieldValue(10, pField, "819687 7197") != 0);
    ProfileManager::getInstance()->newDocSegment(false);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", pDocAccessor->setFieldValue(10, pField, "819687 7197") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_int64");
    pLoadDocAccessor->getRepeatedValue(10, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", iterator.nextInt64() == 819687);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", iterator.nextInt64() == 7197);

    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", pDocAccessor->setFieldValue(10, pField, "81993687 7834197 83295") == 0);
    pLoadDocAccessor->getRepeatedValue(10, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", iterator.nextInt64() == 81993687);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", iterator.nextInt64() == 7834197);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedInt64 failed", iterator.nextInt64() == 83295);

    printf("process testSet_GetRepeatedInt64 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedUInt8()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("repeated_uint8");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt8 failed", pDocAccessor->setFieldValue(2, pField, "57 47 -1") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_uint8");
    pLoadDocAccessor->getRepeatedValue(2, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt8 failed", iterator.nextUInt8() == 57);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt8 failed", iterator.nextUInt8() == 47);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt8 failed", iterator.nextUInt8() == (uint8_t)(-1));
    printf("process testSet_GetRepeatedUInt8 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedUInt16()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("repeated_uint16");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt16 failed", pDocAccessor->setFieldValue(4, pField, "2631 7497 847") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_uint16");
    pLoadDocAccessor->getRepeatedValue(4, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt16 failed", iterator.nextUInt16() == 2631);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt16 failed", iterator.nextUInt16() == 7497);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt16 failed", iterator.nextUInt16() == 847);
    printf("process testSet_GetRepeatedUInt16 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedUInt32()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("field_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", pDocAccessor->setFieldValue(6, pField, "2631 74291") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("field_uint32");
    pLoadDocAccessor->getRepeatedValue(6, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.nextUInt32() == 2631);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.nextUInt32() == 74291);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt32 failed", iterator.nextUInt32() == pField->defaultEmpty.EV_UINT32);
    printf("process testSet_GetRepeatedUInt32 pass!\n");
}

void ProfileDocAccessorTest::testSet_GetRepeatedUInt64()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;
    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("repeated_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt64 failed", pDocAccessor->setFieldValue(9, pField, "83467987 978458126") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("repeated_uint64");
    pLoadDocAccessor->getRepeatedValue(9, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt64 failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt64 failed", iterator.nextUInt64() == 83467987);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedUInt64 failed", iterator.nextUInt64() == 978458126);
    printf("process testSet_GetRepeatedUInt64 pass!\n");
}


void ProfileDocAccessorTest::testSet_GetRepeatedVarint()
{
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
    const ProfileField *pField = NULL;

    ProfileMultiValueIterator iterator;
    
    pField = pDocAccessor->getProfileField("varint_int32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", pDocAccessor->setFieldValue(2, pField, "787364234 554436123 -3171") == 0);
    pField = pDocAccessor->getProfileField("varint_uint32");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", pDocAccessor->setFieldValue(3, pField, "98347598 5983475") == 0);
    pField = pDocAccessor->getProfileField("varint_int64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", pDocAccessor->setFieldValue(1, pField, "-1938578475 -298347598 8345983475") == 0);
    pField = pDocAccessor->getProfileField("varint_uint64");
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", pDocAccessor->setFieldValue(4, pField, "81938578475 6298347598 8345983475") == 0);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->dump();
    ProfileManager::freeInstance();

    ProfileManager *pLoadManager = ProfileManager::getInstance();
    pLoadManager->setProfilePath(".");
    pLoadManager->load();
    ProfileDocAccessor *pLoadDocAccessor = ProfileManager::getDocAccessor();

    pField = pLoadDocAccessor->getProfileField("varint_int32");
    pLoadDocAccessor->getRepeatedValue(2, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt32() == 787364234);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt32() == 554436123);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt32() == -3171);

    pField = pLoadDocAccessor->getProfileField("varint_uint32");
    pLoadDocAccessor->getRepeatedValue(3, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.getValueNum() == 2);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextUInt32() == 98347598);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextUInt32() == 5983475);

    pField = pLoadDocAccessor->getProfileField("varint_int64");
    pLoadDocAccessor->getRepeatedValue(1, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt64() == -1938578475);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt64() == -298347598);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextInt64() == 8345983475);

    pField = pLoadDocAccessor->getProfileField("varint_uint64");
    pLoadDocAccessor->getRepeatedValue(4, pField, iterator);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.getValueNum() == 3);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextUInt64() == 81938578475);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextUInt64() == 6298347598);
    CPPUNIT_ASSERT_MESSAGE("process testSet_GetRepeatedVarint failed", iterator.nextUInt64() == 8345983475);

    printf("process testSet_GetRepeatedVarint pass!\n");
}
