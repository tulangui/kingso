#include "template.h"
#include "test.h"

templateTest::templateTest()
{
    uint64_t seed = 1;
    for(int32_t i = 0; i < 64; i++) {
        _bitRul[i] = seed<<i;
    }
}

templateTest::~templateTest()
{
}

void templateTest::testBitflow()
{
    struct useBit {
        int32_t one : 2; 
    };

    useBit a;
    a.one = 5;
    char message[10240];
    sprintf(message, "bit over flow %d\n", a.one);
    printf("bit over flow %d\n", a.one);
    int32_t value = 5;
    CPPUNIT_ASSERT_MESSAGE(message, a.one != value);
}

void templateTest::testBSearch()
{
    uint32_t array[] = {0, 1, 4, 8, 10, 11, 12, 13, 14, 0xffff0000}; 
    char message[10240];

    uint32_t ret, min = array[0], no = 0, key;
    uint32_t num = sizeof(array) / sizeof(uint32_t);
    for(int i = 0; i < 100; i++) {
        key = i;
        ret = test::bsearch<uint32_t>(array, num, key);
        sprintf(message, "key=%d, ret=%d", i, ret);
        CPPUNIT_ASSERT_MESSAGE(message, ret == min);
        if(i == min) {
            min = array[++no]; 
        } 
    }
}

// 具体宏信息参照
// /usr/include/cppunit/TestAssert.h
void templateTest::testOverflow()
{
    uint64_t seed = 1;
    CPPUNIT_ASSERT((1<<31) == (int32_t)(seed<<31));
    CPPUNIT_ASSERT((0x1ll<<32) == (seed<<32));

    CPPUNIT_ASSERT_EQUAL(1, 1);
}

void templateTest::testOperatorBit()
{
#define RUN_NUM 1 

    uint64_t bitRul[64];
    for(int32_t i = 0; i < 64; i++) {
        bitRul[i] = _bitRul[i];
    }

    srand(time(NULL));
    int32_t num = (80<<20)>>(3 + 3);
    uint64_t* pBitMap = new uint64_t[num];
    for(int32_t i = 0; i < num; i++) {
        pBitMap[i] = rand() * rand() * rand();
    }

    int64_t bitNum1 = 0, bitNum2 = 0;
    uint64_t seed = 1;
    time_t begin, end;

    begin = time(NULL);
    for(int32_t i = 0; i < RUN_NUM; i++) {
        for(int32_t j = 0; j < num; j++) {
            for(int32_t k = 0; k < 64; k++) {
                if(pBitMap[j]&_bitRul[k])
                    bitNum1++;
            }
        }
    }
    end = time(NULL);
    printf("class member use time = %ld, %ld\n", end - begin, bitNum1);

    bitNum2 = bitNum1;
    bitNum1 = 0;

    begin = time(NULL);
    for(int32_t i = 0; i < RUN_NUM; i++) {
        for(int32_t j = 0; j < num; j++) {
            for(int32_t k = 0; k < 64; k++) {
                if(pBitMap[j]&bitRul[k])
                    bitNum1++;
            }
        }
    }
    end = time(NULL);
    printf("local member use time = %ld, %ld\n", end - begin, bitNum1);

    CPPUNIT_ASSERT(bitNum1 == bitNum2);
    bitNum2 = bitNum1;
    bitNum1 = 0;

    begin = time(NULL);
    for(int32_t i = 0; i < RUN_NUM; i++) {
        for(int32_t j = 0; j < num; j++) {
            for(int32_t k = 0; k < 64; k++) {
                if(pBitMap[j]&(0x1ll<<k))
                    bitNum1++;
            }
        }
    }
    end = time(NULL);
    printf("tmp cal use time = %ld, %ld\n", end - begin, bitNum1);

    CPPUNIT_ASSERT(bitNum1 == bitNum2);
    bitNum2 = bitNum1;
    bitNum1 = 0;

    begin = time(NULL);
    for(int32_t i = 0; i < RUN_NUM; i++) {
        for(int32_t j = 0; j < num; j++) {
            for(int32_t k = 0; k < 64; k++) {
                if(pBitMap[j]&(seed<<k))
                    bitNum1++;
            }
        }
    }
    end = time(NULL);
    printf("tmp cal(var) use time = %ld, %ld\n", end - begin, bitNum1);

    CPPUNIT_ASSERT(bitNum1 == bitNum2);
    bitNum2 = bitNum1;
    bitNum1 = 0;

    delete[] pBitMap;
}

