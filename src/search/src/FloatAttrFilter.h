/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 476 $
 *
 * $LastChangedDate: 2011-04-08 15:15:34 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: FloatAttrFilter.h 476 2011-04-08 07:15:34Z boluo.ybb $
 *
 * $Brief: float数值profile类型过滤器 $
 ********************************************************************
 */

#ifndef _FLOAT_ATTR_FILTER_H_
#define _FLOAT_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"

using namespace index_lib;
using namespace util;

namespace search {

class FloatAttrFilter : public AttrFilter
{
public:

    FloatAttrFilter(ProfileDocAccessor *profile_accessor,   //profile读写器
                    const char         *field_name,         //profile 字段名
                    bool                is_negative);       //是(非)  过滤

    virtual ~FloatAttrFilter();

    /**
     * 添加需要过滤的值
     * @param [in] value 过滤值
     */
    virtual void addFiltValue(const char *value);

    /**
     * 添加范围过滤
     * @param [in] value 范围结构值
     */
    virtual void addFiltRange(const Range<char*> value);

    /**
     * 过滤处理函数
     * @param [in]  docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t doc_id);

protected:
    Vector<float>          _values;             /* 需要过滤的值 */
    Vector< Range<float> > _ranges;             /* 值范围 */

};

bool FloatAttrFilter::process(uint32_t doc_id)
{
    float   attr_value = 0;
    bool    is_found   = false;
    int16_t discount   = DISCOUNT_BASE;
    if(_zk_rate_pf && _zk_time_pf) {
        discount = getDiscount(doc_id);
    }
    int32_t multi_num = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(multi_num == 1) {    //单值字段
        attr_value = _profile_accessor->getFloat(doc_id, _profile_field);
        attr_value = (attr_value * discount) / DISCOUNT_BASE;
        for(uint32_t i = 0; i < _value_count; i++) {
            if(is_found = (attr_value == _values[i])) {
                break;
            }
        }
        if(!is_found && _range_count > 0) {
            for(uint32_t i = 0; i < _range_count; i++) {
                if(is_found = _ranges[i].in(attr_value)) {
                    break;
                }
            }
        }
    } else if (likely(multi_num == 0)) {    //变长字段
        _profile_accessor->getRepeatedValue(doc_id, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        for(int32_t j = 0; j < attr_value_num; j++) {
            attr_value = _iterator.nextFloat();
            attr_value = (attr_value * discount) / DISCOUNT_BASE;
            for(uint32_t i = 0; i < _value_count; i++) {
                if(is_found = (attr_value == _values[i])) {
                    break;
                }
            }
            if(is_found) {
                 break;
            }
            if(_range_count > 0) {
                for(uint32_t i = 0; i < _range_count; i++) {
                    if(is_found = _ranges[i].in(attr_value)) {
                         break;
                    }
                }
            }
            if(is_found) {
                 break;
            }
        }
    } else {        //多值字段
        _profile_accessor->getRepeatedValue(doc_id, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        for(int32_t j = 0; j < attr_value_num; j++) {
            attr_value = _iterator.nextFloat();
            attr_value = (attr_value * discount) / DISCOUNT_BASE;
            if (unlikely(attr_value == _profile_field->defaultEmpty.EV_FLOAT)) { 
                break;
            }
            for(uint32_t i = 0; i < _value_count; i++) {
                if(is_found = (attr_value == _values[i])) {
                    break;
                }
            }
            if(is_found) {
                 break;
            }
            if(_range_count > 0) {
                for(uint32_t i = 0; i < _range_count; i++) {
                    if(is_found = _ranges[i].in(attr_value)) {
                         break;
                    }
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
