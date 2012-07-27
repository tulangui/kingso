#include "MoreFilterParser.h"
#include "util/strfunc.h"
namespace queryparser {
MoreFilterParser::MoreFilterParser()
{
}
MoreFilterParser::~MoreFilterParser()
{
}
 
int MoreFilterParser::doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen)
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
    
    char* min = value;
	int min_len = strlen(value);
	bool min_equal = false;
	
	if(*min=='='){
		min ++;
		min_len --;
		min_equal = true;
	}
	
	if(addFiltItem2Node(mempool, &node, min, min_len, min_equal, 0, 0, true)){
		TNOTE("qp_filter_data_insert error, return!");
		return -1;
	}

	list->addFiltNode(node);
	
	return 0;
}
}
