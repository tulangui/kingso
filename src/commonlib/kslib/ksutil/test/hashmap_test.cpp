#include <stdio.h>
#include <string.h>
#include <map>
#include <string>
#include "util/hashmap.h"
#include "util/HashMap.hpp"
#include "util/crc.h"

typedef bool(*TEST_FUNC) (int argc, char *argv[]);

using namespace util;
using namespace std;

typedef struct case_s {
    TEST_FUNC func;
    char name[32];
    char param[64];
    char desc[256];
} case_t;

namespace util{
template <> struct KeyHash <string> {
    inline uint64_t operator() (const string &key) const {
        return get_crc64(key.c_str(), strlen(key.c_str()));
    }
};

template <> struct KeyHash <const string> {
    inline uint64_t operator() (const string &key) const {
        return get_crc64(key.c_str(), strlen(key.c_str()));
    }
};

template <> struct KeyEqual <string> {
    inline bool operator() (const string &k1, const string &k2) const {
        if (strcmp(k1.c_str(), k2.c_str()) == 0){
            return true;
        }
        else{
            return false;
        }
    }
};

template <> struct KeyEqual <const string> {
    inline bool operator() (const string &k1, const string &k2) const {
        if (strcmp(k1.c_str(), k2.c_str()) == 0){
            return true;
        }
        else{
            return false;
        }
    }
};
}

bool test_hashmap_init_str(int argc, char *argv[]){
    if (argc != 1) {
        fprintf(stderr, "test_hashmap_init() require 1 param.\n");
        return false;
    }

    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);

    HashMap <string,string> map(bucketSize, 0);

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }
    fprintf(stderr, "HashMap inited. bucketSize = %u\n", bucketSize);
    return true;
}

bool test_hashmap_init(int argc, char *argv[]){
    if (argc != 1) {
        fprintf(stderr, "test_hashmap_init() require 1 param.\n");
        return false;
    }

    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);

    HashMap <uint32_t,uint32_t> map(bucketSize, 0);

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }
    fprintf(stderr, "HashMap inited. bucketSize = %u\n", bucketSize);
    return true;
}

bool test_hashmap_find_str(int argc, char *argv[]){
    if (argc <= 5 ) {
        fprintf(stderr, "test_hashmap_insert() need 5 or more params.\n");
        return false;
    }


    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);
    HashMap <string,string> map(bucketSize, 0);
    HashMap <string,string>::iterator it;

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }

    int32_t ptr = 1;
    if (strcasecmp(argv[ptr], "INSERT") != 0) {
        fprintf(stderr, "Input error!\n");
        return false;
    }
    ptr++;

    bool ret = false;
    while (1) {
        ret = map.insert(string(argv[ptr]), string(argv[ptr]));
        fprintf(stderr, "Key:%s,Value:%s,Result:%d\n",argv[ptr], argv[ptr],ret);
        ptr++;

        if( ptr >= argc){
            fprintf(stderr, "Input error!\n");
            return false;
        }

        if (strcasecmp(argv[ptr], "FIND") == 0) {
            break;
        }
    }

    ptr++;

    while (ptr < argc) {
        it = map.find(argv[ptr]);
        fprintf(stderr, "FIND_TEST: Key:%s,Result:%s\n", argv[ptr],(it == map.end())?"NOT FOUND":"FOUND");
        ptr++;
    }
    
    return true;
}

bool test_hashmap_find(int argc, char *argv[]){
    if (argc <= 5 ) {
        fprintf(stderr, "test_hashmap_insert() need 5 or more params.\n");
        return false;
    }


    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);
    HashMap <uint32_t,uint32_t> map(bucketSize, 0);
    HashMap <uint32_t,uint32_t>::iterator it;

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }

    int32_t ptr = 1;
    if (strcasecmp(argv[ptr], "INSERT") != 0) {
        fprintf(stderr, "Input error!\n");
        return false;
    }
    ptr++;

    bool ret = false;
    while (1) {
        ret = map.insert(strtoul(argv[ptr], NULL, 10), ptr);
        fprintf(stderr, "Key:%s,Value:%u,Result:%d\n",argv[ptr], ptr,ret);
        ptr++;

        if( ptr >= argc){
            fprintf(stderr, "Input error!\n");
            return false;
        }

        if (strcasecmp(argv[ptr], "FIND") == 0) {
            break;
        }
    }

    ptr++;

    while (ptr < argc) {
        it = map.find(strtoul(argv[ptr], NULL, 10));
        fprintf(stderr, "FIND_TEST: Key:%s,Result:%s\n", argv[ptr],(it == map.end())?"NOT FOUND":"FOUND");
        ptr++;
    }
    
    return true;
}

bool test_hashmap_insert_str(int argc, char *argv[]){
    if (argc <3 ) {
        fprintf(stderr, "test_hashmap_insert() need 3 or more params.\n");
        return false;
    }

    int i = 0;

    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);
    HashMap <string,string> map(bucketSize, 0);

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }

    bool ret = false;
    for (i = 1; i < argc; i = i + 2) {
        ret = map.insert(argv[i], argv[i+1]);
        fprintf(stderr, "Key:%s,Value:%s,Result:%d\n",argv[i], argv[i+1], ret);
    }

    
    return true;
}

bool test_hashmap_insert(int argc, char *argv[]){
    if (argc <3 ) {
        fprintf(stderr, "test_hashmap_insert() need 3 or more params.\n");
        return false;
    }

    int i = 0;

    uint32_t bucketSize = 0;
    bucketSize = strtoul(argv[0], NULL, 10);
    HashMap <uint32_t,uint32_t> map(bucketSize, 0);

    if (!map.isInited()) {
        fprintf(stderr, "HashMap is not inited. bucketSize = %u\n", bucketSize);
        return false;
    }

    bool ret = false;
    for (i = 1; i < argc; i = i + 2) {
        ret = map.insert(strtoul(argv[i], NULL, 10), strtoul(argv[i+1], NULL, 10));
        fprintf(stderr, "Key:%s,Value:%s,Result:%d\n",argv[i], argv[i+1], ret);
    }

    
    return true;
}



#define SHOW_SIZE(map)  fprintf(stdout,"Map.size() = %u\n",map.size());
#define INSERT_UINT(map,key,value) { SHOW_SIZE(map); \
                                     fprintf(stdout,"insert(%u:%u) returned %u\n", key, value, map.insert(key,value));\
                                     SHOW_SIZE(map); }
#define INSERT_STR(map,key,value) { SHOW_SIZE(map); \
                                     fprintf(stdout,"insert(%s:%s) returned %u\n", key, value, map.insert(key,value));\
                                     SHOW_SIZE(map); }

bool test_hashmap_size_str(int argc, char *argv[]){
    HashMap <string,string> map1(16);

    fprintf(stdout, "map1:\n");
    INSERT_STR(map1, "a", "a");
    INSERT_STR(map1, "b", "b");
    INSERT_STR(map1, "c", "c");
    INSERT_STR(map1, "d", "d");


    fprintf(stdout, "\n\nmap2:\n");
    HashMap <string, string> map2(16);
    SHOW_SIZE(map2);
    INSERT_STR(map2, "a", "a");
    INSERT_STR(map2, "b", "b");
    INSERT_STR(map2, "c", "c");
    map2.clear();
    fprintf(stdout, "map2 cleared...\n");
    
    SHOW_SIZE(map2);
    INSERT_STR(map2, "a", "a");
    INSERT_STR(map2, "a", "a");
    return false;
}

bool test_hashmap_size(int argc, char *argv[]){
    HashMap <uint32_t, uint32_t> map1(16);

    fprintf(stdout, "map1:\n");
    INSERT_UINT(map1, 1, 1);
    
    INSERT_UINT(map1, 2, 2);
    
    INSERT_UINT(map1, 3, 3);
    
    INSERT_UINT(map1, 2, 3);


    fprintf(stdout, "\n\nmap2:\n");
    HashMap <uint32_t, uint32_t> map2(16);
    SHOW_SIZE(map2);
    INSERT_UINT(map2, 1, 1);
    INSERT_UINT(map2, 2, 2);
    INSERT_UINT(map2, 3, 3);
    map2.clear();
    fprintf(stdout, "map2 cleared...\n");
    SHOW_SIZE(map2);
    INSERT_UINT(map2, 1, 1);
    INSERT_UINT(map2, 1, 1);
    return false;
}

bool test_hashmap_iterator_str(int argc, char *argv[]){
    HashMap <string, string> map1(16);
    HashMap <string, string>::iterator it;
    fprintf(stdout, "map1, bucketSize = 16\n");
    INSERT_STR(map1, "a", "a");
    INSERT_STR(map1, "b", "b");
    INSERT_STR(map1, "c", "c");
    INSERT_STR(map1, "d", "d");
    
    fprintf(stdout,"Output:\n");
    for (it = map1.begin(); it != map1.end(); it++) {
        fprintf(stdout, "%u:%u\n",it->key,it->value);
    }
    
    HashMap <string, string> map2(1);
    fprintf(stdout, "map2, bucketSize = 1\n");
    INSERT_STR(map1, "a", "a");
    INSERT_STR(map1, "b", "b");
    INSERT_STR(map1, "c", "c");
    INSERT_STR(map1, "d", "d");
    
    fprintf(stdout,"Output:\n");
    for (it = map2.begin(); it != map2.end(); it++) {
        fprintf(stdout, "%u:%u\n",it->key,it->value);
    }
    

    return false;
}

bool test_hashmap_iterator(int argc, char *argv[]){
    HashMap <uint32_t, uint32_t> map1(16);
    HashMap <uint32_t, uint32_t>::iterator it;
    fprintf(stdout, "map1, bucketSize = 16\n");
    INSERT_UINT(map1, 1, 1);
    INSERT_UINT(map1, 2, 2);
    INSERT_UINT(map1, 3, 3);
    INSERT_UINT(map1, 4, 4);
    
    fprintf(stdout,"Output:\n");
    for (it = map1.begin(); it != map1.end(); it++) {
        fprintf(stdout, "%u:%u\n",it->key,it->value);
    }
    
    HashMap <uint32_t, uint32_t> map2(1);
    fprintf(stdout, "map2, bucketSize = 1\n");
    INSERT_UINT(map2, 1, 1);
    INSERT_UINT(map2, 2, 2);
    INSERT_UINT(map2, 3, 3);
    INSERT_UINT(map2, 4, 4);
    
    fprintf(stdout,"Output:\n");
    for (it = map2.begin(); it != map2.end(); it++) {
        fprintf(stdout, "%u:%u\n",it->key,it->value);
    }
    

    return false;
}


case_t g_cases[] = {
    {test_hashmap_init, "hashmap_init", "[bucketSize]",
     "初始化1个HashMap，桶数量为bucketSize，然后销毁，可检测内存泄漏。"},
    {test_hashmap_insert, "hashmap_insert", "[bucketSize] [key] [value] [key] [value] ...", "插入测试"},
    {test_hashmap_find, "hashmap_find", "[bucketSize] INSERT [key] [key] ... FIND [key] [key] ...", "find测试"},
    {test_hashmap_size, "hashmap_size", "", "size()功能测试"},
    {test_hashmap_iterator, "hashmap_iterator", "", "测试hashmap迭代器"},
    {test_hashmap_init_str, "hashmap_init_str", "[bucketSize]",
     "初始化1个HashMap，桶数量为bucketSize，然后销毁，可检测内存泄漏。"},
    {test_hashmap_insert_str, "hashmap_insert_str", "[bucketSize] [key] [value] [key] [value] ...", "插入测试"},
    {test_hashmap_find_str, "hashmap_find_str", "[bucketSize] INSERT [key] [key] ... FIND [key] [key] ...", "find测试"},
    {test_hashmap_size_str, "hashmap_size_str", "", "size()功能测试"},
    {test_hashmap_iterator_str, "hashmap_iterator_str", "", "测试hashmap迭代器"},
};

static uint32_t g_case_count = sizeof(g_cases) / sizeof(case_t);

void
print_usage(const char *prog) {
    unsigned int i = 0;

    fprintf(stderr, "Usage:\n\t%s (case) [params...]\n\n", prog);

    for (i = 0; i < g_case_count; i++) {
        fprintf(stderr, "\t\t%s %s\n", g_cases[i].name, g_cases[i].param);
        fprintf(stderr, "\t\t\t%s\n\n", g_cases[i].desc);
    }

    return;
}

int
main(int argc, char *argv[]) {

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    bool ret = false;
    uint32_t i = 0;
    for (i = 0; i < g_case_count; i++) {
        if (strcmp(g_cases[i].name, argv[1]) == 0) {
            ret = g_cases[i].func(argc - 2, &argv[2]);
            break;
        }
    }

    if (i >= g_case_count) {
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}
