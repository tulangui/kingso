#include "ProfileEncodeFileTest.h"
#include <stdio.h>

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(ProfileEncodeFileTest, "encode_file");

char* intValue[8] = {
    "10 20 30 40",
    "1 2 3 4 5 6 7",
    "100 200 300 400 600",
    "100 50 30 20 0",
    "18 17 16 14 15",
    "1 3 5 7 9",
    "2 10 18",
    ""
};

char* strValue[8] = {
    "kingso aksearch hello world",
    "nihao  erbao  shituo pujian",
    "gongyi gaochao",
    "yewang santai zhanyuan",
    "wufu guijiao hashtable",
    "qiaoyin qianshi pianqian",
    "yinnong jiayi",
    ""
};

char* singleFloatValue[8] = {
    "14.68",
    "235.1",
    "8923.84",
    "74134.1",
    "8215.2",
    "49.23",
    "782.22",
    ""
};

char* singleStrValue[8] = {
    "nihao",
    "pujian",
    "zywlovexiuping",
    "abcdefg",
    "a",
    "b",
    "c",
    ""
};

void ProfileEncodeFileTest::setUp()
{
    EmptyValue  em_value;
    em_value.EV_FLOAT = 0;

    _baseValueFile       = new ProfileEncodeFile(DT_INT32, ".", "field_value", em_value, false);
    _stringFile          = new ProfileEncodeFile(DT_STRING, ".", "field_string", em_value, false);
    _singleStringFile    = new ProfileEncodeFile(DT_STRING, ".", "field_single_str", em_value, true);
    _singleBaseValueFile = new ProfileEncodeFile(DT_FLOAT, ".", "field_single_base", em_value, true);
}

void ProfileEncodeFileTest::tearDown()
{
    if (_baseValueFile != NULL) {
        delete _baseValueFile;
        _baseValueFile = NULL;
    }

    if (_stringFile != NULL) {
        delete _stringFile;
        _stringFile = NULL;
    }

    if (_singleStringFile != NULL) {
        delete _singleStringFile;
        _singleStringFile = NULL;
    }

    if (_singleBaseValueFile != NULL) {
        delete _singleBaseValueFile;
        _singleBaseValueFile = NULL;
    }
}

void ProfileEncodeFileTest::testCreateFile()
{
    bool ret = _baseValueFile->createEncodeFile(true, false, 500001, 200000);
    CPPUNIT_ASSERT_MESSAGE( "process testCreateFile failed", ret);

    ret = _stringFile->createEncodeFile(false, true, 500001, 200000);
    CPPUNIT_ASSERT_MESSAGE( "process testCreateFile failed", ret);

    ret = _singleStringFile->createEncodeFile(false, true, 500001, 200000);
    CPPUNIT_ASSERT_MESSAGE( "process testCreateFile failed", ret);

    ret = _singleBaseValueFile->createEncodeFile(false, true, 500001, 200000);
    CPPUNIT_ASSERT_MESSAGE( "process testCreateFile failed", ret);
    printf("process testCreateFile pass!\n");
}

void ProfileEncodeFileTest::testDumpFile()
{
    testAddEncode();
    bool ret = _baseValueFile->dumpEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testDumpFile failed", ret);

    ret = _stringFile->dumpEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testDumpFile failed", ret);

    ret = _singleStringFile->dumpEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testDumpFile failed", ret);

    ret = _singleBaseValueFile->dumpEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testDumpFile failed", ret);
    printf("process testDumpFile pass!\n");
}

void ProfileEncodeFileTest::testLoadFile()
{
    bool ret = _baseValueFile->loadEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testLoadFile failed", ret);

    ret = _stringFile->loadEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testLoadFile failed", ret);

    ret = _singleStringFile->loadEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testLoadFile failed", ret);

    ret = _singleBaseValueFile->loadEncodeFile();
    CPPUNIT_ASSERT_MESSAGE( "process testLoadFile failed", ret);
    printf("process testLoadFile pass!\n");
}

void ProfileEncodeFileTest::testGetDataType()
{
    PF_DATA_TYPE valuetype = _baseValueFile->getDataType();
    PF_DATA_TYPE strtype   = _stringFile->getDataType();
    PF_DATA_TYPE singlestrtype   = _singleStringFile->getDataType();
    PF_DATA_TYPE singleBasetype  = _singleBaseValueFile->getDataType();

    CPPUNIT_ASSERT_MESSAGE( "process testGetDataType failed", valuetype == DT_INT32 && strtype == DT_STRING && singlestrtype == DT_STRING && singleBasetype == DT_FLOAT);
    
    printf("process testGetDataType pass!\n");
}

void ProfileEncodeFileTest::testGetUnitSize()
{
    testAddEncode();
    size_t value_size       = _baseValueFile->getUnitSize();
    size_t str_size         = _stringFile->getUnitSize();
    size_t single_str_size  = _singleStringFile->getUnitSize();
    size_t single_base_size = _singleBaseValueFile->getUnitSize();

    CPPUNIT_ASSERT_MESSAGE( "process testGetUnitSize failed", value_size == 1 && str_size == 1 && single_str_size == 8 && single_base_size == 4);
    
    printf("process testGetUnitSize pass!\n");
}


void ProfileEncodeFileTest::testAddEncode()
{
    testCreateFile();
    for(int pos=0; pos<8; pos++) {
        CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", _stringFile->addEncode(strValue[pos], strlen(strValue[pos]), offset1[pos]));
        CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", _baseValueFile->addEncode(intValue[pos], strlen(intValue[pos]), offset2[pos]));
        CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", _singleStringFile->addEncode(singleStrValue[pos], strlen(singleStrValue[pos]), offset3[pos]));
        CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", _singleBaseValueFile->addEncode(singleFloatValue[pos], strlen(singleFloatValue[pos]), offset4[pos]));
    }

    uint32_t offset = 0;
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_baseValueFile->addEncode(intValue[1], strlen(intValue[1]), offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_stringFile->addEncode(strValue[2], strlen(strValue[2]), offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_singleBaseValueFile->addEncode(singleFloatValue[1], strlen(singleFloatValue[1]), offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_singleStringFile->addEncode(singleStrValue[2], strlen(singleStrValue[2]), offset));

    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_baseValueFile->addEncode("", 0, offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_stringFile->addEncode("", 0, offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_singleBaseValueFile->addEncode("", 0, offset));
    CPPUNIT_ASSERT_MESSAGE( "process testAddEncode failed", !_singleStringFile->addEncode("", 0, offset));
    printf("process testAddEncode pass!\n");
}

void ProfileEncodeFileTest::testGetEncodeIdx()
{
    testAddEncode();
    uint32_t str_encode, value_encode, single_str_encode, float_encode;
    str_encode        = _stringFile->getEncodeIdx      (strValue[5], strlen(strValue[5]));
    value_encode      = _baseValueFile->getEncodeIdx   (intValue[3], strlen(intValue[3]));
    single_str_encode = _singleStringFile->getEncodeIdx(singleStrValue[2], strlen(singleStrValue[2]));
    float_encode      = _singleBaseValueFile->getEncodeIdx   (singleFloatValue[7], strlen(singleFloatValue[7]));

    CPPUNIT_ASSERT_MESSAGE( "process testGetEncodeIdx failed", str_encode == offset1[5] && value_encode == offset2[3] && single_str_encode == offset3[2] && float_encode == offset4[7]);

    str_encode        = _stringFile->getEncodeIdx("", 0);
    value_encode      = _baseValueFile->getEncodeIdx("", 0);
    single_str_encode = _singleStringFile->getEncodeIdx("", 0);
    float_encode      = _singleBaseValueFile->getEncodeIdx("", 0);
    CPPUNIT_ASSERT_MESSAGE( "process testGetEncodeIdx failed", str_encode == offset1[7] && value_encode == offset2[7] && single_str_encode == offset3[7] && float_encode == offset4[7]);
    
    str_encode        = _stringFile->getEncodeIdx("alsdf", strlen("alsdf"));
    value_encode      = _baseValueFile->getEncodeIdx("17190 7195", strlen("17190 7195"));
    single_str_encode = _singleStringFile->getEncodeIdx("dksdf", strlen("dksdf"));
    float_encode      = _singleBaseValueFile->getEncodeIdx("79128.9", strlen("79128.9"));
    CPPUNIT_ASSERT_MESSAGE( "process testGetEncodeIdx failed", str_encode == INVALID_ENCODEVALUE_NUM && value_encode == INVALID_ENCODEVALUE_NUM && single_str_encode == INVALID_ENCODEVALUE_NUM && float_encode == INVALID_ENCODEVALUE_NUM);
    
    printf("process testGetEncodeIdx pass!\n");
}

void ProfileEncodeFileTest::testGetEncodeValueNum()
{
    testDumpFile();
    testLoadFile();
    uint32_t strNum, valueNum;

    strNum   = _stringFile->getMultiEncodeValueNum(offset1[2]);
    valueNum = _baseValueFile->getMultiEncodeValueNum(offset2[6]);
    CPPUNIT_ASSERT_MESSAGE( "process testGetEncodeValueNum failed", strNum == 2 && valueNum == 3);
    printf("process testGetEncodeValueNum pass!\n");
}
