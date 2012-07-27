/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: QueryRewriter.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: QueryParser在Blender上的改写处理调用接口 $
 ********************************************************************/

#ifndef QUERYPARSER_QUERYREWRITER_H
#define QUERYPARSER_QUERYREWRITER_H

#include "mxml.h"
#include "framework/Context.h"
#include "util/MemPool.h"
#include "util/HashMap.hpp"
#include "queryparser/QueryRewriterResult.h"
#include "commdef/TokenizerFactory.h"


namespace queryparser{
    class StopwordDict;
	class TokenizerPool;
    struct ParameterConfig;
    struct PackageConfigInfo;
}

namespace queryparser{

    typedef UTIL::HashMap<const char*, ParameterConfig*>  ParameterRoleMap;
    typedef UTIL::HashMap<const char*, ParameterConfig*>::iterator ParameterRoleItor;
    typedef UTIL::HashMap<const char*, ParameterConfig*>::const_iterator ParameterRoleConstItor;

    typedef UTIL::HashMap<const char*, PackageConfigInfo*>  PackageConfigMap;
    typedef UTIL::HashMap<const char*, PackageConfigInfo*>::iterator PackageConfigItor;
    typedef UTIL::HashMap<const char*, PackageConfigInfo*>::const_iterator PackageConfigConstItor;

    class QueryRewriter
    { 
        public:
            QueryRewriter();

            ~QueryRewriter();

            /** 
             * @brief     根据配置信息初始化
             *
             * @param[in] p_root xml配置文件root节点
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t init(mxml_node_t *p_root);
            
            /** 
             * @brief      query改写主流程，供外围框架调用，可重入函数
             *
             * @param[in]  p_context            框架提供的资源
             * @param[in]  query                原始query
             * @param[in]  query_len            原始query长度
             * @param[out] p_query_rew_result   query改写后的结果
             *
             * @return    0 成功; 非0 错误码 
             */
            int32_t doRewrite(FRAMEWORK::Context* p_context,
                              const char *query,
                              int32_t query_len,
                              QueryRewriterResult *p_query_rew_result) const;

        private:
            /** 
             * @brief     初始化分词器管理器
             *
             * @param[in] filename 分词器配置文件
			 * @param[in] ws_type  分词器类型
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t initTokenizerPool(const char *filename, const char* ws_type);
            
            /** 
             * @brief     初始化停用词词表
             *
             * @param[in] filename 分词器配置文件
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t initStopwordDict(const char *filename);
            
            /** 
             * @brief     初始化query参数及其作用配置信息
             *
             * @param[in] p_node        xml配置文件query参数节点
             * @param[in] p_parent_node xml配置文件query参数的父节点
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t initParameterRoleMap(mxml_node_t *p_node,
                                         mxml_node_t *p_parent_node);
            
            /** 
             * @brief     初始化pacakge配置信息
             *
             * @param[in] p_node        xml配置文件package节点
             * @param[in] p_parent_node xml配置文件package节点的父节点
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t initPackageConfigMap(mxml_node_t *p_node,
                                         mxml_node_t *p_parent_node);
            
            /** 
             * @brief     初始化字符编码转换器句柄池(暂未使用)
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t initConverter();

        private:
      //    code_conv_t         _converter;             // 字符编码转换器句柄
            TokenizerPool     *_p_tokenizer_pool;    	// 分词器管理器
            StopwordDict                *_p_stopword_dict;      // 停用词表 
            PackageConfigMap            _package_conf_map;      // package配置信息映射表            
            ParameterRoleMap            _param_role_map;        // query中每个参数的作用映射表
    };

}

#endif
