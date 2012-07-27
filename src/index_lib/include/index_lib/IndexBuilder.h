/*
 * IndexBuilder.h
 *
 *  Created on: 2011-3-6
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 提供从文本到二进制的转换  持久化
 */

#ifndef INDEXBUILDER_H_
#define INDEXBUILDER_H_

#include "IndexTerm.h"
#include "commdef/commdef.h"
#include "util/HashMap.hpp"
#include "index_lib/IndexInfoManager.h"

namespace index_lib
{

#pragma pack (1)
struct DocListUnit {
  uint32_t doc_id;         // doc-id
  uint8_t  occ;            // term在该doc中的出现的位置
};
#pragma pack ()

/**
 * 实现 多个字段索引的  的 数据数据转换和持久化
 */
class IndexBuilder
{
public:
    // 构造函数、析构函数
    IndexBuilder(int32_t maxDocNum);
    IndexBuilder(); // reopen 使用
    virtual ~IndexBuilder();

    /**
     * 获取单实例对象指针
     *
     * @return  NULL: error
     */
    static IndexBuilder* getInstance() {return &_indexBuilderInstance;}


    /**
     * 设置最大doc数量，用于计算bitmap大小
     */
    void setMaxDocNum(uint32_t maxDocNum) {_maxDocNum = maxDocNum;}

    /**
     * 在添加了指定的所有类型字段信息后， 打开指定的路径, 开始创建索引。  open后不能再addField
     * reopen 如果有field索引存在，则打开，不存在，创建
     *
     * @param path  DIR of index files
     *
     * @return  < 0: error, >= 0 success
     */
    int open(const char * path);
    int reopen(const char * path);


    /**
     * 关闭整个索引, 记录索引的全局信息
     *
     * @return  < 0: error, >= 0 success
     */
    int close();

    /**
     * 添加一个字段， 并设定这个字段相关的信息
     *
     * @param fieldName   字段名
     * @param maxOccNum   occ最大数
     *
     * @return  < 0: error, >= 0 success
     */
    int addField(const char * fieldName, int maxOccNum);


    /**
     * 添加一个字段， 并设定这个字段相关的信息
     *
     * @param fieldName    字段名
     * @param maxOccNum    occ最大数
     * @param encodeFile   内部排重后的 内部id 和 docId之间的编码映射关系
     *
     * @return  < 0: error, >= 0 success
     */
    int addField(const char * fieldName, int maxOccNum, const char * encodeFile);


    /**
     * 给一个字段 添加一个term和 对应的docid列表
     *
     * @param fieldName   字段名
     * @param line        特定格式的文本行
     * @param lineLen     文本行的长度
     *
     * @return <=0: error;  >0: success. the doc_id count of the term
     */
    int addTerm(const char * fieldName, const char * line, int lineLen);
    int addTerm(const char * fieldName, const uint64_t termSign, const uint32_t* docList, uint32_t docNum);
    int addTerm(const char * fieldName, const uint64_t termSign, const DocListUnit* docList, uint32_t docNum);

    /**
     * 输出所有字段的二进制文件， 包括一些辅助信息
     *
     * @return  < 0: error, >= 0 success
     */
    int dump();

    /**
     * 输出一个字段二进制文件
     *
     * @param fieldName       指定数据的字段名
     *
     * @return  < 0: error, >= 0 success
     */
    int dumpField(const char * fieldName);


    /**
     * 开始遍历所有字段,只能单线程遍历
     *
     * @param travelPos in,  当前遍历位置, 初始值为 -1
     * @param maxOccNum out, 该字段的最大occ数量
     *
     * @return fieldName : 成功;  NULL: 失败
     */
    const char* getNextField(int32_t& travelPos, int32_t& maxOccNum);


    /**
     * 获得一个term对应的索引信息，发生一级索引遍历操作
     *
     * @param fieldName in   字段名
     * @param termSign  out  当前term签名
     *
     * @return NULL: 遍历结束; other: docId遍历结构
     */
    const IndexTermInfo* getFirstTermInfo(const char* fieldName, uint64_t & termSign);
    const IndexTermInfo* getNextTermInfo(const char* fieldName, uint64_t & termSign);


    /**
     * 获得一个term对应的IndexTerm对象，用于遍历docId，发生文件读取操作
     *
     * @param fieldName in   字段名
     * @param dict,     out  遍历位置
     * @param termSign  out  当前term签名
     *
     * @return NULL: 遍历结束; other: docId遍历结构
     */
    IndexTerm * getTerm(const char* fieldName, uint64_t & termSign);

private:

    /**
     * 检查Field的合法性
     *
     * @param fieldName       指定数据的字段名
     * @param fieldSign       out, 输出名字对应的sign
     * @return  < 0: error, >= 0 success
     */
    int checkField(const char* fieldName, uint64_t& fieldSign);

private:

    int32_t        _maxDocNum;                        // 最大文档数量，用于计算bitmap大小
    char           _idxPath[PATH_MAX];                // 存储路径
    bool           _openFlag;                         // 是否已经调用open函数，open后不能再addField

    static IndexBuilder _indexBuilderInstance;         // 惟一实例
    util::HashMap<uint64_t, void*>   _fieldMap;       // 存储 <fieldNameSign IdxFieldBuilder>
};



}



#endif /* INDEXBUILDER_H_ */
