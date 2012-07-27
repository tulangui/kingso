/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: RowInterface.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SORT_ROWINTERFACE_H_
#define SORT_ROWINTERFACE_H_
#include "commdef/commdef.h"

namespace sort_framework_interface_api
{
class RowInterface
{
public:
    RowInterface():_is_resortso_malloc(false), _row_name(NULL), _p_data_buff(NULL), _data_size(0) 
    {
    }
    RowInterface(const char *name, void *p_buff, PF_DATA_TYPE type, int size)
            :_is_resortso_malloc(false), _row_name(name), _p_data_buff(p_buff), _data_type(type), _data_size(size) 
    {
    }
    virtual ~RowInterface(){}
    //返回字段数组
    void *getBuff() const {return _p_data_buff;}

    void setBuff(void *p_buff) {_p_data_buff = p_buff;}
    //返回字段类型
    PF_DATA_TYPE getType() const {return _data_type;}
    //返回字段名
    const char * getName() const {return _row_name;}

    int getSize() const {return _data_size;}
public :
    bool _is_resortso_malloc;
private:
    //字段名称
    const char * _row_name;
    //字段类型,定义在commdef.h中
    PF_DATA_TYPE _data_type;
    //字段数组
    void *_p_data_buff;
    int _data_size;
};

}
#endif //SORT_ROWINTERFACE_H_
