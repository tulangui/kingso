/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: QueryParserDef.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: QueryParser在blender改写所需要的定义和结构 $
 ********************************************************************/

#ifndef QUERYPARSER_QUERYPARSERDEF_H
#define QUERYPARSER_QUERYPARSERDEF_H

#include <stdint.h>

namespace queryparser{


static char *SEARCH_ROLE_NAME = "search";         // 检索参数和子句标识
static char *FILTER_ROLE_NAME = "filter";         // 过滤参数和子句标识
static char *STATISTIC_ROLE_NAME = "statistic";   // 统计参数和子句标识
static char *SORT_ROLE_NAME = "sort";             // 排序参数和子句标识
static char *OTHER_ROLE_NAME = "other";           // 其它参数和子句标识

static int32_t SEARCH_ROLE_NAME_LEN = strlen(SEARCH_ROLE_NAME);         // 检索参数和子句标识长度
static int32_t FILTER_ROLE_NAME_LEN = strlen(FILTER_ROLE_NAME);         // 过滤参数和子句标识长度
static int32_t STATISTIC_ROLE_NAME_LEN = strlen(STATISTIC_ROLE_NAME);   // 统计参数和子句标识长度
static int32_t SORT_ROLE_NAME_LEN  = strlen(SORT_ROLE_NAME);            // 排序参数和子句标识长度
static int32_t OTHER_ROLE_NAME_LEN = strlen(OTHER_ROLE_NAME);           // 其它参数和子句标识长度


static char *OPERATOR_AND = "AND";              // 与操作符
static char *OPERATOR_OR  = "OR";               // 非操作符
static char *OPERATOR_NOT = "NOT";              // 或操作符

static int32_t OPERATOR_AND_LEN = strlen(OPERATOR_AND);   // 与操作符长度
static int32_t OPERATOR_OR_LEN  = strlen(OPERATOR_OR);    // 非操作符长度
static int32_t OPERATOR_NOT_LEN = strlen(OPERATOR_NOT);   // 或操作符长度

static char *QUERY_PARAMETER_KSQ       = "ksq";                        // 改写后的参数ksq
static int32_t QUERY_PARAMETER_KSQ_LEN = strlen(QUERY_PARAMETER_KSQ);  // 改写后的参数ksq长度


static const char LEFT_BRACKET      = '(';            // 左括号
static const char RIGHT_BRACKET     = ')';            // 右括号
static const char SIGN_SPACES       = ' ';            // 空格
static const char SIGN_MINUS        = '-';            // 减号
static const char SIGN_UNDERLINE    = '_';            // 下划线
static const char SIGN_COLON        = ':';            // 冒号

static const int32_t LEFT_BRACKET_LEN  = 1;           // 前半括号长度
static const int32_t RIGHT_BRACKET_LEN = 1;           // 后半括号长度

static const char SUB_QUERY_EQUAL  = '=';             // 子query名与值的分隔符
static const char SUB_QUERY_END    = '#';             // 子query结束符

static const char SUB_QUERY_SEPARATOR = '#';          // 子query分隔符
static const char PARAMETER_KEYVALUE_SEPARATOR = '='; // 子句名与值的分隔符
static const char QUERY_PARAMETER_SEPARATOR = '&';    // 子句结束符
static const char QUERY_PREFIX_SEPARATOR   = '?';     // query前缀和有效query的分隔符
static const char ROLE_SUBQUERY_SEPARATOR  = '=';     // 子query和其功能名间的分隔符
static const char FILTER_CLAUSE_SEPARATOR  = ';';     // 过滤子句间的分隔符
static const char RANGE_FILTER_LEFT_BOUND  = '[';     // 范围过滤子句左区间
static const char RANGE_FILTER_RIGHT_BOUND = ']';     // 范围过滤子句右区间
static const char FILTER_KEYVALUE_SEPARATOR = ':';    // 一般过滤子句字段名和字段值的分隔符

static const char *PACKAGE_FIELD_SEPARATOR  = "::";       // package查询package名和字段名间的分隔符
static const char PACKAGE_FIELD_SEPARATOR_LEN = strlen(PACKAGE_FIELD_SEPARATOR);  // package查询package名和字段名间的分隔符长度

static const char FIELD_KEYVALUE_SEPARATOR  = ':';       // package查询中字段名和字段值间的分隔符

static char *TOKEN_LEFT_BOUND    = "(";             // token间的左边界标识符
static char *TOKEN_RIGHT_BOUND   = ")";             // token间的右边界标识符
static int32_t TOKEN_LEFT_BOUND_LEN = strlen(TOKEN_LEFT_BOUND);             // token间的左边界标识符
static int32_t TOKEN_RIGHT_BOUND_LEN = strlen(TOKEN_RIGHT_BOUND);           // token间的右边界标识符

static const char PACKAGE_LEFT_BOUND  = '[';            // package查询中字段值间的左边界标识符
static const char PACKAGE_RIGHT_BOUND = ']';            // package查询中字段值间的右边界标识符

static const char *PACKAGE_NAME = "phrase";                               // phrase标签名
static const int32_t PACKAGE_NAME_LEN = strlen(PACKAGE_NAME);             // phrase标签名长度
static const char *REW_PACKAGE_NAME = "title";                            // 改写后的标签名
static const int32_t REW_PACKAGE_NAME_LEN = strlen(REW_PACKAGE_NAME);     // 改写后的标签名长度
static const char *PACKAGE_PREFIX = "phrase::";                           // phrase标签名(含分隔符)
static const int32_t PACKAGE_PREFIX_LEN = strlen(PACKAGE_PREFIX);         // phrase标签名长度
static const char *REW_PACKAGE_PREFIX = "title::";                        // 改写后的标签名(含分隔符)
static const int32_t  REW_PACKAGE_PREFIX_LEN = strlen(REW_PACKAGE_PREFIX);// 改写后的标签名长度

static const char *QUERY_PARAMETER_Q             =  "q";             // 参数q
static const char *QUERY_PARAMETER_REWQ          =  "rewq";          // 参数rewq
static const char *QUERY_PARAMETER_QINFO         =  "qinfo";         // 参数qinfo
static const char *QUERY_PARAMETER_ADVSORT       =  "advsort";       // 参数advsort
static const char *QUERY_PARAMETER_ADVSORT_VALUE =  "advtaobao";     // 参数advsort
static const char *QUERY_PARAMETER_FILTER        =  "filter";        // 过滤参数
static const char *QUERY_PARAMETER_MMFILTER      =  "mmfilter";      // 折扣价格过滤参数


static const int32_t MAX_PARAMETER_ROLE_NUM  = 5;           // query参数作用种类的最大数量
static const int32_t MAX_PARAMETER_ROLE_LEN  = 64;          // query参数作用名最大长度

static const int32_t  MAX_QUERY_LEN       = 8192;           // query最大长度
static const int32_t  MAX_VALUE_LEN       = 8192;           // query参数值的最大长度
static const int32_t  MAX_KEYVALUE_NUM    = 1024;           // query参数的最大数量
static const int32_t  MAX_SECTION_NUM     = 1024;           // query中keyword包含段落的最大数量
static const int32_t  MAX_KEYWORD_LEN     = 8192;           // query中keyword的最大长度

static const int32_t  MAX_STOPWORD_LEN    = 3;              // 停用词最大长度(字节数), 需要转换成utf8后计算
static const int32_t  MAX_FILTER_CLAUSE   = 16;             // 过滤子句的最大个数

static const int32_t MAX_PACKAGE_BUCKET_NUM    =  7;        // 保存package的最大桶个数，取素数
static const int32_t MAX_PARAMETER_BUCKET_NUM  =  257;      // 保存query中parameter的最大桶个数，取素数




    /* query中参数的作用 */
    enum QueryParameterRole
    {
        QP_OTHER_ROLE,        // 其它
        QP_SEARCH_ROLE,       // 该参数用于检索
        QP_FILTER_ROLE,       // 该参数用于过滤
        QP_STATISTIC_ROLE,    // 该参数用于统计
        QP_SORT_ROLE,         // 该参数用于排序
        QP_ROLE_MAX,          // 参数枚举最大值
    };

    
    /* query中参数作用配置信息 */
    struct ParameterConfig
    {
        const char *param_name;                                 // 参数名
        QueryParameterRole param_roles[MAX_PARAMETER_ROLE_NUM]; // 参数作用
        int32_t role_num;                                       // 参数作用的个数
    };

    /* query中参数名和值的结构信息 */
    struct ParameterKeyValueItem
    {
        char *param_key;                // 参数名
        char *param_value;              // 参数值
        int32_t  key_len;               // 参数名长度
        int32_t  value_len;             // 参数值长度
        ParameterConfig *p_param_conf;  // 参数配置信息
    };

    /* package相关配置信息 */
    struct PackageConfigInfo
    {
        const char *name;               // package名称
        bool need_tokenize;             // package是否需要分词
    };

    /* 关键词中段落信息类型 */
    enum SectionType
    {
        QRST_TOKEN,             // token组
        QRST_PACKAGENAME,       // package名
        QRST_OPERATOR,          // 逻辑运算符
        QRST_BRACKET,           // 括号
    };

    /* 关键词中段落信息 */
    struct SectionInfo
    {
        char *str;              // 段落起始位置
        int32_t len;            // 段落长度
        char *seg_begin;        // 段落需要分词部分的开始位置
        int32_t seg_len;        // 需要分词段落长度
        char *seg_result;       // 分词结果
        int32_t seg_result_len; // 分词结果长度
        SectionType type;       // 段落分类
        bool valid;             // 是否有效
        int32_t range;        
    };
}

#endif
