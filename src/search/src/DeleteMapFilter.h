#ifndef _DELETE_MAP_FILTER_H_
#define _DELETE_MAP_FILTER_H_

#include "FilterInterface.h"
#include "index_lib/DocIdManager.h"

using namespace index_lib;
using namespace search;

namespace search {

class DeleteMapFilter : public FilterInterface
{
public:
    
    DeleteMapFilter(DocIdManager::DeleteMap *p_delete_map) 
                   : _p_delete_map(p_delete_map)
    {
   
    }

    virtual ~DeleteMapFilter()
    {
        _p_delete_map = NULL; 
    }

    /**
     * 过滤处理函数
     * @param doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    bool process(uint32_t doc_id)
    {
        return !_p_delete_map->isDel(doc_id);
    }

private:
    DocIdManager::DeleteMap * _p_delete_map;

};

}

#endif
