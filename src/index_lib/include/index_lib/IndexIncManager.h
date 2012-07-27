/*
 * IndexIncManager.h
 *
 *  Created on: 2011-3-7
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 维护 多个字段的 增量索引的内存结构， 提供读写接口
 */

#ifndef INDEXINCMANAGER_H_
#define INDEXINCMANAGER_H_

#include <limits.h>

#include "IndexTerm.h"
#include "util/MemPool.h"
#include "util/HashMap.hpp"


namespace index_lib
{

/**
 * 实现 多个字段 增量  索引的  的 数据维护 和 查找, 支持增量数据持久化和加载
 */
class IndexIncManager
{
public:
    /**
     * 获取单实例对象指针
     *
     * @return  NULL: error
     */
    static IndexIncManager * getInstance() {return &_indexIncInstance;}

    /**
     * 设置增量数据持久化标签
     * @param  sync  true: 本地持久化；false: 本地不持久化
     */
    void setDataSync ( bool sync )   {  _sync = sync; }

    /**
     * 设置dump时是否导出索引文件， 
     * * @param  export  true: 导出；false: 不导出
     */
    void setExportIdx( bool et )   {  _export = et; }

    /**
     * 设置增量最大数据量
     * @param maxIncNum 最大增量数据量
     */
    void setMaxIncNum(int32_t maxIncNum) { _maxIncNum = maxIncNum; } 

    /**
     * 获得一个term对应的索引信息， 并保存在一个new出来的对象中
     * 由外部释放
     *
     * @param pMemPool    线程级内存管理类
     * @param fieldName   字段名
     * @param term        字段值
     *
     * @return NULL: 不存在这个term;
     */
    IndexTerm * getTerm(MemPool *pMemPool, const char * fieldName, const char * term);
    IndexTerm * getTerm(MemPool *pMemPool, uint64_t fieldNameSign, uint64_t termSign);


    /**
     * 给一个字段 添加一个term和 对应的docid列表
     * 根据索引中是否已经存在这个term, 确定是进行新增  还是 对已存在的term添加docid
     *
     * @param fieldName   字段名
     * @param termSign    term签名
     * @param docId       doc id
     * @param firstOcc    first occ
     * @return <=0: error;  >0: success. the doc_id count of the term
     */
    int addTerm(const char * fieldName, uint64_t termSign, uint32_t docId, uint8_t firstOcc);


    /**
     * 查看是否有这个字段的倒排索引
     *
     * @param fieldName   字段名
     *
     * @return  true: Yes;  false: No
     */
    bool hasField(const char * fieldName);


    /**
     * 获取字段个数
     * @return  字段个数
     */
    inline int getFieldNum() { return _fieldNum; }


    /**
     * 输出所有字段的二进制文件， 包括一些辅助信息
     *
     * @param flag 是否导出文本索引(索引对比使用)
     *
     * @return  < 0: error, >= 0 success
     */
    int dump(bool flag = false);


    /**
     * 打开指定的路径， 查找并加载所有的索引文件
     *
     * @param path  DIR of index files
     *
     * @return  < 0: error, >= 0 success
     */
    int open(const char * path);


    /**
     * 关闭整个索引
     *
     * @return  < 0: error, >= 0 success
     */
    int close();


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


private:
    // 构造函数、析构函数
    IndexIncManager();
    virtual ~IndexIncManager();

private:
    static IndexIncManager _indexIncInstance;         // 惟一实例

private:
    uint64_t _nidSign;                         // nid 需要做index查询
    uint32_t _maxDocNum;                       // 增量索引文档数量(以全量为基础)
    uint32_t _FullDocNum;                      // 全量索引文档数量
    uint32_t _maxIncNum;                       // 增量最大数据量

    char _idxPath[PATH_MAX];                   // 存储路径
    util::HashMap<uint64_t, void*> _fieldMap;  // 存储 <fieldNameSign IdxFieldBuilder>
    int   _fieldNum;
    bool  _initFlag;
    bool  _sync;                               // 增量数据持久化flag
    bool  _export;                             // 关闭时是否导出增量索引(主要用于测试) 
};



}



#endif /* INDEXINCMANAGER_H_ */
