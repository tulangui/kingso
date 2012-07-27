#include "queryparser/FilterList.h"
#include <stdio.h>
#include "util/Log.h"

namespace queryparser {

FilterList::FilterList() : _node_size(0), _cur_index(0)
{
    _filter_nodes.clear();
}

/* 析构函数 */
FilterList::~FilterList()
{
    _filter_nodes.clear();
}


/* 获取节点个数 */
const int FilterList::getNum() const
{
	return _node_size; 
}

/* 获取旺旺在线状态 */
const OluStatType FilterList::getOluStatType() const
{
    for(int i=0; i<_node_size; i++)	{
        if(_filter_nodes[i].field_name_len == 3 
                && (strncmp(_filter_nodes[i].field_name, "olu", 3) == 0 
                    || strncmp(_filter_nodes[i].field_name, "OLU", 3) == 0)) 
        {
            if(!_filter_nodes[i].items[0].min) {
                return OLU_ALL;
            }
            if(strncasecmp("YES", _filter_nodes[i].items[0].min, 3)==0){
                return (_filter_nodes[i].relation ? OLU_YES : OLU_NO);
            } else if(strncasecmp("NO", _filter_nodes[i].items[0].min, 2)==0){
                return (_filter_nodes[i].relation ? OLU_NO : OLU_YES);
            }
        }
    }
    return OLU_ALL;
}


/* 获取下一组数据 */
int FilterList::next(char          *&fieldName,  char    *&appendFieldName, 
                     FilterItem    *&fieldItems, uint32_t &number,
                     FilterItemType &type,       bool     &relation)
{
    if(_cur_index >= _node_size) {
   		fieldName = 0;
		appendFieldName = 0;
		fieldItems = 0;
		number = 0;
		type = EStringType;
		relation = false;
		return -1;
    }
    	
	// 填充参数返回值
	fieldName       = _filter_nodes[_cur_index].field_name;
	appendFieldName = _filter_nodes[_cur_index].append_field_name;
	fieldItems      = _filter_nodes[_cur_index].items;
	number          = _filter_nodes[_cur_index].item_size;
	type            = _filter_nodes[_cur_index].type;
	relation        = !_filter_nodes[_cur_index].relation;
    
    _cur_index++;
    return 0;
}

void FilterList::addFiltNode(FilterNode node)
{
    _filter_nodes.pushBack(node);
    _node_size++;
}

void FilterList::first()
{
    _cur_index = 0;
}
/* 打印过滤信息链表，调试时使用 */
void FilterList::print()
{
	static const int buflen = 4096;
	
	char* fieldName = 0;
	char* appendFieldName = 0;
	FilterItem* fieldItems;
	uint32_t number = 0;
	FilterItemType type = EStringType;
	bool relation = true;
	int ret = 0;
	int idx = 0;
	OluStatType stat = getOluStatType();
	
	char buffer[buflen];
	char* cur = 0;
	int left = 0;
	
	if(stat==OLU_NO){
		TDEBUG("FilterList fieldName=olu relation=false fieldItems=OLU_NO");
	}
	else if(stat==OLU_YES){
		TDEBUG("FilterList fieldName=olu relation=false fieldItems=OLU_YES");
	}
	else{
		TDEBUG("FilterList fieldName=olu relation=false fieldItems=OLU_ALL");
	}
	
	first();
	
	// 循环打印每个过滤信息
	while(next(fieldName, appendFieldName, fieldItems, number, type, relation)==0){
		cur = buffer;
		left = buflen;
		// 打印过滤域名
		ret = snprintf(cur, left, "FilterList node[%d] fieldName=%s", idx, fieldName);
		cur += ret;
		left -= ret;
		ret = snprintf(cur, left, "FilterList node[%d] appendFieldName=%s", idx, appendFieldName);
		cur += ret;
		left -= ret;
		// 打印过滤域值类型
		if(type==EStringType){
			ret = snprintf(cur, left, " type=EStringType");
			cur += ret;
			left -= ret;
		}
		else if(type==ESingleType){
			ret = snprintf(cur, left, " type=ESingleType");
			cur += ret;
			left -= ret;
		}
		else{
			ret = snprintf(cur, left, " type=ERangeType");
			cur += ret;
			left -= ret;
		}
		// 打印取是还是取非信息
		if(relation==true){
			ret = snprintf(cur, left, " relation=true");
			cur += ret;
			left -= ret;
		}
		else{
			ret = snprintf(cur, left, " relation=false");
			cur += ret;
			left -= ret;
		}
		// 打印域值
		for(int i=0; i<number; i++){
			if(type==EStringType || type==ESingleType){
				ret = snprintf(cur, left, " fieldItems[%d]=%s;", i, fieldItems[i].min);
				cur += ret;
				left -= ret;
				if(left<=0){
					break;
				}
			}
			else{
				ret = snprintf(cur, left, " fieldItems[%d]=", i);
				cur += ret;
				left -= ret;
				if(left<=0){
					break;
				}
				if(fieldItems[i].minEqual){
					ret = snprintf(cur, left, "[");
					cur += ret;
					left -= ret;
					if(left<=0){
						break;
					}
				}
				else{
					ret = snprintf(cur, left, "(");
					cur += ret;
					left -= ret;
					if(left<=0){
						break;
					}
				}
				ret = snprintf(cur, left, "%s,%s", fieldItems[i].min, fieldItems[i].max);
				cur += ret;
				left -= ret;
				if(left<=0){
					break;
				}
				if(fieldItems[i].maxEqual){
					ret = snprintf(cur, left, "];");
					cur += ret;
					left -= ret;
					if(left<=0){
						break;
					}
				}
				else{
					ret = snprintf(cur, left, ");");
					cur += ret;
					left -= ret;
					if(left<=0){
						break;
					}
				}
			}
		}
		idx ++;
	}
}
}
