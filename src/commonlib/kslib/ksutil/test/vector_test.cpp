#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <vector>
#include "util/Vector.h"

#define TEST_MAX (300000000)

using namespace util;
using namespace std;

typedef bool(*TEST_FUNC) (int argc, char *argv[]);

struct tmp_struct {
    uint32_t a;
    uint32_t b;
};

bool
test_vector_assign_array(int argc, char *argv[]) {

    uint32_t ay1[32];
    uint32_t ay2[64];
    memset(ay1, 0, sizeof(uint32_t) * 32);
    memset(ay2, 0, sizeof(uint32_t) * 64);

    Vector < uint32_t > vect;
    vect.__print_info(stderr);
    fprintf(stderr, "Call vect.assign(ay1, 32)\n");
    vect.assign(ay1, 32);
    vect.__print_info(stderr);

    fprintf(stderr, "Call vect.assign(ay2, 64)\n");
    vect.assign(ay2, 64);
    vect.__print_info(stderr);

    return true;
}

bool
test_vector_assign_vector(int argc, char *argv[]) {

    uint32_t ay1[32];
    uint32_t ay2[64];
    memset(ay1, 0, sizeof(uint32_t) * 32);
    memset(ay2, 0, sizeof(uint32_t) * 64);

    Vector < uint32_t > v1(ay1, 32);
    Vector < uint32_t > v2(ay2, 64);
    Vector < Vector < uint32_t > >v3;
    int i = 0;
    for (i = 0; i < 10; i++) {
        v3.pushBack(v1);
    }



    Vector < uint32_t > vect;
    vect.__print_info(stderr);
    fprintf(stderr, "Call vect.assign(v1)\n");
    vect.assign(v1);
    vect.__print_info(stderr);
    fprintf(stderr, "Call vect.assign(v2)\n");
    vect.assign(v2);
    vect.__print_info(stderr);

    Vector < Vector < uint32_t > >vect1;
    vect1.__print_info(stderr);
    fprintf(stderr, "Call vect1.assign(v3)\n");
    vect1.assign(v3);
    vect1.__print_info(stderr);




    return true;
}

bool
test_vector_size(int argc, char *argv[]) {
    Vector < uint32_t > vect;

    fprintf(stderr, "empty. size = %llu\n",
            (unsigned long long) vect.size());
    fprintf(stderr, "call pushBack...\n");
    vect.pushBack(0);
    fprintf(stderr, "size = %llu\n", (unsigned long long) vect.size());
    fprintf(stderr, "call pushBack...\n");
    vect.pushBack(0);
    fprintf(stderr, "call pushBack...\n");
    vect.pushBack(0);
    fprintf(stderr, "size = %llu\n", (unsigned long long) vect.size());
    fprintf(stderr, "call erase(0)...\n");
    vect.erase(0);
    fprintf(stderr, "size = %llu\n", (unsigned long long) vect.size());
    fprintf(stderr, "call clear()...\n");
    vect.clear();
    fprintf(stderr, "size = %llu\n", (unsigned long long) vect.size());
    return true;
}

bool
test_vector_clear(int argc, char *argv[]) {
    Vector < uint32_t > vect;

    for (uint32_t i = 0; i < 10; i++) {
        vect.pushBack(i);
    }

    vect.__print_info(stderr);

    vect.clear();
    fprintf(stderr, "\nclear() called.\n\n");

    vect.__print_info(stderr);

    return true;
}

bool
test_stl_erase(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "%s need 2 params.\n", __FUNCTION__);
        return false;
    }
    uint64_t n;
    n = strtoull(argv[0], NULL, 10);

    uint64_t erase_pos = 0;
    erase_pos = strtoull(argv[1], NULL, 10);

    uint64_t erase_n = 0;
    erase_n = strtoull(argv[2], NULL, 10);

    if (erase_pos + erase_n > n) {
        fprintf(stderr, "erase_pos + erase_n must <= N.\n");
        return false;
    }

    uint32_t i;
    vector < uint32_t > vect;
    vector < uint32_t >::iterator it_start;
    vector < uint32_t >::iterator it_end;

    for (i = 0; i < n; i++) {
        vect.push_back(i);
    }

    for (i = 0; i < vect.size(); i++) {
        fprintf(stdout, "%u ", vect[i]);
    }
    fprintf(stdout, "\n");

    for (i = 0, it_start = vect.begin();
         i < erase_pos && it_start != vect.end(); i++) {
        it_start++;
    }


    for (i = 0, it_end = it_start; i < erase_n && it_end != vect.end();
         i++) {
        it_end++;
    }


    vect.erase(it_start, it_end);


    for (i = 0; i < vect.size(); i++) {
        fprintf(stdout, "%u ", vect[i]);
    }
    fprintf(stdout, "\n");

    return true;
}

bool
test_vector_erase(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "%s need 2 params.\n", __FUNCTION__);
        return false;
    }
    uint64_t n;
    n = strtoull(argv[0], NULL, 10);

    uint64_t erase_pos = 0;
    erase_pos = strtoull(argv[1], NULL, 10);

    uint64_t erase_n = 0;
    erase_n = strtoull(argv[2], NULL, 10);

    if (erase_pos + erase_n > n) {
        fprintf(stderr, "erase_pos + erase_n must <= N.\n");
        return false;
    }

    uint32_t i;
    bool ret = false;
    Vector < uint32_t > vect(1);

    for (i = 0; i < n; i++) {
        ret = vect.pushBack(i);
        if (!ret) {
            fprintf(stderr, "Vector::pushBack() failed.\n");
            return false;
        }
    }

    for (i = 0; i < vect.size(); i++) {
        fprintf(stdout, "%u ", vect[i]);
    }
    fprintf(stdout, "\n");

    for (i = 0; i < erase_n; i++) {
        vect.erase(erase_pos);
    }

    for (i = 0; i < vect.size(); i++) {
        fprintf(stdout, "%u ", vect[i]);
    }
    fprintf(stdout, "\n");

    return true;
}


bool
test_vector_operator_equal(int argc, char *argv[]) {
    uint32_t i = 0;

    Vector < uint32_t > vect1(1);
    Vector < uint32_t > vect2(1);

    Vector < string > vect3(1);
    Vector < string > vect4(1);

    Vector < struct tmp_struct >vect5(1);
    Vector < struct tmp_struct >vect6(1);

    char tmp[16];
    string tstr;
    struct tmp_struct tstruct;


    for (i = 0; i < 256; i++) {
        vect1.pushBack(i);
        snprintf(tmp, sizeof(tmp), "%u", i);
        tstr = tmp;
        vect3.pushBack(tstr);
        tstruct.a = i;
        tstruct.b = i;
        vect5.pushBack(tstruct);
    }

    vect2 = vect1;
    vect4 = vect3;
    vect6 = vect5;

    if (vect2.size() != vect1.size()) {
        return false;
    }
    if (vect4.size() != vect3.size()) {
        return false;
    }
    if (vect6.size() != vect5.size()) {
        return false;
    }


    for (i = 0; i < vect1.size(); i++) {
        if (vect1[i] != vect2[i]) {
            return false;
        }
        if (strcmp(vect3[i].c_str(), vect4[i].c_str()) != 0) {
            return false;
        }
        if ((vect5[i].a != vect6[i].a) || (vect5[i].b != vect6[i].b)) {
            return false;
        }
    }

    return true;
}

bool
test_vector_push_vector(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_push_vector() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);
    fprintf(stderr, "Will push %llu vectors to Vector.\n",
            (unsigned long long) count);
    uint32_t i = 0;

    Vector < uint32_t > vect(1);
    Vector < uint32_t > vect1(1);

    for (uint32_t i = 0; i < 10; i++) {
        vect1.pushBack(i);
    }

    bool ret = false;

    for (i = 0; i < count; i++) {
        ret = vect.pushBack(vect1);
        if (!ret) {
            fprintf(stderr, "vect.pushBack failed.\n");
            break;
        }
        fprintf(stderr, "Times: %u\n", i);
        vect.__print_info(stderr);
    }

    return ret;
}

bool
test_vector_push_array(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_push_array() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);
    fprintf(stderr, "Will push %llu arrays to Vector.\n",
            (unsigned long long) count);
    uint32_t i = 0;

    Vector < uint32_t > vect(1);
    uint32_t array[10];
    memset(array, 0, sizeof(uint32_t) * 10);

    bool ret = false;

    for (i = 0; i < count; i++) {
        ret = vect.pushBack(array, 10);
        if (!ret) {
            fprintf(stderr, "vect.pushBack failed.\n");
            break;
        }
        fprintf(stderr, "Times: %u\n", i);
        vect.__print_info(stderr);
    }

    return ret;
}

bool
test_vector_access_performance(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_access_speed() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);

    uint32_t num;
    uint32_t i;
    Vector < uint32_t > vect(1);
    for (i = 0; i < count; i++) {
        vect.pushBack(i);
    }

    for (i = 0; i < count; i++) {
        num = vect[i];
    }

    return true;
}

bool
test_stl_access_performance(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_stl_access_speed() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);

    uint32_t num;
    uint32_t i;
    vector < uint32_t > vect;
    for (i = 0; i < count; i++) {
        vect.push_back(i);
    }

    for (i = 0; i < count; i++) {
        num = vect[i];
    }

    return true;
}

bool
test_vector_access(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_push() require 1 param.\n");
        return false;
    }

    uint64_t pos = 0;
    char *pe = NULL;
    pos = strtoull(argv[0], &pe, 10);

    uint32_t i;
    Vector < uint32_t > vect(1);
    for (i = 0; i < 70; i++) {
        vect.pushBack(i);
    }

    fprintf(stderr, "Pos:%llu, Value:%u\n", (unsigned long long) pos,
            vect[pos]);

    return true;
}

bool
test_stl_push(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_push() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);
    fprintf(stderr, "Will push %llu elements to Vector.\n",
            (unsigned long long) count);
    uint32_t i = 0;

    vector < uint32_t > vect;
    bool ret = false;

    for (i = 0; i < count; i++) {
        vect.push_back(i);
    }

    return ret;
}

bool
test_vector_push(int argc, char *argv[]) {
    if (argc != 1) {
        fprintf(stderr, "test_vector_push() require 1 param.\n");
        return false;
    }

    uint64_t count = 0;
    char *pe = NULL;
    count = strtoull(argv[0], &pe, 10);
    fprintf(stderr, "Will push %llu elements to Vector.\n",
            (unsigned long long) count);
    uint32_t i = 0;

    Vector < uint32_t > vect(1);
    bool ret = false;

    for (i = 0; i < count; i++) {
        ret = vect.pushBack(i);
        if (!ret) {
            fprintf(stderr, "vect.pushBack failed.\n");
            break;
        }
//        fprintf(stderr,"Times: %u\n",i);
//        vect.__print_info(stderr);
    }

    return ret;
}

bool
test_vector_iterator(int argc, char *argv[]) {
    Vector < uint32_t > vect(32);
    Vector < string > vect1(32);
    Vector < struct tmp_struct > vect2(32);

    Vector < uint32_t >::iterator it;
    Vector < string >::iterator it1;
    Vector < struct tmp_struct >::iterator it2;
    uint32_t i = 0;

    for (i = 0; i < 10; i++) {
        vect.pushBack(i);
        vect1.pushBack(string("aaa"));
        struct tmp_struct ts;
        ts.a = i;
        ts.b = i;
        vect2.pushBack(ts);
    }

    for (it = vect.begin(); it != vect.end(); it++) {
        fprintf(stdout, "%d\n", *it);
    }

    for (it1 = vect1.begin(); it1 != vect1.end(); it1++) {
        fprintf(stdout, "%s\n", it1->c_str());
    }
    
    for (it2 = vect2.begin(); it2 != vect2.end(); it2++) {
        fprintf(stdout, ".a=%u,.b=%u\n", it2->a, it2->b);
    }

    return true;
}

bool
test_vector_init(int argc, char *argv[]) {

    if (argc != 1) {
        fprintf(stderr, "test_vector_init() required 1 param.\n");
        return false;
    }

    uint64_t size = 0;

    size = strtoull(argv[0], NULL, 10);

    Vector < uint32_t > vect1(size);
    Vector < uint32_t > vect2(vect1);

    fprintf(stderr, "vect1:\n");
    vect1.__print_info(stderr);

    fprintf(stderr, "vect2:\n");
    vect2.__print_info(stderr);

    if (!vect1.isInited() || !vect2.isInited()) {
        fprintf(stderr, "Failed.\n");
        return false;
    }
    fprintf(stderr, "Success.\n");
    return true;
}

bool
test_vector_init_array(int argc, char *argv[]) {

    if (argc != 1) {
        fprintf(stderr, "test_vector_init() required 1 param.\n");
        return false;
    }

    uint64_t size = 0;

    size = strtoull(argv[0], NULL, 10);

    uint32_t *array = NULL;
    array = new uint32_t[size];
    if (!array) {
        fprintf(stderr, "out of memory!\n");
        return false;
    }

    Vector < uint32_t > vect(array, size);

    fprintf(stderr, "vect:\n");
    vect.__print_info(stderr);
    delete[]array;
    if (!vect.isInited()) {
        fprintf(stderr, "Failed.\n");
        return false;
    }

    fprintf(stderr, "Success.\n");

    return true;

}

bool
test_vector_init_vector(int argc, char *argv[]) {

    if (argc != 0) {
        fprintf(stderr, "test_vector_init() required 0 param.\n");
        return false;
    }

    Vector < uint32_t > vect_item;

    Vector < uint32_t > vect_uint;
    Vector < string > vect_string;
    Vector < Vector < uint32_t > >vect_vect;

    vect_uint.pushBack(0);
    vect_string.pushBack("hello");
    vect_vect.pushBack(vect_item);

    Vector < uint32_t > vect1(vect_uint);
    Vector < string > vect2(vect_string);
    Vector < Vector < uint32_t > >vect3(vect_vect);

    fprintf(stderr, "vect1:\n");
    vect1.__print_info(stderr);
    fprintf(stderr, "vect2:\n");
    vect2.__print_info(stderr);
    fprintf(stderr, "vect3:\n");
    vect3.__print_info(stderr);

    return true;
}

typedef struct case_s {
    TEST_FUNC func;
    char name[32];
    char param[64];
    char desc[256];
} case_t;


case_t g_cases[] = {
    {test_vector_init, "vector_init", "[size]",
     "初始化2个Vector，初始大小为size，然后销毁，可检测内存泄漏。"},
    {test_vector_init_array, "vector_init_array", "[size]",
     "初始化1个c风格数组，使用它对vector进行初始化。"},
    {test_vector_init_vector, "vector_init_vector", "",
     "初始化三个类型分别为(uint32_t,string,vector)的vector，使用它对vector进行初始化。"},
    {test_vector_push, "vector_push", "[n]",
     "Vector pushBack() 测试，追加n个元素。"},
    {test_stl_push, "stl_push", "[n]",
     "std::vector pushBack() 测试，追加n个元素。"},
    {test_vector_push_array, "vector_push_array", "[n]",
     "Vector pushBack() 测试，追加n个长度为10的数组。"},
    {test_vector_push_vector, "vector_push_vector", "[n]",
     "Vector pushBack() 测试，追加n个长度为10的vector。"},
    {test_vector_access, "vector_access", "[pos]",
     "将在vector中增加70个整型数字，打印pos位置的数字。"},
    {test_stl_access_performance, "stl_access_performance", "[n]",
     "将在vector中增加n个整型数字，然后依次访问所有数字"},
    {test_vector_access_performance, "vector_access_performance", "[n]",
     "将在vector中增加n个整型数字，然后依次访问所有数字"},
    {test_vector_assign_array, "vector_assign_array", "",
     "使用一个c数组对vector进行assign，然后再使用另一个数组进行assign"},
    {test_vector_assign_vector, "vector_assign_vector", "",
     "使用一个Vector对vector进行assign，然后再使用另一个Vector进行assign"},
    {test_vector_operator_equal, "vector_operator_equal", "",
     "生成2个Vector，vect1 pushBack 256个元素，通过vect2=vect1后，比较vect1与vect2的长度，并且逐个比较vect1与vect2中每一个元素是否相等。"},
    {test_vector_erase, "vector_erase", "[n] [erase_pos] [erase_n]",
     "测试erase功能。生成一个含有n个元素的vect，删除erase_pos位置开始的erase_n个元素"},
    {test_stl_erase, "stl_erase", "[n] [erase_pos] [erase_n]",
     "测试stl erase功能。生成一个含有n个元素的vect，删除erase_pos位置开始的erase_n个元素"},
    {test_vector_clear, "vector_clear", "",
     "测试clear功能。生成一个含有10个元素的vect，比较执行clear前后的变化"},
    {test_vector_iterator, "vector_iterator", "测试Vector迭代器"},
    {test_vector_size, "vector_size", "测试size() 功能"}
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
