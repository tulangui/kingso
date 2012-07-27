#include "FilterParseInterface.h"
#include "util/strfunc.h"
namespace queryparser {

FilterParseInterface::FilterParseInterface()
{
}

FilterParseInterface::~FilterParseInterface()
{
}

int FilterParseInterface::addFiltItem2Node(MemPool* mempool, FilterNode *node,
        const char* min, uint32_t min_len, bool min_equal,
        const char* max, uint32_t max_len, bool max_equal)
{
	FilterItem* item = NULL;
	
	if(!mempool || !node){
		TNOTE("qp_filter_data_insert argument error, return!");
		return -1;
	}
	if(node->item_size == ITEMS_LEN_MAX){
		TNOTE("node->item_size == ITEMS_LEN_MAX, return!");
		return -1;
	}
	
	item = node->items + node->item_size;
	
	if(min && min[0]!=0){
        item->min = NEW_VEC(mempool, char, min_len+1);
		if(!item->min){
			TNOTE("NEW_VEC min error, return!");
			return -1;
		}
		memcpy(item->min, min, min_len);
		item->min[min_len] = 0;
	} else {
        item->min = NULL;
    }
	item->minEqual = min_equal;
	
	if(max && max[0]!=0 && max_len>0){
		item->max = NEW_VEC(mempool, char, max_len+1);
		if(!item->max){
			TNOTE("NEW_VEC max error, return!");
			return -1;
		}
		memcpy(item->max, max, max_len);
		item->max[max_len] = 0;
	} else {
        item->max = NULL; 
    }
	item->maxEqual = max_equal;
	
	node->item_size ++;
	
	return 0;
}

int FilterParseInterface::parseName2Node(FilterNode *node, char *name, int name_len)
{
    if(!name || name[0] == '\0') {
        return 0; 
    }
	if(name[0]=='_'){
		name ++;
        name_len--;
        if(name_len < 0) {
            return 0;
        }
        node->relation = false;	
    }
	else{
        node->relation = true;	
    }
    char *ptr = NULL;
    char *append_field_type = NULL;
    char *append_field_name = NULL;
    int append_field_nlen = 0;
    int appedn_field_tlen = 0;
    node->append_field_name[0] = '\0';
    node->append_field_type[0] = '\0';
	if(ptr=strchr(name, '{')){
		if(name[name_len-1]!='}'){
			TNOTE("_qp_filter_node_create append field syntax error, return!");
			return -1;
		}
		name[name_len-1] = 0;
		*ptr = 0;
		append_field_type = ptr+1;
		ptr = strchr(append_field_type, ':');
		if(!ptr){
			TNOTE("_qp_filter_node_create append field syntax error, return!");
			return -1;
		}
		*ptr = 0;
		append_field_name = ptr+1;
		name = str_trim(name);
		name_len = strlen(name);
		append_field_name = str_trim(append_field_name);
		append_field_type = str_trim(append_field_type);
        node->field_name_len = name_len;
        snprintf(node->field_name, MAX_FIELD_NAME_LEN, "%s", name);
        snprintf(node->append_field_name, MAX_FIELD_NAME_LEN, "%s", append_field_name);
        snprintf(node->append_field_type, MAX_FIELD_NAME_LEN, "%s", append_field_type);
	} else {
        snprintf(node->field_name, MAX_FIELD_NAME_LEN, "%s", name);
        node->field_name_len = name_len;
    }
    return 0;
}
}
