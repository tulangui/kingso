/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 68 $
 *
 * $LastChangedDate: 2011-03-22 10:16:32 +0800 (Tue, 22 Mar 2011) $
 *
 * $Id: UUIntAttrFilter.h 68 2011-03-22 02:16:32Z boluo.ybb $
 *
 * $Brief: uint数值profile类型过滤器 $
 ********************************************************************
 */

#ifndef _UINT_ATTR_FILTER_H_
#define _UINT_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"

using namespace index_lib;
using namespace util;

namespace search {

class UIntAttrFilter : public AttrFilter
{
public:

    UIntAttrFilter(ProfileDocAccessor *profile_accessor,
                   const char         *field_name,       //字段名 
                   bool                is_negative);     //是（非）过滤

    virtual ~UIntAttrFilter();

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
     * @param [in] doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t doc_id);

protected:
    Vector<uint64_t>          _values;          /* 需要过滤的值 */
    Vector< Range<uint64_t> > _ranges;          /* 值范围 */

};

bool UIntAttrFilter::process(uint32_t doc_id)
{
    uint64_t attr_value = 0;
    bool     is_found   = false;
    int32_t  multi_num  = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(multi_num == 1) {    //单值字段
        attr_value = _profile_accessor->getUInt(doc_id, _profile_field);
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
        int32_t nAttrValueCnt = _iterator.getValueNum();
        if (_profile_field->type == DT_UINT32) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt32();
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
        } else if (_profile_field->type == DT_UINT64) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt64();
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
        } else if (_profile_field->type == DT_UINT8) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt8();
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
        } else {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt16();
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
    } else { // 定长多值字段
        _profile_accessor->getRepeatedValue(doc_id, _profile_field, _iterator);
        int32_t nAttrValueCnt = _iterator.getValueNum();
        if (_profile_field->type == DT_UINT32) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt32();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT32)) { 
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
        } else if (_profile_field->type == DT_UINT64) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt64();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT64)) { 
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
        } else if (_profile_field->type == DT_UINT8) {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt8();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT8)) { 
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
        } else {
            for(int32_t j = 0; j < nAttrValueCnt; j++) {
                attr_value = _iterator.nextUInt16();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT16)) { 
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
    }

    if(_is_negative) {
        is_found = !is_found;
    }
    return is_found;
}

}

#endif
