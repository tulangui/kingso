/***********************************************************************************
 * Describe : queryparser输出的语法存储结构的虚基类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-10
 **********************************************************************************/

#ifndef _QP_FILTPARSEINTERFACE_H
#define _QP_FILTPARSEINTERFACE_H

#include <stdint.h>

#include "queryparser/FilterList.h"
#include "util/MemPool.h"

namespace queryparser {
class FilterParseInterface 
{
public:
    FilterParseInterface();
    virtual ~FilterParseInterface(); 
    
    virtual int doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen) = 0;

protected:        
    int addFiltItem2Node(MemPool* mempool, FilterNode *node,
            const char* min, uint32_t min_len, bool min_equal,
            const char* max, uint32_t max_len, bool max_equal);

    int parseName2Node(FilterNode *node, char *name, int name_len);

private:
    char *_name;
    int _name_len; 
    char *_value;
    int _value_len; 
};
}
#endif
