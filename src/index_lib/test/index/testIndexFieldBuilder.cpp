#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test.h"
#include "util/HashMap.hpp"
#include "IndexTermParse.h"
#include "IndexFieldBuilder.h"
#include "testIndexFieldBuilder.h"

using namespace index_lib;

testIndexFieldBuilder::testIndexFieldBuilder()
{
    _termNum = 0;
    _pTermSign = NULL;

    _pDocNumArray = NULL; 
    _ppDocArray = NULL;
    createDocInfo(MAX_TERM_NUM, 0);
}

testIndexFieldBuilder::~testIndexFieldBuilder()
{
    if (_pTermSign) delete _pTermSign;
    if (_pDocNumArray) delete _pDocNumArray;
    if (_ppDocArray) {
        for(int32_t i = 0; i < _termNum; i++) {
            if(_ppDocArray[i])delete[] _ppDocArray[i];
        }
        delete _ppDocArray;
    }
}

void testIndexFieldBuilder::setUp()
{
    remove(ENCODEFILE);
    test::mkdir(NORMAL_PATH);
}

void testIndexFieldBuilder::tearDown()
{
    remove(ENCODEFILE);
    test::rmdir(NORMAL_PATH);
}

void testIndexFieldBuilder::testOpen()
{
    char message[1024];
    {// 正常返回
        IndexFieldBuilder cBuilder(FIELDNAME, 1, _termNum, NULL);
        int ret = cBuilder.open(NORMAL_PATH);
        snprintf(message, sizeof(message), "ret=%d %s", ret, NORMAL_PATH);
        CPPUNIT_ASSERT_MESSAGE(message, 0 == ret);
    }

    {// 路径不存在， 或者没有权限
        IndexFieldBuilder cBuilder(FIELDNAME, 1, _termNum, NULL);
        int ret = cBuilder.open(NO_POWER_PATH);
        snprintf(message, sizeof(message), "ret=%d %s", ret, NORMAL_PATH);
        CPPUNIT_ASSERT_MESSAGE(message, 0 > ret);
    }
}

void testIndexFieldBuilder::testAddNullOcc()
{
    IndexFieldBuilder cBuilder(FIELDNAME, 0, MAX_DESIGN_CAPACITY, NULL);
    int ret = cBuilder.open(NORMAL_PATH);
    snprintf(_message, sizeof(_message), "%s ret=%d", NORMAL_PATH, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);

    uint32_t* pDocList = new uint32_t[MAX_DESIGN_CAPACITY];

    int32_t docNum = 0;
    uint64_t sign = 1;
    for (int32_t i = 0; i < MAX_DESIGN_CAPACITY; i++) {
        pDocList[i] = i;
        docNum++;
    }

    ret = cBuilder.addTerm(sign, pDocList, docNum);
    snprintf(_message, sizeof(_message), "true=%d, return=%d", docNum, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == docNum);
    delete[] pDocList;

    // 输入数据顺序不符合要求,数据错误，退出
    sign = 100;
    uint32_t pDocList1[] = {0, 8, 6};
    ret = cBuilder.addTerm(sign, pDocList1, 3);
    CPPUNIT_ASSERT(ret < 0);

    // docId 超过一定范围，退出
    sign = 100;
    uint32_t pDocList2[] = {0, 8, MAX_DESIGN_CAPACITY + 1};
    ret = cBuilder.addTerm(sign, pDocList2, 3);
    CPPUNIT_ASSERT(ret < 0);

    CPPUNIT_ASSERT(cBuilder.dump() == 0);
    CPPUNIT_ASSERT(cBuilder.close() == 0);
}

void testIndexFieldBuilder::testAddOneOcc()
{
    IndexFieldBuilder cBuilder(FIELDNAME, 1, MAX_DESIGN_CAPACITY, NULL);
    int ret = cBuilder.open(NORMAL_PATH);
    snprintf(_message, sizeof(_message), "%s ret=%d", NORMAL_PATH, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);

    DocListUnit* pDocList = new DocListUnit[MAX_DESIGN_CAPACITY];

    int32_t docNum = 0;
    uint64_t sign = 1;
    for (int32_t i = 0; i < MAX_DESIGN_CAPACITY; i++) {
        pDocList[i].doc_id = i;
        pDocList[i].occ = 0;
        docNum++;
    }

    ret = cBuilder.addTerm(sign, pDocList, docNum);
    snprintf(_message, sizeof(_message), "true=%d, return=%d", docNum, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == docNum);

    // 输入数据顺序不符合要求,数据错误，退出
    sign = 100;
    DocListUnit pDocList1[] = {{0, 7}, {8, 10}, {6, 11}};
    ret = cBuilder.addTerm(sign, pDocList1, 3);
    CPPUNIT_ASSERT(ret < 0);

    // docId 超过一定范围，退出
    DocListUnit pDocList2[] = {{0, 7}, {8, 10}, {MAX_DESIGN_CAPACITY + 1, 11}};
    ret = cBuilder.addTerm(sign, pDocList2, 3);
    CPPUNIT_ASSERT(ret < 0);

    // 重复的doc只会保留一个
    sign = 101;
    DocListUnit pDocList3[] = {{0, 7}, {0, 8}, {6, 11}, {6, 13}};
    ret = cBuilder.addTerm(sign, pDocList3, 4);
    CPPUNIT_ASSERT(ret == 2);


    CPPUNIT_ASSERT(cBuilder.dump() == 0);
    CPPUNIT_ASSERT(cBuilder.close() == 0);
    
    delete[] pDocList;
}

void testIndexFieldBuilder::testAddMulOcc()
{
    IndexFieldBuilder cBuilder(FIELDNAME, 255, MAX_DESIGN_CAPACITY, NULL);
    int ret = cBuilder.open(NORMAL_PATH);
    snprintf(_message, sizeof(_message), "%s ret=%d", NORMAL_PATH, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);

    DocListUnit* pDocList = new DocListUnit[MAX_DESIGN_CAPACITY];

    int32_t docNum = 0;
    uint64_t sign = 1;
    for (int32_t i = 0; i < MAX_DESIGN_CAPACITY; i++) {
        pDocList[i].doc_id = i;
        pDocList[i].occ = 0;
        docNum++;
    }

    ret = cBuilder.addTerm(sign, pDocList, docNum);
    snprintf(_message, sizeof(_message), "true=%d, return=%d", docNum, ret);
    CPPUNIT_ASSERT_MESSAGE(_message, ret == docNum);

    // 输入数据顺序不符合要求,数据错误，退出
    sign = 100;
    DocListUnit pDocList1[] = {{0, 7}, {8, 10}, {6, 11}};
    ret = cBuilder.addTerm(sign, pDocList1, 3);
    CPPUNIT_ASSERT(ret < 0);

    // docId 超过一定范围，退出
    DocListUnit pDocList2[] = {{0, 7}, {8, 10}, {MAX_DESIGN_CAPACITY + 1, 11}};
    ret = cBuilder.addTerm(sign, pDocList2, 3);
    CPPUNIT_ASSERT(ret < 0);

    // 重复的doc只会保留一个
    sign = 101;
    DocListUnit pDocList3[] = {{0, 7}, {0, 8}, {6, 11}, {6, 13}};
    ret = cBuilder.addTerm(sign, pDocList3, 4);
    CPPUNIT_ASSERT(ret == 4);


    CPPUNIT_ASSERT(cBuilder.dump() == 0);
    CPPUNIT_ASSERT(cBuilder.close() == 0);
    
    delete[] pDocList;
}

void testIndexFieldBuilder::testBuildIndex()
{
    int32_t maxOccNum[3] = {0, 1, 255};

    for(int x = 0; x < 3; x++) {
        DocListUnit* pDocUnit = NULL;
        uint32_t* pDocList = NULL;
        int maxListNum = MAX_DOC_NUM * 1.5;
        if(maxOccNum[x] > 0) {
            pDocUnit = new DocListUnit[maxListNum];
        } else {
            pDocList = new uint32_t[MAX_DOC_NUM];
        }
        
        std::vector <int32_t> vect;
        IndexFieldBuilder cBuilder(FIELDNAME, maxOccNum[x], MAX_DOC_NUM, NULL);
        int ret = cBuilder.open(NORMAL_PATH);
        snprintf(_message, sizeof(_message), "%s ret=%d", NORMAL_PATH, ret);
        CPPUNIT_ASSERT_MESSAGE(_message, ret == 0);

        srand(time(NULL));

        for(int i = 0; i < _termNum; i++) {
            int listNum = 0;
            int docNum = _pDocNumArray[i];
            int j = 0;
            if(maxOccNum[x] > 0) {
                for(j = 0; j < docNum && listNum < maxListNum; j++) {
                    vect.clear();
                    for(int k = 0; k < OCC_NUM; k++) {
                        vect.push_back(rand() % MAX_OCC_NUM);
                    }
                    std::sort(vect.begin(), vect.end());

                    for(int k = 0; k < vect.size() && listNum < maxListNum; k++, listNum++) {
                        pDocUnit[listNum].doc_id =  _ppDocArray[i][j];
                        pDocUnit[listNum].occ = vect[k];
                    }
                }
                if(j < docNum) docNum = j;
                ret = cBuilder.addTerm(_pTermSign[i], pDocUnit, listNum);
            } else {
                for(j = 0; j < docNum; j++, listNum++) {
                    pDocList[listNum] =  _ppDocArray[i][j];
                }
                ret = cBuilder.addTerm(_pTermSign[i], pDocList, listNum);
            }

            snprintf(_message, sizeof(_message), "maxocc=%d, i = %d ret=%d", maxOccNum[x], i, ret);
            if(maxOccNum[x] > 1) {
                CPPUNIT_ASSERT_MESSAGE(_message, ret == listNum);
            } else {
                CPPUNIT_ASSERT_MESSAGE(_message, ret == docNum);
            }
        }
        CPPUNIT_ASSERT(cBuilder.dump() == 0);
        CPPUNIT_ASSERT(cBuilder.close() == 0);
    
        if(pDocUnit) delete[] pDocUnit;
        if(pDocList) delete[] pDocList;
    }
}

void testIndexFieldBuilder::testAbandon()
{
    IndexFieldBuilder builder("abc", 1, 100, NULL);
    CPPUNIT_ASSERT(builder.open(NORMAL_PATH) >= 0);
    /*
       char line[1024];
       int len, ret;
       uint32_t base_num1, useLen1, doc_count1; 
       uint32_t base_num2, useLen2, doc_count2; 

       len = sprintf(line, "1 1 0 2 0 10 0");
       int ret = builder->buildNZipIndex(&base_num1, builder->pbuilder->raw_buf,
       builder->pbuilder->raw_buf_size, useLen1);
       CPPUNIT_ASSERT(ret > 0);
       doc_count1 = ret;

       ret = builder->abandonOverflow(builder->pbuilder->raw_buf, uint32_t& baseNum, uint32_t& useBufLen);
       (&base_num1, builder->pbuilder->raw_buf,
       builder->pbuilder->raw_buf_size, useLen1);
       CPPUNIT_ASSERT(ret > 0);
       doc_count1 = ret;
       */
}

int testIndexFieldBuilder::createDocInfo(int termNum, int docNum)
{
    _termNum = termNum;
    _pTermSign = new uint64_t[_termNum];
    _pDocNumArray = new int32_t[_termNum];
    _ppDocArray = new int32_t*[_termNum];

    srand(time(NULL));
    uint64_t sign = rand() - _termNum;
    util::HashMap<int32_t, int32_t> cHash;

    for(int i = 0; i < _termNum; i++) {
        _pTermSign[i] = sign + i;

        docNum = rand() % MAX_DOC_NUM;
        if(docNum <= 0) docNum++;

        cHash.clear();
        for(int j = 0; j < docNum; j++) {
            int32_t doc = rand() % MAX_DOC_NUM;

            util::HashMap<int32_t, int32_t>::iterator pNode  = cHash.find(doc);
            if (pNode != cHash.end()) {
                pNode->value++;
            } else {
                int32_t value = 1;
                cHash.insert(doc, value);
            }
        }

        int32_t value;
        _pDocNumArray[i] = cHash.size();
        _ppDocArray[i] = new int32_t[_pDocNumArray[i]];
        cHash.itReset();
        std::vector < int > vect;
        for(uint32_t j = 0; j < cHash.size(); j++) {
            cHash.itNext(_ppDocArray[i][j], value);
            vect.push_back(_ppDocArray[i][j]);
        }
        std::sort(vect.begin(), vect.end());
        for(uint32_t j = 0; j < cHash.size(); j++) {
            _ppDocArray[i][j] = vect[j];
        }
    }
    return 0;
}


