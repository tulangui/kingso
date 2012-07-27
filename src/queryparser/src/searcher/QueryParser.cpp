#include "queryparser/QueryParser.h"
#include "SyntaxParseInterface.h"
#include "util/strfunc.h"
#include "index_lib/IndexReader.h"
#include "index_lib/ProfileManager.h"
#include "qp_header.h"
#include "qp_commdef.h"
#include "util/py_url.h"
#include "util/py_url.h"
#include "util/charsetfunc.h"
#include "SyntaxParseFactory.h"
#include "FilterParseFactory.h"
#include <stdlib.h>


using namespace index_lib;
char* qp_arg_dest_name[QP_DEST_MAX] = 
{
	"other",        // 其它目标
	"search",       // 该参数用于检索
	"filter",       // 该参数用于过滤
	"statistic",    // 该参数用于统计
	"sort",         // 该参数用于排序
};

namespace queryparser {
    
/* _qp_kvitem_t的比较方法 */
int _qp_kvitem_cmp(const void* a, const void* b)
{
	_qp_kvitem_t* item1 = (_qp_kvitem_t*)a;
	_qp_kvitem_t* item2 = (_qp_kvitem_t*)b;
	
	// 没有域信息的项往后排
	if(item1->field_info==0){
		if(item2->field_info==0){
			return 0;
		}
		return -1;
	}
	if(item2->field_info==0){
		return 1;
	}
	
	// 参数目标编号按升序排列
	if(item1->field_info->arg_dest<item2->field_info->arg_dest){
		return -1;
	}
	if(item1->field_info->arg_dest>item2->field_info->arg_dest){
		return 1;
	}
	if(item1->field_info->arg_dest==item2->field_info->arg_dest){
		// 参数解析优先级按降序排列
		if(item1->field_info->priority>item2->field_info->priority){
			return -1;
		}
		if(item1->field_info->priority<item2->field_info->priority){
			return 1;
		}
		return 0;
	}
}

/* 无参构造函数 */
QueryParser::QueryParser()
{
}

/* 析构函数 */
QueryParser::~QueryParser()
{
    _field_dict.itReset();

    const char* key = NULL;
    qp_field_info_t* value = NULL;
    while(_field_dict.itNext(key, value)){
        if(value){
            delete value;
            value = NULL;
            key = NULL;   
        }         
    }
    _field_dict.clear();
}

void QueryParser::_qp_query_hlkey_cat(qp_syntax_node_t* root, char* buffer, int &start, int &buflen)
{
    char *pos = 0;
    char real_name[1024];
	qp_field_info_t* field_info = 0;

    util::HashMap<const char*, qp_field_info_t*>::iterator it;
    if(!root) {
        return;
    }
    if(root->left_child == NULL && root->right_child == NULL) {
        if((pos = strstr(root->field_name, "::")) != NULL) {
            memcpy(real_name, root->field_name, pos-root->field_name);
            real_name[pos-root->field_name] = '\0';
        } else {
            memcpy(real_name, root->field_name, strlen(root->field_name));
            real_name[strlen(root->field_name)] = '\0';
        }
        it = _field_dict.find(real_name);
        if(it!=_field_dict.end() && it->value->arg_dest==QP_SYNTAX_DEST
                && strncmp(it->value->parser, SYNTAX_KEYWORD_PARSER, 20) == 0) {
			if(root->field_value_len+1>buflen){
				return;
			}
			memcpy(buffer+start, root->field_value, root->field_value_len);
			start += root->field_value_len;
            buflen -= root->field_value_len;
			*(buffer+start) = ' ';
			start ++;
            buflen --;
            *(buffer+start) = 0;
            return;
        }
    } else {
        _qp_query_hlkey_cat(root->left_child, buffer, start, buflen);
        _qp_query_hlkey_cat(root->right_child, buffer, start, buflen);
    }
    return ;
}


/* 初始化QueryParser */
int QueryParser::init(mxml_node_t* config)
{
	if(!config){
		TERR("QueryParser::init argument error, return!");
		return -1;
	}
    mxml_node_t* module = 0;
	mxml_node_t* qp_root = 0;
	mxml_node_t* qp_args = 0;
	mxml_node_t* qp_arg = 0;
	qp_field_info_t *field_info      = NULL;
	qp_field_info_t *o_info          = NULL;
	qp_field_info_t *qprohibite_info = NULL;
	qp_field_info_t *nk_info         = NULL;
	qp_field_info_t *olu_info        = NULL;
	const char* string = 0;
	
	IndexReader* reader = IndexReader::getInstance();
	ProfileDocAccessor* accesser = ProfileManager::getDocAccessor();
    
    o_info          = new qp_field_info_t;
    qprohibite_info = new qp_field_info_t;
    nk_info         = new qp_field_info_t;
    olu_info        = new qp_field_info_t;

	/* insert o argument */{
		snprintf(o_info->argument, MAX_FIELD_NAME_LEN+1, "o");
		snprintf(o_info->field, MAX_FIELD_NAME_LEN+1, "o");
		snprintf(o_info->parser, MAX_FIELD_NAME_LEN+1, SYNTAX_OTHER_PARSER);
        o_info->flen = 1;
		o_info->arg_dest = QP_SYNTAX_DEST;
		o_info->priority = 10;
		o_info->optimize = false;
		if(!_field_dict.insert(o_info->argument, o_info)){
			TNOTE("qp_field_dict_insert o_info error, return!");
            delete o_info;
            o_info = NULL;
			return -1;
		}
        else{
            o_info = NULL;
        }
	}
	/* insert qprohibite argument */{
		snprintf(qprohibite_info->argument, MAX_FIELD_NAME_LEN+1, "qprohibite");
		snprintf(qprohibite_info->field, MAX_FIELD_NAME_LEN+1, "qprohibite");
		snprintf(qprohibite_info->parser, MAX_FIELD_NAME_LEN+1, SYNTAX_OTHER_PARSER);
        qprohibite_info->flen = 10;
		qprohibite_info->arg_dest = QP_SYNTAX_DEST;
		qprohibite_info->priority = 10;
		qprohibite_info->optimize = false;
		if(!_field_dict.insert(qprohibite_info->argument, qprohibite_info)){
			TNOTE("qp_field_dict_insert qprohibite_info error, return!");
            delete qprohibite_info;
            qprohibite_info = NULL;
			return -1;
		}
        else{
            qprohibite_info = NULL;
        }
	}
	/* insert nk argument */{
		snprintf(nk_info->argument, MAX_FIELD_NAME_LEN+1, "nk");
		snprintf(nk_info->field, MAX_FIELD_NAME_LEN+1, "nk");
		snprintf(nk_info->parser, MAX_FIELD_NAME_LEN+1, SYNTAX_OTHER_PARSER);
        nk_info->flen = 2;
		nk_info->arg_dest = QP_SYNTAX_DEST;
		nk_info->priority = 10;
		nk_info->optimize = false;
		if(!_field_dict.insert(nk_info->argument, nk_info)){
			TNOTE("qp_field_dict_insert nk_info error, return!");
            delete nk_info;
            nk_info = NULL;
			return -1;
		}
        else{
            nk_info = NULL;
        }
	}
	/* insert olu argument */{
		snprintf(olu_info->argument, MAX_FIELD_NAME_LEN+1, "olu");
		snprintf(olu_info->field, MAX_FIELD_NAME_LEN+1, "olu");
		snprintf(olu_info->parser, MAX_FIELD_NAME_LEN+1, FILTER_MULTIVALUE_PARSER);
        olu_info->flen = 3;
		olu_info->arg_dest = QP_FILTER_DEST;
		olu_info->priority = 5;
		olu_info->optimize = false;
		if(!_field_dict.insert(olu_info->argument, olu_info)){
			TNOTE("qp_field_dict_insert olu_info error, return!");
            delete olu_info;
            olu_info = NULL;
			return -1;
		}
        else{
            olu_info = NULL;
        }
	}
	
    qp_root = mxmlFindElement(config, config, KS_MODULE_NAME_QUERY_PARSER, 0, 0, MXML_DESCEND);
	if(!qp_root){
        qp_root = config;
	}
	
	qp_args = mxmlFindElement(qp_root, qp_root, "qp_searcher", 0, 0, MXML_DESCEND_FIRST);
	if(!qp_args){
		TNOTE("mxmlFindElement qp_searcher error, return!");
		return -1;
	}
	
	qp_arg = mxmlFindElement(qp_args, qp_args, "qp_argumet", 0, 0, MXML_DESCEND_FIRST);

	while(qp_arg) { 
        field_info = new qp_field_info_t;
        string = mxmlElementGetAttr(qp_arg, "name");
        if(!string){
            TNOTE("no name argument, discard");
            if(field_info){
                delete field_info;
                field_info = NULL;
            }
            qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
            continue;
        }
        snprintf(field_info->argument, MAX_FIELD_NAME_LEN+1, "%s", string);
        // 获取字段的索引名字
        string = mxmlElementGetAttr(qp_arg, "field");
        if(string){
            field_info->flen = strlen(string);
            if(field_info->flen>MAX_FIELD_NAME_LEN){
                field_info->flen = MAX_FIELD_NAME_LEN;
            }
            snprintf(field_info->field, MAX_FIELD_NAME_LEN+1, "%s", string);
        } else{
            snprintf(field_info->field, MAX_FIELD_NAME_LEN+1, "%s", field_info->argument);
        }
        // 获取字段的目的地
        string = mxmlElementGetAttr(qp_arg, "dest");
        if(string){
            if(strcasecmp("syntax", string)==0){
                field_info->arg_dest = QP_SYNTAX_DEST;
            } else if(strcasecmp("filter", string)==0){
                field_info->arg_dest = QP_FILTER_DEST;
            } else{
                TNOTE("argument %s dest is %s, not QP_SYNTAX_DEST or QP_FILTER_DEST, discard", field_info->argument, string);
                if(field_info){
                    delete field_info;
                    field_info = NULL;
                } 
                qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
                continue;
            }
        } else{
            TNOTE("argument %s is no dest, discard", field_info->argument);
            if(field_info){
                delete field_info;
                field_info = NULL;
            }
            qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
            continue;
        }
        string = mxmlElementGetAttr(qp_arg, "parser");
        if(string){
            snprintf(field_info->parser, MAX_FIELD_NAME_LEN+1, "%s", string);
        } else {
            field_info->parser[0] = '\0';
        }
        // 获取字段的解析优先级
        string = mxmlElementGetAttr(qp_arg, "priority");
        if(string){
            field_info->priority = atoi(string);
            if(field_info->priority>5){
                field_info->priority = 5;
            } else if(field_info->priority<1){
                field_info->priority = 1;
            }
        } else{
            field_info->priority = 3;
        }
        // 优化字段的目的地
        field_info->optimize = false;
        if(field_info->arg_dest==QP_SYNTAX_DEST){
            if(reader && reader->hasField(field_info->field)){
                if(accesser && accesser->getProfileField(field_info->field)){
                    string = mxmlElementGetAttr(qp_arg, "tofilter");
                    if(string){
                        if(strncasecmp(string, "true", 4)==0){
                            field_info->priority = 1;
                            field_info->optimize = true;
                            field_info->opt_arg_dest = QP_FILTER_DEST;
                            field_info->opt_priority = field_info->priority;
                            TNOTE("agument %s optimize to filter", field_info->argument);
                        }
                    } 
                }
            } else{
                TNOTE("argument %s isn't in index, discard", field_info->argument);
                if(field_info){
                    delete field_info;
                    field_info = NULL;
                }
                qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
                continue;
            }
        } else if(field_info->arg_dest==QP_FILTER_DEST){
            if(accesser && accesser->getProfileField(field_info->field)){
                field_info->optimize = false;
            } else{
                TNOTE("argument %s isn't in profile, discard", field_info->argument);
                if(field_info){
                    delete field_info;
                    field_info = NULL;
                }
                qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);           
                continue;
            }
        } else{
            if(field_info){
                delete field_info;
                field_info = NULL;
            }
            qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
            continue;
        }
        // 将信息插入词典
        if(!_field_dict.insert(field_info->argument, field_info)){
            delete field_info;
            field_info = NULL;
            TNOTE("_field_dict.insert %s_info error, return!", field_info->argument);
        }
        else{
            field_info = NULL;
        }
        qp_arg = mxmlFindElement(qp_arg, qp_args, "qp_argumet", 0, 0, MXML_NO_DESCEND);
    }
	
	return 0;
}

void QueryParser::_qp_query_info_init(_qp_query_info_t* qinfo, QPResult* qpresult, const char* querystring, int querylen)
{
	qinfo->query_str_len = querylen>(MAX_QYERY_LEN-1)?(MAX_QYERY_LEN-1):querylen;
	memcpy(qinfo->query_str, querystring, qinfo->query_str_len);
	qinfo->query_str[qinfo->query_str_len] = 0;
	qinfo->query_utf[0] = 0;
	qinfo->query_utf_len = 0;
	
	qinfo->search_query = 0;
	qinfo->filter_query = 0;
	qinfo->sort_query = 0;
	qinfo->stat_query = 0;
	qinfo->other_query = 0;
	
	qinfo->search_kvlen = 0;
	qinfo->filter_kvlen = 0;
	qinfo->sort_kvlen = 0;
	qinfo->stat_kvlen = 0;
	qinfo->other_kvlen = 0;
	
}

int QueryParser::_qp_query_info_split(_qp_query_info_t* qinfo, QPResult *qpresult)
{
	char* querystring = qinfo->query_str;
	int querylen = qinfo->query_str_len;
	char* fields[MAX_QYERY_LEN];
	char sort_query[MAX_QYERY_LEN] = {0};
	int sort_query_len = 0;
	char* nvstr[2];
	int query_cnt = 0;
	int field_cnt = 0;
	char* equal = 0;
	int retval = 0;
	int i = 0;
	
	// 找到真正的query部分
	retval = str_split_ex(querystring, '?', fields, MAX_QYERY_LEN);
	if(retval<=0){
		TNOTE("str_split_ex \'?\' error, return!");
		return -1;
	}
	querystring = fields[retval-1];
	for(i=0; i<retval-1; i++){
		sort_query_len += snprintf(sort_query+sort_query_len, MAX_QYERY_LEN-sort_query_len, "%s?", fields[i]);
	}
	
	// 分解search/filter/sort/stat/other query子句
	retval = str_split_ex(querystring, SUB_QUERY_END, fields, MAX_QYERY_LEN);
	if(retval<=0){
		TNOTE("str_split_ex \'%c\' error, return!", SUB_QUERY_END);
		return -1;
	}
	query_cnt = retval;
	for(i=0; i<query_cnt; i++){
		fields[i] = str_trim(fields[i]);
		if(fields[i][0]==0){
			continue;
		}
		retval = str_split_ex(fields[i], SUB_QUERY_EQUAL, nvstr, 2);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_QUERY_EQUAL);
			return -1;
		}
		if(retval==1){
			TNOTE("rewrite query syntax error, return!", SUB_QUERY_EQUAL);
			return -1;
		}
		nvstr[0] = str_trim(nvstr[0]);
		nvstr[1] = str_trim(nvstr[1]);
		if(strcmp(nvstr[0], qp_arg_dest_name[QP_SYNTAX_DEST])==0){
			qinfo->search_query = nvstr[1];
		}
		else if(strcmp(nvstr[0], qp_arg_dest_name[QP_FILTER_DEST])==0){
			qinfo->filter_query = nvstr[1];
		}
		else if(strcmp(nvstr[0], qp_arg_dest_name[QP_SORT_DEST])==0){
			qinfo->sort_query = nvstr[1];
		}
		else if(strcmp(nvstr[0], qp_arg_dest_name[QP_STATISTIC_DEST])==0){
			qinfo->stat_query = nvstr[1];
		}
		else if(strcmp(nvstr[0], qp_arg_dest_name[QP_OTHER_DEST])==0){
			qinfo->other_query = nvstr[1];
		}
	}
	
	// split search query 
	if(qinfo->search_query){
		// 按"&"分割成多个参数域
		retval = str_split_ex(qinfo->search_query, SUB_STRING_END, fields, MAX_QYERY_LEN);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_STRING_END);
			return -1;
		}
		field_cnt = retval;
		// 按"="分割每个参数域的key和value
		for(i=0; i<field_cnt; i++){
			fields[i] = str_trim(fields[i]);
			if(fields[i][0]==0){
				continue;
			}
			equal = strchr(fields[i], SUB_STRING_EQUAL);
			if(!equal){
				TNOTE("there is no \'%c\', return![field:%s]", SUB_STRING_EQUAL, fields[i]);
				return -1;
			}
			*equal = 0;
			qinfo->search_kvitem[qinfo->search_kvlen].key = fields[i];
			qinfo->search_kvitem[qinfo->search_kvlen].value = equal+1;
			qinfo->search_kvitem[qinfo->search_kvlen].key = str_trim(qinfo->search_kvitem[qinfo->search_kvlen].key);
			qinfo->search_kvitem[qinfo->search_kvlen].value = str_trim(qinfo->search_kvitem[qinfo->search_kvlen].value);
			qinfo->search_kvitem[qinfo->search_kvlen].klen = strlen(qinfo->search_kvitem[qinfo->search_kvlen].key);
			qinfo->search_kvitem[qinfo->search_kvlen].vlen = strlen(qinfo->search_kvitem[qinfo->search_kvlen].value);
			if(qinfo->search_kvitem[qinfo->search_kvlen].klen>0 && qinfo->search_kvitem[qinfo->search_kvlen].vlen>0){
				qinfo->search_kvlen ++;
			}
		}
		// 将每个参数域的value解码(decode)
		for(i=0; (i<qinfo->search_kvlen)&&(qinfo->query_utf_len<MAX_QYERY_LEN); i++){
			retval = decode_url(qinfo->search_kvitem[i].value, qinfo->search_kvitem[i].vlen,
				qinfo->query_utf+qinfo->query_utf_len, MAX_QYERY_LEN-qinfo->query_utf_len-1);
			if(retval<0){
				TNOTE("decode_url error, return!");
				return -1;
			}
			qinfo->search_kvitem[i].value = qinfo->query_utf+qinfo->query_utf_len;
			qinfo->search_kvitem[i].vlen = retval;
			*(qinfo->query_utf+qinfo->query_utf_len+retval) = 0;
			qinfo->query_utf_len += retval+1;
			qinfo->search_kvitem[i].value = str_trim(qinfo->search_kvitem[i].value);
			qinfo->search_kvitem[i].vlen = strlen(qinfo->search_kvitem[i].value);
		}
	}
	
	// split filter query
	if(qinfo->filter_query){
		// 按"&"分割成多个参数域
		retval = str_split_ex(qinfo->filter_query, SUB_STRING_END, fields, MAX_QYERY_LEN);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_STRING_END);
			return -1;
		}
		field_cnt = retval;
		// 按'='/'<'/'>'分割每个参数域的key和value
		for(i=0; i<field_cnt; i++){
			fields[i] = str_trim(fields[i]);
			if(fields[i][0]==0){
				continue;
			}
			do{
				equal = strchr(fields[i], '<');
				if(equal){
					break;
				}
				equal = strchr(fields[i], '>');
				if(equal){
					break;
				}
				equal = strchr(fields[i], '=');
				if(equal){
					break;
				}
				TNOTE("there is no \'=\' or \'<\' or \'>\', return![field:%s]", fields[i]);
				return -1;
			}while(0);
			qinfo->filter_kvitem[qinfo->filter_kvlen].key = fields[i];
			qinfo->filter_kvitem[qinfo->filter_kvlen].klen = equal-fields[i];
			qinfo->filter_kvitem[qinfo->filter_kvlen].value = fields[i];
			qinfo->filter_kvitem[qinfo->filter_kvlen].vlen = strlen(qinfo->filter_kvitem[qinfo->filter_kvlen].value);
			if(qinfo->filter_kvitem[qinfo->filter_kvlen].klen>0 && 
			   qinfo->filter_kvitem[qinfo->filter_kvlen].vlen-qinfo->filter_kvitem[qinfo->filter_kvlen].klen>1){
				qinfo->filter_kvlen ++;
			}
		}
		// 将每个参数域的value解码(decode)
		for(i=0; (i<qinfo->filter_kvlen)&&(qinfo->query_utf_len<MAX_QYERY_LEN); i++){
			retval = decode_url(qinfo->filter_kvitem[i].value, qinfo->filter_kvitem[i].vlen,
				qinfo->query_utf+qinfo->query_utf_len, MAX_QYERY_LEN-qinfo->query_utf_len-1);
			if(retval<0){
				TNOTE("decode_url error, return!");
				return -1;
			}
			qinfo->filter_kvitem[i].value = qinfo->query_utf+qinfo->query_utf_len;
			qinfo->filter_kvitem[i].vlen = retval;
			*(qinfo->query_utf+qinfo->query_utf_len+retval) = 0;
			qinfo->query_utf_len += retval+1;
			qinfo->filter_kvitem[i].value = str_trim(qinfo->filter_kvitem[i].value);
			qinfo->filter_kvitem[i].vlen = strlen(qinfo->filter_kvitem[i].value);
			qinfo->filter_kvitem[i].key[qinfo->filter_kvitem[i].klen] = 0;
		}
	}
	
	// split sort query
	if(qinfo->sort_query){
		sort_query_len += snprintf(sort_query+sort_query_len, MAX_QYERY_LEN-sort_query_len, "%s", qinfo->sort_query);
		qpresult->getParam()->setQueryString(sort_query, sort_query_len);
		// 按"&"分割成多个参数域
		retval = str_split_ex(qinfo->sort_query, SUB_STRING_END, fields, MAX_QYERY_LEN);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_STRING_END);
			return -1;
		}
		field_cnt = retval;
		// 按"="分割每个参数域的key和value
		for(i=0; i<field_cnt; i++){
			fields[i] = str_trim(fields[i]);
			if(fields[i][0]==0){
				continue;
			}
			equal = strchr(fields[i], SUB_STRING_EQUAL);
			if(!equal){
				TNOTE("there is no \'%c\', return![field:%s]", SUB_STRING_EQUAL, fields[i]);
				return -1;
			}
			*equal = 0;
			qinfo->sort_kvitem[qinfo->sort_kvlen].key = fields[i];
			qinfo->sort_kvitem[qinfo->sort_kvlen].value = equal+1;
			qinfo->sort_kvitem[qinfo->sort_kvlen].key = str_trim(qinfo->sort_kvitem[qinfo->sort_kvlen].key);
			qinfo->sort_kvitem[qinfo->sort_kvlen].value = str_trim(qinfo->sort_kvitem[qinfo->sort_kvlen].value);
			qinfo->sort_kvitem[qinfo->sort_kvlen].klen = strlen(qinfo->sort_kvitem[qinfo->sort_kvlen].key);
			qinfo->sort_kvitem[qinfo->sort_kvlen].vlen = strlen(qinfo->sort_kvitem[qinfo->sort_kvlen].value);
			if(qinfo->sort_kvitem[qinfo->sort_kvlen].klen>0 && qinfo->sort_kvitem[qinfo->sort_kvlen].vlen>0){
				qinfo->sort_kvlen ++;
			}
		}
		// 将每个参数域的value解码(decode)
		for(i=0; (i<qinfo->sort_kvlen)&&(qinfo->query_utf_len<MAX_QYERY_LEN); i++){
			retval = decode_url(qinfo->sort_kvitem[i].value, qinfo->sort_kvitem[i].vlen,
				qinfo->query_utf+qinfo->query_utf_len, MAX_QYERY_LEN-qinfo->query_utf_len-1);
			if(retval<0){
				TNOTE("decode_url error, return!");
				return -1;
			}
			qinfo->sort_kvitem[i].value = qinfo->query_utf+qinfo->query_utf_len;
			qinfo->sort_kvitem[i].vlen = retval;
			*(qinfo->query_utf+qinfo->query_utf_len+retval) = 0;
			qinfo->query_utf_len += retval+1;
			qinfo->sort_kvitem[i].value = str_trim(qinfo->sort_kvitem[i].value);
			qinfo->sort_kvitem[i].vlen = strlen(qinfo->sort_kvitem[i].value);
		}
	}
	
	// split stat query
	if(qinfo->stat_query){
		// 按"&"分割成多个参数域
		retval = str_split_ex(qinfo->stat_query, SUB_STRING_END, fields, MAX_QYERY_LEN);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_STRING_END);
			return -1;
		}
		field_cnt = retval;
		// 按"="分割每个参数域的key和value
		for(i=0; i<field_cnt; i++){
			fields[i] = str_trim(fields[i]);
			if(fields[i][0]==0){
				continue;
			}
			equal = strchr(fields[i], SUB_STRING_EQUAL);
			if(!equal){
				TNOTE("there is no \'%c\', return![field:%s]", SUB_STRING_EQUAL, fields[i]);
				return -1;
			}
			*equal = 0;
			qinfo->stat_kvitem[qinfo->stat_kvlen].key = fields[i];
			qinfo->stat_kvitem[qinfo->stat_kvlen].value = equal+1;
			qinfo->stat_kvitem[qinfo->stat_kvlen].key = str_trim(qinfo->stat_kvitem[qinfo->stat_kvlen].key);
			qinfo->stat_kvitem[qinfo->stat_kvlen].value = str_trim(qinfo->stat_kvitem[qinfo->stat_kvlen].value);
			qinfo->stat_kvitem[qinfo->stat_kvlen].klen = strlen(qinfo->stat_kvitem[qinfo->stat_kvlen].key);
			qinfo->stat_kvitem[qinfo->stat_kvlen].vlen = strlen(qinfo->stat_kvitem[qinfo->stat_kvlen].value);
			if(qinfo->stat_kvitem[qinfo->stat_kvlen].klen>0 && qinfo->stat_kvitem[qinfo->stat_kvlen].vlen>0){
				qinfo->stat_kvlen ++;
			}
		}
		// 将每个参数域的value解码(decode)
		for(i=0; (i<qinfo->stat_kvlen)&&(qinfo->query_utf_len<MAX_QYERY_LEN); i++){
			retval = decode_url(qinfo->stat_kvitem[i].value, qinfo->stat_kvitem[i].vlen,
				qinfo->query_utf+qinfo->query_utf_len, MAX_QYERY_LEN-qinfo->query_utf_len-1);
			if(retval<0){
				TNOTE("decode_url error, return!");
				return -1;
			}
			qinfo->stat_kvitem[i].value = qinfo->query_utf+qinfo->query_utf_len;
			qinfo->stat_kvitem[i].vlen = retval;
			*(qinfo->query_utf+qinfo->query_utf_len+retval) = 0;
			qinfo->query_utf_len += retval+1;
			qinfo->stat_kvitem[i].value = str_trim(qinfo->stat_kvitem[i].value);
			qinfo->stat_kvitem[i].vlen = strlen(qinfo->stat_kvitem[i].value);
		}
	}
	
	// split other query
	if(qinfo->other_query){
		// 按"&"分割成多个参数域
		retval = str_split_ex(qinfo->other_query, SUB_STRING_END, fields, MAX_QYERY_LEN);
		if(retval<=0){
			TNOTE("str_split_ex \'%c\' error, return!", SUB_STRING_END);
			return -1;
		}
		field_cnt = retval;
		// 按"="分割每个参数域的key和value
		for(i=0; i<field_cnt; i++){
			fields[i] = str_trim(fields[i]);
			if(fields[i][0]==0){
				continue;
			}
			equal = strchr(fields[i], SUB_STRING_EQUAL);
			if(!equal){
				TNOTE("there is no \'%c\', return![field:%s]", SUB_STRING_EQUAL, fields[i]);
				return -1;
			}
			*equal = 0;
			qinfo->other_kvitem[qinfo->other_kvlen].key = fields[i];
			qinfo->other_kvitem[qinfo->other_kvlen].value = equal+1;
			qinfo->other_kvitem[qinfo->other_kvlen].key = str_trim(qinfo->other_kvitem[qinfo->other_kvlen].key);
			qinfo->other_kvitem[qinfo->other_kvlen].value = str_trim(qinfo->other_kvitem[qinfo->other_kvlen].value);
			qinfo->other_kvitem[qinfo->other_kvlen].klen = strlen(qinfo->other_kvitem[qinfo->other_kvlen].key);
			qinfo->other_kvitem[qinfo->other_kvlen].vlen = strlen(qinfo->other_kvitem[qinfo->other_kvlen].value);
			if(qinfo->other_kvitem[qinfo->other_kvlen].klen>0 && qinfo->other_kvitem[qinfo->other_kvlen].vlen>0){
				qinfo->other_kvlen ++;
			}
		}
		// 将每个参数域的value解码(decode)
		for(i=0; (i<qinfo->other_kvlen)&&(qinfo->query_utf_len<MAX_QYERY_LEN); i++){
			retval = decode_url(qinfo->other_kvitem[i].value, qinfo->other_kvitem[i].vlen,
				qinfo->query_utf+qinfo->query_utf_len, MAX_QYERY_LEN-qinfo->query_utf_len-1);
			if(retval<0){
				TNOTE("decode_url error, return!");
				return -1;
			}
			qinfo->other_kvitem[i].value = qinfo->query_utf+qinfo->query_utf_len;
			qinfo->other_kvitem[i].vlen = retval;
			*(qinfo->query_utf+qinfo->query_utf_len+retval) = 0;
			qinfo->query_utf_len += retval+1;
			qinfo->other_kvitem[i].value = str_trim(qinfo->other_kvitem[i].value);
			qinfo->other_kvitem[i].vlen = strlen(qinfo->other_kvitem[i].value);
		}
	}
	
	return 0;
}

/* 按照处理方法和优先级排序 */
void QueryParser::_qp_query_info_sort(_qp_query_info_t* qinfo)
{
	static qp_field_info_t default_syntax_field_info = {"", "", 0,  SYNTAX_MULTIVALUE_PARSER, QP_SYNTAX_DEST, 3, false};
	static qp_field_info_t default_filter_field_info = {"", "", 0,  FILTER_MULTIVALUE_PARSER, QP_FILTER_DEST, 1, false};
	
	IndexReader* reader = IndexReader::getInstance();
	ProfileDocAccessor* accesser = ProfileManager::getDocAccessor();
	
	int i = 0;
	qp_field_info_t* field_info = 0;
	char* real_key = 0;
	int real_klen = 0;
	char* ptr = 0;
	
    util::HashMap<const char*, qp_field_info_t*>::iterator it;
	
    // 循环处理每个search参数
	for(i=0; i<qinfo->search_kvlen; i++){
		if(qinfo->search_kvitem[i].klen==0 || qinfo->search_kvitem[i].vlen==0){
			continue;
		}
		if(strchr(qinfo->search_kvitem[i].key, ' ')){
			continue;
		}
		real_key = qinfo->search_kvitem[i].key;
		real_klen = qinfo->search_kvitem[i].klen;
		// 查找参数对应的信息
        it = _field_dict.find(real_key);
        if(it != _field_dict.end() && it->value->arg_dest == QP_SYNTAX_DEST) {
			qinfo->search_kvitem[i].field_info = it->value;
		} else {
			if(real_key[0]=='_'){
				real_key ++;
				real_klen --;
			}
            it = _field_dict.find(real_key);
			if(it != _field_dict.end() && it->value->arg_dest==QP_SYNTAX_DEST){
				qinfo->search_kvitem[i].field_info = it->value;
			} else {
				// 如果在index中则选择默认信息
				if(reader && reader->hasField(real_key)){
					qinfo->search_kvitem[i].field_info = &default_syntax_field_info;
				} else {
					qinfo->search_kvitem[i].field_info = NULL;
				}
			}
		}
	}
	
	// 循环处理每个filter参数
	for(i=0; i<qinfo->filter_kvlen; i++){
		if(qinfo->filter_kvitem[i].klen==0 || qinfo->filter_kvitem[i].vlen==0){
			continue;
		}
		if(strchr(qinfo->filter_kvitem[i].key, ' ')){
			continue;
		}
		
		real_key = qinfo->filter_kvitem[i].key;
		real_klen = qinfo->filter_kvitem[i].klen;
		ptr = strchr(real_key, '{');
		if(ptr){
			real_klen = ptr-real_key;
			*ptr = 0;
		}
        it = _field_dict.find(real_key);
		// 查找参数对应的信息
        if(it != _field_dict.end() && it->value->arg_dest==QP_FILTER_DEST){
			qinfo->filter_kvitem[i].field_info = it->value;
		} else {
			if(real_key[0]=='_'){
				real_key ++;
				real_klen --;
			}
            it = _field_dict.find(real_key);
            if(it != _field_dict.end() && it->value->arg_dest==QP_FILTER_DEST){
				qinfo->filter_kvitem[i].field_info = it->value;
			} else {
				// 如果在index中则选择默认信息
				if(accesser && accesser->getProfileField(real_key)){
					qinfo->filter_kvitem[i].field_info = &default_filter_field_info;
				} else if(real_klen==3 && strncmp(real_key, "olu", 3)==0){
					qinfo->filter_kvitem[i].field_info = &default_filter_field_info;
				} else {
					qinfo->filter_kvitem[i].field_info = NULL;
				}
			}
		}
		if(ptr){
			*ptr = '{';
		}
	}
	
	qsort(qinfo->search_kvitem, qinfo->search_kvlen, sizeof(_qp_kvitem_t), _qp_kvitem_cmp);
}
void QueryParser::qp_syntax_special_init(qp_syntax_special_t* special)
{
    if(special){
        special->default_relation = LT_AND;
        special->qprohibite = false;
        special->no_keyword = false;
    }
}

int QueryParser::_qp_query_info_parse(_qp_query_info_t* qinfo, QPResult *qpresult)
{
    SyntaxParseInterface *p_syntax = NULL;
    FilterParseInterface *p_filter = NULL;

    MemPool* mempool = qpresult->getMemPool();
    qp_syntax_tree_t* tree = qpresult->getSyntaxTree();
    FilterList* list = qpresult->getFilterList();
    Param* param = qpresult->getParam();

    qp_syntax_tree_t* not_tree = 0;
    qp_syntax_tree_t* other_not_tree = 0;

    char* key = 0;
    int klen = 0;
    char* value = 0;
    int vlen = 0;

    char* append = "";

    char key_tmp[MAX_FIELD_NAME_LEN+1];
    char hlkey[MAX_QYERY_LEN+1];
    char *hash_key = NULL;
    char *hash_value = NULL;
    int hash_key_len = 0;
    int hash_value_len = 0;
    int i = 0;
    int ret = 0;
    SyntaxParseFactory syntax_factory;
    FilterParseFactory filter_factory;
    // 申请取非语法树
    other_not_tree = qp_syntax_tree_create(mempool);
    if(!other_not_tree){
        TNOTE("qp_syntax_tree_create error, return!");
        return -1;
    }

    // 循环处理每个filter key-value对
    for(i=0; i<qinfo->filter_kvlen; i++){
        if(qinfo->filter_kvitem[i].klen==0 || qinfo->filter_kvitem[i].vlen==0){
            continue;
        }
        if(strchr(qinfo->filter_kvitem[i].key, ' ')){
            continue;
        }
        // 如果没有域信息，不处理
        if(!qinfo->filter_kvitem[i].field_info){
            continue;
        }
        param->setParam(qinfo->filter_kvitem[i].key, qinfo->filter_kvitem[i].value);
        
        if(qinfo->filter_kvitem[i].field_info->flen==0){
            key = qinfo->filter_kvitem[i].key;
            klen = qinfo->filter_kvitem[i].klen;
        } else {   //query中的名字可能不是索引中的名字，要用索引中的名字即配置文件中的field。field_info->field
            char* ptr = 0;
            append = "";
            if(ptr=strchr(qinfo->filter_kvitem[i].key, '{')){
                append = ptr;
            }
            if(qinfo->filter_kvitem[i].key[0]=='_'){
                klen = snprintf(key_tmp, MAX_FIELD_NAME_LEN+1, "_%s%s", qinfo->filter_kvitem[i].field_info->field, append);
            }
            else{
                klen = snprintf(key_tmp, MAX_FIELD_NAME_LEN+1, "%s%s", qinfo->filter_kvitem[i].field_info->field, append);
            }
            key = key_tmp;
            if(klen>MAX_FIELD_NAME_LEN){
                klen = MAX_FIELD_NAME_LEN;
            }
        }
        value = qinfo->filter_kvitem[i].value;
        vlen = qinfo->filter_kvitem[i].vlen;
        p_filter = filter_factory.createFilterParser(mempool, value, NULL, false);
        if(p_filter && p_filter->doFilterParse(mempool, list, key, klen, value, vlen)){
            TNOTE("filter_parse_func error![value=%s]", value);
            return -1;
        }
    }
    // 循环处理每个search key-value对
    for(i=0; i<qinfo->search_kvlen; i++){
        if(qinfo->search_kvitem[i].klen==0 || qinfo->search_kvitem[i].vlen==0){
            continue;
        }
        if(strchr(qinfo->search_kvitem[i].key, ' ')){
            continue;
        }
        // 如果没有域信息，不处理
        if(!qinfo->search_kvitem[i].field_info){
            continue;
        }
        char str_tmp[1024]; 
        const char *p_value = param->getParam(qinfo->search_kvitem[i].key);
        if(p_value) {
            snprintf(str_tmp, 1024, "%s ", p_value);
            snprintf(str_tmp+strlen(p_value)+1, 1024-strlen(p_value)-1, "%s", qinfo->search_kvitem[i].value);
            param->setParam(qinfo->search_kvitem[i].key, str_tmp);
        } else {
            param->setParam(qinfo->search_kvitem[i].key, qinfo->search_kvitem[i].value);
        }
		
        if(qinfo->search_kvitem[i].field_info->flen==0){
			key = qinfo->search_kvitem[i].key;
			klen = qinfo->search_kvitem[i].klen;
		} else{
			if(qinfo->search_kvitem[i].key[0]=='_'){
				snprintf(key_tmp, MAX_FIELD_NAME_LEN+1, "_%s", qinfo->search_kvitem[i].field_info->field);
				key = key_tmp;
				klen = qinfo->search_kvitem[i].field_info->flen+1;
			} else {
				key = qinfo->search_kvitem[i].field_info->field;
				klen = qinfo->search_kvitem[i].field_info->flen;
			}
		}
		value = qinfo->search_kvitem[i].value;
		vlen = qinfo->search_kvitem[i].vlen;
		
		
		if(qinfo->search_kvitem[i].field_info->arg_dest == QP_SYNTAX_DEST){
            if(qinfo->search_kvitem[i].field_info->optimize==false || tree->node_count == 0){
                p_syntax = syntax_factory.createSyntaxParser(mempool, qinfo->search_kvitem[i].field_info->parser);
                // 解析参数
                if(p_syntax && p_syntax->doSyntaxParse(mempool, &_field_dict, tree, other_not_tree, key, klen, value, vlen) < 0){
                    TNOTE("syntax_parse_func error![parser=%s]", qinfo->search_kvitem[i].field_info->parser);
                    return -1;
                }
            } else {
                p_filter = filter_factory.createFilterParser(mempool, value, qinfo->search_kvitem[i].field_info->parser, true);
                // 解析参数
                if(p_filter && p_filter->doFilterParse(mempool, list, key, klen, value, vlen)){
                    TNOTE("filter_parse_func error![value=%s]", value); 
                    return -1;
                }
            }
        }
    }
	
    list->print();
	if(!tree->root){
		TNOTE("tree->leaf_count==0, return!");
		return -1;
	}
    hlkey[0] = '\0';
    int len = MAX_QYERY_LEN+1;
    int start = 0;
    _qp_query_hlkey_cat(tree->root, hlkey, start,  len);
    if(qp_syntax_tree_append_tree(mempool, tree, other_not_tree, LT_NOT)){
        TNOTE("qp_syntax_tree_append_tree error, return!");
        return -1;
    }
	
	param->setHlkeyString(hlkey, strlen(hlkey), true);
	
	// 循环处理每个sort key-value对
	for(i=0; i<qinfo->sort_kvlen; i++){
		if(qinfo->sort_kvitem[i].klen==0 || qinfo->sort_kvitem[i].vlen==0){
			continue;
		}
		if(strchr(qinfo->sort_kvitem[i].key, ' ')){
			continue;
		}

        param->setParam(qinfo->sort_kvitem[i].key, qinfo->sort_kvitem[i].value);
	}
	
	// 循环处理每个stat key-value对
	for(i=0; i<qinfo->stat_kvlen; i++){
		if(qinfo->stat_kvitem[i].klen==0 || qinfo->stat_kvitem[i].vlen==0){
			continue;
		}
		if(strchr(qinfo->stat_kvitem[i].key, ' ')){
			continue;
		}

        param->setParam(qinfo->stat_kvitem[i].key, qinfo->stat_kvitem[i].value);
	}
	
	// 循环处理每个other key-value对
	for(i=0; i<qinfo->other_kvlen; i++){
		if(qinfo->other_kvitem[i].klen==0 || qinfo->other_kvitem[i].vlen==0){
			continue;
		}
		if(strchr(qinfo->other_kvitem[i].key, ' ')){
			continue;
		}
        
        param->setParam(qinfo->other_kvitem[i].key, qinfo->other_kvitem[i].value);
	}
	
	return 0;
}

/* 解析一个查询条件 */
int QueryParser::doParse(const FRAMEWORK::Context* context, QPResult* qpresult, const char* querystring)
{
	if(!context || !qpresult || !querystring){
		TNOTE("QueryParser::doParse argument error, return!");
		return -1;
	}
    
    _qp_query_info_t qinfo;

    int querylen = strlen(querystring);	

	_qp_query_info_init(&qinfo, qpresult, querystring, querylen);
	
	if(_qp_query_info_split(&qinfo, qpresult)){
		TNOTE("_qp_query_info_split error, return!");
		return -1;
	}
	
	_qp_query_info_sort(&qinfo);
	if(_qp_query_info_parse(&qinfo, qpresult)){
		TNOTE("_qp_query_info_parse error, return!");
		return -1;
	}
	
	return 0;

}

}
