/*
 * IndexTerm.h
 *
 *  Created on: 2011-3-4
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 这是一个基类。  根据索引的类型和压缩类型 ， 生成各种子类
 *              数据成员： 字段名  字段签名 对应的倒排链的指针  bitmap的指针
 *                    索引包含的doc数， 检索过程中的临时位置
 */

#ifndef INDEXTERM_H_
#define INDEXTERM_H_

#include <stdint.h>
#include "commdef/commdef.h"

namespace index_lib
{

struct IndexTermInfo {
  uint32_t docNum;      // doc链表中含有的docid数量
  uint32_t occNum;      // occ链表中含有的occ数量
  uint8_t zipType;      // 压缩类型，0:没有倒排链  1: 不压缩 2: 高位压缩
  uint8_t bitmapFlag;   // 是否有bitmap索引，0: 没有 1： 有

  uint8_t delRepFlag;   // 是否排重信息，0: 不排重， 1： 排重
  uint32_t bitmapLen;   // bitmap索引长度(如果存在), uint64 数组长度
  uint32_t maxOccNum;   // field本身限制的最多支持的occ数量
  uint32_t maxDocNum;   // 总的doc数量(容量)
  uint32_t not_orig_flag;    // 是否是重新打开所后追加的token的标记，0表示不是， 1表示是
};

/**
 * 实现倒排链的 查找和遍历
 */
class IndexTerm
{
public:
    // 构造函数、析构函数
    IndexTerm();
    virtual ~IndexTerm();

    /**
     * 查询指定的docId 是否在链条中存在
     *
     * @param  docId 需要查找的docId
     *
     * @return 找到:原id; 没有找到:比传入大的id; 链条结束:INVALID_DOCID
     */
    inline virtual uint32_t seek(uint32_t docId);


    /**
     * 获取当前docid对应的occ
     *
     * @param count       传出的occ个数
     *
     * @return ; 正常：occ数组的指针; NULL： 无occ
     */
    virtual uint8_t* const getOcc(int32_t &count);


    /**
     * 顺序向下取一个docid
     *
     * @param int32_t step, 结果集cache专用，next 不能获取occ信息
     *
     * @return 正常docid; INVALID_DOCID： 链条结束
     */
    inline virtual uint32_t next();
    inline virtual uint32_t next(int32_t step);


    /**
     * 将当前指针定位到指定的docId处，不存在则定位到比指定docid大的最小docid处
     *
     * @param  docId 需要定位的docId
     *
     * @return 找到:原id; 没有找到:比传入大的id; 链条结束:INVALID_DOCID
     */
    inline virtual uint32_t setpos(uint32_t docId);


    /**
     * 获得一个term对应的bitmap索引信息
     *
     * @param term        字段值
     *
     * @return NULL: 不存在bitmap索引
     */
    const uint64_t* getBitMap() { return _pBitMap; }


    /**
     * 顺序向下取一个重复文档的docid
     *
     * @param docid seek或next得到的docid,检测当前docid是否为指定的docid，
     *              如与当前不同，则设置当前docid为指定docid
     *
     * @return 正常docid; INVALID_DOCID： 链条结束
     */
    //inline virtual uint32_t expand() = 0;
    //inline virtual uint32_t expand(uint32_t docId) = 0;

    /**
     * 获取倒排链信息
     *
     * @return 倒排链信息结构体指针
     */
    const IndexTermInfo* getTermInfo() {return &_cIndexTermInfo;}

    /**
     * 获取倒排链包括的doc数
     *
     * @return doc数
     */
    uint32_t getDocNum() {return _cIndexTermInfo.docNum;}

    /**
     * 获取倒排链的类型
     *
     * @return 类型
     */
    IDX_DISK_TYPE getIdxType()
    {
        if ( _cIndexTermInfo.zipType    == 1 )  return TS_IDT_NOT_ZIP;
        if ( _cIndexTermInfo.bitmapFlag == 1 )  return TS_IDT_ZIP_BITMAP;
        if ( _cIndexTermInfo.zipType    == 2 )  return TS_IDT_ZIP;

        return TS_IDT_NOT_ZIP;
    }


    /**
     * 获取 occ的最大数量。
     *
     * @return 类型
     */
    uint32_t getMaxOccNum() {return _cIndexTermInfo.maxOccNum;}


    /**
     * 设置bitMap指针
     *
     * @param bitMap
     */
    void setBitMap(const uint64_t * bitMap) {_pBitMap = bitMap;}

    /**
     * 设置倒排链指针
     *
     * @param invertList
     */
    void setInvertList(const char * invertList) {_invertList = invertList;}

protected:
    IndexTermInfo _cIndexTermInfo;        // term相关的信息
    const uint64_t* _pBitMap;             // 倒排链的bitmap格式存储
    const char * _invertList;             // 倒排链的指针
};


}



#endif /* INDEXTERM_H_ */
