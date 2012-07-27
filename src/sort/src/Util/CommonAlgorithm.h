/*********************************************************************
 * 
 * File:	通用算法函数集合
 * Desc:	二分查找算法,一些仿函数等
 * Log :
 * 			Create by victor, 2007/4/23
 *
 *********************************************************************/
#ifndef __COMMON_ALGORITHM_H
#define __COMMON_ALGORITHM_H
#include <stdint.h>
#include <string.h>

/* 哈希函数模板类声明 */
template <typename Key>
struct KeyHash 
{
};

/* 字符类型哈希函数 */
template<>
struct KeyHash<char> 
{
    inline uint32_t operator()(char cKey) const
    {
        uint32_t nHash = cKey;
        return nHash;
    }
};
/* 字符串哈希函数 */
template<> 
struct KeyHash<char*> 
{
    inline uint32_t operator()(char* szKey) const
    { 
        uint32_t res = 0;
        const unsigned char *p;

        p = reinterpret_cast<const unsigned char *>(szKey);
        while (*p) {
            res = (res << 7) + (res >> 25) + *p++;
        }

        return res;
    }
};
/* 常量字符串哈希函数 */
template<> 
struct KeyHash<const char*> 
{
    inline uint32_t operator()(const char* szKey) const
    { 
        uint32_t res = 0;
        const unsigned char *p;

        p = reinterpret_cast<const unsigned char *>(szKey);
        while (*p) {
            res = (res << 7) + (res >> 25) + *p++;
        }

        return res; 
    }
};

/* 整数哈希函数 */
template<> 
struct KeyHash<int32_t>
{
    inline uint32_t operator()(int32_t nKey) const
    { 
        return nKey; 
    }
};
/* 无符号整数哈希函数 */
template<> 
struct KeyHash<uint32_t>
{
    inline uint32_t operator()(uint32_t nKey) const
    { 
        return nKey;
    }
};

/* 无符号64位整数哈希函数 */
template<>
struct KeyHash<uint64_t> 
{
    inline uint32_t operator()(uint64_t nKey) const
    {
        /*nKey = (~nKey) + (nKey << 18); // nKey = (nKey << 18) - nKey - 1;
          nKey = nKey ^ (nKey >> 31);
          nKey = nKey * 21; // nKey = (nKey + (nKey << 2)) + (nKey << 4);
          nKey = nKey ^ (nKey >>11);
          nKey = nKey + (nKey << 6);
          nKey = nKey ^ (nKey >>22); */
        return (uint32_t)(nKey);
    }
};
/* 带符号64位整数哈希函数 */
template<>
struct KeyHash<int64_t> 
{
    inline uint32_t operator()(int64_t nKey) const
    {
        return (uint32_t)(nKey);
    }
};
/* 等于比较函数 */
template <typename Key>
struct KeyEqual {
};
/* 字符类型比较函数 */
template<>
struct KeyEqual<char> 
{
    inline bool operator()(char cKey1, char cKey2) const 
    {
        return cKey1 == cKey2;
    }
};
/* 字符串比较函数 */
template<> 
struct KeyEqual<char*> 
{
    inline bool operator()(char* szKey1, char* szKey2) const 
    { 
        return strcmp(szKey1,szKey2) == 0;
    }
};
/* 常量字符串比较函数 */
template<>
struct KeyEqual<const char*> 
{
    inline bool operator()(const char* szKey1, const char* szKey2) const 
    { 
        return strcmp(szKey1,szKey2) == 0;
    }
};

/* 整数比较函数 */
template<>
struct KeyEqual<int32_t>
{
    inline bool operator()(int32_t nKey1, int32_t nKey2) const
    { 
        return nKey1 == nKey2;
    }
};
/* 无符号整数比较函数 */
template<> 
struct KeyEqual<uint32_t>
{
    inline bool operator()(uint32_t nKey1, uint32_t nKey2) const
    { 
        return nKey1 == nKey2;
    }
};
/* 长整数比较函数,如果是32位机器,long是32位的,如果是64位机器,long是64位的 */
template<>
struct KeyEqual<int64_t>
{
    inline bool operator()(int64_t nKey1, int64_t nKey2) const
    {
        return nKey1 == nKey2;
    }
};

/* 无符号64位整数比较函数 */
template<>
struct KeyEqual<uint64_t> 
{
    inline uint32_t operator()(uint64_t nKey1, uint64_t nKey2) const
    {
        return nKey1 == nKey2;
    }
};

/* lessThan比较模板类 */
template <typename Key>
struct KeyCompare {
};
/* 字符串比较函数,小于返回true */
template<> 
struct KeyCompare<char*> 
{
    inline bool operator()(char* szKey1, char* szKey2) const 
    { 
        return strcmp(szKey1,szKey2) < 0;
    }
};
/* 常量字符串比较函数,小于返回true */
template<> 
struct KeyCompare<const char*> 
{
    inline bool operator()(const char* szKey1, const char* szKey2) const 
    { 
        return strcmp(szKey1,szKey2) < 0;
    }
};
/* 整数类型比较函数,小于返回true */
template<> 
struct KeyCompare<int32_t>
{
    inline bool operator()(int32_t nKey1, int32_t nKey2) const
    { 
        return nKey1 < nKey2;
    }
};
/* 无符号整数类型比较函数,小于返回true */
template<> 
struct KeyCompare<uint32_t>
{
    inline bool operator()(const uint32_t nKey1, const uint32_t nKey2) const
    { 
        return nKey1 < nKey2;
    }
};

/* 二分查找函数,返回不小于比较值的数组元素 */
template < typename Type, typename Compare = KeyCompare<Type> >
struct BinarySearch 
{
    Compare comp;
    inline int32_t operator()(Type *szElements, int32_t nBegin, int32_t nEnd, Type _Value)
    {
        int32_t nPos;
        while (nBegin <= nEnd) {
            nPos = (nBegin + nEnd) / 2;
            if (!comp(szElements[nPos], _Value)) {
                nEnd = nPos - 1;
            }
            else {
                nBegin = nPos + 1;
            }
        }
        return nBegin; 
    }
};

/* 快速排序模板类 */
template < typename Type, typename Compare = KeyCompare<Type> >
struct QuickSort {
    Compare comp;
    inline void operator()(Type *pArray, int32_t nBegin, int32_t nEnd)
    {
        QuickSort<Type,Compare> quickSort;
        while (nBegin < nEnd) 
        {
            /* quickly sort short lists */
            if (nEnd - nBegin <= 12) {
                insertSort(pArray, nBegin, nEnd);
                return;
        }
            /* 分为两部分 */
            int32_t nMid = partition(pArray, nBegin, nEnd);
            /* 排序较小的部分 */
            if (nMid - nBegin <= nEnd - nMid) {
                quickSort(pArray, nBegin, nMid - 1);
                nBegin = nMid + 1;
            } else {
                quickSort(pArray, nMid + 1, nEnd);
                nEnd = nMid - 1;
            }
        }
    }
    inline int32_t partition(Type *pArray, int32_t nBegin, int32_t nEnd)
    {
        Type t, pivot;
        int32_t i, j, p;

        p = nBegin + ((nEnd - nBegin)>>1);
        pivot = pArray[p];
        pArray[p] = pArray[nBegin];

        i = nBegin + 1;
        j = nEnd;
        while (true) {
            while (i < j && !comp(pivot, pArray[i])) {
                i++;
            }
            while (j >= i && !comp(pArray[j], pivot)) {
                j--;
            }
            if (i >= j) {
                break;
            }
            t = pArray[i];
            pArray[i] = pArray[j];
            pArray[j] = t;
            j--; i++;
        }
        pArray[nBegin] = pArray[j];
        pArray[j] = pivot;

        return j;
    }
    inline void insertSort(Type *pArray, int32_t nBegin, int32_t nEnd)
    {
        int32_t i, j;
        Type t;

        for (i = nBegin + 1; i <= nEnd; i++) {
            t = pArray[i];
            for (j = i-1; j >= nBegin && !comp(pArray[j], t); j--) {
                pArray[j+1] = pArray[j];
            }
            /* insert */
            pArray[j+1] = t;
        }
    }
};

/*  二分查找函数, 返回 等于比较值的位置 或 NULL, 在索引中使用.
    pIdxAddr_TermOffsetBegin    是该Term在Term数组中开始位置指针, 
    nTermTotalCount     是Term的个数, 
    value   是要查找的KeyHash值,
    nTermSize   是Term的大小.
    调用:   BinarySearchIndex bsearch;
    char *pIdxAddr_Term = bsearch(pIdxAddr_TermOffsetBegin, 10, val, sizeof(SExtPosKeywordTerm));
    */
template < typename KeyHashType >
struct BinarySearchIndex 
{
    inline char* operator()(char *pIdxAddr_TermOffsetBegin, const int32_t nTermTotalCount,
                            const KeyHashType value, const int32_t nTermSize)
    {
        int32_t nTermCountBegin = 0, nTermCountEnd = nTermTotalCount - 1, nTermCountMid;
        char *pIdxAddr_TermCountMid;
        KeyHashType mid_value;
        while (nTermCountBegin <= nTermCountEnd) {
            nTermCountMid = (nTermCountBegin + nTermCountEnd) >> 1;
            pIdxAddr_TermCountMid = pIdxAddr_TermOffsetBegin + nTermCountMid * nTermSize;
            mid_value = *(KeyHashType*)(pIdxAddr_TermCountMid);
            if (value == mid_value) {
                return pIdxAddr_TermCountMid;
            } 
            else if (value < mid_value) {
                nTermCountEnd = nTermCountMid - 1;
            }
            else {
                nTermCountBegin = nTermCountMid + 1;
            }
        }
        return NULL;
    }
};



#endif
