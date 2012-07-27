#ifndef __UTIL_STRING_UTIL_H
#define __UTIL_STRING_UTIL_H
#include <stdint.h>
#include <vector>
#include "util/MemPool.h"


#ifndef __cplusplus
extern "C" {
#endif 

    extern char * replicate_str(const char* sz);
    extern char * replicate_str(const char* sz, uint32_t len);
    extern char * replicate_str(const char* szStr, MemPool *pHeap, uint32_t nLength);
    extern bool equal_nocase(const char *sz1, const char *sz2);
    extern bool str2int32(const char *sz1, int32_t &num);
    extern bool str2uint32(const char *sz1, uint32_t &num);
    extern bool str2double(const char *sz1, double &num);
    extern char * merge_string(const char* s1, const char * s2);
    extern void str_toupper(char * sz);
    extern uint64_t hash_string64(const char * sz);
    extern uint64_t hash_string64(const char *sz);
    extern int strtok(std::vector<char *>& res, char *szInput, int len, char *szChars, MemPool *pHeap);
#ifndef __cplusplus
};
#endif 

#endif //__INDEXENGINESTRING_UTIL_H
