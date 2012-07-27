/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: SDMInterface.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SORT_SDMINTERFACE_H_
#define SORT_SDMINTERFACE_H_

#include "sort/RowInterface.h"
#include <map>
#include <string>

class SDMInterface
{
public:
    SDMInterface() {}
    virtual ~SDMInterface(){}
    RowInterface *findRow(const char *pName) 
    {
        std::map<std::string, RowInterface*>::iterator it = _row_map.find(pName);
        if (it ==  _row_map.end()) {
            return NULL;
        }
        return it->second;      
    }
    RowInterface *addRow(const char *name, int size, PF_DATA_TYPE type) 
    {
        std::map<std::string, RowInterface*>::iterator it = _row_map.find(name);
        if (it == _row_map.end()) {
            void *p_buff = malloc(_topdocs * size); 
            if(!p_buff) {
                return NULL;
            }
            RowInterface  *p_row = new RowInterface(name, p_buff, type, size);
            p_row->_is_resortso_malloc = true;
            _row_map.insert(std::make_pair(name, p_row));
            return p_row;
        }
        return it->second;      
    }
    int32_t *getSortArrayList() {return _sort_array_list;}
    void setSortArrayList(int32_t* p_list) { _sort_array_list = p_list;}
    uint32_t getTopDocs() {return _topdocs;}
    void setTopDocs(uint32_t doc_num) { _topdocs = doc_num;}
    void dump() 
    {
        RowInterface *p_row_interface = NULL;
        std::map<std::string, RowInterface*>::iterator iter;
        iter = _row_map.begin(); 
        for(; iter!=_row_map.end(); iter++) {
            p_row_interface = iter->second;
            const char *name = p_row_interface->getName();
            string row_name = iter->first;
            PF_DATA_TYPE type = p_row_interface->getType();
            int size = p_row_interface->getSize();
            void *p_interface_buff = p_row_interface->getBuff();
            fprintf(stderr, "file:%s, row_name:%s\n", __FILE__, row_name.c_str());
        }
        return;
    }
    void setIsMergerCall(bool is_merger_call) 
    {
        _is_merger_call = is_merger_call;
    }
    bool isMergerCall() 
    {
        return _is_merger_call;
    }

public:
    //字段map
    std::map<std::string, RowInterface*> _row_map;
private:
    uint32_t _topdocs;
    int32_t *_sort_array_list;
    bool    _is_merger_call;
};
#endif //SORT_SDMINTERFACE_H_
