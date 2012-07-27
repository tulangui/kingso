#ifndef INDEX_LIB_SEARCH_INDEX_H_
#define INDEX_LIB_SEARCH_INDEX_H_

#include "util/MemPool.h"
#include "util/ThreadLock.h"
#include "index_lib/IndexTerm.h"

struct TermNode 
{
  const char* fieldName;
  const char* termName;
  uint64_t fieldSign; 
  uint64_t termSign;
  uint32_t docNum;
};

class SearchIndex
{ 
public:
  SearchIndex(int32_t maxRetDocNum, int32_t circleNum);
  ~SearchIndex();

  int32_t init(const char* inFile, const char* outFile);

  // 
  bool getLine(TermNode*& pNode, int32_t& nodeNum);
  
  //
  int32_t doSeek(TermNode* pNode, int32_t nodeNum, MemPool* pPool, uint32_t*& result);
  
  int32_t start(int32_t threadNum);

  // format: title:a title:b
  // flag: true: 明文 , false : sign  
  int32_t parseLine(const char* line, int32_t len, TermNode* pNode, bool flag = true);

  // 输出term信息
  int32_t outTermInfo(TermNode* pNode, int32_t nodeNum);

public:
  static void* doSearch(void* para);

private:
  void PrintLog(uint32_t* docList, uint32_t docNum, TermNode* pNode, int32_t nodeNum, uint32_t useTime);

  void StatInfo();

private:
  int32_t _circleNum;
  uint32_t _maxRetDocNum;

  int32_t _termLineNum;
  int32_t* _pTermNodeNum;
  TermNode** _ppTermNode;

  int32_t _termNodeNum;
  TermNode* _pTermNode;
  char* _queryBuf;

  int32_t _curLineNo;
  int32_t _curCircle;
  time_t _beginTime;
  
  // 输出结果
  FILE* _fpOut;
  util::Mutex _lock;
  // 结果内存
  uint32_t* _pDocList;
    

  // 总的结果数量
  int64_t _allRetNum;
  // 平均结果数量
  int64_t _perRetNum;
  // 空结果行数
  int32_t _nullLineNum;
  // 访问内存总量
  int64_t _visitMemSize;
  // 结果内存大小
  int64_t _retMemSize;
  
  // 无结果token数量
  int64_t _nullRetNum;
  // bitmap索引数量
  int64_t _bitmapNum;

  // 压缩token数量
  int64_t _zipTermNum;
  // 压缩token总链长
  int64_t _zipTermAllDocNum;
  // 压缩token平均链长
  int64_t _zipTermDocNum;

  // 未压缩token数量
  int64_t _uzipTermNum;
  // 未压缩token平均链长
  int64_t _uzipTermAllDocNum;
  // 未压缩token总链长
  int64_t _uzipTermDocNum;
};

#endif // INDEX_LIB_SEARCH_INDEX_H_
