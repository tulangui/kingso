#include "StringUtil.h"
#include "util/strfunc.h"
#include "MultivalueFilterParser.h"
namespace queryparser {

MultivalueFilterParser::MultivalueFilterParser()
{
}
MultivalueFilterParser::~MultivalueFilterParser()
{
}

int MultivalueFilterParser::doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen)
{
	FilterNode node;
    memset(&node, 0, sizeof(node));
    node.item_size = 0;
	char** values = 0;
	int value_cnt = 0;
	int i = 0;
	
	if(!mempool || !list || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_filter_multivalue_parse argument error, return!");
		return -1;
	}
    char *equal = NULL;
    char *value = str_trim(string);
    char *flag = NULL;
	// 对参数值按';'或' '分割
	values = NEW_VEC(mempool, char*, slen);
	if(!values){
		TNOTE("NEW_VEC values error, return!");
		return -1;
	}
	value_cnt = str_split_ex(value, ',', values, strlen(value));
	if(value_cnt<=0){
		TNOTE("str_split_ex \',\' error, return!");
		return -1;
	}
    if(parseName2Node(&node, name, strlen(name))<0) {
        return -1;
    }
    node.type = ESingleType; 
    // 将分割出的每个段当做一个value添加到过滤项节点中
	for(; i<value_cnt; i++){
		values[i] = str_trim(values[i]);
		if(values[i][0]==0){
			continue;
		}
		if(addFiltItem2Node(mempool, &node, values[i], strlen(values[i]), true, 0, 0, false)){
			TNOTE("qp_filter_data_insert error, return!");
			return -1;
		}
	}   
    list->addFiltNode(node);
	
	return 0;

}
}
