/*
 * IndexFieldReader.h
 *
 *  Created on: 2011-3-5
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 打开一个字段所属的索引文件, 提供查找功能
 */


#ifndef INDEXFIELDREADER_H_
#define INDEXFIELDREADER_H_

#include "MMapFile.h"
#include "index_struct.h"
#include "IndexTermFactory.h"
#include "util/MemPool.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexTerm.h"

namespace index_lib
{

/**
 * 实现 1个字段索引的  的 数据加载和 查找
 */
class IndexFieldReader
{
public:
    // 构造函数、析构函数
    IndexFieldReader(int32_t maxOccNum, int32_t maxDocNum);
    virtual ~IndexFieldReader();



    /**
     * 打开指定的路径， 查找并加载所有的索引文件
     *
     * @param path  DIR of index files
     *
     * @return  false: error
     */
    int open(const char * path, const char * fieldName, bool bPreLoad = false);


    /**
     * 关闭整个索引
     */
    void close();


    /**
     * 获得一个term对应的索引信息， 并保存在一个new出来的对象中
     *
     * @param pMemPool    内存分配器
     * @param term        字段值
     *
     * @return NULL: 不存在这个term;
     */
    IndexTerm * getTerm(MemPool * pMemPool, uint64_t termSign);


    /**
     * * 输出index部分的信息，包括有哪些字段、文档数量、term数量等等
     * *
     * * @param info   输出的index信息内存
     * * @param maxlen 输出信息的最大长度
     * *
     * * @return < 0 失败， >=0 信息长度
     * */
    int getIndexInfo(char* info, int maxlen);


    /**
     * 遍历当前字段的所有term
     *
     * @param termSign    返回的当前term的签名
     *
     * @return 0：成功;   -1：失败
     */
    int  getFirstTerm(uint64_t &termSign);
    int  getNextTerm(uint64_t  &termSign);


    friend class testBitmapNext;
private:

    char  _path[PATH_MAX];             // 索引所在目录
    char  _fieldName[PATH_MAX];        // 字段名字

    int32_t _maxOccNum;                // 最大occ数量
    int32_t _maxDocNum;                // 最大doc数量
    int32_t _bitMapLen;                // bitmap的长度

    idx_dict_t* _invertedDict;         // 1级倒排索引hash表
    idx_dict_t* _bitmapDict;           // 1级bitmap索引hash表

    uint32_t _invertedIdx2Num;                       // 2级倒排索引文件个数
    store::CMMapFile* _invertedIdx2[MAX_IDX2_FD];    // 2级倒排索引文件mmap到内存后的指针, MMapFile 句柄数组
    store::CMMapFile*  _bitmapIdx2;                  // 2级bitmap索引mmap到内存后的指针, MMapFile 句柄

    IndexTermFactory _indexTermFactory;              // 根据指定条件生成IndexTerm的子类

    uint32_t      _travelPos;          // term遍历当前位置
    idx_dict_t  * _travelDict;         // term遍历当前索引，bitmap/invert

};



}



#endif /* INDEXFIELDREADER_H_ */
