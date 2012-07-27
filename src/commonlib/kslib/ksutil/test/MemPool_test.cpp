#include "util/MemPool.h"
#include <stdio.h>    // 仅用于测试
#include <assert.h>   // 仅用于测试

// 测试用例，兼做使用说明。
class MyClass
{
public:
    MyClass()
    { 
        m_Count = 0;
    }
    MyClass(int i)
    { 
        m_Count = i;
    }
    int getCount()
    {
        return m_Count;
    }
private:
    int m_Count;
};
int main()
{
    // TestCase Group1:
    MemPool objMemPool;
    MemPool *pMemPool = &objMemPool;
    int32_t nCount = 10;
    char *pszArray = NEW_VEC(pMemPool, char, nCount);
    int32_t *pnArray = NEW_VEC(pMemPool, int32_t, nCount);
    float *pfArray = NEW_VEC(pMemPool, float, nCount);
    for (int i = 0; i < nCount; i++) {
        pszArray[i] = 'a' + i;
        pnArray[i] = i;
        pfArray[i] = 1.0 * i;
    }
    for (int i = 0; i < nCount; i++) {
        fprintf(stdout, "pszArray[%d]:\t%c\n", i, pszArray[i]);
        fprintf(stdout, "pnArray[%d]:\t%d\n", i, pnArray[i]);
        fprintf(stdout, "pfArray[%d]:\t%f\n", i, pfArray[i]);
    }
    pMemPool->reset();

    // TestCase Group2:
    MemPool objMemPool2;
    MemPool *pMemPool2 = &objMemPool2;
    int32_t nCount2 = 10;
    MyClass *pMyClass = NEW(pMemPool2, MyClass);
    fprintf(stdout, "pMyClass:\t%d\n", pMyClass->getCount());
    MyClass *pMyClass2 = NEW(pMemPool2, MyClass)(100);
    fprintf(stdout, "pMyClass2:\t%d\n", pMyClass2->getCount());
    MyClass *pMyClassArray = NEW_ARRAY(pMemPool2, MyClass, nCount2);
    for (int i = 0; i < nCount2; i++) {
        fprintf(stdout, "pMyClassArray[%d]:\t%d\n", i, pMyClassArray[i].getCount());
    }
    pMemPool2->reset();

    // TestCase Group3:
    MemPool objMemPool3;
    MemPool *pMemPool3 = &objMemPool3;
    MemMonitor memMonitor(pMemPool3, 1);
    pMemPool3->setMonitor(&memMonitor);
    memMonitor.enableException();
    int32_t nCount3 = 1;
    char *pszArray3 = NEW_VEC(pMemPool3, char, nCount3);
    if (!pszArray3) {
        printf("pszArray3 NEW_VEC Err!\n");
    }
    nCount3 = 1024;
    int32_t *pnArray3 = NEW_VEC(pMemPool3, int32_t, nCount3);
    if (!pnArray3) {
        printf("pnArray3 NEW_VEC Err!\n");
    }
    float *pfArray3 = NEW_VEC(pMemPool3, float, nCount3);
    if (!pfArray3) {
        printf("pfArray3 NEW_VEC Err!\n");
    }
    for (int i = 0; i < nCount3; i++) {
        pszArray3[i] = 'a' + i;
        pnArray3[i] = i;
        pfArray3[i] = 1.0 * i;
    }
    for (int i = 0; i < nCount3; i++) {
        fprintf(stdout, "pszArray3[%d]:\t%c\n", i, pszArray3[i]);
        fprintf(stdout, "pnArray3[%d]:\t%d\n", i, pnArray3[i]);
        fprintf(stdout, "pfArray3[%d]:\t%f\n", i, pfArray3[i]);
    }
    pMemPool3->reset();

    // TestCase Group4:
    MemPool objMemPool4;
    MemPool *pMemPool4 = &objMemPool4;
    MemMonitor memMonitor4(pMemPool4, 1);
    pMemPool4->setMonitor(&memMonitor4);
    memMonitor4.enableException();
    
    int32_t nCountsz4 = 1024;
    char *pszArraysz4 = NEW_VEC(pMemPool4, char, nCountsz4);
    if (!pszArraysz4) {
        printf("pszArraysz4 is NULL\n");
    }
    MyClass *pMyClass4 = NEW(pMemPool4, MyClass);
    if (!pMyClass4) {
        printf("pMyClass4 is NULL\n");
    }
    fprintf(stdout, "pMyClass4:\t%d\n", pMyClass4->getCount());

    int32_t nCountsz4_1 = 1024;
    char *pszArraysz4_1 = NEW_VEC(pMemPool4, char, nCountsz4_1);
    if (!pszArraysz4_1) {
        printf("pszArraysz4_1 is NULL\n");
    }
    MyClass *pMyClass4_1 = NEW(pMemPool4, MyClass)(100);
    if (!pMyClass4_1) {
        printf("pMyClass4_1 is NULL\n");
    }
    fprintf(stdout, "pMyClass4_1:\t%d\n", pMyClass4_1->getCount());
    
    int32_t nCountsz4_2 = 1024;
    char *pszArraysz4_2 = NEW_VEC(pMemPool4, char, nCountsz4_2);
    if (!pszArraysz4_2) {
        printf("pszArraysz4_2 is NULL\n");
    }
    int32_t nCount4_2 = 1;
    MyClass *pMyClassArray4_2 = NEW_ARRAY(pMemPool4, MyClass, nCount4_2);
    if (!pMyClassArray4_2) {
        printf("pMyClassArray4_2 is NULL\n");
    }
    for (int i = 0; i < nCount4_2; i++) {
        fprintf(stdout, "pMyClassArray4_2[%d]:\t%d\n", i, pMyClassArray4_2[i].getCount());
    }
    pMemPool4->reset();

    return 0;
}

