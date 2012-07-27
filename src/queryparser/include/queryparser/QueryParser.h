/***********************************************************************************
 * Describe : Query Parser 调用接口类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-04-05
 **********************************************************************************/


#ifndef _QP_CLASS_QUERYPARSER_H_
#define _QP_CLASS_QUERYPARSER_H_


#include "qp_header.h"
#include "QPResult.h"
#include "framework/Context.h"
#include "framework/namespace.h"
#include "mxml.h"
#include "qp_syntax_tree.h"
#include "util/MemPool.h"
#include "util/HashMap.hpp"

namespace queryparser {

typedef struct
{
    NodeLogicType default_relation;    // o
    bool qprohibite;                   // qprohibite
    bool no_keyword;                   // nk
}qp_syntax_special_t;


typedef struct
{
	char argument[MAX_FIELD_NAME_LEN+1];    // 参数名
	char field[MAX_FIELD_NAME_LEN+1];       // 参数对应的索引名
    int flen;	
    char parser[MAX_FIELD_NAME_LEN+1];
	
    qp_arg_dest_t arg_dest;                 // 参数的目标
	unsigned int priority;                  // 参数解析的优先级

	bool optimize;                          // 是否可以优化
	qp_arg_dest_t opt_arg_dest;             // 参数优化后的目标
	unsigned int opt_priority;              // 参数优化后解析的优先级
} qp_field_info_t;


/* 按"&"切割成的域结构 */
typedef struct
{
    char* key;                        // 按"&"切割成的域的key指针
    int klen;                         // 按"&"切割成的域的key长度
    char* value;                      // 按"&"切割成的域的value指针
    int vlen;                         // 按"&"切割成的域的value长度

    qp_field_info_t* field_info;      // key对应的处理信息
} _qp_kvitem_t;


/* query解析的中间结构 */
typedef struct qp_query_info
{
	char query_str[MAX_QYERY_LEN];                // 查询条件字符串副本，用于解析时切割和字符替换
	int query_str_len;                            // 查询条件字符串副本长度
	char query_utf[MAX_QYERY_LEN];                // 查询条件字符串转utf8副本，用于解析时切割和字符替换
	int query_utf_len;                            // 查询条件字符串转utf8副本长度
	
	char* search_query;                           // 检索query子句
	char* filter_query;                           // 过滤query子句
	char* sort_query;                             // 排序query子句
	char* stat_query;                             // 统计query子句
	char* other_query;                            // 其它query子句
	
	_qp_kvitem_t search_kvitem[MAX_QYERY_LEN];    // 检索key-value指针对
	int search_kvlen;                             // 已经使用了多少检索key-value指针对
	_qp_kvitem_t filter_kvitem[MAX_QYERY_LEN];    // 过滤key-value指针对
	int filter_kvlen;                             // 已经使用了多少过滤key-value指针对
	_qp_kvitem_t sort_kvitem[MAX_QYERY_LEN];      // 排序key-value指针对
	int sort_kvlen;                               // 已经使用了多少排序key-value指针对
	_qp_kvitem_t stat_kvitem[MAX_QYERY_LEN];      // 统计key-value指针对
	int stat_kvlen;                               // 已经使用了多少统计key-value指针对
	_qp_kvitem_t other_kvitem[MAX_QYERY_LEN];     // 其它key-value指针对
	int other_kvlen;                              // 已经使用了多少其它key-value指针对
} _qp_query_info_t;





class SyntaxParserInterface;
class FilterParserInterface;

class QueryParser
{
public:

    QueryParser();

    ~QueryParser();

    /*
     * 初始化QueryParser
     *
     * @param config 配置集合
     *
     * @return < 0 初始化出错
     *         ==0 初始化成功
     */
    int init(mxml_node_t* config);

    /*
     * 解析一个查询条件
     *
     * @param context 上下文环境
     * @param qpresult 解析结果
     * @param querystring 查询条件字符串
     *
     * @return < 0 解析出错
     *         ==0 解析成功
     */
    int doParse(const FRAMEWORK::Context* context, QPResult* qpresult, const char* querystring);

private:

    
    void _qp_query_hlkey_cat(qp_syntax_node_t* tree, char* buffer, int &start, int &buflen);
   
    void _qp_query_info_init(_qp_query_info_t* qinfo, QPResult* qpresult, const char* querystring, int querylen);
    
    int  _qp_query_info_split(_qp_query_info_t* qinfo, QPResult *qpresult);
    
    void _qp_query_info_sort(_qp_query_info_t* qinfo);
   
    int _qp_query_info_parse(_qp_query_info_t* qinfo, QPResult *qpresult);

    void qp_syntax_special_init(qp_syntax_special_t* special);
private:
    MemPool _mempool; 
    util::HashMap<const char *, qp_field_info_t*> _field_dict;     
};
}


#endif
