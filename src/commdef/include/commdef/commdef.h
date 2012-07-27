/** \file
 *******************************************************************
 * $Author: taiyi $
 *
 * $LastChangedBy: taiyi $
 *
 * $Revision: 293 $
 *
 * $LastChangedDate: 2011-04-01 17:32:50 +0800 (Fri, 01 Apr 2011) $
 *
 * $Id: commdef.h 293 2011-04-01 09:32:50Z taiyi $
 *
 * $Brief: kingso common macro defines and data struct$
 *******************************************************************
 */

#ifndef _COMMON_DEFINE_H_
#define _COMMON_DEFINE_H_

// error code define
#define KS_SUCCESS    0 //success
#define KS_EFAILED   -1 //程序处理失败
#define KS_ENOENT    -2 //实体不存在
#define KS_EINTR     -3 //调用被中断
#define KS_EIO       -4 //IO错误
#define KS_EAGAIN    -5 //需要重试
#define KS_ENOMEM    -6 //内存不足，比如分配内存失败时，会返回这个错误
#define KS_EEXIST    -7 //实体已存在
#define KS_EINVAL    -8 //参数无效
#define KS_ETIMEDOUT -9 //超时错误

// search error code
#define KS_SE_QUERYERR -0x1001 //search 查询语句错误
#define KS_SE_INITERR -0x1002 //search init 失败
#define KS_SE_STOPWORD -0x1003 //search 查询传中存在stopword

// 非法值定义
#define   INVALID_DOCID                       0xFFFF0000
#define   INVALID_NID                         0xFFFFFFFFFFFFFFFE
#define   INVALID_DATATYPE_VALUE(DATA_TYPE)  ((DATA_TYPE)-1)          // 各种数据类型的默认错误返回值
#define   INVALID_ENCODE_VALUE               ((uint32_t)-1)           // Profile编码表内部编码值的上限值

#define   MAX_TOKEN_LEN                32                  /* 最大的token字符个数 */
#define   MAX_QYERY_LEN                8192                /* 最大的query字符个数 */
#define   MAX_FIELD_NAME_LEN           64                  /* 最大的字段名字符个数 */
#define   MAX_FIELD_VALUE_LEN          1024                /* 最大的字段值字符个数 (包括 查倒排的条件的总和)*/

// 模块名称，配置文件解析时使用
#define KS_XML_ROOT_NODE_NAME           "modules"
#define KS_MODULE_NAME_INDEX_LIB        "indexLib"
#define KS_MODULE_NAME_UPDATER          "updater"
#define KS_MODULE_NAME_WANGWANG_ONLINE  "wangwangOnline"
#define KS_MODULE_NAME_QUERY_PARSER     "queryParser"
#define KS_MODULE_NAME_SEARCH           "search"
#define KS_MODULE_NAME_FILTER           "filter"
#define KS_MODULE_NAME_SORT             "sort"
#define KS_MODULE_NAME_STATISTIC        "statistic"

// 释放内存， 避免野指针
#define SAFE_FREE(p)   if (p != NULL) {free (p); p = NULL;}
#define SAFE_DELETE(p) if (p != NULL) {delete (p); p = NULL;}

// 索引的存储类型
enum IDX_DISK_TYPE
{
    TS_IDT_NOT_ZIP,               // 只有非压缩类型索引
    TS_IDT_ZIP,                   // 只有压缩类型索引
    TS_IDT_ZIP_BITMAP,            // 同时有压缩索引和bitMap索引
    TS_IDT_BITMAP                 // 只有bitMap索引
};

// profile中存储的数据类型, 对外
enum PF_DATA_TYPE
{
    DT_INT8,
    DT_UINT8,
    DT_INT16,
    DT_UINT16,
    DT_INT32,
    DT_UINT32,
    DT_INT64,
    DT_UINT64,
    DT_FLOAT,
    DT_DOUBLE,
    DT_STRING,
    DT_KV32,
    DT_UNSUPPORT
};

// Attribute Field Flag
enum PF_FIELD_FLAG {
    F_NO_FLAG,          //无FLAG标签的字段
    F_FILTER_BIT,       //FLAG标签为F_FILTER_BIT
    F_PROVCITY,         //此字段是行政区划字段
    F_REALPOSTFEE,      //此字段是精确邮费编码字段
    F_PROMOTION,        //折扣价格展示使用字段
    F_TIME,             //时间字段, 用于ends字段的排序与展示
    F_LATLNG_DIST,       //经纬度字段，用于进行经纬度距离计算
    F_ZK_GROUP       //折扣字段，根据时间对zk_group进行过滤
};

// 语法树子节点之间的逻辑关系
typedef enum _NodeLogicType
{
    LT_NONE,                 /* 无关系 */
    LT_AND,                  /* 与 */
    LT_OR,                   /* 或 */
    LT_NOT                   /* 非 */
}NodeLogicType;

// 根据用户在线状态进行宝贝过滤选项
enum OluStatType
{
    OLU_ALL,                 /* 不管卖家是否在线    把全部宝贝都取出来   */
    OLU_YES,                 /* 只取出卖家在线的宝贝   */
    OLU_NO                   /* 只取出卖家不在线的宝贝   */
};

//输出格式选项
enum OutputFormatType
{
    OUTPUTFORMAT_DEFAULT,
    OUTPUTFORMAT_VERSION_3_0,
    OUTPUTFORMAT_EN
};

#endif

