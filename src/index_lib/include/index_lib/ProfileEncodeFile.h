/** ProfileEncodeFile.h
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision:  $
 *
 * $LastChangedDate: 2011-03-16 09:50:55 +0800 (Wed, 16 Mar 2011) $
 *
 * $Id: ProfileEncodeFile.h  2011-03-16 01:50:55 pujian $
 *
 * $Brief: 编码表功能的实现类 $
 *******************************************************************
*/
#ifndef _KINGSO_INDEXLIB_PROFILE_ENCODE_FILE_H
#define _KINGSO_INDEXLIB_PROFILE_ENCODE_FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <unistd.h>

#include "IndexLib.h"
#include "ProfileStruct.h"
#include "ProfileMMapFile.h"
#include "ProfileHashTable.h"
#include "ProfileVarintWrapper.h"
#include "ProfileSyncManager.h"

/* 宏定义如下 */
/** 无效的编码值 */
#define INVALID_ENCODEVALUE_NUM  INVALID_NODE_POS

/** 默认的哈希桶数量 */
#define DEFAULT_ENCODE_HASHSIZE  10000019
/** 默认的哈希表节点数量 */
#define DEFAULT_ENCODE_BLOCKNUM  10000000

/** 编码字段单个值的最大字符串长度 */
#define MAX_VALUE_STR_LEN        1024
/** String类型字段值的长度描述数据占用的空间大小, 2个byte */
#define STRING_DESC_SIZE         2
/** 最大编码表文件的段个数 */
#define MAX_ENCODE_SEG_NUM       100
/** 初始基础划分倍数  */
#define BASE_DEGREE_TIMES  20

/** 段空间大小对应的最小位长 */
#define BASE_DEGREE_MIN    20

#ifdef DEBUG

/** 读取使用varint编码的字段值的通用逻辑定义  */
#define GET_ENCODE_VARINT_VALUE(DATA_TYPE, INVALID_RETURN) \
        if (unlikely((_curContentSize >> 1) <= offset)) { \
            return INVALID_RETURN; \
        } \
        uint32_t segPos          = ((size_t)offset << 1) >> _cntDegree;\
        uint32_t inSegPos        = ((size_t)offset << 1) &  _cntMask; \
        char     *pValue         = _cntFileAddr[segPos] + inSegPos; \
        uint32_t num = *((uint16_t *)pValue); \
        DATA_TYPE value = 0; \
        if (unlikely(_pVarintWrapper->decode((uint8_t *)(pValue + sizeof(uint16_t)), num, index, value) < 0)) { \
            return INVALID_RETURN; \
        } \
        return value; \

#else

/** 读取使用varint编码的字段值的通用逻辑定义  */
#define GET_ENCODE_VARINT_VALUE(DATA_TYPE, INVALID_RETURN) \
        uint32_t segPos          = ((size_t)offset << 1) >> _cntDegree;\
        uint32_t inSegPos        = ((size_t)offset << 1) &  _cntMask; \
        char     *pValue         = _cntFileAddr[segPos] + inSegPos; \
        uint32_t num = *((uint16_t *)pValue); \
        DATA_TYPE value = 0; \
        _pVarintWrapper->decode((uint8_t *)(pValue + sizeof(uint16_t)), num, index, value); \
        return value; \

#endif

namespace index_lib
{

#pragma pack(1)
typedef union _union_str_payload
{
    uint32_t offset;           //存储字符串的偏移量
    char     str[7];           //存储实际字符串(长度不超过6)
}StringPayload;

typedef struct _struct_single_str
{
    uint8_t       flag;       //0, 表示string为空值；1，表示payload中为实际字符串；2，表示payload中为字符串偏移量
    StringPayload payload;    //字符串偏移量
}SingleString;

/**
 * 定义编码字段描述信息
 */
typedef struct _encode_desc_cnt_info
{
    size_t       cur_content_size;   //字段对应的实际字段内容占用的空间单元大小,对应.cnt文件的有效数据空间
    uint32_t     cur_encode_value;   //字段对应的编码值数量（编码值顺序递增）
    uint32_t     cntNewFileNum;      //新增cnt分段文件的个数
    uint32_t     cntSegNum;          //cnt文件总的分段个数
}encode_desc_cnt_info;

typedef struct _encode_desc_ext_info
{
    uint32_t     cur_extend_size;    //扩展单值字符串存储占用空间的大小，对应.ext文件的有效数据空间
    uint32_t     extNewFileNum;      //新增ext分段文件的个数
    uint32_t     extSegNum;          //ext文件总的分段个数
}encode_desc_ext_info;

typedef struct _encode_desc_meta_info
{
    PF_DATA_TYPE type;               //字段值的数据类型
    size_t       unit_size;          //字段值单元的空间大小（byte）,如uin32_t类型空间大小为4字节
    bool         varintEncode;       //字段是否进行了varint压缩编码
    bool         update_overlap;     //更新过程中是否需要排重
    uint32_t     cntDegree;          //cnt分段单位的位长
    uint32_t     cntMask;            //cnt分段掩码
    uint32_t     extDegree;          //ext分段单位的位长
    uint32_t     extMask;            //ext分段掩码
}encode_desc_meta_info;

#pragma pack()

/**
 * 编码表功能实现类
 */
class ProfileEncodeFile
{
public:
    /**
     * 构造单个字段对应的编码表对象，ProfileEncodeFile构造函数
     * @param type   字段存储的数据类型
     * @param path   编码数据的存储路径
     * @param name   编码字段对应的外部名称，作为不同类型编码文件的名字前缀
     * @param single 是否是单值字段(true,单值；false，多值)
     */
    ProfileEncodeFile(PF_DATA_TYPE type, const char* path, const char* name, EmptyValue empty, bool single = false);

    /**
     * 释放编码表对象，ProfileEncodeFile类的析构函数
     */
    virtual ~ProfileEncodeFile();

    /**
     * 获取字段编码表对应的字段数据类型
     * @return 字段类型
     */
    inline PF_DATA_TYPE getDataType()  { return _dataType; }

    /**
     * 判断字段是否为单值字段
     * @return  true,单值；false，多值
     */
    inline bool isSingle() { return _single;}

    /**
     * 获取字段单个字段值对应的存储空间大小
     * @return 字段数据单元大小
     */
    inline size_t   getUnitSize()  { return _unitSize; }

    /**
     * 获取当前的编码值个数
     * @return 编码值个数
     */
    inline uint32_t getCurEncodeValue() { return _curEncodeValue; }

    /**
     * 获取是否使用了 groupvarint压缩
     * @return true:是   false:否
     */
    inline bool isEncoded() { return (_pVarintWrapper != NULL)?true:false; }

    /**
     * 创建字段编码文件，用于创建字段编码表,包括: .idx文件 .unit文件 .cnt文件
     * @param encode    编码表存储数据是否进行varint编码压缩
     * @param overlap   是否支持更新排重功能(是否dump哈希表)
     * @param hashsize  哈希映射文件中的哈希桶数量
     * @param blocknum  哈希映射文件中的初始哈希节点数量
     * @return          true 创建成功; false 创建失败
     */
    bool      createEncodeFile(bool encode = false, bool overlap = true, uint32_t hashsize = DEFAULT_ENCODE_HASHSIZE, uint32_t blocknum = DEFAULT_ENCODE_BLOCKNUM);

    /**
     * 加载字段编码文件,用于读取字段编码表
     * @param  update 是否需要针对更新进行空间扩充
     * @return        true加载成功；false 加载失败
     */
    bool      loadEncodeFile(bool update = true);


    /**
     * 加载字段编码文件,用于重新修改
     * @return  true加载成功；false 加载失败
     */
    bool  loadEncodeFileForModify();


    /**
     * 将内存中的编码表数据dump到硬盘
     * @return      true dump成功;false dump失败
     */
    bool      dumpEncodeFile();

    /**
     * 将内存中的编码表描述信息到硬盘
     * @param  sync   是否同步更新到硬盘
     * @return      true dump成功;false dump失败
     */
    bool      flushDescFile(bool sync = true);

    /**
     * 向编码表中添加单个字段值
     * @param str    单个宝贝字段的取值字符串
     * @param len    取值字符串的长度
     * @param delim  取值字符串中字段值的分隔符号(默认空格)
     * @param offset 编码存储后对应的偏移位置
     * @return       true 添加编码值成功; false 添加编码值失败
     */
    bool      addEncode(const char* str, uint32_t len, uint32_t &offset, char delim = ' ');


    /**
     * 获取编码值对应的单值(基本类型)字段的字段值起始地址
     * @param  index        目标值下标位置
     * @return              字段值起始地址
     */
    inline char*     getSingleBaseValuePtr(uint32_t index)
    {
        #ifdef DEBUG
        if (unlikely((index >= _curEncodeValue))) {
            return NULL;
        }
        #endif
        uint32_t segPos   = ((size_t)index << _unitSizeDegree) >> _cntDegree;
        uint32_t inSegPos = ((size_t)index << _unitSizeDegree) &  _cntMask;
        return (_cntFileAddr[segPos] + inSegPos);
    }

    /**
     * 获取编码值对应的单值(字符串类型)字段的字段值起始地址
     * @param  index        目标值下标位置
     * @return              字段值起始地址
     */
    inline char*     getSingleStrValuePtr(uint32_t index)
    {
        #ifdef DEBUG
        if (unlikely((index >= _curEncodeValue))) {
            return NULL;
        }
        #endif
        uint32_t     segPos    = ((size_t)index << _unitSizeDegree) >> _cntDegree;
        uint32_t     inSegPos  = ((size_t)index << _unitSizeDegree) &  _cntMask;
        SingleString *pCntNode = (SingleString *)(_cntFileAddr[segPos] + inSegPos);

        if (likely(pCntNode->flag == 1)) {
            return pCntNode->payload.str;
        }
        if (pCntNode->flag == 2) {
            uint32_t offset      = pCntNode->payload.offset;
            uint32_t extSegPos   = offset >> _extDegree;
            uint32_t extInSegPos = offset & _extMask;
            return (_extFileAddr[extSegPos] + extInSegPos);
        }
        return NULL;
    }

    /**
     * 获取编码值对应的多值字段值单元的数量
     * @param offset        目标偏移量
     * @return              编码值对应的字段值数量
     */
    inline uint32_t  getMultiEncodeValueNum(uint32_t offset) {
        #ifdef DEBUG
        if (unlikely((_curContentSize >> 1) <= offset)) {
            return 0;
        }
        #endif
        uint32_t segPos          = ((size_t)offset << 1) >> _cntDegree;
        uint32_t inSegPos        = ((size_t)offset << 1) &  _cntMask;
        return *((uint16_t*)(_cntFileAddr[segPos] + inSegPos));
    }

    /**
     * 查找字段取值字符串对应的编码值
     * @param str   字段取值字符串
     * @param len   取值字符串长度
     * @return      存在:对应的编码值;不存在:INVALID_NODE_POS
     */
    inline uint32_t  getEncodeIdx(const char* str, uint32_t len)
    {
        if (unlikely( _pHashTable == NULL )) return INVALID_NODE_POS;

        // 传入的原始字符串是

        uint32_t * pValue = (uint32_t *) _pHashTable->findValuePtr(str, len);

        if (unlikely( pValue == NULL )) return INVALID_NODE_POS;

        return *pValue;
    }

    /**
     * 通过编码值，获取对应字段的时间字段内容，int32类型
     * @param offset  字段值偏移量
     * @param index   字段数据单元的位置下标（用于查询多值字段，从0开始，默认获取第一个字段值)
     * @return        字段值;INVALID_INT32 for ERROR
     */
    inline int32_t   getEncodeVarInt32 (uint32_t offset, uint32_t index = 0)
    {
        GET_ENCODE_VARINT_VALUE(int32_t, INVALID_INT32);
    }

    /**
     * 通过编码值，获取对应字段的时间字段内容，uint32类型
     * @param offset  字段值偏移量
     * @param index   字段数据单元的位置下标（用于查询多值字段，从0开始，默认获取第一个字段值)
     * @return        字段值;INVALID_UINT32 for ERROR
     */
    inline uint32_t  getEncodeVarUInt32(uint32_t offset, uint32_t index = 0)
    {
        GET_ENCODE_VARINT_VALUE(uint32_t, INVALID_UINT32);
    }

    /**
     * 通过编码值，获取对应字段的时间字段内容，int64类型
     * @param offset  字段值偏移量
     * @param index   字段数据单元的位置下标（用于查询多值字段，从0开始，默认获取第一个字段值)
     * @return        字段值;INVALID_INT64 for ERROR
     */
    inline int64_t   getEncodeVarInt64 (uint32_t offset, uint32_t index = 0)
    {
        GET_ENCODE_VARINT_VALUE(int64_t, INVALID_INT64);
    }

    /**
     * 通过编码值，获取对应字段的时间字段内容，uint64类型
     * @param offset  字段值偏移量
     * @param index   字段数据单元的位置下标（用于查询多值字段，从0开始，默认获取第一个字段值)
     * @return        字段值;INVALID_UINT64 for ERROR
     */
    inline uint64_t  getEncodeVarUInt64(uint32_t offset, uint32_t index = 0)
    {
        GET_ENCODE_VARINT_VALUE(uint64_t, INVALID_UINT64);
    }

    /**
     * 通过编码值，获取对应字段值的个数和起始地址信息
     * @param   offset     字段值偏移量
     * @param   num [out]  字段值个数
     * @return             字段值对应的起始地址位置
     */
    inline char*     getMultiEncodeValueInfo(uint32_t offset, uint32_t &num)
    {
        #ifdef DEBUG
        if (unlikely((_curContentSize >> 1) <= offset)) {
            return NULL;
        }
        #endif
        uint32_t segPos          = ((size_t)offset << 1) >> _cntDegree;
        uint32_t inSegPos        = ((size_t)offset << 1) &  _cntMask;
        char     *pValue         = _cntFileAddr[segPos] + inSegPos;
        num = *((uint16_t *)pValue);
        return (pValue + sizeof(uint16_t));
    }

    /**
     * 通过编码值，获取对应字段值的起始地址信息
     * @param   offset     字段值偏移量
     * @return             字段值对应的起始地址位置
     */
    inline char*     getMultiEncodeValuePtr(uint32_t offset)
    {
        #ifdef DEBUG
        if (unlikely((_curContentSize >> 1) <= offset)) {
            return NULL;
        }
        #endif
        uint32_t segPos          = ((size_t)offset << 1) >> _cntDegree;
        uint32_t inSegPos        = ((size_t)offset << 1) &  _cntMask;
        return (_cntFileAddr[segPos] + inSegPos + sizeof(uint16_t));
    }

    /**
     * 设置数据同步管理器
     */
    void setSyncManager(ProfileSyncManager * syncMgr);


    /**
     * 重建hashtable，
     * 因为updateOverlap==false的字段没有idx文件
     * 需要初始化一个hashtable
     *
     * @return  成功返回true，失败返回false。
     */
    bool rebuildHashTable( uint32_t hashsize = DEFAULT_ENCODE_HASHSIZE );


    /**
     * 将内容打印输出成文本， 用于测试
     */
    void print_test( );


    /**
     * 返回_updateOverlap的值
     * @return  true:是, false:否：表明没有内含的hash表了
     */
    inline bool isUpdateOverlap()  {  return _updateOverlap;  }


    /**
     * 将2个编码表合并起来
     *
     * @param    pEncode    待合并的编码表对象
     * @param    signMap    合并后，被合并掉的编码表中的 旧编码值 和 新编码值的对应关系
     *
     * @return   添加成功返回0；否则返回-1。
     */
    int merge( ProfileEncodeFile * pEncode, hashmap_t signMap );

private:

    /**
     * 添加同步数据到同步管理器中
     */
    void putSyncInfo(char * addr, uint32_t len, ProfileMMapFile * file);

    /**
     * 通过数据类型获取对应的存储空间，String类型返回0，需要通过setUnitSize进行自定义设置
     * @param type 数据类型
     * @return     字段空间大小
     */
    size_t getDataTypeSize(PF_DATA_TYPE type);

    /**
     * 释放资源（哈希表，内存映射）
     */
    void   freeResource();

    /**
     * 加载编码表相关的.desc文件
     * @param  szFileName    desc文件名
     * @return               true,加载成功；false,加载失败
     */
    bool  loadEncodeDescFile(const char *szFileName);

    /**
     * 完成编码表的分段加载逻辑(初始加载索引/增量持久化后重新加载等情况)
     * @param  update     是否支持更新
     * @return            true；OK; false；ERROR
     */
    bool  loadEncodeSegInfo(bool update);

    /**
     * 向编码表中添加一个单值段值
     * @param str    单个宝贝字段的取值字符串
     * @param len    取值字符串的长度
     * @param delim  取值字符串中字段值的分隔符号(默认空格)
     * @param offset 编码存储的偏移位置
     * @return       true 添加编码值成功; false 添加编码值失败
     */
    bool  addSingleEncodeValue(const char* str, uint32_t len, uint32_t &offset);

    /**
     * 向编码表中添加一个多值字段值
     * @param str    单个宝贝字段的取值字符串
     * @param len    取值字符串的长度
     * @param delim  取值字符串中字段值的分隔符号(默认空格)
     * @param offset 编码存储的偏移位置
     * @return       true 添加编码值成功; false 添加编码值失败
     */
    bool  addMultiEncodeValue(const char* str, uint32_t len, char delim, uint32_t &offset);

    /**
     * 解析并添加单值基本类型(float/double/int/uint)字段值，被addSingleEncodeValue调用
     * @param  sign      字段值对应的哈希值
     * @param  str       字段值的字符串
     * @param  len       字段值的字符串长度
     * @param  offset    编码存储的偏移位置(单元位置)
     * @return           true, OK; false, ERROR
     */
    bool  addSingleBaseTypeEncode(int64_t sign, const char* str, uint32_t len, uint32_t &offset);

    /**
     * 解析并添加单值字符串类型(DT_STRING)字段值，被addSingleEncodeValue调用
     * @param  sign      字段值对应的哈希值
     * @param  str       字段值的字符串
     * @param  len       字段值的字符串长度
     * @param  offset    编码存储的偏移位置(单元位置)
     * @return           true, OK; false, ERROR
     */
    bool  addSingleStringTypeEncode(int64_t sign, const char* str, uint32_t len, uint32_t &offset);

    /**
     * 解析并添加字段值，不进行varint压缩，被addMultiEncodeValue调用
     * @param  sign      字段值对应的哈希值
     * @param  str       字段值的字符串
     * @param  len       字段值的字符串长度
     * @param  delem     字段值的分隔符
     * @param  offset    编码存储的偏移位置(字节位置)
     * @return           true, OK; false, ERROR
     */
    bool   addNoVarintEncode(int64_t sign, const char* str, uint32_t len, char delim, uint32_t &offset);

    /**
     * 解析并添加字段值，进行varint压缩，被addMultiEncodeValue调用
     * @param  sign      字段值对应的哈希值
     * @param  str       字段值的字符串
     * @param  len       字段值的字符串长度
     * @param  delem     字段值的分隔符
     * @param  offset    编码存储的偏移位置(字节位置)
     * @return           true, OK; false, ERROR
     */
    bool   addVarintEncode(int64_t sign, const char* str, uint32_t len, char delim, uint32_t &offset);

    /**
     * 根据编码表元数据，设置编码表的unitSize信息
     * @param  encode    是否是varint压缩编码字段
     */
    void   setEncodeUnitSize(bool encode);

    /**
     * 根据文件内容大小自动分段
     * @param  num       内容大小
     * @param  segNum    分段个数
     * @param  rmask     段的掩码值
     * @param  diff      最后一个段的剩余空间大小
     * @return           段的位长
     */
    uint32_t getBaseDegree(size_t num, uint32_t &segNum, uint32_t &rmask, size_t &diff);

    /**
     * 获取字段值单元长度对应的指数(1->0, 2->1, 4->2, 8->3)
     * @return           字段值对应的指数
     */
    uint32_t getUnitSizeDegree(size_t _unitSize);

    /**
     * 添加新的cnt段文件
     * @return           true,OK; false,ERROR
     */
    bool    newContentSegmentFile();

    /**
     * 添加新的ext段文件
     * @return           true,OK; false,ERROR
     */
    bool    newExtendSegmentFile();

    /**
     * 重置contentSize值, 保证contentSize为2的整数倍
     * @param  add       contentSize新增空间大小
     */
    inline  size_t resetCurContentSize(size_t add)
    {
        if (add % 2 == 0) {
            _curContentSize += add;
            return add;
        }
        _curContentSize += (add + 1);
        return (add + 1);
    }

    /**
     * 根据逻辑段号获得对应的MMapFile
     * @param segNum     逻辑段号
     * @param cnt        是否是实际存储内容文件,false:扩展字符串文件
     */
    ProfileMMapFile * getMMapFileBySegNum(uint32_t segNum, bool cnt = true);


    /**
     * 将输入的字符串，解析成值的数组
     *
     * @return       true:成功; false:失败
     */
    /*
    bool  parseStr2Val(  const char * str,              // 单个宝贝字段的取值字符串
                         uint32_t     strLen,           // 取值字符串的长度
                         char         delim = ' ',      // 取值字符串中字段值的分隔符号(默认空格)
                         char       * outBuf,           // 用于保存输出的buffer
                         uint32_t     outBufLen,        // 输入的buffer长度
                         int        & outLen,           // 输出到buffer中的总共的byte数
                         uint32_t   & itemNum           // 输出的元素得个数
                      );
    */

protected:
    char _encodePath[PATH_MAX];          //字段编码表存储的目标路径（目录）
    char _encodeName[MAX_FIELD_NAME_LEN];//字段编码表的外部名称（文件名前缀)
    bool             _single;            //编码字段是否是单值字段
    bool             _updateOverlap;     //字段是否需要支持更新排重

    ProfileMMapFile  *_pCntFile;         //字段值实际存储文件的mmap文件指针
    ProfileHashTable *_pHashTable;       //字符串签名到编码值映射关系哈希表指针

    PF_DATA_TYPE     _dataType;          //字段对应的数据类型
    size_t           _unitSize;          //字段值存取单元的空间大小
    uint32_t         _unitSizeDegree;    //字段值单元空间的对应指数(1->0, 2->1, 4->2, 8->3)

    uint32_t         _curEncodeValue;    //当前的字段编码值个数
    size_t           _curContentSize;    //当前的实际内容的存储空间
    size_t           _totalContentSize;  //当前cnt文件的总空间

    ProfileVarintWrapper *_pVarintWrapper;//VarintWrapper指针

    bool             _loadFlag;                             //当前的编码表文件是否load过，true:load过；false：create产生
    char             *_cntFileAddr [MAX_ENCODE_SEG_NUM];    //cnt文件对应的分段起始地址数组结构
    ProfileMMapFile  *_pNewCntFile [MAX_ENCODE_SEG_NUM];    //更新过程新分配的cnt分段文件的mmap对象指针数组
    uint32_t         _cntNewFileNum;                        //新分配的cnt分段文件的个数
    uint32_t         _cntSegNum;                            //cnt文件分段总数
    uint32_t         _cntDegree;                            //cnt文件分段的单元空间对应指数
    uint32_t         _cntMask;                              //cnt文件分段的计算掩码(1 << _cntDegree -1)

    EmptyValue       _emptyValue;                           //字段对应的空值结构

    /********************** 单值字符串类型专用 ************************/
    uint32_t         _totalExtContentSize;                  //扩展文件中的总空间大小
    uint32_t         _extContentSize;                       //扩展文件中的可用空间大小
    ProfileMMapFile  *_pSingleStrExtFile;                   //用于存储超长（超过6个byte)的单值字符串

    char             *_extFileAddr   [MAX_ENCODE_SEG_NUM];  //ext文件对应的分段起始地址数组结构
    ProfileMMapFile  *_pNewStrExtFile[MAX_ENCODE_SEG_NUM];  //更新过程新分配的ext扩展文件的mmap对象指针数组
    uint32_t         _extNewFileNum;                        //新分配的ext文件的个数
    uint32_t         _extSegNum;                            //ext文件分段总数
    uint32_t         _extDegree;                            //ext文件分段的空间大小对应指数
    uint32_t         _extMask;                              //ext文件分段的计算掩码

    ProfileSyncManager *_syncMgr;                           //更新数据使用的Sync管理器对象

    int              _desc_fd;                              //描述文件句柄
};

}

#endif //_KINGSO_INDEXLIB_PROFILE_ENCODE_FILE_H
