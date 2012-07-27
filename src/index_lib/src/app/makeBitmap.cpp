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
#include "util/Log.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexBuilder.h"

using namespace index_lib;

int main(int argc, char** argv)
{
  if(argc != 2) {
    fprintf(stderr, "%s path", argv[0]);
    return -1;
  }

  IndexBuilder builder;
  if(builder.reopen(argv[1]) < 0) {
    return -1;
  }

  int32_t travelPos = -1;
  int32_t maxOccNum;
  do {
    const char* fieldName = builder.getNextField(travelPos, maxOccNum);
    if(fieldName == NULL) break;
    fprintf(stdout, "%s %d\n", fieldName, maxOccNum);
  } while(1);

  return 0;
#define FIELD_NUM 2
  const char* field[FIELD_NUM] = {"ratesum", "cat_id_path"};
  uint32_t* pDocList = NULL;

  for(int32_t i = 0; i < 1; i++) {
    uint64_t termSign = 0;
    const IndexTermInfo* pTermInfo = builder.getFirstTermInfo(field[i], termSign);
    if(NULL == pTermInfo) {
      continue;
    }
    if(NULL == pDocList) {
      pDocList = new uint32_t[pTermInfo->maxDocNum];
    }

    do {
      if(pTermInfo->bitmapFlag == 0) { // 没有bitmap，生成
        IndexTerm * pTerm = builder.getTerm(field[i], termSign);
        uint32_t docNum = 0;
        while((pDocList[docNum] = pTerm->next()) < INVALID_DOCID) {
          docNum++;
        }
        int ret = builder.addTerm(field[i], termSign, pDocList, docNum);
        if(ret <= 0) {
          fprintf(stderr, "addTerm %lu error\n", termSign);
          return -1;
        }
      }
    } while((pTermInfo = builder.getNextTermInfo(field[i], termSign)));
  }
  
  if(pDocList) {
    delete[] pDocList;
    pDocList = NULL;
  }
  builder.dump();
  builder.close();

  return 0;
}

