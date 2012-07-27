/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-11-27 09:50:55 +0800 $
 *
 * $Id: QueryRewriterResult.h 2577 2011-11-27 09:50:55 yanhui.tk $
 *
 * $Brief: queryparser在blender上进行query改写后的结果 $
 ********************************************************************/

#ifndef QUERYPARSER_QUERYREWRITERRESULT_H
#define QUERYPARSER_QUERYREWRITERRESULT_H

#include <stdint.h>

namespace queryparser {

    class QueryRewriterResult
    { 
        public:
            QueryRewriterResult();
            
            ~QueryRewriterResult();
            
            /**
             * @brief     获取改写后的query
             * @return    改写后的query
             */
            const char *getRewriteQuery();
            
            /**
             * @brief     获取改写后的query的长度
             * @return    改写后的query的长度
             */
            int32_t getRewriteQueryLen();

            /**
             * @brief     设置改写后的query和长度
             * @param[in] rew_query 改写后的query
             * @param[in] len       改写后的query的长度
             * @return    无
             */
            void setRewriteQuery(char *rew_query, int32_t len = 0);
            
            /**
             * @brief     设置改写后的query的长度
             * @param[in] len       改写后的query的长度
             * @return    无
             */
            void setRewriteQueryLen(int32_t len);

        private:
            char   *_rew_query;        // 改写后的query
            int32_t _rew_query_len;    // 改写后的query长度
    };

}

#endif
