/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2199 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 $
 *
 * $Id: CustomStatistic.h 2199 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: Custom类型统计子类，指定统计字段值的统计 $
 ********************************************************************/

#ifndef _CUSTOM_STATISTIC_H_
#define _CUSTOM_STATISTIC_H_

#include "CountStatistic.h"

namespace statistic {

    class CustomStatistic : public CountStatistic
    { 
        public:

            /**
             * @brief 构造函数
             * @param pContext 框架提供的环境信息
             * @param pProfileAccessor index_lib提供的profile读取接口
             * @param pStatConf 统计配置信息
             * @param pStatParm 统计参数子句信息
             */
            CustomStatistic(const FRAMEWORK::Context *pContext,
                            index_lib::ProfileDocAccessor *pProfileAccessor,
                            StatisticConfig *pStatConf, 
                            StatisticParameter *pStatParam);
            
            /**
             * @brief 析构函数
             */
            virtual ~CustomStatistic();

            
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


        protected:

            /**
             * @brief 对指定值字符串进行处理，切分成单个值
             * @param szCusValue 指定字段值串, 形如: cusvalue=50038469|50044400|50042824
             * @return 切分后的指定值的个数
             */
            int32_t processCusValue(char *szCusValue);

            /**
             * @brief 将指定值插入到统计用到的HashMap中，初始化HashMap
             * @param szCusItem 单个指定字段值
             * @param nLen szCusItem长度
             * @return 0 成功; 非0 错误码 
             */
            int32_t insertValue(char *szCusItem, PF_DATA_TYPE eType);
 
            /**
             * @brief 向统计过程所利用的HashMap中插入元素
             * @param statKey 待插入元素
             * @return return 0 成功; 非0 错误码
             */
            virtual int32_t insertStatMap(StatisticKey &statKey);
    
    };

}

#endif
