/*
 * IndexFieldBuilder.h
 *
 *  Created on: 2011-3-6
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 提供 一个字段 从文本到二进制的转换  持久化
 */

#ifndef INDEXFIELDBUILDER_H_
#define INDEXFIELDBUILDER_H_

#include <limits.h>

#include "IndexTermParse.h"
#include "IndexTermFactory.h"
#include "util/MemPool.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexTerm.h"
#include "IndexCompression.h"

namespace index_lib
{

// class testIndexFieldBuilder;

/**
 * 实现 1个字段索引的  的 数据数据转换和持久化
 */
class IndexFieldBuilder
{
public:
    // 构造函数、析构函数
    IndexFieldBuilder(const char * fieldName, int maxOccNum, int maxDocNum, const char * encodeFile);
    virtual ~IndexFieldBuilder();


    /**
     * 打开指定的路径， 查找并加载所有的索引文件
     *
     * @param path  DIR of index files
     *
     * @return  < 0: error, >= 0 success
     */
    int open(const char * path);
    int reopen(const char * path);


    /**
     * 关闭整个索引
     *
     * @return  < 0: error, >= 0 success
     */
    int close();


    /**
     * 给一个字段 添加一个term和 对应的docid列表
     *
     * @param line        特定格式的文本行
     * @param lineLen     文本行的长度
     *
     * @return <=0: error;  >0: success. the doc_id count of the term
     */
    int addTerm(const char * line, int lineLen);
    int addTerm(const uint64_t termSign, const uint32_t* docList, uint32_t docNum);
    int addTerm(const uint64_t termSign, const DocListUnit* docList, uint32_t docNum);

    /**
     * 输出二进制文件
     *
     * @return  < 0: error, >= 0 success
     */
    int dump();

    /**
     * 输出字段名称
     */
    const char* getFieldName() { return _fieldName;}

    /**
     * 输出字段最大occ数量
     */
    int32_t getMaxOccNum() { return _maxOccNum; }

    // 顺序遍历所有的term
    const IndexTermInfo* getFirstTerm(uint64_t & termSign);
    const IndexTermInfo* getNextTerm(uint64_t & termSign);
    IndexTerm * getTerm(uint64_t termSign);

private:
    // 根据传入的解析器，增加一个新的term
    int addTerm();

    /*
     * func : 将一行文本倒排索引解析为非压缩格式的2进制倒排索引
     *
     * args : [input]  line, 以\0结尾的一行文本，存储一个token对应的2级倒排索引
     *        [output] sign, token的64位签名
     *        [output] base_num, 独立doc_id高16bit的个数
     *        [in/out] buf, 存储非压缩格式2进制倒排索引的buffer
     *        [input]  buf_len, 上一参数buf的长度
     *
     * ret  : <=0, error
     *      : >0,  2进制倒排索引链条中doc_id的个数
     */
    int build_nzip_index(const char *line, int lineLen, uint64_t *sign,
                         uint32_t *base_num, char *buf, const uint32_t buf_len);
    int buildNZipIndex(uint32_t *base_num, char *buf, const uint32_t buf_len, uint32_t& useBufLen);

    /*
     * func : 根据非压缩格式的2进制倒排索引构建压缩格式的2进制倒排索引
     *
     * args : [input]  nzip, 存储非压缩格式2级倒排索引的buffer
     *        [input]  doc_count, doc_id个数
     *        [input]  base_num, 独立doc_id高16bit的个数
     *        [in/out] zip, 存储压缩格式2进制倒排索引的buffer
     *
     * ret  : <=0, error
     *      : >0, 压缩格式2进制倒排索引的长度（以字节为单位）
     */
    int build_zip_index(const char *nzip, const unsigned int doc_count,
                        const unsigned int base_num, char *zip);
    int buildZipIndex(const char *nzip, const unsigned int doc_count,
                        const unsigned int base_num, char *zip);
    int buildP4DIndex(const char *nzip, const unsigned int doc_count, char *zip);

    /*
     * func : 根据非压缩格式的2进制倒排索引构建bitmap索引
     *
     * args : [input]  nzip, 存储非压缩格式2级倒排索引的buffer
     *        [input]  doc_count, doc_id个数
     *        [in/out] final_buf, 存储bitmap索引的buffer
     *
     */
    void build_bitmap_index(const char *nzip, const unsigned int doc_count,
                            char *final_buf);
    void buildBitmapIndex(const char *nzip, const unsigned int doc_count,
                            char *final_buf, uint32_t& maxDocId);

    /*
     * func : 截断超过 MAX_IDX2_NUM_BIT（26bit) 范围的倒排链条
     *
     * args : [input]  buf, 存储非压缩倒排链条的索引结构
     *        [in/out] docCount doc_id个数, 返回截断后的doc_id个数
     *        [in/out] base_num 截断后独立doc_id高16bit的个数
     *        [out]    useBufLen, 截断后实际存储需要的内存大小
     */
    int abandonOverflow(char *buf, uint32_t& docCount, uint32_t& baseNum, uint32_t& useBufLen);


    /*
     * func : 根据输入参数确定2进制索引的存储格式
     *
     * args : [input]  doc_count, doc_id个数
     *        [input]  base_num, 独立doc_id高16bit的个数
     *        [input]  bitmap_size, bitmap索引的大小
     *
     * ret  : 2进制索引的存储格式
     */
    IDX_DISK_TYPE set_idt(const unsigned int doc_count,
                          const unsigned int base_num, const unsigned int bitmap_size);


    /*
     * func : 更新当前2级倒排索引文件的已写入长度，
     *        若大于指定长度，则新开一个2级倒排索引文件
     *
     * args : [in/out] pbuilder, 建索引句柄
     *        [input]  len, 最近一次写入2级索引文件的索引长度
     *
     * ret  : 0, success
     *        <0, error
     */
    int update_cur_size(const unsigned int len);


public:
    friend class testIndexFieldBuilder;
private:
  char _path[PATH_MAX];
  char _fieldName[MAX_FIELD_NAME_LEN];
  char _encodeFile[PATH_MAX];

  uint32_t _maxOccNum;     // term允许在一篇文档中出现的最大次数
  uint32_t _maxDocNum;     // 创建索引的文档总数,主要用来计算bitmap大小
  index_builder_t* _pIndexBuilder;   // index主结构体，类型为 index_builder_t

  IndexTermParse* _pLineParse;       // 解析输入的文本格式的倒排term，全局使用指针
  IndexTermParse* _pParseInstance;   // 解析输入的文本格式的倒排term, 需要释放

  unsigned int _travelPos;           // 遍历一级索引的当前位置
  uint32_t _inverted_file_num;       // 当前打开的二级索引文件号
  MemPool* _pMemPool;                // 遍历输出所有的term信息使用
  idx_dict_t* _pDictTravel;          // 当前遍历的idx_dict
  FILE* _fpInvertList;               // 倒排索引文件句柄
  FILE* _fpBitMapList;               // bitmap索引文件句柄
  IndexTermInfo _cIndexTermInfo;     // 当前索引信息
  IndexTermFactory _indexTermFactory;// indexTerm 生成器

  bool             _rebuildFlag;     // 是否为重新打开添加索引.默认为false
  IndexCompression _idxComp;         // 索引压缩器
};

}


#endif /* INDEXFIELDBUILDER_H_ */
