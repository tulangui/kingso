/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: test.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_TEST_TEST_H_
#define INDEX_TEST_TEST_H_

#include <sys/stat.h>
#include <sys/types.h>
#include "util/MemPool.h"

#define NORMAL_PATH   "./test_idx"    // 正常路径
#define NO_POWER_PATH "/xxxxxx.xxx"   // 无权限路径

// 开发测试数据路径
#define BIN_IDX_PATH "/home/nfs/xiaojie.lgx/own_test/rundata/index"
#define TXT_IDX_PATH "/home/nfs/xiaojie.lgx/own_test/input/index"

#define FIELDNAME     "title"
#define ENCODEFILE    "encodeFile.txt"
//#define FULL_TEST

#ifdef FULL_TEST
  #define MAX_TERM_NUM  1000             // test term num 
  #define MAX_DOC_NUM   10000            // doc number
  #define MAX_OCC_NUM   65               // max occ value
  #define OCC_NUM       3                // 每篇文档出现次数
  #define MAX_DESIGN_CAPACITY 60000000   // 极限设计容量
  
  // 测试bitmap seek next等操作
  #define RUN_NUM       5            // 测试性能时的循环次数                         
#else
  #define MAX_TERM_NUM  1000         // test term num 
  #define MAX_DOC_NUM   10000        // doc number
  #define MAX_OCC_NUM   65           // max occ value
  #define OCC_NUM       3            // 每篇文档出现次数
  #define MAX_DESIGN_CAPACITY 60000  // 极限设计容量

  // 测试bitmap seek next等操作
  #define RUN_NUM       1            // 测试性能时的循环次数                         

  // dict 优化，正确性对比
  #define DICT_HASH_SIZE ((1<<20)+1)
  #define DICT_NODE_SIZE (1<<20)
  #define DICT_NODE_NUM (1<<20)

  // dict 优化，哈希碰撞对比
  #define DICT_COLLISION_HASH_SIZE (1024)
  #define DICT_COLLISION_NODE_SIZE (1<<20)
  #define DICT_COLLISION_NODE_NUM (40960)

  // 增量更新测试
  #define INC_MAX_DOC_NUM 1000  // 更新测试时全量库容量
  #define INC_TERM_NUM 10       // 更新term数量
  #define INC_DOC_NUM  1000     // 更新文档数量
#endif

class test
{ 
public:
    test();
    ~test();

    static int mkdir(const char *pathname);
    static int rmdir(const char *pathname);
//    static uint32_t bsearch(uint32_t* list, uint32_t num, uint32_t key);

    template<typename T>
    static uint32_t bsearch(const T * list, uint32_t num, T& key)
    {
        uint32_t begin = 0;  
        uint32_t end = num - 1;
        uint32_t mid; 

        while(begin < end) {
            mid = begin + ((end - begin)>>1);
            if(list[mid] > key) {
                end = mid -1; 
            } else if(list[mid] < key) {
                begin = mid + 1;  
            } else {
                return key;
            }        
        }        

        if(list[begin] < key) { // 有监视哨存在，不会越界
            return list[begin+1]; 
        } else {
            return list[begin];
        }    
        return key;
    }
private:
  
};

#endif //INDEX_TEST_TEST_H_
