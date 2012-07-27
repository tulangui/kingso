/*********************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: IndexFieldInc.h 2577 2011-03-09 01:50:55Z xiaojie.lgx $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INDEX_INDEXFIELDINC_H_
#define INDEX_INDEXFIELDINC_H_

#include <limits.h>

#include "Common.h"
#include "idx_dict.h"
#include "IndexTermParse.h"
#include "IndexTermFactory.h"
#include "IndexIncMemManager.h"

#include "util/MemPool.h"
#include "index_lib/IndexTerm.h"
#include "index_lib/IndexLib.h"
#include "index_lib/DocIdManager.h"

INDEX_LIB_BEGIN

class IndexFieldInc
{
public:
  IndexFieldInc(int32_t maxOccNum, uint32_t fullDocNum, index_mem::ShareMemPool & pool);
  ~IndexFieldInc();

  int open(const char * path, const char* fieldName, bool incLoad, bool sync, int32_t incSkipCnt, int32_t incBitmapSize);
  int close();

  // 是否导出文本索引
  int dump(bool flag = false);

  IndexTerm * getTerm(MemPool *pMemPool, uint64_t termSign);

  int addTerm(uint64_t termSign, uint32_t docId, uint8_t firstOcc);



  /**
   * 遍历当前字段的所有term
   *
   * @param termSign    返回的当前term的签名
   *
   * @return 0：成功;   -1：失败
   */
  int  getFirstTerm(uint64_t &termSign);
  int  getNextTerm(uint64_t  &termSign);


private:

  // 内存空间扩容
  int32_t expend(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end);

  // 非压缩to压缩扩容
  int32_t nzip2zip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t baseNum, uint32_t validCount);

  // 非压缩to非压缩，压缩to压缩扩容
  //int32_t otherExpend(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint8_t toZip, uint32_t validCount);

  // 非压缩to非压缩
  int32_t nzip2nzip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t validCount);

  // 压缩to压缩
  int32_t zip2zip(inc_idx2_header_t *&head, inc_idx1_unit_t *&pind, char *&begin, char *&end, uint32_t validCount);

  // 获取扩容大小
  int32_t getPageSize(uint32_t currDocCnt, uint8_t zipFlag);

  // dump时将索引导出txt格式文本，以方便索引升级对比
  int32_t idx2Txt(idx_dict_t* pdict);

  uint32_t getDocNumByEndOff(uint32_t endOff, int zipFlag);

private:
  int32_t _maxOccNum;
  uint32_t _fullDocNum;                  // 全量doc数
  char _idxPath[PATH_MAX];               // 数据存储路径
  char _fieldName[MAX_FIELD_NAME_LEN];   // fieldName

  IndexTermFactory    _indexTermFactory; // 根据条件创建indexTerm子类
  index_mem::ShareMemPool & _pool;


  idx_dict_t          * _invertedDict;           // 1级倒排索引hash表
  idx_dict_t          * _bitmapDict;             // 1级bitmap索引hash表
  IndexIncMemManager  * _pIncMemManager;         // 2级倒排索引内存管理
  DocIdManager        * _docIdMgr;               // 用于expend过程剔除删除的docid

  uint32_t              _travelPos;              // term遍历当前位置
  idx_dict_t          * _travelDict;             // term遍历当前索引，bitmap/invert

  uint32_t              _maxIncSkipCnt;          // 增量状态下zip格式倒排的skipblock最大数 
  uint32_t              _maxIncBitmapSize;       // 增量bitmap最大空间 1M (差8字节用于结尾保护)

};

INDEX_LIB_END


#endif //INDEX_INDEXFIELDINC_H_
