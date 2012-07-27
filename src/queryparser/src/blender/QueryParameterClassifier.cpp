#include "QueryParameterClassifier.h"
#include "util/py_url.h"
#include "util/py_code.h"
#include "util/strfunc.h"
#include "util/charsetfunc.h"
#include "util/StringUtility.h"

namespace queryparser{
 
    QueryParameterClassifier::QueryParameterClassifier(MemPool *p_mem_pool,
                                                       const ParameterRoleMap &param_role_map)
        : _p_mem_pool(p_mem_pool),
          _param_role_map(param_role_map),
        _keyword(NULL), _keyword_len(0),
        _rew_query(NULL), _rew_query_len(0),
        _prefix(NULL), _prefix_len(0),
        _query_params(NULL), _param_num(0),
        _normal_query(NULL)
    {
    
    }

    QueryParameterClassifier::~QueryParameterClassifier()
    {
        _p_mem_pool = NULL;
        _keyword = NULL;
        _keyword_len = 0;
        _rew_query = NULL;
        _rew_query_len = 0;
        _query_params = NULL;
        _param_num = 0;
        _normal_query = NULL;
    }

    int32_t QueryParameterClassifier::doProcess(char *query_copy, int32_t len)
    {
       int32_t ret = -1; 

       ret = doQuerySplit(query_copy, len);
       if(unlikely(ret != 0))
       {
           TWARN("QUERYPARSER: do query spliting failed.");
           return KS_EFAILED;
       }
       
       ret = doEncodingConvert();
       if(unlikely(ret != 0))
       {
           TWARN("QUERYPARSER: do query encoding converting failed.");
           return KS_EFAILED;
       }

       ret = doParameterClassify();
       if(unlikely(ret != 0))
       {
           TWARN("QUERYPARSER: do query classify failed.");
           return KS_EFAILED;
       }

       return KS_SUCCESS;
    }

    int32_t QueryParameterClassifier::doQuerySplit(char *query, int32_t len)
    {
        int32_t ret = -1;
        
        // 获取query前缀，形如: /bin/search?auction?
        char *ptr = strrchr(query, QUERY_PREFIX_SEPARATOR);
        if(unlikely(ptr == NULL || (ptr+1) == NULL || *(ptr+1) == '\0'))
        {
            TWARN("QUERYPARSER: can't find query prefix, invalid query format.");
            return KS_EFAILED;
        }
        ptr++;
        if(ptr == NULL || *ptr == '\0')
        {
            TWARN("QUERYPARSER: can't find query content, invalid query format.");
            return KS_EFAILED;
        }
        _prefix = query;
        _prefix_len = ptr - query;
        len -= _prefix_len;

        char *query_params[MAX_KEYVALUE_NUM];
        // 按"&"分割成多个参数kv对
        int32_t param_num = str_split_ex(ptr, QUERY_PARAMETER_SEPARATOR,
                                         query_params, MAX_KEYVALUE_NUM);
        if(unlikely(param_num <= 0))
        {
            TWARN("QUERYPARSER: split query failed. invalid query format.");
            return KS_EFAILED;
        }

        _query_params = NEW_VEC(_p_mem_pool, ParameterKeyValueItem, param_num);
        if((unlikely(_query_params == NULL)))
        {
            TWARN("QUERYPARSER: alloc memory for parameter keyvalue failed.");
            return KS_EFAILED;
        }
        // 按"="将每个参数域的切分成key和value
        for(int32_t i = 0; i < param_num; ++i)
        {
            char *equal = strchr(query_params[i], PARAMETER_KEYVALUE_SEPARATOR);
            if(unlikely(equal == NULL))
            {
                continue;
            }
            *equal = '\0';
            // 只保存key和value都非空的参数
            if(query_params[i] != NULL && *query_params[i] != '\0'
                    && (equal+1) != NULL && *(equal+1) != '\0')
            {
                _query_params[_param_num].param_key = query_params[i];
                _query_params[_param_num].key_len = equal-query_params[i];
                _query_params[_param_num].param_value = equal + 1;
                _query_params[_param_num].value_len = strlen(equal+1);
                _param_num++;
            }
        }
        return KS_SUCCESS;
    }

    int32_t QueryParameterClassifier::doEncodingConvert()
    {
        int32_t len = -1;
        int32_t normal_len = 0;
        char buffer[MAX_VALUE_LEN];
        _normal_query = NEW_VEC(_p_mem_pool, char, MAX_QUERY_LEN);

        if(unlikely(_normal_query == NULL))
        {
            TWARN("QUERYPARSER: alloc memory for normalized query failed.");
            return -1;
        }

        code_conv_t conver = code_conv_open(CC_UTF8, CC_GBK, 0);

        for(int32_t i = 0; i < _param_num; ++i)
        {
            // url decode
            len = decode_url(_query_params[i].param_value, _query_params[i].value_len,
                                   buffer, MAX_VALUE_LEN-1);
            if(unlikely(len < 0))
            {
                TWARN("QUERYPARSER: url decode query failed.");
                code_conv_close(conver);
                return KS_EFAILED;
            }
            buffer[len] = '\0';

            // gbk-->utf8
            len = code_conv(conver, _normal_query+normal_len,
                                   MAX_QUERY_LEN-normal_len-1, buffer);
            // 转码失败绝大数情况为输入即为utf-8编码，因此将原串拷贝
            if(unlikely(len < 0))
            {
                TWARN("QUERYPARSER: convert encoding failed, input may be utf-8 encoding.");
                len = strlen(buffer);
                memcpy(_normal_query+normal_len, buffer, len);
            }
            
            // q和rewq参数分词后再进行规范化；qinfo参数不进行规范化
            if(strcmp(_query_params[i].param_key, QUERY_PARAMETER_Q) != 0
                    && strcmp(_query_params[i].param_key, QUERY_PARAMETER_REWQ) != 0 
                    && strcmp(_query_params[i].param_key, QUERY_PARAMETER_QINFO) != 0)
            {
                len = py_utf8_normalize(_normal_query+normal_len, len,
                                               _normal_query+normal_len, len+1);
                if(unlikely(len < 0))
                {
                    TWARN("QUERYPARSER: query normalize failed.");
                    continue;
                }
            }
            _query_params[i].param_value = _normal_query+normal_len;
            _query_params[i].value_len = len;
            *(_normal_query + normal_len + len) = '\0';
            normal_len += len+1;
        }
        code_conv_close(conver);

        return KS_SUCCESS;
    }

    int32_t QueryParameterClassifier::doParameterClassify()
    {
        int32_t ret = -1;
        char *param_q = NULL;
        int32_t param_q_len = 0;
        char *param_rewq = NULL;
        int32_t param_rewq_len = 0;
        char *param_advsort = NULL;
        ParameterRoleConstItor const_itor;

        for(int32_t i = 0; i < _param_num; ++i)
        {
            char *param_key = _query_params[i].param_key;
            char *param_value = _query_params[i].param_value;
            int32_t key_len = _query_params[i].key_len;
            int32_t value_len = _query_params[i].value_len;
            
            if(unlikely(key_len <=0 || param_key == NULL
                        || value_len <= 0 ||  param_value == NULL))
            {
                continue;
            }
            if((const_itor = _param_role_map.find(param_key)) != _param_role_map.end()
               || (_query_params[i].param_key[0] == SIGN_UNDERLINE
                  && (const_itor = _param_role_map.find(param_key+1)) != _param_role_map.end()))
            {
                _query_params[i].p_param_conf = const_itor->value;
            }
            else
            {
                _query_params[i].p_param_conf = NULL;
            }

            if(strcmp(param_key, QUERY_PARAMETER_Q) == 0)
            {
                param_q = param_value;
                param_q_len = value_len;
            }
            if(strcmp(param_key, QUERY_PARAMETER_REWQ) == 0)
            {
                param_rewq = param_value;
                param_rewq_len = value_len;
            }
            if(strcmp(param_key, QUERY_PARAMETER_ADVSORT) == 0)
            {
                param_advsort = param_value;
            }
        }

        if(param_rewq != NULL && param_advsort != NULL
                && strcmp(param_advsort, QUERY_PARAMETER_ADVSORT_VALUE) == 0)
        {
            _keyword = param_rewq;
            _keyword_len = param_rewq_len;
        }
        else
        {
            _keyword = param_q;
            _keyword_len = param_q_len;
        }

        return KS_SUCCESS;
    }

    int32_t QueryParameterClassifier::doParameterCombine()
    {   
        int32_t ret = -1;
        char search_query[MAX_QUERY_LEN];
        char filter_query[MAX_QUERY_LEN];
        char stat_query[MAX_QUERY_LEN];
        char sort_query[MAX_QUERY_LEN];
        char other_query[MAX_QUERY_LEN];

        int32_t search_query_len = 0;
        int32_t filter_query_len = 0;
        int32_t stat_query_len = 0;
        int32_t sort_query_len = 0;
        int32_t other_query_len = 0;

        if(_keyword_len > 0 && _keyword != NULL && _keyword[0] != '\0')
        {
            ret = append2SearchQuery(search_query, search_query_len,
                                    QUERY_PARAMETER_KSQ, QUERY_PARAMETER_KSQ_LEN,
                                    _keyword, _keyword_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append rewrite keyword to search query failed.");
                return KS_EFAILED;
            }
        }
        for(int32_t i = 0; i < _param_num; ++i)
        {
            if(unlikely(_query_params[i].key_len <= 0 || _query_params[i].param_key == NULL
                        || _query_params[i].value_len <= 0 || _query_params[i].param_value == NULL))
            {
                continue;
            }
            if(unlikely(_query_params[i].p_param_conf == NULL))
            {
                ret = append2SearchQuery(search_query, search_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                if(unlikely(ret != 0))
                {
                    TWARN("QUERYPARSER: append undefined parameter to search query failed.");
                    //return KS_EFAILED;
                }

                ret = append2FilterQuery(filter_query, filter_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                if(unlikely(ret != 0))
                {
                    TWARN("QUERYPARSER: append undefined parameter to filter query failed.");
                    //return KS_EFAILED;
                }

                ret = append2StatisticQuery(stat_query, stat_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                if(unlikely(ret != 0))
                {
                    TWARN("QUERYPARSER: append undefined parameter to statistic query failed.");
                    //return KS_EFAILED;
                }

                ret = append2SortQuery(sort_query, sort_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                if(unlikely(ret != 0))
                {
                    TWARN("QUERYPARSER: append undefined parameter to sort query failed.");
                    //return KS_EFAILED;
                }

                ret = append2OtherQuery(other_query, other_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                if(unlikely(ret != 0))
                {
                    TWARN("QUERYPARSER: append undefined parameter to other query failed.");
                    //return KS_EFAILED;
                }
                continue;
            }
            for(int32_t j = 0; j < _query_params[i].p_param_conf->role_num; ++j)
            {
                switch(_query_params[i].p_param_conf->param_roles[j])
                {
                    case QP_SEARCH_ROLE:
                        ret = append2SearchQuery(search_query, search_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                        if(unlikely(ret != 0))
                        {
                            TWARN("QUERYPARSER: append parameter to search query failed.");
                            //return KS_EFAILED;
                        }
                        break;
                    case QP_FILTER_ROLE:
                        ret = append2FilterQuery(filter_query, filter_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                        if(unlikely(ret != 0))
                        {
                            TWARN("QUERYPARSER: append parameter to filter query failed.");
                            //return KS_EFAILED;
                        }
                        break;
                    case QP_STATISTIC_ROLE:
                        ret = append2StatisticQuery(stat_query, stat_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                        if(unlikely(ret != 0))
                        {
                            TWARN("QUERYPARSER: append parameter to statistic query failed.");
                            //return KS_EFAILED;
                        }
                        break;
                    case QP_SORT_ROLE:
                        ret = append2SortQuery(sort_query, sort_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                        if(unlikely(ret != 0))
                        {
                            TWARN("QUERYPARSER: append parameter to sort query failed.");
                            //return KS_EFAILED;
                        }
                        break;
                    case QP_OTHER_ROLE:
                        ret = append2OtherQuery(other_query, other_query_len,
                                        _query_params[i].param_key, _query_params[i].key_len,
                                        _query_params[i].param_value, _query_params[i].value_len);
                        if(unlikely(ret != 0))
                        {
                            TWARN("QUERYPARSER: append parameter to other query failed.");
                            //return KS_EFAILED;
                        }
                        break;
                    default:
                        TWARN("QUERYPARSER: invalid query parameter role, ignor it.");
                        break;
                }
            }
        }
        
        // 合并入最终query中
        char *rew_query = NEW_VEC(_p_mem_pool, char, MAX_QUERY_LEN + 1);
        int32_t rew_query_len = 0;
        if(unlikely(rew_query == NULL))
        {
            TWARN("QUERYPARSER: alloc memory for rewrite query failed.");
            return KS_ENOMEM;
        }
 
        ret = append2RewriteQuery(rew_query, rew_query_len,
                                  _prefix, _prefix_len,
                                  NULL, 0);
        if(unlikely(ret != 0))
        {
            TWARN("QUERYPARSER: append query prefix to rewrite query failed.");
            return KS_EFAILED;
        }
        if(search_query_len > 0)
        {
            ret = append2RewriteQuery(rew_query, rew_query_len,
                                      SEARCH_ROLE_NAME, SEARCH_ROLE_NAME_LEN,
                                      search_query, search_query_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append search query to rewrite query failed.");
                return KS_EFAILED;
            }
        }
        if(filter_query_len > 0)
        {
            ret = append2RewriteQuery(rew_query, rew_query_len,
                                      FILTER_ROLE_NAME, FILTER_ROLE_NAME_LEN,
                                      filter_query, filter_query_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append filter query to rewrite query failed.");
                return KS_EFAILED;
            }
        }
        if(stat_query_len > 0)
        {
            ret = append2RewriteQuery(rew_query, rew_query_len,
                                      STATISTIC_ROLE_NAME, STATISTIC_ROLE_NAME_LEN,
                                      stat_query, stat_query_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append statistic query to rewrite query failed.");
                return KS_EFAILED;
            }
        }
        if(sort_query_len > 0)
        {
            ret = append2RewriteQuery(rew_query, rew_query_len,
                                      SORT_ROLE_NAME, SORT_ROLE_NAME_LEN,
                                      sort_query, sort_query_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append sort query to rewrite query failed.");
                return KS_EFAILED;
            }
        }
        if(other_query_len > 0)
        {
            ret = append2RewriteQuery(rew_query, rew_query_len,
                                      OTHER_ROLE_NAME, OTHER_ROLE_NAME_LEN,
                                      other_query, other_query_len);
            if(unlikely(ret != 0))
            {
                TWARN("QUERYPARSER: append other query to rewrite query failed.");
                return KS_EFAILED;
            }
        }
        rew_query[rew_query_len] = '\0';

        _rew_query = rew_query;
        _rew_query_len = rew_query_len;
        
        return KS_SUCCESS;
    }

    int32_t QueryParameterClassifier::append2SearchQuery(char *sub_query, int32_t &query_len,
                                                         char *param_key, int32_t key_len,
                                                         char *param_value, int32_t value_len)
    {
        return append2SubQuery(sub_query, query_len, param_key, key_len, param_value, value_len);
    }

    int32_t QueryParameterClassifier::append2FilterQuery(char *sub_query, int32_t &query_len,
                                                         char *param_key, int32_t key_len,
                                                         char *param_value, int32_t value_len)
    {
        int32_t ret = -1;
        char *filter_clauses[MAX_FILTER_CLAUSE];
        char *pos = NULL; // 过滤参数中字段名和字段值的分隔位置
        char *key = NULL;
        char *value = NULL;
        int32_t filter_key_len = 0;
        int32_t filter_value_len = 0;

        // filter参数特殊处理
        if(strcmp(param_key, QUERY_PARAMETER_FILTER) == 0
                || strcmp(param_key, QUERY_PARAMETER_MMFILTER) == 0)
        {
            int32_t filter_num = str_split_ex(param_value, FILTER_CLAUSE_SEPARATOR,
                                              filter_clauses,  MAX_FILTER_CLAUSE);
            if(unlikely(filter_num < 0))
            {
                TWARN("QUERYPARSER: split filter clause failed.");
                return -1;
            }
            for(int32_t i = 0; i < filter_num; ++i)
            {
                // 解析range类型的filter语句
                if((pos = strchr(filter_clauses[i], RANGE_FILTER_LEFT_BOUND)) != NULL)
                {
                    ret = 0;
                    if(strchr(filter_clauses[i], ',') == NULL) {
                        char buffer[256];
                        char *p = strchr(filter_clauses[i], RANGE_FILTER_RIGHT_BOUND);
                        if(p) {
                            int clause_len = strlen(filter_clauses[i]);
                            int pre_len    = p - filter_clauses[i];
                            int post_len   = clause_len - pre_len;
                            memcpy(buffer, filter_clauses[i], pre_len);
                            buffer[pre_len] = ',';
                            memcpy(buffer + pre_len + 1, p, post_len);
                            buffer[clause_len + 1] = '\0';
                            ret = append2SubQuery(sub_query, query_len,
                                    filter_clauses[i], pos-filter_clauses[i],
                                    buffer, clause_len + 1);
                        }
                    } else {
                        ret = append2SubQuery(sub_query, query_len,
                                filter_clauses[i], pos-filter_clauses[i],
                                pos, param_value+value_len-pos);
                    }
                    if(unlikely(ret != 0))
                    {
                        TWARN("QUERYPARSER: append filter clause to filter query failed.");
                        return -1;
                    }
                }
                else if((pos = strchr(filter_clauses[i], FILTER_KEYVALUE_SEPARATOR)) != NULL 
                        && strcmp(param_key, QUERY_PARAMETER_FILTER) == 0)
                {
                    ret = append2SubQuery(sub_query, query_len,
                            filter_clauses[i], pos-filter_clauses[i],
                            pos+1, param_value+value_len-pos-1);
                    if(unlikely(ret != 0))
                    {
                        TWARN("QUERYPARSER: append filter clause to filter query failed.");
                        return -1;
                    }
                }
                else if((pos = strchr(filter_clauses[i], '>')) != NULL)
                {
                    if((query_len > 0))    //第二个参数对开始加分隔符&
                    {
                        sub_query[query_len] = QUERY_PARAMETER_SEPARATOR;
                        query_len++;
                    }
                    memcpy(sub_query+query_len, filter_clauses[i], strlen(filter_clauses[i]));
                    query_len += strlen(filter_clauses[i]);
                }
                else if((pos = strchr(filter_clauses[i], '<')) != NULL)
                {
                    if((query_len > 0))    //第二个参数对开始加分隔符&
                    {
                        sub_query[query_len] = QUERY_PARAMETER_SEPARATOR;
                        query_len++;
                    }
                    memcpy(sub_query+query_len, filter_clauses[i], strlen(filter_clauses[i]));
                    query_len += strlen(filter_clauses[i]);
                }
                else if((pos = strchr(filter_clauses[i], '=')) != NULL)
                {
                    ret = append2SubQuery(sub_query, query_len,
                            filter_clauses[i], pos-filter_clauses[i],
                            pos+1, param_value+value_len-pos-1);
                    if(unlikely(ret != 0))
                    {
                        TWARN("QUERYPARSER: append filter clause to filter query failed.");
                        return -1;
                    }
                }
                else
                {
                    TWARN("QUERYPARSER: invalid filter clause.");
                    return -1;
                }
            }
        }
        else
        {
            return append2SubQuery(sub_query, query_len,
                                   param_key, key_len, param_value, value_len);
        }
        return 0;
    }

    int32_t QueryParameterClassifier::append2StatisticQuery(char *sub_query, int32_t &query_len,
                                                            char *param_key, int32_t key_len,
                                                            char *param_value, int32_t value_len)
    {
        return append2SubQuery(sub_query, query_len,
                               param_key, key_len, param_value, value_len);
    }

    int32_t QueryParameterClassifier::append2SortQuery(char *sub_query, int32_t &query_len,
                                                       char *param_key, int32_t key_len,
                                                       char *param_value, int32_t value_len)
    {
        return append2SubQuery(sub_query, query_len,
                               param_key, key_len, param_value, value_len);
    }

    int32_t QueryParameterClassifier::append2OtherQuery(char *sub_query, int32_t &query_len,
                                                        char *param_key, int32_t key_len,
                                                        char *param_value, int32_t value_len)
    {
        return append2SubQuery(sub_query, query_len,
                               param_key, key_len, param_value, value_len);
    }

    int32_t QueryParameterClassifier::append2SubQuery(char *sub_query, int32_t &query_len,
                                                      char *param_key, int32_t key_len,
                                                      char *param_value, int32_t value_len)
    {
        if(unlikely(param_key == NULL || key_len <= 0 
                    || param_value == NULL || value_len <= 0))
        {
            TWARN("QUERYPARSER: null value is existed in parameters.");
            return -1;
        }

        if((query_len > 0))    //第二个参数对开始加分隔符&
        {
            sub_query[query_len] = QUERY_PARAMETER_SEPARATOR;
            query_len++;
        }
        memcpy(sub_query+query_len, param_key, key_len);
        query_len += key_len;
        sub_query[query_len] = PARAMETER_KEYVALUE_SEPARATOR;
        query_len++;

        // 写入value前，需要url encode
        UTIL::urlEncode2(sub_query+query_len, MAX_QUERY_LEN-query_len, param_value);
        query_len += strlen(sub_query+query_len);

        return 0;
    }
    
    int32_t QueryParameterClassifier::append2RewriteQuery(char *rew_query, int32_t &rew_query_len,
                                                          char *sub_prefix, int32_t sub_prefix_len,
                                                          char *sub_query, int32_t sub_query_len)
    {
        if(unlikely(rew_query_len + sub_prefix_len + sub_query_len >= MAX_QUERY_LEN))
        {
            TWARN("QUERYPARSER: no enough space for rewrited query.");
            return -1;
        }

        if(rew_query_len > _prefix_len) // 第二个子句开始加分隔符#
        {
            *(rew_query+rew_query_len) = SUB_QUERY_SEPARATOR;
            rew_query_len++;
        }
        if(sub_prefix_len)
        {
            memcpy(rew_query+rew_query_len, sub_prefix, sub_prefix_len);
            rew_query_len += sub_prefix_len;
        }
        if(sub_query_len)
        {
            *(rew_query+rew_query_len) = ROLE_SUBQUERY_SEPARATOR;
            rew_query_len++;
            memcpy(rew_query+rew_query_len, sub_query, sub_query_len);
            rew_query_len += sub_query_len;
        }

        return 0;
    }

    char* QueryParameterClassifier::getKeyword()
    {
        return _keyword;
    }
 
    int32_t QueryParameterClassifier::getKeywordLen()
    {
        return _keyword_len;
    }

    void  QueryParameterClassifier::setKeyword(char *keyword, int32_t len)
    {
        _keyword = keyword;
        _keyword_len = len;
    }

    char* QueryParameterClassifier::getRewriteQuery()
    {
        return _rew_query;
    }

    int32_t QueryParameterClassifier::getRewriteQueryLen()
    {
        return _rew_query_len;
    }

}
