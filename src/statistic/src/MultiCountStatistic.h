/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2442 $
 *
 * $LastChangedDate: 2011-06-08 09:50:55 $
 *
 * $Id: MultiCountStatistic.h 2442 2011-06-08 09:50:55Z yanhui.tk $
 *
 * $Brief: 多维计数统计子类,指定1到2个限定字段,对第三个字段进行统计 $
 ********************************************************************/

#ifndef _MULTI_COUNT_STATISTIC_H_
#define _MULTI_COUNT_STATISTIC_H_

#include "CountStatistic.h"

namespace statistic {

    /* 多维统计限定字段信息 */
    struct RestrictFieldInfo
    {
        char*   szFieldName;            // 限制字段名
        char*   szRestrictValue;        // 限定字段值
        PF_DATA_TYPE eType;             // 限定字段类型
        StatMap* pRestrictMap;          // 限制字段用HashMap存储,用于查找
        int32_t nMultiValNum;           // 标识限定字段单值多值类型    
        const index_lib::ProfileField *pProfileField;   // profile字段类型
    };

    /* 多维统计限定字段值结构,包含一个或多个StatKey */
    struct RestrictValue
    {
        StatisticKey *pStatKeys;    // 单个限定字段值数组
        int32_t     nCount;         // 限定字段值个数
    };


    class MultiCountStatistic : public CountStatistic
    { 
        public:

            /**
             * @brief 构造函数
             * @param pContext 框架提供的环境信息
             * @param pProfileAccessor index_lib提供的profile读取接口
             * @param pStatConf 统计配置信息
             * @param pStatParm 统计参数子句信息
             */
            MultiCountStatistic(const FRAMEWORK::Context *pContext,
                                index_lib::ProfileDocAccessor *pProfileAccessor,
                                StatisticConfig *pStatConf, 
                                StatisticParameter *pStatParam);

            /**
             * @brief 析构函数
             */
            virtual ~MultiCountStatistic();


            /**
             * @brief 实现指定值类型统计主方法
             * @param pSearchResult 检索结果结合
             * @param pStatResItem 作为输出的统计子句结果，需调用端分配内存
             * @return 0 成功; 非0 错误码 
             */
            virtual int32_t doStatistic(SearchResult *pSearchResult,
                                        StatisticResultItem *pStatResItem);


            /**
             * @brief 预留接口，用于将来实现渐进式统计，将统计模块作为检索的插件
             * @param nDocId 检索到的docid
             * @param ...
             * @return 0 成功; 非0 错误码 
             */
            virtual int32_t process(uint32_t nDocId,
                                    StatisticResultItem *pStatResItem);

            /**
             * @brief 读取profile并进行统计接口
             * @param pSearchResult 检索结果集合
             * @return 进行统计的doc数量(抽样即返回抽样个数)
             */
            virtual int32_t readAttributeInfo(SearchResult *pSearchResult);


        protected:

            /**
             * @brief 根据限定字段的指定值，读取该字段的Profile等信息
             * @param szFieldName 限定字段名
             * @param szFieldValue 限定字段指定值,形如:field2value=50038469|50044400|50042824
             * @return 0 成功; 非0 错误码 
             */
            int32_t prepareRestrictInfo(char* szFieldName,
                                        char *szFieldValue,
                                        RestrictFieldInfo &restrictInfo);

            /**
             * @brief 对限定字段值字符串进行处理, 切分成单个值, 并插入到pRestrictMap中
             * @param szValue 指定字段值串,形如:field2value=50038469|50044400|50042824
             * @param eType 限定字段类型
             * @param pRestrictMap 作为输出的HashMap
             * @return 切分后的指定值的个数
             */
            int32_t processMultiValue(char *szValue,
                                      PF_DATA_TYPE eType,
                                      StatMap *pRestrictMap);

            /**
             * @brief 读取指定docid的profile值, 判断是否包含限定值
             * @param pRestrictFieldInfo 限定字段信息结构
             * @param pRestrictValue 作为输出的字段值集合
             * @return true 包含; false 不包含
             */
            bool readRestrictAttrInfo(uint32_t nDocId,
                                      RestrictFieldInfo *pRestrictFieldInfo,
                                      RestrictValue *pRestrictValue);

            /**
             * @brief 将统计字段Key和限定字段进行拼串，并插入到HashMap中
             * @param statKey 统计字段Key
             * @param ppRestrictValue 限定字段值信息
             * @param nCount 限定字段个数, 取值为1或2
             * @return 0 成功; 非0 错误码 
             */
            int32_t mergeStatKey(StatisticKey &statKey,
                                 RestrictValue **ppRestrictValue,
                                 int32_t nCount);

            /**
             * @brief 将切分出的单个限定字段值插入到HashMap中 
             * @param szItem 切分出的单个限定字段值
             * @param eType 限定字段类型
             * @return 0 成功; 非0 错误码 
             */
            int32_t insertValue(char * szItem,
                                PF_DATA_TYPE eType,
                                StatMap *pRestrictMap);
            
            /**
             * @brief 将非String类型的字段值转成String类型来存储，便于拼接和展示
             * @param statKey 统计或限定字段值Key, 同时是输入和输出
             * @return 无
             */
            void transStatKey(StatisticKey &statKey);

        private:
            UTIL::Vector<RestrictFieldInfo*> _restrictInfos;    // 参数中限定字段信息集合

    };

}

#endif
