/*
 * IndexReader.h
 *
 *  Created on: 2011-3-5
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 打开索引所在的目录， 加载所有字段的索引。 并对外提供统一的查询接口
 */

#ifndef INDEXREADER_H_
#define INDEXREADER_H_

#include <limits.h>

#include "IndexLib.h"
#include "IndexTerm.h"
#include "util/MemPool.h"
#include "util/HashMap.hpp"

namespace index_lib
{

/**
 * 实现 多个字段索引的  的 数据加载和 查找
 */
class IndexReader
{
public:
    /**
     * 获取单实例对象指针
     *
     * @return  NULL: error
     */
    static IndexReader * getInstance() {return &_indexReaderInstance;}


    /**
     * 打开指定的路径， 查找并加载所有的索引文件
     *
     * @param path  DIR of index files
     * @param bPreLoad 是否预读
     * @return  < 0: error, >= 0 success
     */
    int open(const char * path, bool bPreLoad = true);


    /**
     * 关闭整个索引
     */
    void close();


    /**
     * 获得一个term对应的索引信息， 并保存在一个new出来的对象中
     *
     * @param fieldName   字段名
     * @param term        字段值
     *
     * @return NULL: field不存在; term不存在正常返回，但是docnum为0
     */
    IndexTerm * getTerm(MemPool* pMemPool, const char * fieldName, const char * term);
    IndexTerm * getTerm(MemPool* pMemPool, uint64_t fieldNameSign, uint64_t termSign);


    /**
     * 取得高频词的截断索引
     *
     * @param pMemPool
     * @param fieldName       倒排字段名
     * @param term            原始的term
     * @param psFieldName     ps排序字段的名字
     * @param sortType        排序类型， 0：正排   1:倒排
     * @return
     */
    IndexTerm * getTerm(MemPool     * pMemPool,
                        const char  * fieldName,
                        const char  * term,
                        const char  * psFieldName,
                        uint32_t      sortType);


    /**
     * 查看是否有这个字段的倒排索引
     *
     * @param fieldName   字段名
     *
     * @return  true: Yes;  false: No
     */
    bool hasField(const char * fieldName);
    bool hasField(uint64_t fieldNameSign);



    /**
     * 内部测试专用函数; 外部用户勿用
     *
     * 输出index部分的信息，包括有哪些字段、文档数量、term数量等等
     *
     * @param info   输出的index信息内存
     * @param maxlen 输出信息的最大长度
     *
     * @return < 0 失败， >=0 信息长度
     */
    int getIndexInfo(char* info, int maxlen);


    /**
     * 遍历字段的所有term签名
     *
     * @param fieldSign   字段名称
     * @param termSign    返回的当前term的签名
     *
     * @return 0: 成功 ；   -1： 失败
     */
    int getFirstTerm(const char * fieldName, uint64_t &termSign);
    int getNextTerm( const char * fieldName, uint64_t &termSign);
    int getFirstTerm(uint64_t     fieldSign, uint64_t &termSign);
    int getNextTerm( uint64_t     fieldSign, uint64_t &termSign);


    /**
     * 获取当前索引的根路径
     *
     * @param buf
     * @param bufLen
     *
     * @return  0:成功;  -1: 失败
     */
    int getDataDir( char * buf, int bufLen )
    {
        if ( NULL == buf )        return -1;
        if ( bufLen < PATH_MAX )  return -1;

        memcpy( buf, _path, PATH_MAX );

        return 0;
    };


    /**
     * 获取全量索引里面的doc数量
     */
    uint32_t  getDocNum()
    {
        return _maxDocNum;
    };


    // 构造函数、析构函数
    IndexReader();
    virtual ~IndexReader();


private:
    static IndexReader  _indexReaderInstance;      // 惟一实例

private:

    uint64_t _nidSign;                             // nid字段名的签名，  需要做  nid->docId 查询
    uint32_t _maxDocNum;                           // 全量索引包含的最大的docId数
    char _path[PATH_MAX];                          // 索引所在目录
    util::HashMap<uint64_t, void*> _fieldMap;      // 存储 <fieldNameSign, indexFieldReader>

};



}





#endif /* INDEXREADER_H_ */
