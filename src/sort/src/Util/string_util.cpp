#include "string_util.h"
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "util/MemPool.h"
#include "util/crc.h"

char * replicate_str(const char * sz) 
{
    return replicate_str(sz, (sz ? strlen(sz) : 0));
}

char * replicate_str(const char * sz, uint32_t len) 
{
    if(!sz) return NULL;
    char * szRet = new char[len+1];
    if(len > 0) memcpy(szRet, sz, len);
    szRet[len] = '\0';
    return szRet;
}

char* replicate_str(const char* szStr, MemPool *pHeap, uint32_t nLength) 
{
    if (szStr == NULL) return NULL;
    size_t len = 0;
    if (nLength == 0)
    {
        len = strlen(szStr);
    } 
    else
    {
        len = nLength;
    }
    char *szRet = (char*)NEW_VEC(pHeap, char, len+1);
    if(!szRet) {
        return NULL;
    }
    memcpy(szRet, szStr, len);
    szRet[len] = '\0';
    return szRet;
}


bool equal_nocase(const char *sz1, const char *sz2)
{
    assert(sz1);
    return strcmp(sz1, sz2) == 0; 
}
bool str2int32(const char *sz1, int32_t &num)
{
    assert(sz1);
    num = atoi(sz1);
    return true;
}
bool str2uint32(const char *sz1, uint32_t &num)
{
    assert(sz1);
    num = (uint32_t)atoi(sz1);
    return true;
}

bool str2double(const char *sz1, double &num)
{
    assert(sz1);
    num = atof(sz1);
    return true;
}
char * merge_string(const char* szStr1, const char * szStr2)
{
    assert(szStr1&&szStr2);
    size_t len1 = strlen(szStr1);
    size_t len2 = strlen(szStr2);
    char* szRet = new char[len1 + len2 + 1];
    strcpy(szRet, szStr1);
    strcpy(szRet+len1, szStr2);
    return szRet;

}
void str_toupper(char * sz)
{
    assert(sz);
    char *p = sz;
    while(*p) {
        *p = toupper(*p);
        p++;
    }
}
uint64_t hash_string64(const char * sz)
{
    uint64_t h = get_crc64(sz, strlen(sz));
    return h;
}
int strtok(std::vector<char *>& res, char *szInput, int len, char *szChars, MemPool *mempool)
{
    res.clear();
    char *p1 = szInput;
    while (p1<szInput + len && *p1 != '\0') {
        char *p2 = szChars;
        //将我们预定义的做切分的字符标记为\0
        while(p2 && *p2 != '\0') {
            if (*p1 == *p2) {
                *p1 = '\0';
                break;
            }
            p2++;
        }
        p1++;
    }

    p1 = szInput;
    while(p1 < szInput + len) {
        //跳过标记为\0的字符
        if(*p1 == '\0') { p1++; continue; }
        char *e = replicate_str(p1, mempool, strlen(p1));
        if(!e) {
            TLOG("replicate_str %s failed", p1);
            return -1;
        }
        res.push_back(e);
        //跳过已经处理的字符串
        while (*p1!='\0' && p1 < szInput + len) { p1++; }
    }
    return 0;
}
