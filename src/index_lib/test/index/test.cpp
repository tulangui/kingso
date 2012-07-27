#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "test.h" 
#include "template.h"
#include "testIdxDict.h"
#include "testIndexBuilder.h"
#include "testIndexFieldBuilder.h"
#include "testIndexIncManager.h"
#include "testIndexFieldReader.h"
#include "testIndexReader.h"
#include "testIndexTermParse.h"
#include "testIndexTerm.h"

#define MAX_PATH 1024

// 简单函数测试
CPPUNIT_TEST_SUITE_REGISTRATION(templateTest);

// 字典优化测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIdxDict);

// IndexTerm seek next 测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexTerm);

// 文本倒排解析测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexTermParse);

// 二进制倒排测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexBuilder);
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexFieldBuilder);

// 二进制单字段读取测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexFieldReader);

// 增量更新测试
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexIncManager);

#ifdef FULL_TEST
// 属于半集成测试了
CPPUNIT_TEST_SUITE_REGISTRATION(testIndexReader);
#endif

int test::mkdir(const char *pathname)
{
  char path[MAX_PATH];
  const char *p1 = pathname;
  char *p2 = path;

  while(*p1) {
    while(*p1 && *p1 != '\\' && *p1 != '/') {
      *p2++ = *p1++;
    }
    *p2 = 0;
    if(access(path, W_OK)) { // 写权限
      int ret = ::mkdir(path, 0755);
      if(ret) {
        fprintf(stderr, "mkdir %s error\n", path);
        return ret;
      }
    }
    if(*p1) *p2++ = *p1++;
  }
  return 0;
}

int test::rmdir(const char *pathname)
{
  char path[MAX_PATH];
  if(NULL == pathname) {
    return -1;
  }
  // 不允许删除当前目录
  const char* p1 = pathname;
  while(*p1 && (*p1 == '.' || *p1 == '/' || *p1 == '\\')) {
    p1++;
  }
  if(*p1 == 0) {
    return -1;
  }

  DIR *dir = opendir(pathname);
  if(dir == NULL) {
    fprintf(stderr, "open dir %s error, info=%s\n", pathname, strerror(errno));
    return -1;
  }
 
  struct dirent *it = NULL;
  while((it = readdir(dir))) {
    if(it->d_name[0] == '.' &&
      (it->d_name[1] == '\0' || (it->d_name[1] == '.' && it->d_name[2] == '\0'))) {
      continue;
    }
    sprintf(path, "%s/%s", pathname, it->d_name);
    if(S_ISDIR(it->d_type)) {
      ::rmdir(path);
    } else {
      remove(path);
    }
  }
  closedir(dir);

  int ret = ::rmdir(pathname);
  if(ret) {
    fprintf(stderr, "rmdir %s error\n", pathname);
    return ret;
  }
  return 0;
}
/*
uint32_t test::bsearch(uint32_t* list, uint32_t num, uint32_t key)
{
    int32_t begin = 0; 
    int32_t end = num - 1;
    int32_t mid; 

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
*/

