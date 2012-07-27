/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: QueryParameterClassifier.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: query中参数的分类器, 根据配置文件，将query中的参数分为
 *         search, filter, statistic, sort和other五种类型的查询子句 $
 ********************************************************************/

#ifndef QUERYPARSER_QUERYPARAMETERCLASSIFIER_H
#define QUERYPARSER_QUERYPARAMETERCLASSIFIER_H

#include "mxml.h"
#include "util/MemPool.h"
#include "QueryRewriterDef.h"
#include "queryparser/QueryRewriter.h"

namespace queryparser{

    class QueryParameterClassifier
    { 
        public:
            
            /** 
             * @brief     构造函数
             *
             * @param[in] p_mem_pool 内存管理器
             * @param[in] param_role_map query参数作用表
             */
            QueryParameterClassifier(MemPool *p_mem_pool,
                                     const ParameterRoleMap &param_role_map);
            
            ~QueryParameterClassifier();

            /** 
             * @brief     query参数分类器的对外调用接口，主控流程
             *
             * @param[in] query 原始query副本
             * @param[in] len   原始query长度
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doProcess(char *query_copy, int32_t len);
            
            /** 
             * @brief     将分类好的query参数按照一定规则进行合并
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t doParameterCombine();

            /** 
             * @brief     获取query中的keyword，即q或rewq
             *
             * @return    query中的keyword，没有返回NULL 
             */
            char* getKeyword();
            
            /** 
             * @brief     获取query中的keyword的长度
             *
             * @return    query中的keyword的长度，没有返回0
             */
            int32_t getKeywordLen();
            
            /** 
             * @brief     将改写后的的keyword及其长度的写回
             *
             * @param[in] keyword  改写后的keyword
             * @param[in] len      改写后的keyword长度
             *
             * @return    无
             */
            void  setKeyword(char *keyword, int32_t len);
            
            /** 
             * @brief     获取改写后的query
             *
             * @return    改写后的query
             */
            char* getRewriteQuery();
            
            /** 
             * @brief     获取改写后的query的query长度
             *
             * @return    改写后的query长度
             */
            int32_t getRewriteQueryLen();
 

        private:
        
            /** 
             * @brief     对query进行切分，要求是原始query副本
             *
             * @param[in] query 原始query副本
             * @param[in] len   原始query长度
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doQuerySplit(char *query, int32_t len);
        
            /** 
             * @brief     对query参数值进行编码转换和归一化处理
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t doEncodingConvert();
        
            /** 
             * @brief     按照query参数作用表，对query参数进行分类
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t doParameterClassify();
    
            /** 
             * @brief     将分类为检索子句的参数追加写入到检索子句中
             *
             * @param[in|out]   sub_query     待写入的检索子句buffer
             * @param[in|out]   query_len     当前已写入检索子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2SearchQuery(char *sub_query, int32_t &query_len,
                                       char *param_key, int32_t key_len,
                                       char *param_value, int32_t value_len);

            /** 
             * @brief     将分类为过滤子句的参数追加写入到过滤子句中
             *
             * @param[in|out]   sub_query     待写入的过滤子句buffer
             * @param[in|out]   query_len     当前已写入过滤子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2FilterQuery(char *sub_query, int32_t &query_len,
                                       char *param_key, int32_t key_len,
                                       char *param_value, int32_t value_len);

            /** 
             * @brief     将分类为统计子句的参数追加写入到统计子句中
             *
             * @param[in|out]   sub_query     待写入的统计子句buffer
             * @param[in|out]   query_len     当前已写入统计子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2StatisticQuery(char *sub_query, int32_t &query_len,
                                       char *param_key, int32_t key_len,
                                       char *param_value, int32_t value_len);

            /** 
             * @brief     将分类为排序子句的参数追加写入到排序子句中
             *
             * @param[in|out]   sub_query     待写入的排序子句buffer
             * @param[in|out]   query_len     当前已写入排序子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2SortQuery(char *sub_query, int32_t &query_len,
                                       char *param_key, int32_t key_len,
                                       char *param_value, int32_t value_len);

            /** 
             * @brief     将分类为其它子句的参数追加写入到其它子句中
             *
             * @param[in|out]   sub_query     待写入的其它子句buffer
             * @param[in|out]   query_len     当前已写入其它子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2OtherQuery(char *sub_query, int32_t &query_len,
                                       char *param_key, int32_t key_len,
                                       char *param_value, int32_t value_len);

            /** 
             * @brief     将分类后的query参数追加写入对应的子句中
             *
             * @param[in]       sub_query     待写入的子句buffer
             * @param[in|out]   query_len     当前已写入子句的长度，即当前写入的起始位置
             * @param[in]       param_key     写入query参数名
             * @param[in]       param_len     写入query参数名的长度
             * @param[in]       param_value   写入query参数值
             * @param[in]       value_len     写入query参数值的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2SubQuery(char *sub_query, int32_t &query_len,
                                    char *param_key, int32_t key_len,
                                    char *param_value, int32_t value_len);
 
            /** 
             * @brief     将分类后的子句追加写入到改写后的query中
             *
             * @param[in]       rew_query       待写入的改写后query的buffer
             * @param[in|out]   rew_query_len   当前已写入query的长度，即当前写入的起始位置
             * @param[in]       sub_prefix      写入子句的名称标识
             * @param[in]       sub_prefix_len  写入子句的名称标识的长度
             * @param[in]       sub_query       写入子句的内容
             * @param[in]       sub_query_len   写入子句内容的长度
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t append2RewriteQuery(char *rew_query, int32_t &rew_query_len,
                                        char *sub_prefix, int32_t sub_prefix_len,
                                        char *sub_query, int32_t sub_query_len);

        private:
            MemPool *_p_mem_pool;                       // 内存管理器
            const ParameterRoleMap &_param_role_map;    // query中每个参数的作用映射表

            char *_prefix;                              // query前缀(包含areaname)
            int32_t _prefix_len;                        // query前缀的长度
            char *_keyword;                             // query中的关键词: q或rewq
            int32_t _keyword_len;                       // query中的关键词长度
            
            char *_rew_query;                           // 改写后的query
            int32_t _rew_query_len;                     // 改写后的query长度

            ParameterKeyValueItem *_query_params;       // query中切分出的有效参数(key-value对形式)
            int32_t _param_num;                         // query中的有效参数个数 
            char *_normal_query;                        // 归一化后的query参数值buffer
    };
}

#endif
