/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: LatLngAttrFilter.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: LatLng经纬度类型字段最短距离过滤器 $
 ********************************************************************/
#ifndef _LATLNG_ATTRFILTER_H_
#define _LATLNG_ATTRFILTER_H_

#include "AttrFilter.h"
#include "util/Vector.h"
#include "index_lib/LatLngProcessor.h"

using namespace index_lib;
using namespace util;

namespace search {

class LatLngAttrFilter : public AttrFilter
{ 
public:
    LatLngAttrFilter(ProfileDocAccessor *profile_accessor,   //profile读写器
                     const char         *field_name,         //profile 字段名
                     bool                is_negative);       //是(非)  过滤

    virtual ~LatLngAttrFilter();

    /**
     * 添加无线客户端的经纬度信息
     * @param [in]  lat  纬度
     * @param [in]  lng  经度
     *
     */
    virtual bool setLatLng(float lat, float lng);

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
    Vector<float>          _values;             /* 需要过滤的值 */
    Vector< Range<float> > _ranges;             /* 值范围 */
    LatLngProcessor        _latlngReader;       /* 经纬度距离计算接口对象 */
    float                  _client_lat;         /* 客户端经度位置 */
    float                  _client_lng;         /* 客户端纬度位置 */
};

bool LatLngAttrFilter::process(uint32_t doc_id)
{
    bool is_found = false;
    double minDist = _latlngReader.getMinDistance(_client_lat, _client_lng, doc_id);

    if (minDist == INVALID_DOUBLE) {
        return false;
    }

    for(uint32_t i = 0; i < _value_count; ++i) {
        if (is_found = ((float)minDist == _values[i])) {
            break;
        }
    }

    if (!is_found && _range_count > 0) {
        for(uint32_t i = 0; i < _range_count; ++i) {
            if (is_found = _ranges[i].in((float)minDist)) {
                break;
            }
        }
    }

    if (_is_negative) {
        is_found = !is_found;
    }
    return is_found;
}

}

#endif //SRC_LATLNGATTRFILTER_H_
