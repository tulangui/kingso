/*
 *  IndexTermFactory.h
 *
 *  Author: xiaojie.lgx@taobao.com
 *
 *  Desc  : IndexTerm 工厂类，根据不同的条件（压缩、非压缩、occ个数等)
 *          生成不同的IndexTerm的子类
 */

#ifndef _INDEX_TERM_FACTORY_H_
#define _INDEX_TERM_FACTORY_H_

#include "Common.h"
#include "index_lib/IndexTerm.h"
#include "IndexTermActualize.h"
#include "util/MemPool.h"

INDEX_LIB_BEGIN

class IndexTermFactory
{
public:
  IndexTermFactory(uint32_t maxOccNum);
  ~IndexTermFactory();

  /**
   * 根据参数生成特定的IndexTerm子类,并初始化
   *
   * @param pMemPool   由此分配内存
   * @param zipFlag    倒排链是否压缩, 0:没有倒排链  1: 不压缩 2: 高位压缩
   * @param bitmap     bitmap指针
   * @param invertList 倒排链指针
   * @param docCount   倒排链中doc个数
   * @param maxDocNum  索引的全部倒排文档数量
   * @param maxDocId   倒排链中最大的docId(只有only bitmap idx 需要)
   */
  IndexTerm* make( MemPool        * pMemPool,
                   uint32_t         zipFlag,
                   const uint64_t * bitmap,
                   const char     * invertList,
                   uint32_t         docCount,
                   uint32_t         maxDocNum,
                   uint32_t         maxDocId = 0,
                   uint32_t         not_orig_flag = 0);

private:
  uint32_t _maxOccNum;
};

INDEX_LIB_END


#endif // _INDEX_TERM_FACTORY_H_
