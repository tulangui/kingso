/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 68 $
 *
 * $LastChangedDate: 2011-03-22 10:16:32 +0800 (Tue, 22 Mar 2011) $
 *
 * $Id: IntAttrFilter.h 68 2011-03-22 02:16:32Z boluo.ybb $
 *
 * $Brief: int数值profile类型过滤器 $
 ********************************************************************
 */

#ifndef _INT_ATTR_FILTER_H_
#define _INT_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"

using namespace index_lib;
using namespace util;

namespace search {

class IntAttrFilter : public AttrFilter
{
public:
    
    IntAttrFilter(ProfileDocAccessor *profile_accessor,   //profile读写器
                  const char         *field_name,         //profile 字段名
                  bool                is_negative);       //是(非)  过滤

    virtual ~IntAttrFilter();

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
    Vector<int64_t>          _values;            /* 需要过滤的值 */
    Vector< Range<int64_t> > _ranges;            /* 值范围 */

};

bool IntAttrFilter::process(uint32_t doc_id)
{
    int64_t attr_value     = 0;
    bool    is_found  = false;
    int32_t multi_num = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(multi_num == 1) {        //单值字段
        attr_value = _profile_accessor->getInt(doc_id, _profile_field);
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
        if (_profile_field->type == DT_INT32)  {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt32();
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
        } else if (_profile_field->type == DT_INT64) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt64();
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
        } else if (_profile_field->type == DT_INT8)  {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt8();
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
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt16();
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
        int32_t attr_value_num = _iterator.getValueNum();
        if (_profile_field->type == DT_INT32) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt32();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_INT32)) { 
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
        } else if (_profile_field->type == DT_INT64) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt64();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_INT64)) { 
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
        } else if (_profile_field->type == DT_INT8) {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt8();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_INT8)) { 
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
        } else  {
            for(int32_t j = 0; j < attr_value_num; j++) {
                attr_value = _iterator.nextInt16();
                if (unlikely(attr_value == _profile_field->defaultEmpty.EV_INT16)) { 
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
