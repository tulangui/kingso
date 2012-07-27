#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "SearchIndex.h"
#include "util/idx_sign.h"
#include "util/MemPool.h"
#include "util/Log.h"
#include "util/Timer.h"
#include "index_lib/IndexReader.h" 
#include "index_lib/DocIdManager.h"

#define TERM_MAX_NUM 1024 // 最多允许的term操作数量

using namespace index_lib;

SearchIndex::SearchIndex(int32_t maxRetDocNum, int32_t circleNum)
{
  _termLineNum = 0;
  _termNodeNum = 0;
  _curLineNo = 0;
  _curCircle = 0;
  _beginTime = 0;

  _fpOut = NULL;
  _pTermNode = NULL;
  _pTermNodeNum = NULL;
  _ppTermNode = NULL;
  _queryBuf = NULL;
  _pDocList = new uint32_t[60000000];

  _circleNum = circleNum;
  _maxRetDocNum = maxRetDocNum;

  // 总的结果数量
  _allRetNum = 0;
  // 平均结果数量
  _perRetNum = 0;
  // 无结果行数
  _nullLineNum = 0;
  // 访问的内存大小
  _visitMemSize = 0;
  // 结果内存大小
  _retMemSize = 0;
}
SearchIndex::~SearchIndex()
{
  if(_pTermNode) delete[] _pTermNode;
  if(_pTermNodeNum) delete[] _pTermNodeNum;
  if(_ppTermNode) delete[] _ppTermNode;
  if(_fpOut) fclose(_fpOut);
  if(_queryBuf) delete[] _queryBuf;
  if(_pDocList) delete[] _pDocList;
}

int32_t SearchIndex::init(const char* inFile, const char* outFile)
{

  struct stat st;
  if(stat(inFile, &st)) {
    printf("stat %s error\n", inFile);
    return -1;
  }
  FILE* fp = fopen(inFile, "rb");
  if(NULL == fp) {
    printf("open %s error\n", inFile);
    return -1;
  }
  
  char* buf = new char[st.st_size + 1];
  fread(buf, st.st_size, 1, fp); 
  fclose(fp);
  _queryBuf = buf;

  char* begin = buf;
  char* end = buf + st.st_size;
  _termLineNum = 1;
  _termNodeNum = 1;

  while(begin < end) {
    if(*begin == '\n') {
      _termLineNum++;
    } else if(*begin == ':') {
      _termNodeNum++;
    }
    begin++;
  }
  
  _ppTermNode = new TermNode*[_termLineNum];
  _pTermNodeNum = new int32_t[_termLineNum];
  _pTermNode = new TermNode[_termNodeNum];

  _termLineNum = 0;
  TermNode* pNode = _pTermNode;

  // 
  begin = buf;
  while(begin && begin < end) {
    while(begin < end && isspace(*begin)) begin++;
    char* line = begin;
    begin = strchr(begin, '\n');
    if(begin) {
      *begin++ = 0;
    } else {
      begin = end;
    }
    
    _pTermNodeNum[_termLineNum] = parseLine(line, begin - line, pNode, true);
    if(_pTermNodeNum[_termLineNum] <= 0) continue;
    
    _ppTermNode[_termLineNum] = pNode;
    pNode += _pTermNodeNum[_termLineNum];
    _termLineNum++;
  }

  if(outFile) {
    _fpOut = fopen(outFile, "w");
  }
  if(NULL == _fpOut) {
    _fpOut = stdout;
  }
  _termNodeNum = pNode-_pTermNode;
  fprintf(_fpOut, "termLineNum=%d, NodeNum=%d\n", _termLineNum, _termNodeNum);

  // 统计信息
  StatInfo();
  return 0;
}

void SearchIndex::StatInfo()
{
  _nullRetNum = 0;
  // 压缩token数量
  _zipTermNum = 0;
  // 压缩token总链长
  _zipTermAllDocNum = 0;
  _bitmapNum = 0;

  // 未压缩token数量
  _uzipTermNum = 0;
  // 未压缩token总链长
  _uzipTermAllDocNum = 0;
  
  TermNode* pNode = _pTermNode;
  IndexReader* pReader = IndexReader::getInstance();

  MemPool cPool;
  MemPool* pPool = &cPool;

  for(int32_t i = 0; i < _termNodeNum; i++, pNode++) {
    IndexTerm* pTerm = pReader->getTerm(pPool, pNode->fieldSign, pNode->termSign);
    if(NULL == pTerm) {
      _nullRetNum++;
      continue;
    }
    const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
    pNode->docNum = pTermInfo->docNum;
    if(pTermInfo->docNum <= 0) {
      _nullRetNum++;
      continue;
    }
    if(pTermInfo->bitmapFlag == 1) {
      _bitmapNum++;
    }
    if(pTermInfo->zipType == 2) { // 压缩
      _zipTermNum++;
      _zipTermAllDocNum += pTermInfo->docNum;
    } else {
      _uzipTermNum++;
      _uzipTermAllDocNum += pTermInfo->docNum;
    }
  }

  if(_zipTermNum <= 0) _zipTermNum = 1;
  if(_uzipTermNum <= 0) _uzipTermNum = 1;
  _zipTermDocNum =  _zipTermAllDocNum / _zipTermNum;
  _uzipTermDocNum = _uzipTermAllDocNum / _uzipTermNum;

  printf("null=%ld, bitmap=%ld, zip=(%ld,%ld/%ld), uzip=(%ld,%ld/%ld)\n", _nullRetNum, _bitmapNum,
      _zipTermNum, _zipTermDocNum, _zipTermAllDocNum, _uzipTermNum, _uzipTermDocNum, _uzipTermAllDocNum);
}

int32_t SearchIndex::start(int32_t threadNum)
{
  _beginTime = time(NULL);

  pthread_t threadId[threadNum];
  for(int32_t i = 0; i < threadNum; i++) {
    int ret = pthread_create(&threadId[i], NULL, doSearch, this);
    if (ret!=0){
      TERR("pthread_create() failed.\n");
      return -1;
    }
  }

  // 等待其余的线程结束
  void *retval = NULL;
  for (int32_t i = 0; i < threadNum; i++) {
    int ret = pthread_join(threadId[i], &retval);
    if(ret != 0){
      TERR("pthread_join failed.\n");
    }
  }
  return 0;
}

void* SearchIndex::doSearch(void* para)
{
  SearchIndex* pClass = (SearchIndex*)para;

  MemPool cPool;

  int32_t nodeNum = 0;
  TermNode* pNode = NULL;
  uint32_t* docList = NULL;
  int32_t docNum = 0;

  while(pClass->getLine(pNode, nodeNum)) {
    
    uint64_t begin = util::Timer::getMicroTime();
    docNum = pClass->doSeek(pNode, nodeNum, &cPool, docList);
    begin = util::Timer::getMicroTime() - begin;

    pClass->PrintLog(docList, docNum, pNode, nodeNum, begin);
    cPool.reset();
  }

  return NULL;
}

int32_t SearchIndex::parseLine(const char* line, int32_t len, TermNode* pNode, bool flag)
{
  char* p = (char*)line;
  int32_t nodeNum = 0;

  while(p && *p) {
    while(*p && isspace(*p)) *p++;
    char* fieldName = p;
    p = strchr(p, ':');
    if(NULL == p) break;
    *p++ = 0;

    while(*p && isspace(*p)) *p++;
    char* termSignTxt = p;
    p = strchr(p, ' ');
    if(p) {
      *p++ = 0;
    } else {
      p = termSignTxt;
      while(*p && !isspace(*p)) p++;
      *p = 0;
    }
    pNode->fieldName = fieldName;
    pNode->fieldSign = idx_sign64(fieldName, strlen(fieldName));
    if(strcmp(fieldName, "nid") == 0) {
      const char* pre = termSignTxt;
      char* cur = NULL;
      pNode->termSign = (uint64_t)strtoll(pre, &cur, 10);
      pNode->termName = termSignTxt;
    } else if(flag) {
      pNode->termName = termSignTxt;
      pNode->termSign = idx_sign64(termSignTxt, strlen(termSignTxt));
    } else {
      const char* pre = termSignTxt;
      char* cur = NULL;
      pNode->termSign = strtoull(pre, &cur, 10);
      pNode->termName = "unknown";
    }
    pNode++;
    nodeNum++;
    if(nodeNum >= TERM_MAX_NUM) {
      return nodeNum;
    }
  }
  return nodeNum;
}

void SearchIndex::PrintLog(uint32_t* docList, uint32_t docNum, TermNode* pNode, int32_t nodeNum, uint32_t useTime)
{
  int32_t outNum = 0;

  _lock.lock();
  if(docNum > 0) {
    _allRetNum += docNum;
  } else {
    _nullLineNum++;
  }
  _lock.unlock();

  if(docNum > _maxRetDocNum) {
    outNum = _maxRetDocNum;
  } else { 
    outNum = docNum;
  }

  for(int32_t i = 0; i < nodeNum; i++) {
    fprintf(_fpOut, "%s:%s:%lu(%u) ", pNode[i].fieldName, pNode[i].termName, pNode[i].termSign, pNode[i].docNum);
  }
  if(docNum <= 0)
    fprintf(_fpOut, "null, time=%u\n", useTime);
  else
    fprintf(_fpOut, "num=%d, time=%u\n", docNum, useTime);
  uint32_t pre = 0;
  for(int32_t i = 0; i < outNum; i++) {
    if(i > 0 && pre >= docList[i]) {
      printf("error %d pre=%u cur=%u\n", i, pre, docList[i]);
    }
    pre = docList[i];
    fprintf(_fpOut, "%d ", docList[i]);
  }
  fprintf(_fpOut, "\n\n");
  fflush(_fpOut);
}

inline int comparIndexTerm(const void* p1, const void* p2)
{
  if((*(IndexTerm**)p1)->getDocNum() < (*(IndexTerm**)p2)->getDocNum())
    return -1;
  if((*(IndexTerm**)p1)->getDocNum() > (*(IndexTerm**)p2)->getDocNum())
    return 1;
  return 0;
}

int32_t SearchIndex::doSeek(TermNode* pNode, int32_t nodeNum, MemPool* pPool, uint32_t*& docList)
{
  if(nodeNum > TERM_MAX_NUM) {
    nodeNum = TERM_MAX_NUM;
  }

  int32_t bitmapNum = 0;
  IndexTerm* termArray[TERM_MAX_NUM];
  const uint64_t* bitmapArray[TERM_MAX_NUM];
  IndexReader* pReader = IndexReader::getInstance();
 
  for(int32_t i = 0; i < nodeNum; i++, pNode++) {
    termArray[i] = pReader->getTerm(pPool, pNode->fieldSign, pNode->termSign);
    if(NULL == termArray[i]) {
      return 0;
    }
    const IndexTermInfo* pTermInfo = termArray[i]->getTermInfo();
    if(pTermInfo->docNum <= 0) {
      return 0;
    }
  }

  if(nodeNum == 1) {
    IndexTerm* pTerm = termArray[0];
    uint32_t resultNum = pTerm->getDocNum();   // 结果数量
    docList = NEW_VEC(pPool, uint32_t, resultNum + 1);
    resultNum = 0;

    uint32_t docId;
    while((docId = pTerm->next()) < INVALID_DOCID) {
      docList[resultNum++] = docId;
    }
    return resultNum;
  }

  qsort(termArray, nodeNum, sizeof(IndexTerm*), comparIndexTerm);

  int32_t invertNum = 0;
  for(int32_t i = 0; i < nodeNum; i++) {
    const IndexTermInfo* pTermInfo = termArray[i]->getTermInfo();
    if(pTermInfo->zipType == 1) {
      _visitMemSize += pTermInfo->docNum * sizeof(uint32_t);
    } else {
      _visitMemSize += pTermInfo->docNum * sizeof(uint16_t);
    }

    bitmapArray[bitmapNum] = termArray[i]->getBitMap();
    if(bitmapArray[bitmapNum]) {
      bitmapNum++;
    } else {
      termArray[invertNum++] = termArray[i];
    }
  }
  if(invertNum + bitmapNum != nodeNum) {
    printf("error\n");
  } else if(bitmapNum == nodeNum) {
    // 全部bitmap索引,找一个带倒排链的
    int32_t i;
    for(i = 0; i < nodeNum; i++) {
      const IndexTermInfo* pTermInfo = termArray[i]->getTermInfo();
      if(pTermInfo->zipType != 0) {
        break;
      }
    }
    if(i < nodeNum) { // 找到了一个有倒排链的
      termArray[0] = termArray[i];
    } else { // 只能取第一个了
      i = 0;
    }
    invertNum++;
    for(--bitmapNum; i < bitmapNum; i++) {
      bitmapArray[i] = bitmapArray[i+1];
    }
  }

  // and 关系查询
  uint32_t resultNum = termArray[0]->getDocNum();   // 结果数量
  docList = NEW_VEC(pPool, uint32_t, resultNum + 1);
  resultNum = 0;

  int32_t i = 0;
  uint32_t doc, tmpDoc;

  if(bitmapNum <= 0) {
    doc = 0xffffffff;
    do {
      tmpDoc = ++doc;
      while(i < nodeNum) {
        tmpDoc = termArray[i]->seek(tmpDoc);
        if(tmpDoc == doc) {
          i++;
        } else {
          i = 0;
          doc = tmpDoc;
        }
      }
      i = 0;
      docList[resultNum++] = doc;
    } while(doc < INVALID_DOCID);
    resultNum--;
  } else {// have bitmap
    doc = 0;
NEXT_DOC:
    while((doc = termArray[0]->seek(doc)) < INVALID_DOCID) {
      for(i = 0; i < bitmapNum; i++) {
        if(!(bitmapArray[i][doc>>6]&bit_mask_tab[doc&0x3F])) {
          doc++;
          goto NEXT_DOC;
        }
      }
      for(i = 1, tmpDoc = doc; i < invertNum; i++) {
        doc = termArray[i]->seek(tmpDoc);
        if(tmpDoc != doc) {
          goto NEXT_DOC;
        }
      }
      docList[resultNum++] = doc++;
    }
  }
  _retMemSize += resultNum * sizeof(uint32_t);

  return resultNum;
}

bool SearchIndex::getLine(TermNode*& pNode, int32_t& nodeNum)
{
  int32_t line = -1;
  
  _lock.lock();
  if(_curLineNo < _termLineNum) {
    line = _curLineNo++;
  } else if(_curCircle < _circleNum) {
    time_t useTime = time(NULL) - _beginTime;
    if(useTime <= 0) useTime = 1;
    printf("finish %d, useTime=%ld, qps=%ld, ", _termLineNum, useTime, _termLineNum / useTime); 
    fprintf(_fpOut, "finish %d, useTime=%ld, ", _curCircle, time(NULL) - _beginTime);
    fprintf(_fpOut, "ret=(%d/%d),%ld\n", _nullLineNum, _termLineNum, _allRetNum / _termLineNum);

    _curCircle++;
    _curLineNo = 0;
    _allRetNum = 0;
    _nullLineNum = 0;
    _visitMemSize = 0;
    _retMemSize = 0;
    line = _curLineNo++;
    _beginTime = time(NULL);
  }
  _lock.unlock();

  if(line < 0) {
    time_t useTime = time(NULL) - _beginTime;
    if(useTime <= 0) useTime = 1;
    printf("finish %d, useTime=%ld, qps=%ld, ", _termLineNum, useTime, _termLineNum / useTime); 
    fprintf(_fpOut, "finish %d, useTime=%ld, ", _curCircle, time(NULL) - _beginTime);
    fprintf(_fpOut, "ret=(%d/%d),%ld\n", _nullLineNum, _termLineNum, _allRetNum / _termLineNum);
    return false;
  }
  
  if(line % 1000 == 1) {
    time_t useTime = time(NULL) - _beginTime;
    if(useTime <= 0) useTime = 1;
    printf("finish %d, useTime=%ld, qps=%ld, ", line, useTime, line / useTime); 
    printf("visit=(%lu,%lu), ret=(%ld,%ld)\n", _visitMemSize / line, _visitMemSize, _retMemSize / line, _retMemSize);
  }
  
  pNode = _ppTermNode[line];
  nodeNum = _pTermNodeNum[line];

  return true;
}

int32_t SearchIndex::outTermInfo(TermNode* pNode, int32_t nodeNum)
{ 
  IndexReader* pReader = IndexReader::getInstance();
  MemPool cPool;
  for(int32_t i = 0; i < nodeNum; i++, pNode++) {
    IndexTerm* pTerm = pReader->getTerm(&cPool, pNode->fieldSign, pNode->termSign);
    if(unlikely(pTerm == NULL)) {
      printf("%s:%s(%lu) NULL\n", pNode->fieldName, pNode->termName, pNode->termSign);
      continue;
    }

    const IndexTermInfo* pTermInfo = pTerm->getTermInfo();
    printf("%s:%s:%lu docNum(%d/%d), occNum(%d/%d), ", pNode->fieldName, pNode->termName, pNode->termSign,
        pTermInfo->docNum, pTermInfo->maxDocNum, pTermInfo->occNum, pTermInfo->maxOccNum);
    if(pTermInfo->zipType == 1) {
      printf("非压缩 ");
    } else {
      printf("压缩 ");
    }
    if(pTermInfo->bitmapFlag) {
      printf("有bitmap, len（%d) ", pTermInfo->bitmapLen);
    } else {
      printf("没有bitmap ");
    }
    printf("\n");
  }
  return 0;
}

void usage(char * name)
{
	printf("./%s [-p idx_path], 索引文件路径, [-d dict_idx_path]\n", name);
	printf("批量查询参数:\n");
  printf("[-q query_file]\n");
  printf("[-t thread_num]\n");
  printf("[-r result_file]\n");
  printf("[-m max_retNum]\n");
  printf("[-c circle_num]\n");
  printf("[-l pre_load(true,false)]\n");
  printf("[-s input sign(true,false)]\n");
  printf("[-h help]\n");
}

int statEnds(int32_t etTime)
{
    ProfileManager * pflMgr = ProfileManager::getInstance();
    ProfileDocAccessor        * pd = pflMgr->getDocAccessor();
    const ProfileField        * pf = pd->getProfileField("ends");
    DocIdManager * mgr = DocIdManager::getInstance();
    
    if(etTime <= 0) etTime = time(NULL);
    int32_t second = 0;
    for(int i = 0; i < mgr->getDocIdCount(); i++) {
        uint32_t cnt = pd->getValueNum(i, pf);
        int32_t endTime = pd->getRepeatedInt32(i, pf, 0);
        if(endTime < etTime) second++; 
    }
    fprintf(stderr, "all=%d, et=%d, second=%d\n", mgr->getDocIdCount(), etTime, second);
    return 0;
}

int main(int argc, char** argv)
{
  char idxPath[PATH_MAX] = {0};    // 索引文件路径
  char dictIdxPath[PATH_MAX] = {0};// nid dict 文件路径
  char queryFile[PATH_MAX] = {0};  // query文件路径， 格式: title:a title:b catmap:0 ...
  char retFile[PATH_MAX] = {0};    // 结果文件路径，记录结果数和部分结果id
  char profileFile[PATH_MAX] = {0};    // 结果文件路径，记录结果数和部分结果id
  uint32_t maxRetNum = 0;          // 最大输出结果id个数
  uint32_t threadNum = 1;          // 查询线程个数
  uint32_t circleNum = 0;          // query文件循环使用次数
  bool preLoadFag = false;         // 是否预读
  bool signFlag = false;           // 输入termSign

	// parse argv
  for(int ch; -1 != (ch = getopt(argc, argv, "p:d:q:t:r:m:c:l:s:h:f:?"));) {
    switch(ch) {
      case 'p':
        {
          if(strlen(optarg) >= PATH_MAX) {
            printf("-p idx_path %s over len\n", optarg);
            return -1;
          }
          strcpy(idxPath, optarg);
          break;
        }
      case 'd':
        {
          if(strlen(optarg) >= PATH_MAX) {
            printf("-d idx_path %s over len\n", optarg);
            return -1;
          }
          strcpy(dictIdxPath, optarg);
          break;
        }
      case 'f':
        {
          if(strlen(optarg) >= PATH_MAX) {
            printf("-d idx_path %s over len\n", optarg);
            return -1;
          }
          strcpy(profileFile, optarg);
          break;
        }
      case 'q':
        {
          if(strlen(optarg) >= PATH_MAX) {
            printf("-q query_file %s over len\n", optarg);
            return -1;
          }
          strcpy(queryFile, optarg);
          break;
        }
      case 'r':
        {
          if(strlen(optarg) >= PATH_MAX) {
            printf("-r result_file %s over len\n", optarg);
            return -1;
          }
          strcpy(retFile, optarg);
          break;
        }
      case 'l':
        {
          if(strcasecmp(optarg, "true") == 0) {
            preLoadFag = true;
          } else { 
            preLoadFag = false;
          }
          break;
        }
      case 's':
        {
          if(strcasecmp(optarg, "true") == 0) {
            signFlag = true;
          } else { 
            signFlag = false;
          }
          break;
        }
      case 't':
        {
          threadNum = strtoul(optarg, NULL, 10);
          break;
        }
      case 'm':
        {
          maxRetNum = strtoul(optarg, NULL, 10);
          break;
        }
      case 'c':
        {
          circleNum = strtoul(optarg, NULL, 10);
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
  if(idxPath[0] == 0) {
    usage(argv[0]);
    return -1;
  }

  alog::Configurator::configureRootLogger();
  alog::Logger::MAX_MESSAGE_LENGTH = 10240;
  IndexReader* pReader = IndexReader::getInstance();

  if(profileFile[0] && dictIdxPath[0]) {
      ProfileManager * f = ProfileManager::getInstance();
      f->setProfilePath( profileFile );
      if( 0 != f->load( false ) )
      {
          TERR("load profile failed! path:%s", profileFile );
          return -1;
      }
      DocIdManager * mgr = DocIdManager::getInstance();
      if ( false == mgr->load( dictIdxPath ) ) {
          printf("load nid dict failed\n");
          return -1;
      }
      statEnds(maxRetNum);
      printf("load nid dict success\n");
  } else {
      printf("no nid dict, can't search nid\n");
  }

  if(pReader->open(idxPath, preLoadFag) < 0) {
    printf("open indexlib %s error\n", argv[1]);
    return -1;
  }

  // 获取字段信息等内容
  char libInfo[10240];
  if(pReader->getIndexInfo(libInfo, 10240) < 0) {
    printf("get indexlib info error\n");
    return -1;
  }
  TLOG("%s\n\n", libInfo);
  
  if(queryFile[0]) {// 批量查询文件
    SearchIndex cSearch(maxRetNum, circleNum);
    if(cSearch.init(queryFile, retFile) < 0) {
      TERR("init query=%s error\n", queryFile);
      return -1;
    }
    cSearch.start(threadNum);
    return 0;
  }
  
  // 交互式查询
  SearchIndex cSearch(0xFFFFFFFF, 0);
  char inTerm[10240];
  MemPool cPool;

  uint32_t termNum = 0;    // and 操作的term数量
  uint32_t resultNum = 0;  // 结果数量
  
  TermNode termNodeArray[TERM_MAX_NUM];
 
  while(1) {
    cPool.reset();
    printf("in search mode, input format is:fieldName:termSign fieldName:termSign ...\n");

    int ch;
    uint32_t i = 0;
    while((ch = getchar()) != EOF) {
      if(ch == '\n') break;
      if(i < 10240) {
        inTerm[i++] = (char)ch;
      }
    }
    if(i <= 0) break;
    if(i >= 10240) {
      printf("over len limit, please fix it\n");
      continue;
    }
    inTerm[i] = 0;
    
    // 解析term信息
    termNum = cSearch.parseLine(inTerm, i, termNodeArray, signFlag);
    if(termNum <= 0) {
      printf("parse line %s error, num=%d\n", inTerm, termNum);
      continue;
    }

    uint32_t* docList = NULL;
    resultNum = cSearch.doSeek(termNodeArray, termNum, &cPool, docList);
    // 输出term信息
    cSearch.outTermInfo(termNodeArray, termNum);
    
    if(resultNum == 0) {
      printf("all result num:0\n\n\n");
    } else {
      printf("all result num:%d\n", resultNum);
      for(i = 0; i < resultNum; i++) {
        printf("%d ", docList[i]);
      }
      printf("\n\n");
    }
  }
  pReader->close();
  printf("exit success, thanks!!!\n");
  alog::Logger::shutdown();

  return 0;
}

