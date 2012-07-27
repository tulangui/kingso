#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "mxml.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexBuilder.h"
#include "util/Log.h"

void usage(char * name)
{
	printf("./%s [-f config_name], [-d max_doc_id], [-t thread_num]\n", name);
}

struct ThreadPara {
  index_lib::IndexBuilder* pBuilder;
  char inputFile[PATH_MAX];
  char fieldName[PATH_MAX];
  int error;
};


// 每个field一个线程
void* worker(void* vpPara)
{
  ThreadPara* pPara = (ThreadPara*)vpPara;

  index_lib::IndexBuilder* pBuilder = pPara->pBuilder;
  const char* pInputFile = pPara->inputFile;
  const char* fieldName  = pPara->fieldName;

  time_t begin = time(NULL);

//	int fd = open(pInputFile, O_RDONLY, 0644);
//	if (fd == -1) {
  FILE* fp = fopen(pInputFile, "rb");
  if(NULL == fp) {
		TERR("打开文件出错 %s, errno: %d, %s\n", pInputFile, errno, strerror(errno));
		return NULL;
	}
//	struct stat st;
//	if (fstat(fd, &st) < 0) {
//		fprintf (stderr, "文件 %s fstat失败!\n", pInputFile);
//		close(fd);
//		return NULL;
//	}

//	int length = st.st_size;		//文件长度
//	if (length <= 0) {
//		close(fd);
 //   return NULL;
//  }

//	char* buf = (char*)mmap(0, length, PROT_READ, MAP_SHARED, fd, 0);
//	if(buf == MAP_FAILED) {
//		fprintf (stderr, "文件 %s 映射内存文件出错! error code:%d\n", pInputFile ,errno);
//		close(fd);
//		return NULL;
//	}
  // 设置顺序读取最优
//	madvise(buf, length, MADV_SEQUENTIAL);
/*
  char* begin = buf;
  char* end = buf + length;
  while (begin < end) {
    char* line = begin;
    while(begin < end && *begin != '\n') begin++;
    if (pBuilder->addTerm(fieldName, line, begin - line) <= 0) {
      fprintf (stderr, "addTerm %s error, line=", fieldName);
      fwrite (line, begin - line + 1, 1, stderr);
      break;
    }
    if (*begin == '\n') {
      begin++;
    }
  }
*/
//  if(pBuilder->dumpField(fieldName) < 0) {
//    fprintf (stderr, "dumpField %s error\n", fieldName);
//  }

//  close(fd);
//  munmap(buf, length);

  char* line = NULL;
  size_t len = 0;
  ssize_t read = 0;

  while ((read = getline(&line, &len, fp)) != -1) {
    if (pBuilder->addTerm(fieldName, line, read) <= 0) {
      TERR("addTerm %s error, line= %s", fieldName, line);
      break;
    }
  }
  if (line)           free(line);
  fclose(fp);

  TLOG("build %s success, use time=%ld", fieldName, time(NULL) - begin);
  return NULL;
}


int main(int argc, char ** argv)
{
	uint32_t max_doc_id = 50000000;    // 最大docid
  uint32_t max_thread_num = 8;       // 最大线程数量
  char cfgFile[PATH_MAX] = "";       // 配置文件名

	// parse argv
	for(int ch; -1 != (ch = getopt(argc, argv, "d:f:t:h?"));) {
		switch(ch) {
			case 'd':
				{
					max_doc_id = strtoul(optarg, NULL, 10);
					break;
				}
			case 'f':
				{
          int32_t len = strlen(optarg);
          if (len >= PATH_MAX) {
            usage(basename(argv[0]));
            return -1;
          }
          strcpy(cfgFile, optarg);
          break;
        }
      case 't':
        {
					max_thread_num = strtoul(optarg, NULL, 10);
					break;
        }
			case 'h':
			case '?':
			default:
				{
					usage(basename(argv[0]));
					return -1;
				}
		}
	}

  if (strlen(cfgFile) <= 0) {
    usage(basename(argv[0]));
    return -1;
  }

  time_t beginTime = time(NULL);
  alog::Configurator::configureRootLogger();

  // 解析配置文件
  FILE* fp = fopen(cfgFile, "r");
  if(fp == NULL) {
    usage(basename(argv[0]));
    return -1;
  }
  mxml_node_t* pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
  if(pXMLTree == NULL) {
    fclose(fp);
    TERR("config file %s format error", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  fclose(fp);

  // 创建build对象
  char szIdxPath[PATH_MAX];     // 输出路径
  char szInputPath[PATH_MAX];   // 输入路径
  index_lib::IndexBuilder cIndexBuilder(max_doc_id);

  // globals
  mxml_node_t* pRoot = mxmlFindElement(pXMLTree, pXMLTree, "globals", NULL, NULL, MXML_DESCEND);
  if (NULL == pRoot) {
    TERR("config file %s, cant find field globals", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  const char* pPathRoot = mxmlElementGetAttr(pRoot, "root");
  if (NULL == pPathRoot) {
    TERR("config file %s, cant find field root", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }

  // <index name="index" sub_dir="index">
  pRoot = mxmlFindElement(pXMLTree, pXMLTree, "index", NULL, NULL, MXML_DESCEND);
  if(pRoot == NULL) {
    TERR("config file %s, cant find field index", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  const char* pSubDir = mxmlElementGetAttr(pRoot, "sub_dir");
  if (NULL == pSubDir) {
    TERR("config file %s, cant find field sub_dir", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  int32_t len = strlen(pPathRoot) + strlen(pSubDir) + 1;
  if (len >= PATH_MAX) {
    TERR("config file %s, root path + sub path len over", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  sprintf(szIdxPath, "%s/%s", pPathRoot, pSubDir);
  mkdir(szIdxPath, 0755);

  // 输入文件路径
  const char* pInputDir = mxmlElementGetAttr(pRoot, "input_dir");
  if (NULL == pInputDir) {
    TERR("config file %s, cant find field input_dir", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  if (strlen(pInputDir) >= PATH_MAX) {
    TERR("config file %s, input path len over", cfgFile);
    mxmlDelete(pXMLTree);
    return -1;
  }
  strcpy(szInputPath, pInputDir);

  //  <index_field name="title" max_occ="1"  />
  int32_t maxOcc = 0;
  int32_t nFieldNum = 0;
  ThreadPara threadPara[MAX_INDEX_FIELD_NUM];   // 每个field一个线程

  mxml_node_t* cpNode = mxmlFindElement(pRoot, pRoot, "index_field", NULL, NULL, MXML_DESCEND_FIRST);
  while(cpNode) {
    const char* name = mxmlElementGetAttr(cpNode, "name");
    const char* max_occ = mxmlElementGetAttr(cpNode, "max_occ");
    if (NULL == name) {
      TERR("config file %s, cant find name", cfgFile);
      mxmlDelete(pXMLTree);
      return -1;
    }
    if (NULL == max_occ) {
      maxOcc = 0;
    } else {
      maxOcc = atol(max_occ);
    }
    if (cIndexBuilder.addField(name, maxOcc) < 0) {
      mxmlDelete(pXMLTree);
      TERR("addField %s occ=%d error", name, maxOcc);
      return -1;
    }

    if (nFieldNum >= MAX_INDEX_FIELD_NUM) {
      mxmlDelete(pXMLTree);
      TERR("field num over limit=%d\n", MAX_INDEX_FIELD_NUM);
      return -1;
    }

    snprintf(threadPara[nFieldNum].inputFile,
        sizeof(threadPara[nFieldNum].inputFile), "%s/%s.idx.txt", szInputPath, name);
    snprintf(threadPara[nFieldNum].fieldName,
        sizeof(threadPara[nFieldNum].fieldName), name);
    nFieldNum++;
    cpNode = mxmlFindElement(cpNode, pRoot, "index_field", NULL, NULL, MXML_NO_DESCEND);
  }
  mxmlDelete(pXMLTree);

  if(nFieldNum <= 0) {
    TERR("config file %s, index field num = %d error", cfgFile, nFieldNum);
    return -1;
  }

  char filename[PATH_MAX];
  snprintf(filename, sizeof(filename), "%s/%s", szInputPath, "max_doc_id");
  fp = fopen(filename, "rb");
  if(fp) {
    char buf[128];
    fgets(buf, 128, fp);
    fclose(fp);
    max_doc_id = atol(buf);
    TLOG("use max_doc_id(%u) from %s", max_doc_id, filename);
  } else {
    TLOG("use max_doc_id(%u) from para", max_doc_id);
  }

  // 准备线程参数，检测txt文件是否存在
  for (int32_t i = 0; i < nFieldNum; i++) {
    threadPara[i].pBuilder = &cIndexBuilder;
    threadPara[i].error = 0;

    if(access(threadPara[i].inputFile, 0)) {
      TERR( "field %s not exit\n", threadPara[i].inputFile);
      return -1;
    }
  }

  if (cIndexBuilder.open(szIdxPath) < 0) {
    TERR( "open %s error\n", szIdxPath);
    return -1;
  }

  // 启动线程
  void *retval = NULL;
  int32_t threadNum = 0;
  const int32_t maxThreadNum = max_thread_num;
  pthread_t threadId[maxThreadNum];

  for (int32_t i = 0, j = 0; i < nFieldNum; i++) {
    int ret = pthread_create(&threadId[j], NULL, worker, &threadPara[i]);
    if (ret!=0){
      TERR("pthread_create() failed.\n");
      return -1;
    }

    if(threadNum < maxThreadNum) {
      threadNum++;
      j++;
      continue;
    }

    // 等待一个结束的
    struct timespec ts;
    do {
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += 5;

      for(j = 0; j < threadNum; j++) {
        if(pthread_timedjoin_np(threadId[j], &retval, &ts)) {
          continue;
        }
        // 线程结束
        if (threadPara[i].error) {
          TERR("build %s error, code=%d\n", threadPara[i].fieldName, threadPara[i].error);
        }
        break;
      }
    } while(j >= threadNum);
    threadId[j] = 0;
  }

  // 等待其余的线程结束
  for (int32_t i = 0; i < threadNum; i++) {
    if(0 == threadId[i]) continue;
    int ret = pthread_join(threadId[i], &retval);
    if(ret != 0){
      TERR("pthread_join failed.\n");
    }
    if (threadPara[i].error) {
      TERR("build %s error, code=%d\n", threadPara[i].fieldName, threadPara[i].error);
    }
  }

  cIndexBuilder.dump();
  // close index lib
  cIndexBuilder.close();
  TLOG("build success, use Time = %ld\n", time(NULL) - beginTime);

  return 0;
}

