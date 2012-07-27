/** \file
 ********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 8147 $
 *
 * $LastChangedDate: 2011-11-24 19:17:04 +0800 (Thu, 24 Nov 2011) $
 *
 * $Id: ProvCityAttrFilter.h 8147 2011-11-24 11:17:04Z xiaoleng.nj $
 *
 * $Brief: 行政区编码profile类型过滤器 $
 ********************************************************************
 */

#ifndef _PROVCITY_ATTR_FILTER_H_
#define _PROVCITY_ATTR_FILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"
#include "index_lib/ProvCityManager.h"

using namespace index_lib;
using namespace util;

namespace search {

class ProvCityAttrFilter : public AttrFilter
{
public:

    ProvCityAttrFilter(ProfileDocAccessor *profile_accessor, //profile读写器
                       ProvCityManager    *provcity_manager, //行政区域字段管理器 
                       const char         *field_name,       //字段名 
                       bool                is_negative);     //是（非）过滤

    virtual ~ProvCityAttrFilter();
 
    /**
     * 添加需要过滤的值
     * @param [in] attr_value 过滤值
     */
    virtual void addFiltValue(const char *attr_value);

    /**
     * bit过滤处理函数，通过位与操作，判定是否过滤
     * @param [in] doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t doc_id);

protected:
    util::Vector<ProvCityID> _values;                  /* 需要过滤的值 */
    ProvCityManager         *_provcity_manager;        /* 行政区编码管理器 */

};

bool ProvCityAttrFilter::process(uint32_t doc_id)
{
    uint16_t attr_value = 0;
    ProvCityID prov_id;
    bool is_found = false;
    int32_t multi_num = _profile_accessor->getFieldMultiValNum(_profile_field);
    if(likely(multi_num == 1)) {    //单值字段
        attr_value = _profile_accessor->getUInt16(doc_id, _profile_field);
        prov_id.pc_id = attr_value;
        for(uint32_t i = 0; i < _value_count; i++) {
            if(_values[i].pc_id == 0) {
                continue;
            }
            if(_values[i].pc_stu.city_id != 0 
                    && _values[i].pc_stu.city_id != prov_id.pc_stu.city_id) {
                continue;
            }
            if(_values[i].pc_stu.prov_id != 0 
                    && _values[i].pc_stu.prov_id != prov_id.pc_stu.prov_id) {
                continue;
            }
            if(_values[i].pc_stu.area_id != 0 
                    && _values[i].pc_stu.area_id != prov_id.pc_stu.area_id) {
                continue;
            }
            is_found = true;
            break;
        }
    } else if (multi_num == 0) {    //变长字段
        _profile_accessor->getRepeatedValue(doc_id, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        for(int32_t j = 0; j < attr_value_num; j++) {
            attr_value = _iterator.nextUInt16();
            prov_id.pc_id = attr_value;
            for(uint32_t i = 0; i < _value_count; i++) {
                if(_values[i].pc_id == 0) {
                    continue;
                }
                if(_values[i].pc_stu.city_id != 0 
                        && _values[i].pc_stu.city_id != prov_id.pc_stu.city_id) {
                    continue;
                }
                if(_values[i].pc_stu.prov_id != 0 
                        && _values[i].pc_stu.prov_id != prov_id.pc_stu.prov_id) {
                    continue;
                }
                if(_values[i].pc_stu.area_id != 0 
                        && _values[i].pc_stu.area_id != prov_id.pc_stu.area_id) {
                    continue;
                }
                is_found = true;
                break;
            }
            if(is_found) {
                break;
            }
        }
    } else {                        //定长多值
        _profile_accessor->getRepeatedValue(doc_id, _profile_field, _iterator);
        int32_t attr_value_num = _iterator.getValueNum();
        for(int32_t j = 0; j < attr_value_num; j++) {
            attr_value = _iterator.nextUInt16();
            if (unlikely(attr_value == _profile_field->defaultEmpty.EV_UINT16)) { 
                break;
            }
            prov_id.pc_id = attr_value;
            for(uint32_t i = 0; i < _value_count; i++) {
                if(_values[i].pc_id == 0) {
                    continue;
                }
                if(_values[i].pc_stu.city_id != 0 
                        && _values[i].pc_stu.city_id != prov_id.pc_stu.city_id) {
                    continue;
                }
                if(_values[i].pc_stu.prov_id != 0 
                        && _values[i].pc_stu.prov_id != prov_id.pc_stu.prov_id) {
                    continue;
                }
                if(_values[i].pc_stu.area_id != 0 
                        && _values[i].pc_stu.area_id != prov_id.pc_stu.area_id) {
                    continue;
                }
                is_found = true;
                break;
            }
            if(is_found) {
                break;
            }
        }
    }

    if(_is_negative)
        is_found = !is_found;

    return is_found;
}

}

#endif
