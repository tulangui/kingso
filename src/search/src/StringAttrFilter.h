/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 476 $
 *
 * $LastChangedDate: 2011-04-08 15:15:34 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: StringAttrFilter.h 476 2011-04-08 07:15:34Z boluo.ybb $
 *
 * $Brief: string profile类型过滤器 $
 ********************************************************************
 */

#ifndef _STRING_ATTR_FILTER_H_
#define _STRING_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"
#include <string.h>

using namespace index_lib;
using namespace util;

namespace search {

class StringAttrFilter : public AttrFilter
{
public:

    StringAttrFilter(ProfileDocAccessor *profile_accessor,   //profile读写器
                     const char         *field_name,         //profile 字段名
                     bool                is_negative);       //是(非)  过滤

    virtual ~StringAttrFilter();

    /**
     * 添加需要过滤的值
     * @param [in] value 过滤值
     */
    virtual void addFiltValue(const char *value);

    /**
     * 过滤处理函数
     * @param [in] doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t nDocId);

protected:
    Vector<const char*> _values;                 /* 需要过滤的值 */

};

bool StringAttrFilter::process(uint32_t nDocId)
{
    const char* attr_value = NULL;
    bool        is_found   = false;
    int32_t multi_num  = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(multi_num == 1) {
        attr_value = _profile_accessor->getString(nDocId, _profile_field);
        if(likely(attr_value != NULL)) {
            for(uint32_t i = 0; i < _value_count; i++) {
                if(is_found = (strcmp(attr_value, _values[i]) == 0)) {
                    break;
                }
            }
        } else {
            is_found = false;
        }
    } else {
        _profile_accessor->getRepeatedValue(nDocId, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        for(int32_t j = 0; j < attr_value_num; j++) {
            attr_value = _iterator.nextString();
            for(uint32_t i = 0; i < _value_count; i++) {
                if(is_found = (strcmp(attr_value, _values[i]) == 0)) {
                    break;
                }
            }
            if(is_found) {
                 break;
            }
        }
    }

    if(_is_negative) {
        is_found = !is_found;
    }

    return is_found;
}

}

#endif
