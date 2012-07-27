/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2119 $
 *
 * $LastChangedDate: 2011-05-26 09:50:55 +0800 (Thu, 26 May 2011) $
 *
 * $Id: StatisticResult.h 2119 2011-05-26 09:50:55Z yanhui.tk $
 *
 * $Brief: 统计结果结构以及相关类型定义 $
 ********************************************************************/

#ifndef _STATISTIC_RESULT_H_
#define _STATISTIC_RESULT_H_

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#include "util/MemPool.h"
#include "util/errcode.h"
#include "util/namespace.h"
#include "commdef/commdef.h"


namespace statistic {

    /* Profile字段值类型 */
    union ValueType
    {
        int64_t  lvalue;    // 含int8_t, int16_t, int32_t和int64_t
        uint64_t ulvalue;   // 含uint8_t, uint16_t, uint32_t和uint64_t
        float    fvalue;
        double   dvalue;
        char *   svalue;
    };

    /* 统计过程中使用到HashMap的Key结构 */
    struct StatisticKey
    {
        /**
         * @brief 构造函数
         */
        StatisticKey()
        {
            eFieldType = DT_INT64;
            memset(&uFieldValue, 0, sizeof(ValueType));
        }

        ValueType       uFieldValue;    //  Profile字段值  
        PF_DATA_TYPE    eFieldType;     //  Profile字段类型
    };

    /* 单个统计子句结果中统计项结构 */
    struct StatisticItem
    {
        StatisticKey statKey;   // 统计字段值
        int32_t nCount;	        // 相同统计字段值个数
        ValueType sumCount;     // Sum类型统计求和值(unuse)
    };
 
    /* 单个统计子句结果头部信息 */
    struct StatisticHeader
    {
        const char *szFieldName;                // 统计字段名
        int32_t nCount;                         // 统计子句要求返回的最多结果数
        const char *szRestrictFieldName;        // 限定字段名
        const char *szSumFieldName;             // Sum类型统计字段名
        PF_DATA_TYPE sumFieldType;              // Sum类型统计字段类型

        /**
         * @brief 构造函数
         */
        StatisticHeader()
            : szFieldName(NULL), nCount(0),
              szRestrictFieldName(NULL),
              szSumFieldName(NULL), sumFieldType(DT_INT64)
        {

        }

    };

    /* 单个统计子句结果信息 */
    struct StatisticResultItem
    {
        StatisticHeader *pStatHeader;   // 统计子句结果头部信息
        StatisticItem **ppStatItems;    // 统计子句结果中统计项
        int32_t nCount;                 // 统计子句结果中统计项个数


        /**
         * @brief 构造函数
         */
        StatisticResultItem()
            : pStatHeader(NULL), ppStatItems(NULL), nCount(0)
        {
            
        }

    };

    /* 统计参数全部结果集合 */
    struct StatisticResult
    {
        StatisticResultItem **ppStatResItems;   // 统计子句结果
        int32_t nCount;                         // 统计子句个数


        /**
         * @brief 构造函数
         */
        StatisticResult()
            : ppStatResItems(NULL), nCount(0)
        {
            
        }

        /**
         * @brief 计算统计结果ppStatResItems序列化大小
         * @return 统计结果序列化大小
         */
        int32_t getSerialSize();

        /**
         * @brief 计算统计子句结果序列化大小
         * @param pStatResItem 统计子句结果
         * @return 统计子句结果序列化大小
         */
        int32_t getSerialSize(StatisticResultItem *pStatResItem);

        /**
         * @brief 计算统计项序列化大小
         * @param pStatItem 统计项
         * @return 统计项序列化大小
         */
        int32_t getSerialSize(StatisticItem *pStatItem);

        /**
         * @brief 计算统计子句结果头部信息序列化大小
         * @param pStatHeader 统计子句结果头部信息
         * @return 统计子句结果头部信息序列化大小
         */
        int32_t getSerialSize(StatisticHeader *pStatHeader);

        /**
         * @brief 将统计结果ppStatResItems序列化成字符串buffer，
         *          需要调用端为指针分配空间
         * @param buffer 作为输出的序列化形成的字符串
         * @param size 序列化后大小
         * @return 序列化后大小
         */
        int32_t serialize(char *buffer, int32_t size);

        /**
         * @brief 将字符串buffer反序列化成统计结果ppStatResItems
         * @param buffer 作为输入的序列化字符串
         * @param size 序列化后大小
         * @param pMemPool 内存管理器
         * @return 0 成功; 非0 错误码
         */
        int32_t deserialize(char *buffer, int32_t size, MemPool *pMemPool);
 
        /**
         * @brief 将统计结果pStatisticResult追加到当前统计结果后面。注：只是简单数组追加，不去重
         * @param pMemPool 内存管理器
         * @param pStatisticResult 待追加的统计结果
         * @return 0 成功; 非0 错误码
         */
        int32_t append(MemPool *pMemPool, const StatisticResult *pStatisticResult);

        /**
         * @brief 将统计结果以xml格式写到buffer中
         * @param buffer 统计结果写入对象
         * @param size   buffer空间大小
         * @return 写入统计结果大小，如果超过size则返回size+1
         */
        int32_t outputXMLFormat(char *buffer, int32_t size);
     
        /**
         * @brief 将统计结果以v3格式写到buffer中
         * @param buffer 统计结果写入对象
         * @param size   buffer空间大小
         * @return 写入统计结果大小，如果超过size则返回size+1
         */
        int32_t outputV3Format(char *buffer, int32_t size);
    };
}

#endif // _STATISTIC_RESULT_H_
