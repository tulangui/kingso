/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2199 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 $
 *
 * $Id: StatisticFactory.h 2199 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: 统计工厂类 $
 ********************************************************************/

#ifndef _STATISTIC_FACTORY_H_
#define _STATISTIC_FACTORY_H_

#include "framework/Context.h"
#include "index_lib/ProfileDocAccessor.h"

#include "StatisticDef.h"
#include "StatisticInterface.h"
#include "CountStatistic.h"
#include "CustomStatistic.h"
#include "MultiCountStatistic.h"


namespace statistic {

    class StatisticFactory
    {
        public:
            /**
             * 构造函数
             */
            StatisticFactory(void) { };

            /**
             * 析构函数
             */
            ~StatisticFactory(void) { };

            /**
             * 根据统计参数中的统计类型信息，利用Context中提供的MemPool接口，构造具体的统计对象
             * @param pContext 框架提供的环境信息
             * @param pProfileAccessor index_lib提供的profile读取接口
             * @param pStatConf 统计配置信息
             * @param pStatParm 统计参数子句信息
             * @return StatisticInterface 统计子类对象
             */
            static StatisticInterface *create(const FRAMEWORK::Context *pContext,
                                              index_lib::ProfileDocAccessor *pProfileAccessor,
                                              StatisticConfig *pStatConf,
                                              StatisticParameter *pStatParam, const queryparser::Param &param);
    };


    StatisticInterface *StatisticFactory::create(const FRAMEWORK::Context *pContext,
                                                 index_lib::ProfileDocAccessor *pProfileAccessor,
                                                 StatisticConfig *pStatConf,
                                                 StatisticParameter *pStatParam, const queryparser::Param &param)
    {
        if(pContext == NULL || pProfileAccessor == NULL || pStatParam == NULL)
        {
            TWARN("信息不全，无法构建统计对象子类");
            return NULL;
        }

        MemPool *pMemPool = pContext->getMemPool();
        StatisticType eStatType = pStatParam->eStatType;
        StatisticInterface *pStatistic = NULL;

        switch(eStatType)
        {
            case ECountStatType:
                pStatistic = NEW(pMemPool, CountStatistic)
                                (pContext, pProfileAccessor, pStatConf, pStatParam, &param);
                break;
            case ECustomStatType:
                pStatistic = NEW(pMemPool, CustomStatistic)
                                (pContext, pProfileAccessor, pStatConf, pStatParam);
                break;
            case EMultiCountStatType:
                pStatistic = NEW(pMemPool, MultiCountStatistic)
                                (pContext, pProfileAccessor, pStatConf, pStatParam);
                break;

            default:
                break;
        }
        return pStatistic;
    }
}

#endif //_STATISTIC_FACTORY_H_
