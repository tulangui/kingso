#include "LessFilterParser.h"
#include "util/strfunc.h"
namespace queryparser {
LessFilterParser::LessFilterParser()
{
}
LessFilterParser::~LessFilterParser()
{
}
 
int LessFilterParser::doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen)
{
	if(!mempool || !list || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_filter_sentence_parse argument error, return!");
		return -1;
	}
    char *equal = NULL; 
    char *value = str_trim(string); 
    FilterNode node;
    memset(&node, 0, sizeof(node));
    if(parseName2Node(&node, name, strlen(name)) < 0) {
        return -1;
    }
    node.item_size = 0;
    node.type = ERangeType;
    char* max = value;
	int max_len = strlen(value);
	bool max_equal = false;
	
	if(*max=='='){
		max ++;
		max_len --;
		max_equal = true;
	}
	
	if(addFiltItem2Node(mempool, &node, 0, 0, true, max, max_len, max_equal)){
		TNOTE("qp_filter_data_insert error, return!");
		return -1;
	}
	
	list->addFiltNode(node);
	
	return 0;

}
}
