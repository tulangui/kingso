/** \file
 ********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 10495 $
 *
 * $LastChangedDate: 2012-02-23 17:28:37 +0800 (Thu, 23 Feb 2012) $
 *
 * $Id: UBitAttrFilter.h 10495 2012-02-23 09:28:37Z xiaoleng.nj $
 *
 * $Brief: 无符号bit数值profile类型过滤器 $
 ********************************************************************
 */

#ifndef _UBIT_ATTR_FILTER_H_
#define _UBIT_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"

using namespace index_lib;
using namespace util;

namespace search {

class UBitAttrFilter : public AttrFilter
{
public:
   
    UBitAttrFilter(ProfileDocAccessor *profile_accessor,
                   const char         *field_name,       //字段名 
                   bool                is_negative);     //是（非）过滤

    virtual ~UBitAttrFilter();
 
    /**
     * 添加需要过滤的值
     * @param [in] value 过滤值
     */
    virtual void addFiltValue(const char *value);

    /**
     * bit过滤处理函数，通过位与操作，判定是否过滤
     * @param [in] doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t nDocId);

protected:
    util::Vector<uint64_t> _values;             /* 需要过滤的值 */

};

bool UBitAttrFilter::process(uint32_t nDocId)
{
    
    uint64_t attr_value = 0;
    bool is_found = false;
    int32_t multi_num = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(multi_num == 1) {    //单值字段
        attr_value = _profile_accessor->getUInt(nDocId, _profile_field);
        for(uint32_t i = 0; i < _value_count; i++) {
            if(is_found = ((attr_value & _values[i]) == _values[i])) {
                break;
            }
        }
    } else if (likely(multi_num == 0)) { //定长字段
        _profile_accessor->getRepeatedValue(nDocId, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        if (_profile_field->type == DT_UINT8) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt8();
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found)
                    break;
            }
        } else if (_profile_field->type == DT_UINT16) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt16();
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found)
                    break;
            }
        } else if (_profile_field->type == DT_UINT32) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt32();
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
            }
        } else {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt64();
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
            }
        }
    } else {            //定长多值字段
        _profile_accessor->getRepeatedValue(nDocId, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        if (_profile_field->type == DT_UINT8) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt8();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT8)) { 
                    break;
                }
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
            }
        } else if (_profile_field->type == DT_UINT16) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt16();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT16)) { 
                    break;
                }
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
            }
        } else if (_profile_field->type == DT_UINT32) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt32();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT32)) { 
                    break;
                }
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
            }
        } else {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextUInt64();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT64)) { 
                    break;
                }
                for(uint32_t i = 0; i < _value_count; i++) {
                    if(is_found = ((attr_value & _values[i]) == _values[i])) {
                        break;
                    }
                }
                if(is_found) {
                    break;
                }
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
