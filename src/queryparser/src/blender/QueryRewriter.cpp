#include "queryparser/QueryRewriter.h"
#include "StopwordDict.h"
#include "TokenizerPool.h"
#include "QueryRewriterDef.h"
#include "QueryParameterClassifier.h"
#include "KeywordRewriter.h"
#include "util/strfunc.h"
#include "util/StringUtility.h"


namespace queryparser{

    QueryRewriter::QueryRewriter()
        : _p_tokenizer_pool(NULL),
        _p_stopword_dict(NULL),
        _param_role_map(MAX_PARAMETER_BUCKET_NUM),
        _package_conf_map(MAX_PACKAGE_BUCKET_NUM)
    {
    
    }

    QueryRewriter::~QueryRewriter()
    {
        const char* key = NULL;
        PackageConfigInfo* packageValue = NULL;
        ParameterConfig* parameterValue = NULL;
        
        if(_p_tokenizer_pool)
        {
            delete _p_tokenizer_pool;
            _p_tokenizer_pool = NULL;
        }
        if(_p_stopword_dict)
        {
            delete _p_stopword_dict;
            _p_stopword_dict = NULL;
        }
        _package_conf_map.itReset();
        while(_package_conf_map.itNext(key, packageValue)){
            if(packageValue){
                delete packageValue;
                packageValue = NULL;
                key = NULL;   
            }         
        }
        _package_conf_map.clear();  
        key = NULL;
        _param_role_map.itReset();
        while(_param_role_map.itNext(key, parameterValue)){
            if(parameterValue){
                delete parameterValue;
                parameterValue = NULL;
                key = NULL;   
            }         
        }
        _param_role_map.clear();
    }

    int32_t QueryRewriter::init(mxml_node_t *p_root)
    {
        if(unlikely(p_root == NULL))
        {
            TERR("QUERYPARSER: read QueryParser config file on blender failed.");
            return KS_EFAILED;
        }

        const char* attr = NULL;    // xml配置文件的属性节点
        mxml_node_t *p_modules_node = mxmlFindElement(p_root, p_root,
                                        KS_XML_ROOT_NODE_NAME, NULL, NULL, MXML_DESCEND);
        mxml_node_t *p_qrw_node = mxmlFindElement(p_modules_node, p_root,
                KS_MODULE_NAME_QUERY_PARSER, NULL, NULL, MXML_DESCEND);
        if(unlikely(p_qrw_node == NULL || p_qrw_node->parent != p_modules_node))
        {   
            TERR("QUERYPARSER: can't find QueryRewriter configure.");
            return KS_EFAILED;
        }
        
        mxml_node_t *p_tokenizer_node = mxmlFindElement(p_qrw_node, p_modules_node,
                "qp_tokenizer_conf", NULL, NULL, MXML_DESCEND);
        if(unlikely(p_tokenizer_node == NULL || p_tokenizer_node->parent != p_qrw_node))
        {   
            TERR("QUERYPARSER: can't find tokenizer configure.");
            return KS_EFAILED;
        }

        attr = mxmlElementGetAttr(p_tokenizer_node, "path");
        const char * ws_type = mxmlElementGetAttr(p_tokenizer_node, "ws_type");
        if(ws_type == NULL || ws_type[0] == '\0' || initTokenizerPool(attr, ws_type) != 0)
        {
            TERR("QUERYPARSER: can't find ws configure or init tokenizer failed.");
            return KS_EFAILED;
        }
        
        mxml_node_t *p_stopword_node = mxmlFindElement(p_qrw_node, p_modules_node,
                "qp_stopword_path", NULL, NULL, MXML_DESCEND);
        if(unlikely(p_stopword_node == NULL || p_stopword_node->parent != p_qrw_node))
        {   
            TERR("QUERYPARSER: can't find stopword list configure.");
            return KS_EFAILED;
        }
        attr = mxmlElementGetAttr(p_stopword_node, "path");
        if(attr == NULL || attr[0] == '\0' || initStopwordDict(attr) != 0)
        {
            TERR("QUERYPARSER: can't find stopword list configure or init stopword failed.");
            return KS_EFAILED;
        }

        mxml_node_t *p_qrw_blender_node = mxmlFindElement(p_qrw_node, p_modules_node,
                "qp_blender", NULL, NULL, MXML_DESCEND);
        if(p_qrw_blender_node == NULL || p_qrw_blender_node->parent != p_qrw_node
                || initParameterRoleMap(p_qrw_blender_node, p_qrw_node) != 0)
        {
            TERR("QUERYPARSER: can't find qp_argument configure or init failed.");
            return KS_EFAILED;
        }
  
        mxml_node_t *p_packages_node = mxmlFindElement(p_qrw_node, p_modules_node,
                "packages", NULL, NULL, MXML_DESCEND);
        if(p_packages_node == NULL || p_packages_node->parent != p_qrw_node
                || initPackageConfigMap(p_packages_node, p_qrw_node) != 0)
        {
            TERR("QUERYPARSER: can't find package configure or init failed.");
            return KS_EFAILED;
        }
        
        return KS_SUCCESS;
    }

    int32_t QueryRewriter::initTokenizerPool(const char *filename, const char* ws_type)
    {
        _p_tokenizer_pool = new TokenizerPool();
        if(unlikely(_p_tokenizer_pool == NULL))
        {
            TERR("QUERYPARSER: alloc memory for tokenizer pool failed.");
            return KS_ENOMEM;
        }
        if(_p_tokenizer_pool->init(filename, ws_type) != 0)
        {
            TERR("QUERYPARSER: create tokenizer pool failed.");
            return KS_EFAILED;
        }
        return KS_SUCCESS;
    }
 
    int32_t QueryRewriter::initStopwordDict(const char *filename)
    {
        _p_stopword_dict = new StopwordDict();
        if(unlikely(_p_stopword_dict == NULL))
        {
            TERR("QUERYPARSER: create stopword dict instance failed.");
            return KS_ENOMEM;
        }

        return _p_stopword_dict->init(filename, MAX_STOPWORD_LEN);
    }
    
    int32_t QueryRewriter::initParameterRoleMap(mxml_node_t *p_node,
                                                mxml_node_t *p_parent_node)
    {
        assert(p_node && p_parent_node);

        const char *name = NULL;
        const char *dest = NULL;
        char *roles[MAX_PARAMETER_ROLE_NUM];
        mxml_node_t *p_qrw_args_node = mxmlFindElement(p_node, p_parent_node,
                                       "qp_argumet", NULL, NULL, MXML_DESCEND);

        while(p_qrw_args_node)
        {
            ParameterConfig *p_param_conf = new ParameterConfig();
            if(unlikely(p_param_conf == NULL))
            {
                TERR("QUERYPARSER: alloc memory for parameter config failed.");
                return KS_EFAILED;
            }
            // 解析查询参数名
            name = mxmlElementGetAttr(p_qrw_args_node, "name");
            if(unlikely(name == NULL || name[0] == '\0'))
            {
                TERR("QUERYPARSER: QueryParser argument config error.");
                if(p_param_conf)
                {
                    delete p_param_conf;
                    p_param_conf = NULL;
                }
                return KS_EFAILED;
            }
            p_param_conf->param_name = name;

            // 解析查询参数的作用
            dest = mxmlElementGetAttr(p_qrw_args_node, "dest");
            if(unlikely(dest == NULL || dest[0] == '\0'))
            {
                TERR("QUERYPARSER: QueryParser argument config error.");
                if(p_param_conf)
                {
                    delete p_param_conf;
                    p_param_conf = NULL;
                }
                return KS_EFAILED;
            }
            char buffer[MAX_PARAMETER_ROLE_LEN] = {0};
            memcpy(buffer, dest, strlen(dest));
            buffer[strlen(dest)] = '\0';
            p_param_conf->role_num = str_split_ex(buffer, ',', roles, MAX_PARAMETER_ROLE_NUM);
            for(int32_t i = 0; i < p_param_conf->role_num; ++i)
            {
                if(strcmp(roles[i], SEARCH_ROLE_NAME) == 0)
                {
                    p_param_conf->param_roles[i] = QP_SEARCH_ROLE;
                }
                else if(strcmp(roles[i], FILTER_ROLE_NAME) == 0)
                {
                    p_param_conf->param_roles[i] = QP_FILTER_ROLE;
                }
                else if(strcmp(roles[i], STATISTIC_ROLE_NAME) == 0)
                {
                    p_param_conf->param_roles[i] = QP_STATISTIC_ROLE;
                }
                else if(strcmp(roles[i], SORT_ROLE_NAME) == 0)
                {
                    p_param_conf->param_roles[i] = QP_SORT_ROLE;
                }
                else if(strcmp(roles[i], OTHER_ROLE_NAME) == 0)
                {
                    p_param_conf->param_roles[i] = QP_OTHER_ROLE;
                }
                else
                {
                    TERR("QUERYPARSER: invalid parameter role.");
                    if(p_param_conf)
                    {
                        delete p_param_conf;
                        p_param_conf = NULL;
                    }
                    return KS_EFAILED;
                }
            }
            if(_param_role_map.insert(name, p_param_conf)){
                p_param_conf = NULL;
            }
            else{
                delete p_param_conf;
                p_param_conf = NULL;
            }
            p_qrw_args_node = mxmlFindElement(p_qrw_args_node, p_node,
                                              "qp_argumet", NULL, NULL, MXML_DESCEND);
        }

        return KS_SUCCESS;
    }

    int32_t QueryRewriter::initPackageConfigMap(mxml_node_t *p_node,
                                                mxml_node_t *p_parent_node)
    {
        assert(p_node && p_parent_node);

        const char *name = NULL;
        const char *need_tokenize = NULL;
        mxml_node_t *p_package_node = mxmlFindElement(p_node, p_parent_node,
                                       "package", NULL, NULL, MXML_DESCEND);

        while(p_package_node)
        {
            PackageConfigInfo *p_package_conf = new PackageConfigInfo();
            if(unlikely(p_package_conf == NULL))
            {
                TERR("QUERYPARSER: alloc memory for package config failed.");
                return KS_EFAILED;
            }
            // 解析查询参数名
            name = mxmlElementGetAttr(p_package_node, "name");
            if(unlikely(name == NULL || name[0] == '\0'))
            {
                TERR("QUERYPARSER: QueryParser argument config error.");
                if(p_package_conf)
                {
                    delete p_package_conf;
                    p_package_conf = NULL;
                }
                return KS_EFAILED;
            }
            p_package_conf->name = name;

            // 解析查询参数的作用
            need_tokenize = mxmlElementGetAttr(p_package_node, "need_tokenize");
            if(unlikely(need_tokenize == NULL || need_tokenize[0] == '\0'))
            {
                TERR("QUERYPARSER: package config error.");
                if(p_package_conf)
                {
                    delete p_package_conf;
                    p_package_conf = NULL;
                }
                return KS_EFAILED;
            }
            if(strcasecmp(need_tokenize, "yes") == 0)
            {
                p_package_conf->need_tokenize = true;
            }
            else if(strcasecmp(need_tokenize, "no") == 0)
            {
                p_package_conf->need_tokenize = false;
            }
            else
            {
                TERR("QUERYPARSER: package config error.");
                if(p_package_conf)
                {
                    delete p_package_conf;
                    p_package_conf = NULL;
                }
                return KS_EFAILED;
            }

            if(_package_conf_map.insert(name, p_package_conf)){
                p_package_conf = NULL;
            }
            else{
                delete p_package_conf;
                p_package_conf = NULL;
            }
            p_package_node = mxmlFindElement(p_package_node, p_node,
                                              "package", NULL, NULL, MXML_DESCEND);
        }

        return KS_SUCCESS;
    }

    int32_t QueryRewriter::initConverter()
    {
        // 转码器(gbk=>utf8)
    //    _converter = code_conv_open(CC_UTF8, CC_GBK, 0);
        return KS_SUCCESS;
    }
 
    int32_t QueryRewriter::doRewrite(FRAMEWORK::Context* p_context,
                                     const char *query,
                                     int32_t query_len,
                                     QueryRewriterResult *p_query_rew_result) const 
    {
        int32_t ret = -1;
        MemPool *p_mem_pool = p_context->getMemPool();
        if(unlikely(query_len > MAX_QUERY_LEN))
        {
            TWARN("QUERYPARSER: query is too long, ignore it.");
            return KS_EFAILED;
        }
        char *query_copy = UTIL::replicate(query, p_mem_pool, query_len);    
        if(unlikely(query_copy == NULL))
        {
            TWARN("QUERYPARSER: alloc memory for query copy failed.");
            return KS_EFAILED;
        }
        QueryParameterClassifier *p_classifier = NEW(p_mem_pool, QueryParameterClassifier)
                                                    (p_mem_pool, _param_role_map);
        if(unlikely(p_classifier == NULL))
        {
            TWARN("QUERYPARSER: create QueryParameterClassifier instance failed.");
            return KS_EFAILED;
        }

        ret = p_classifier->doProcess(query_copy, query_len);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: do query classify failed.");
            return KS_EFAILED;
        }
        
        const char *keyword = p_classifier->getKeyword();
        if(keyword != NULL)
        {
            KeywordRewriter *p_keyword_rewriter = NEW(p_mem_pool, KeywordRewriter)
                                                    (p_mem_pool, _p_tokenizer_pool,
                                                     _p_stopword_dict, _package_conf_map);
            if(unlikely(p_keyword_rewriter == NULL))
            {
                TWARN("QUERYPARSER: create KeywordRewriter object failed.");
                return KS_EFAILED;
            }
            ret = p_keyword_rewriter->doRewrite(keyword, p_classifier->getKeywordLen());
            if(unlikely(ret != KS_SUCCESS))
            {
                TWARN("QUERYPARSER: do keyword parsing failed.");
                return KS_EFAILED;
            }
            p_classifier->setKeyword(p_keyword_rewriter->getRewriteKeyword(),
                                     p_keyword_rewriter->getRewriteKeywordLen());
        }
 
        ret = p_classifier->doParameterCombine();
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: do query parameter combining failed.");
            return KS_EFAILED;
        }
  
        p_query_rew_result->setRewriteQuery(p_classifier->getRewriteQuery(),
                                            p_classifier->getRewriteQueryLen());
     
        return KS_SUCCESS;
    }

}
