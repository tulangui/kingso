/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2119 $
 *
 * $LastChangedDate: 2011-05-24 09:50:55 +0800 (Tue, 26 Apr 2011) $
 *
 * $Id: StatisticParser.h 2119 2011-05-24 09:50:55Z yanhui.tk $
 *
 * $Brief: 查询query中统计参数解析器 $
 ********************************************************************/

#ifndef _STATISTIC_PARSER_H_
#define _STATISTIC_PARSER_H_


#include "framework/Context.h"
#include "util/Vector.h"

#include "StatisticDef.h"
#include "index_lib/ProfileDocAccessor.h"


namespace statistic {

    class StatisticParser
    { 
        public:
            /**
             * @brief 构造函数
             * @param pProfileAccessor Profile读取接口,用于字段有效性检查
             */
            StatisticParser(index_lib::ProfileDocAccessor *pProfileAccessor);
            
            /**
             * @brief 析构函数
             */
            virtual ~StatisticParser();
            
            /**
             * @brief 将统计参数解析成内部表示形式
             * @param pMemPool 内存分配管理器
             * @param szStatClause 统计参数，形如：field=prop_vid,cattype=count,count=500
             * @return 0 成功 非0 错误码
             */
            virtual int32_t doParse(MemPool *pMemPool, char *szStatClause);

            /**
             * @brief 获取统计参数子句
             * @param idx 统计参数子句序号 从0开始
             * @return 获取指定序号的统计参数子句 或 NULL
             */
            virtual StatisticParameter* getStatParam(int32_t idx);

            /**
             * @brief 获取统计参数子句的个数
             * @return 返回统计参数子句的个数
             */
            virtual int32_t getStatNum();

        private:
            /**
             * @brief 统计字段有效性检查
             * @param szFieldName 字段名称
             * @return true 有效; false 无效
             */
            bool isValidField(char *szFieldName);

            index_lib::ProfileDocAccessor *_pProfileAccessor;   // Profile读取接口,这里用于字段有效性检查
            UTIL::Vector<StatisticParameter*> _statParams;      // 统计参数，包含一个或若干个统计子句
    };

}

#endif // _STATISTIC_PARSER_H_
