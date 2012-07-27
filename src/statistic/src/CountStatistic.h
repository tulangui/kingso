/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2199 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 $
 *
 * $Id: CountStatistic.h 2199 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: Count类型统计子类，最基本的统计类型 $
 ********************************************************************/

#ifndef _COUNT_STATISTIC_H_
#define _COUNT_STATISTIC_H_

#include "StatisticInterface.h"
#include "StatisticDef.h"

#include "util/HashMap.hpp"
#include "commdef/commdef.h"
#include "framework/Context.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/ProfileMultiValueIterator.h"
#include "queryparser/Param.h"

namespace statistic {
    
    typedef UTIL::HashMap<StatisticKey, StatisticItem*, StatHashFunc, StatEqualFunc> StatMap;
    typedef UTIL::HashMap<StatisticKey, StatisticItem*, StatHashFunc, StatEqualFunc>::iterator StatIterator;
    
    class CountStatistic : public StatisticInterface
    {
        public:
            /**
             * @brief 构造函数
             * @param pContext 框架提供的环境信息
             * @param pProfileAccessor index_lib提供的profile读取接口
             * @param pStatConf 统计配置信息
             * @param pStatParm 统计参数子句信息
             */
            CountStatistic(const FRAMEWORK::Context *pContext,
                           index_lib::ProfileDocAccessor *pProfileAccessor,
                           StatisticConfig *pStatConfig,
                           StatisticParameter *pStatParam, const queryparser::Param *param);

            /**
             * @brief 析构函数
             */
            virtual ~CountStatistic();

            /**
             * @brief 实现简单计数类型统计主方法
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
            * @brief 获取抽样统计步长，即采样点间距
            * @param nDocs 检索结果集合doc个数
            * @return 抽样统计步长
            */
            virtual int32_t getSampleStep(uint32_t nDocs);
           
            /**
             * @brief 向统计过程所利用的HashMap中插入元素
             * @param szValue 待插入元素
             * @return return 0 成功; 非0 错误码
             */
            virtual int32_t insertStatMap(StatisticKey &statKey);
 
            /**
             * @brief 读取profile并进行统计接口
             * @param pSearchResult 检索结果集合
             * @return 进行统计的doc数量(抽样即返回抽样个数)
             */
            virtual int32_t readAttributeInfo(SearchResult *pSearchResult);
          
            /**
             * @brief 读取单值profile值
             * @param nDocId 指定的docid
             * @param pProfileField profile字段类型
             * @param eType profile字段值类型
             * @param statKey 作为输出的Profile字段信息,需要调用端分配
             * @return return 0 成功; 非0 错误码
             */
            virtual int32_t getSingleAttrValue(uint32_t nDocId,
                                               const index_lib::ProfileField *pProfileField,
                                               PF_DATA_TYPE eType,
                                               StatisticKey &statKey);

            
            /**
             * @brief 读取多值或变长profile值
             * @param profileItor 多值或变长profile读取迭代器
             * @param eType profile字段值类型
             * @param statKey 作为输出的Profile字段信息,需要调用端分配
             * @return return 0 成功; 非0 错误码
             */
            virtual int32_t getMultiAttrValue(index_lib::ProfileMultiValueIterator &profileItor,
                                              PF_DATA_TYPE eType,
                                              StatisticKey &statKey);

        
        protected:
            const FRAMEWORK::Context *_pContext;                // 里面包含MemPool, ErrorMessage
            index_lib::ProfileDocAccessor *_pProfileAccessor;   // Profile读取接口
            StatisticConfig *_pStatConf;                        // 统计配置信息
            StatisticParameter *_pStatParam;                    // query中的统计参数
            
            StatMap _statMap;                                   // 统计过程中使用的HashMap

            const queryparser::Param *_param;
    };

}

#endif //_COUNT_STATISTIC_H_
