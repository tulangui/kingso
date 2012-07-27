/******************************************************************
 * IndexLib.h
 *
 *  Created on: 2011-4-7
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 提供 index_lib 这个模块的全局的函数功能
 *
 ******************************************************************/

#ifndef INDEXLIB_H_
#define INDEXLIB_H_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "commdef/commdef.h"
#include "util/hashmap.h"
#include "util/common.h"
#include "util/Log.h"
#include "mxml.h"

#define   NID_FIELD_NAME             "nid"                       // nid 对应的字段名称
#define   MAX_PROFILE_FIELD_NUM      256                         // 正排索引支持的最大字段个数
#define   MAX_INDEX_FIELD_NUM        128                         // 倒排索引支持的  最大字段个数
#define   MAX_SEGMENT_NUM            127                         // Profile索引的最大分段数量
#define   MAX_ENCODE_VALUE_NUM       1024                        // 编码表支持的最大字段值的个数
#define   MAX_BITFIELD_NUM           MAX_PROFILE_FIELD_NUM       // bit_record中最多支持的字段数量
#define   MAX_BITFIELD_VALUE(length) ((uint32_t)(1<<length) - 1)   // 位域字段对应的最大值
#define   MAX_PROFILE_ORGINSTR_LEN   (MAX_ENCODE_VALUE_NUM * 100)

#define   INVALID_BITFIELD_LEN       32                          // bit_record中支持的单个字段最大位数值+1
#define   INVALID_MULTI_VALUE_NUM    (MAX_ENCODE_VALUE_NUM + 1)
#define   INVALID_BITFIELD_VALUE     0xFFFFFFFF                  // 非法的位域字段值，用于读取失败情况的返回值

#define   PROFILE_SEG_SIZE_BIT_NUM   20
#define   DOCNUM_PER_SEGMENT         (1<<PROFILE_SEG_SIZE_BIT_NUM) // 每个段文件中的doc数量上限（1M）

#define   PROFILE_HEADER_DOCID_STR   "vdocid"                  // doc_header文件的第一个字段（内部docid）字符串标识
#define   PROFILE_KV_DELIM           ':'

/** 编码表文件相关后缀定义 */
#define   ENCODE_IDX_SUFFIX        ".encode_idx"
#define   ENCODE_CNT_SUFFIX        ".encode_cnt"
#define   ENCODE_EXT_SUFFIX        ".encode_ext"
#define   ENCODE_DESC_SUFFIX       ".encode_desc"

/** Profile主表段文件的前/后缀定义 */
#define   PROFILE_MTFILE_PREFIX    "profile_group"
#define   PROFILE_MTFILE_SUFFIX    ".seg"
#define   PROFILE_MTFILE_DESC      "profile.desc"
#define   PROFILE_MTFILE_DOCCOUNT  "profile.count"


/** Profile非法字段值定义  */
#define   INVALID_INT8      0x7F
#define   INVALID_INT16     0x7FFF
#define   INVALID_INT32     0x7FFFFFFF
#define   INVALID_INT64     0x7FFFFFFFFFFFFFFF
#define   INVALID_UINT8     ((uint8_t)-1)
#define   INVALID_UINT16    ((uint16_t)-1)
#define   INVALID_UINT32    ((uint32_t)-1)
#define   INVALID_UINT64    ((uint64_t)-1)
#define   INVALID_FLOAT     3.40282347e+38F
#define   INVALID_DOUBLE    1.7976931348623157e+308


/** docId高低位拆分 */
#define IDX_GET_BASE(doc_id)  ((doc_id) >> 16)             // 取得docId的高16位
#define IDX_GET_DIFF(doc_id)  ((doc_id) &  0x0000FFFF)     // 取得docId的低16位



/** 打印调试信息  */
#ifdef DEBUG
/* 不满足 即返回 */
#   define return_if_fail(p) if (!(p)) {\
                                 printf("%s:%d Warning: "#p" failed.\n", __FUNCTION__, __LINE__);\
                                 return;\
                             }
/* 不满足 即返回指定值 */
#   define return_val_if_fail(p, ret) if (!(p)) \
                                      { \
                                          printf("%s:%d Warning: "#p" failed.\n", __FUNCTION__, __LINE__); \
                                          return (ret); \
                                      }
#else
#   define return_if_fail(p)                if (!(p)) { return; }
#   define return_val_if_fail(p, ret)       if (!(p)) { return (ret); }
#endif





namespace index_lib
{


const uint32_t  PROFILE_SEG_SIZE_BIT_MASK = 0xFFFFF;



/**
 * 解析配置文件， 初始化 index_lib内部的各个模块
 *
 * @param   config  xml的配置节点。  内部的子节点包括了 索引的各个配置项
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int init( mxml_node_t * config );




/**
 * 为重新进行索引内部数据的整理，  解析配置文件， 初始化 index_lib内部的各个模块
 *
 * @param   config  xml的配置节点。  内部的子节点包括了 索引的各个配置项
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int init_rebuild( mxml_node_t * config );



/**
 * 关闭index_lib内部开辟和打开的各种资源
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int destroy();


/**
 * 计算在特定排序条件下的 高频词的 新的签名，  在索引build时使用
 *
 * @param psFieldName       ps排序字段的名字
 * @param sortType          排序类型， 0：正排   1:倒排
 * @param termSign          term的签名
 *
 * @return 64 bit signature;  0： error
 */
uint64_t HFterm_sign64(const char * psFieldName,
                       uint32_t     sortType,
                       uint64_t     termSign);


/**
 * 计算在特定排序条件下的 高频词的 新的签名，  在  检索   时使用
 * 计算规则: 排序字段名 + 空格 + 排序方式  + 空格 + 原64位签名
 *
 * @param psFieldName       ps排序字段的名字
 * @param sortType          排序类型， 0：正排   1:倒排
 * @param term
 *
 * @return 64 bit signature;  0： error
 */
uint64_t HFterm_sign64(const char * psFieldName,
                       uint32_t     sortType,
                       const char * term);





/**
 * 供hashMap回调的比较函数
 *
 * @return  <>0:不相等;  ==0:相等
 */
inline int32_t hashmap_cmpInt (const void * key1, const void * key2)
{
    if ( key1 == key2 )  return 0;

    return -1;
}


/**
 * 供hashMap回调的hash函数, 计算 uint64_t 的hash值
 *
 * @param key  uint64的值 转成的指针
 *
 * @return
 */
inline uint64_t hashmap_hashInt (const void * key)
{
    return ( uint64_t ) key;
}


}


#endif /* INDEXLIB_H_ */
