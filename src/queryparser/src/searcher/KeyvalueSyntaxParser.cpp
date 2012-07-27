#include "KeyvalueSyntaxParser.h"
#include "StringUtil.h"
#include "queryparser/qp_syntax_tree.h"
#include "util/strfunc.h"
#include "util/MemPool.h"

namespace queryparser {

KeyvalueSyntaxParser::KeyvalueSyntaxParser()
{
}
KeyvalueSyntaxParser::~KeyvalueSyntaxParser()
{
}

int KeyvalueSyntaxParser::doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
        qp_syntax_tree_t* not_tree,
        char* name, int nlen, char* string, int slen)
{
	static char* default_field_type = "number";
	static int default_field_tlen = 6;
		
	qp_syntax_tree_t* sub_tree = 0;
	char** subs = 0;
	int sub_cnt = 0;
	char** values = 0;
	int value_cnt = 0;
	char* field_name = 0;
	int field_nlen = 0;
	char* field_value = 0;
	int field_vlen = 0;
	char* field_type = 0;
	int field_tlen = 0;
	int i = 0;
	int j = 0;
	bool prohibite = false;
	
	char field[MAX_FIELD_VALUE_LEN+1];
	
	if(!mempool || !field_dict || !tree || !not_tree || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_syntax_keyvalue_parse argument error, return!");
		return -1;
	}
	
	// 申请一棵语法子树

	// 为按';'或".and"分割参数值申请缓存数组
	subs = NEW_VEC(mempool, char*, slen);
	if(!subs){
		TNOTE("NEW_VEC subs error, return!");
		return -1;
	}
	
	// 为按','分割参数值子句值申请缓存数组
	values = NEW_VEC(mempool, char*, slen);
	if(!values){
		TNOTE("NEW_VEC values error, return!");
		return -1;
	}
	
	// 按';'或".and."分割参数值
	sub_cnt = str_split_ex(string, ';', subs, slen);
	if(sub_cnt<=0){
		TNOTE("str_split_ex error, return!");
		return -1;
	}
	
	// 循环处理每个参数值子句
	for(i=0; i<sub_cnt; i++){
		subs[i] = str_trim(subs[i]);
		if(subs[i][0]==0){
			continue;
		}
        sub_tree = qp_syntax_tree_create(mempool);
        if(!sub_tree){
            TNOTE("qp_syntax_tree_create error, return!");
            return -1;
        }
	
		// 将每个子句分析成type,name,value
		if(_parse_keyvalue_string(subs[i], strlen(subs[i]), 
		   &field_name, &field_nlen, &field_value, &field_vlen, &field_type, &field_tlen)){
			TNOTE("_parse_keyvalue_string error, return!");
			return -1;
		}
		if(field_nlen<=0 || field_vlen<=0){
			TNOTE("field name or value error, return![name:%s][value:%s]", field_name, field_value);
			return -1;
		}
		if(!field_type || field_tlen==0){
			field_type = default_field_type;
			field_tlen = default_field_tlen;
		}
		else{
			field_type = str_trim(field_type);
			field_tlen = strlen(field_type);
		}
		field_name = str_trim(field_name);
		field_nlen = strlen(field_name);
		field_value = str_trim(field_value);
		field_vlen = strlen(field_value);
		if(field_nlen<=0 || field_vlen<=0){
			TNOTE("field name or value error, return![name:%s][value:%s]", field_name, field_value);
			return -1;
		}
		// 当前子句是否取非
		if(field_name[0]=='_'){
			prohibite = true;
			field_name ++;
			field_nlen --;
			if(field_nlen<=0){
				TNOTE("field name error, return!");
				return -1;
			}
		}
		else{
			prohibite = false;
		}
		// 将子句值按','分割成多值
		value_cnt = str_split_ex(field_value, ',', values, slen);
		if(value_cnt<=0){
			TNOTE("str_split_ex \',\' error, return!");
			return -1;
		}
		// 将每个值添加入子树，它们之间是或关系
		for(j=0; j<value_cnt; j++){
			values[j] = str_trim(values[j]);
			if(values[j][0]==0){
				continue;
			}
			snprintf(field, MAX_FIELD_VALUE_LEN+1, "%s:%s", field_name, values[j]);
			if(qp_syntax_tree_append_new_node(mempool, sub_tree, name, nlen,
			   field, strlen(field), false, LT_OR)){
				TNOTE("_qp_syntax_tree_append_leaf error, return!");
				return -1;
			}
		}
		// 将生成的子树加入结果语法树中
		if(prohibite){
			if(qp_syntax_tree_append_tree(mempool, not_tree, sub_tree, LT_OR)){
				TNOTE("qp_syntax_tree_append_tree error, return!");
				return -1;
			}
		}
		else{
			if(qp_syntax_tree_append_tree(mempool, tree, sub_tree, LT_AND)){
				TNOTE("qp_syntax_tree_append_tree error, return!");
				return -1;
			}
		}
	}
	
	return 0;
}
}
