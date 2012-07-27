/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2119 $
 *
 * $LastChangedDate: 2011-05-26 09:50:55 +0800 (Thu, 26 May 2011) $

 * $Id: StatisticDef.h 2119 2011-05-26 09:50:55Z yanhui.tk $
 *
 * $Brief: 统计模块使用的配置信息，统计参数类型以及比较函数的定义 $
 ********************************************************************/

#ifndef _STATISTIC_DEFINE_H_
#define _STATISTIC_DEFINE_H_


#include "statistic/StatisticResult.h"
#include "util/crc.h"
#include "util/common.h"
#include "util/StringUtility.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

namespace statistic {

    #define MAX_CUSVALUE_LENGTH 1024    // Custom类型统计指定值的最大长度
    #define MAX_MULTIVALUE_LENGTH 512   // 多维统计指定限定字段值的最大长度
    #define MAX_FILEPATH_LENGTH 1024    // 配置文件中文件路径的最大长度

    /* 统计配置信息 */
    struct StatisticConfig
    {
        int32_t nStatisticNum;		    // 进行统计的doc条数，0代表全部精确统计
        int32_t nSampleStatisticNum;	// 进行抽样统计的最少doc条数, 小于或者为0代表不抽样，全部精确统计
        int32_t nSampleStatisticRate;	// 进行抽样统计的比例，比如配置成10，就表示抽样1/10.
        int32_t nSampleStatisticMaxCount;   // 进行抽样的最大次数; 这与m_nSampleStatisticsRate不能同时配置
    };

    /* 统计类型 */
    enum StatisticType
    {
        ECountStatType,       // 计数统计(一维)
        ESumStatType,         // 求和统计(unuse)
        ERangeStatType,       // 范围统计(unuse)
        ECustomStatType,      // 指定类目统计
        EMultiCountStatType,  // 多维计数统计
    };

    /* query中解析出的统计参数 */
    struct StatisticParameter
    {
        /**
         * @brief 构造函数
         */
        StatisticParameter()
        {
            szFieldName = NULL;
            nCount = 0;
            szSumFieldName = NULL;
            nParentCatid = 1;
            nPercent = 0;
            nExact = 0;
            szCusValue = NULL;
            szFirstFieldName = NULL;
            szSecondFieldName = NULL;
            szFirstValue = NULL;
            szSecondValue = NULL;
            eStatType = ECountStatType;
        }

        char* szFieldName;          // 统计字段名
        int32_t nCount;             // 统计时字段取值数量(TopN)
        char* szSumFieldName;       // Sum统计类型指定的求和字段序号
        int32_t nParentCatid;       // 类目统计时，用户指定的父类Id
        int32_t nPercent;           // 指定某类目占多少以上展开,百分比
        int32_t nExact;             // 是否进行精确统计，0表示根据配置进行抽样或精确统计; 1表示强制用精确统计
        char* szCusValue;           // custom类型统计指定类目集合
        char* szFirstFieldName;     // 多维统计中指定的第一个限定字段,该字段设定取值,类似custom
        char* szSecondFieldName;    // 多维统计中指定的第二个限定字段,该字段设定取值,类似custom
        char* szFirstValue;         // 多维统计中指定的第一个限定字段的值,类似cusvalue
        char* szSecondValue;        // 多维统计中指定的第二个限定字段的值,类似cusvalue
        StatisticType eStatType;    // 统计类型
    };

    /* StatisticKey到uint64_t的哈希映射函数 */
    struct StatHashFunc
    {
        uint64_t operator() (const StatisticKey &key) const
        {
            if(key.eFieldType == DT_INT8 || key.eFieldType == DT_INT16
               || key.eFieldType == DT_INT32 || key.eFieldType == DT_INT64
               || key.eFieldType == DT_KV32)
            {
                return (uint64_t)key.uFieldValue.lvalue;
            }
            if(key.eFieldType == DT_UINT8 || key.eFieldType == DT_UINT16
               || key.eFieldType == DT_UINT32 || key.eFieldType == DT_UINT64)
            {
                return key.uFieldValue.ulvalue;
            }
            if(key.eFieldType == DT_FLOAT)
            {
                return *(uint32_t*)(void *)(&key.uFieldValue.fvalue);
            }
            if(key.eFieldType == DT_DOUBLE)
            {
                return *(uint64_t*)(void *)(&key.uFieldValue.dvalue);
            }
            if(key.eFieldType == DT_STRING)
            {
                return get_crc64(key.uFieldValue.svalue, strlen(key.uFieldValue.svalue));
            }
            
            return 0;
        }
    };

    /* StatisticKey的比较函数(判相等) */
    struct StatEqualFunc
    {
        bool operator() (const StatisticKey &key1, const StatisticKey &key2) const
        {
            if(unlikely(key1.eFieldType != key2.eFieldType))
            {
                return false;
            }

            if(key1.eFieldType == DT_INT8 || key1.eFieldType == DT_INT16
               || key1.eFieldType == DT_INT32 || key1.eFieldType == DT_INT64
               || key1.eFieldType == DT_KV32)
            {
                return key1.uFieldValue.lvalue == key2.uFieldValue.lvalue;
            }
            if(key1.eFieldType == DT_UINT8 || key1.eFieldType == DT_UINT16
               || key1.eFieldType == DT_UINT32 || key1.eFieldType == DT_UINT64)
            {
                return key1.uFieldValue.ulvalue == key2.uFieldValue.ulvalue;
            }
            if(key1.eFieldType == DT_FLOAT)
            {
                return key1.uFieldValue.fvalue == key2.uFieldValue.fvalue;
            }
            if(key1.eFieldType == DT_DOUBLE)
            {
                return key1.uFieldValue.dvalue == key2.uFieldValue.dvalue;
            }
            if(key1.eFieldType == DT_STRING)
            {
                return strcmp(key1.uFieldValue.svalue, key2.uFieldValue.svalue) == 0;
            }
            
            return false;
        }
    };


    /* 筛选统计结果使用比较条件 */
    struct ItemLess 
    {
        bool operator() (StatisticItem *p1, StatisticItem *p2)
        {
            if(p1->nCount != p2->nCount)
            {
                return p1->nCount > p2->nCount;
            }
            
            StatisticKey key1 = p1->statKey;
            StatisticKey key2 = p2->statKey;

            if(unlikely(key1.eFieldType != key2.eFieldType))
            {
                return false;
            }
            switch(key1.eFieldType)
            {
                case DT_INT8:
                case DT_INT16:
                case DT_INT32:
                case DT_INT64:
                case DT_KV32:
                    return key1.uFieldValue.lvalue > key2.uFieldValue.lvalue;
                    break;
                case DT_UINT8:
                case DT_UINT16:
                case DT_UINT32:
                case DT_UINT64:
                    return key1.uFieldValue.ulvalue > key2.uFieldValue.ulvalue;
                    break;
                case DT_FLOAT:
                    return key1.uFieldValue.fvalue > key2.uFieldValue.fvalue;
                    break;
                case DT_DOUBLE:
                    return key1.uFieldValue.dvalue > key2.uFieldValue.dvalue;
                    break;
                case DT_STRING:
                    return strcmp(key1.uFieldValue.svalue, key2.uFieldValue.svalue) > 0;
                    break;
                default:
                    break;
            }
            
            return false;
        } 
    };

}

#endif //_STATISTIC_DEFINE_H_
