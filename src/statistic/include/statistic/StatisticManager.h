/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2119 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 $
 *
 * $Id: StatisticManager.h 2119 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: 统计管理类，提供统计与框架的接口, 负责根据配置调用工厂构造具体的统计子类 $
 ********************************************************************/

#ifndef _STATISTIC_MANAGER_H_
#define _STATISTIC_MANAGER_H_


#include "framework/Context.h"
#include "index_lib/ProfileDocAccessor.h"
#include "queryparser/Param.h"
#include "statistic/StatisticResult.h"
#include "commdef/SearchResult.h"
#include "commdef/ClusterResult.h"

struct StatisticConfig;
class StatisticParser;

namespace statistic {

    class StatisticManager
    { 
        public:
            /**
             * @brief 构造函数
             */
            StatisticManager();
            
            /**
             * @brief 析构函数
             */
            ~StatisticManager();

            /**
             * @brief 对searcher框架的接口，用于初始化统计配置信息
             * @param pConfig xml配置文件root节点
             * @return 0 成功; 非0 错误码
             */
            int32_t init(mxml_node_t *pConfig);
            
            /**
             * @brief searcher框架调用的process，统计的主流程
             * @param pContext 框架提供的资源
             * @param szStatisticClause queryparser解析出的统计参数, 统计模块自行解析
             * @param pSearchResult 检索过滤结果集合
             * @param pStatisticResult searcher上的统计结果，需要调用端为指针分配空间
             * @return 0 成功; 非0 错误码
             */
            int32_t doStatisticOnSearcher(const FRAMEWORK::Context *pContext,
                                          const queryparser::Param &param,
                                          SearchResult *pSearchResult,
                                          StatisticResult *pStatisticResult) const;

            /**
             * @brief merger框架调用的process, 用于统计结果合并
             * @param pContext 框架提供的资源
             * @param ppClusterResults searcher机器返回结果, 内含统计结果
             * @param nReqSize merger请求searcher的个数
             * @param pMergeStatcResult merger上的统计结果，需要调用端为指针分配空间
             * @return 0 成功; 非0 错误码
             */
            int32_t doStatisticOnMerger(const FRAMEWORK::Context *pContext,
                                        commdef::ClusterResult **ppClusterResults,
                                        int32_t nReqSize,
                                        StatisticResult *pMergeStatResult) const;

        private:
            StatisticConfig *_pStatConf;                        // 统计配置信息
            index_lib::ProfileDocAccessor *_pProfileAccessor;   // Profile读取接口
    };

}

#endif //_STATISTIC_MANAGER_H_
