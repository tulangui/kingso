#include "MultivalueSyntaxParser.h"
#include "util/strfunc.h"
namespace queryparser {
MultivalueSyntaxParser::MultivalueSyntaxParser()
{
}

MultivalueSyntaxParser::~MultivalueSyntaxParser()
{
}

int MultivalueSyntaxParser::doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
        qp_syntax_tree_t* not_tree,
        char* name, int nlen, char* string, int slen)
{
    qp_syntax_tree_t* sub_tree = 0;
	char* field_name = name;
	int field_nlen = nlen;
	bool prohibite = false;
	char** values = 0;
	int value_cnt = 0;
	int i = 0;
	
	if(!mempool || !field_dict || !tree || !not_tree || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_syntax_multivalue_parse argument error, return!");
		return -1;
	}
	
	// 检查是否是取非参数
	if(field_name[0]=='_'){
		field_name ++;
		field_nlen --;
		if(field_nlen<=0){
			TNOTE("qp_syntax_multivalue_parse field name error, return!");
			return -1;
		}
		prohibite = true;
	}
	
	// 为该参数申请一个语法子树
	sub_tree = qp_syntax_tree_create(mempool);
	if(!sub_tree){
		TNOTE("qp_syntax_tree_create error, return!");
		return -1;
	}
	
	// 将参数值按';'或' '分段
	values = NEW_VEC(mempool, char*, slen);
	if(!values){
		TNOTE("NEW_VEC values error, return!");
		return -1;
	}
	value_cnt = split_multi_delims(string, values, slen, ",;");
	if(value_cnt<=0){
		TNOTE("str_split_ex \',\' error, return!");
		return -1;
	}
	
	// 将每个段落作为一个value，添加到语法子树中
    if(strchr(values[i], ';')) {
        for(; i<value_cnt; i++){
            values[i] = str_trim(values[i]);
            if(values[i][0]==0){
                continue;
            }
            if(qp_syntax_tree_append_new_node(mempool, sub_tree, field_name, field_nlen,
                        values[i], strlen(values[i]), false, LT_AND)){
                TNOTE("_qp_syntax_tree_append_leaf error, return!");
                return -1;
            }
        }
    } else {
        for(; i<value_cnt; i++){
            values[i] = str_trim(values[i]);
            if(values[i][0]==0){
                continue;
            }
            if(qp_syntax_tree_append_new_node(mempool, sub_tree, field_name, field_nlen,
                        values[i], strlen(values[i]), false, LT_OR)){
                TNOTE("_qp_syntax_tree_append_leaf error, return!");
                return -1;
            }
        }
    }
	// 将语法子树添加到语法树或取非语法树中
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
	
	return 0;

}
}
