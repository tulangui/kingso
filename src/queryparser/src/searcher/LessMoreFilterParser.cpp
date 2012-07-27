#include "LessMoreFilterParser.h"
#include "util/strfunc.h"
namespace queryparser {

LessMoreFilterParser::LessMoreFilterParser()
{
}
LessMoreFilterParser::~LessMoreFilterParser()
{
}
 
int LessMoreFilterParser::doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen)
{
	if(!mempool || !list || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_filter_sentence_parse argument error, return!");
		return -1;
	}
    char *equal = NULL; 
    char *value = str_trim(string); 
    char *flag = NULL; 
   
    FilterNode node;	
    memset(&node, 0, sizeof(node));
    if(parseName2Node(&node, name, strlen(name)) < 0) {
        return -1;
    }
    node.type = ERangeType;
    node.item_size = 0;
	
    char* end = value+strlen(value);
	char* min = 0;
	int min_len = 0;
	bool min_equal = true;
	char* max = 0;
	int max_len = 0;
	bool max_equal = true;
	char* sub_string[3];
	int sub_string_cnt = 0;
	
	if(*(end-1)!=']'){
		TNOTE("syntax error, no \']\', return!");
		return -1;
	}
	end --;
	*end = 0;
	
	sub_string_cnt = str_split_ex(value, ',', sub_string, 3);
	if(sub_string_cnt==2){
		min = str_trim(sub_string[0]);
		max = str_trim(sub_string[1]);
		min_len = strlen(min);
		max_len = strlen(max);
	}
	else{
		TNOTE("syntax error, too many \',\', return!");
		return -1;
	}
	
	if(addFiltItem2Node(mempool, &node, min, min_len, min_equal, max, max_len, max_equal)){
		TNOTE("qp_filter_data_insert error, return!");
		return -1;
	}
	
    list->addFiltNode(node);	
	return 0;

}
}
