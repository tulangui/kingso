/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2199 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 $
 *
 * $Id: StatisticInterface.h 2199 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: 统计业务接口类 $
 ********************************************************************/

#ifndef _STATISTIC_INTERFACE_H_
#define _STATISTIC_INTERFACE_H_


#include "statistic/StatisticResult.h"
#include "commdef/SearchResult.h"


namespace statistic {

    class StatisticInterface
    {
        public:
            /**
             * @brief 构造函数
             */
            StatisticInterface() { }

            /**
             * @brief 析构函数
             */
            virtual ~StatisticInterface() { }

            /**
             * @brief 具体统计方法，各统计子类需要重载这个接口，实现特定的统计业务
             * @param pSearchResult 检索结果结合
             * @param pStatResItem 作为输出的统计子句结果，需调用端分配内存
             * @return 0 成功; 非0 错误码
             */
            virtual int32_t doStatistic(SearchResult *pSearchResult,
                                        StatisticResultItem *pStatResItem) = 0;
            /**
             * @brief 预留接口，用于将来实现渐进式统计，将统计模块作为检索的插件
             * @param nDocId 检索到的docid
             * @param ...
             * @return 0 成功; 非0 错误码 
             */
            virtual int32_t process(uint32_t nDocId,
                                    StatisticResultItem *pStatResItem) = 0;

    };
}

#endif // _STATISTIC_INTERFACE_H_
