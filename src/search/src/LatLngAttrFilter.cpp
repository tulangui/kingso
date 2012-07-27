#include "LatLngAttrFilter.h"
#include <stdlib.h>
#include <values.h>

#define LATLNG_PARAM_DELIM ':'

using namespace index_lib;

namespace search {

LatLngAttrFilter::LatLngAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative),
      _latlngReader(_profile_field)
{
    _client_lat = 0.0f;
    _client_lng = 0.0f;
}

LatLngAttrFilter::~LatLngAttrFilter()
{
}

void LatLngAttrFilter::addFiltRange(const Range<char*> value)
{
    float min = (value._min == NULL)?  -MAXFLOAT : strtof(value._min, NULL);
    float max = (value._max == NULL)?  MAXFLOAT : strtof(value._max, NULL);
    _ranges.pushBack(Range<float>(min, max, value._is_Lequal, value._is_Requal));
    _range_count++;
}

void LatLngAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack(strtof(value, NULL));
    _value_count++;
}

bool LatLngAttrFilter::setLatLng(float lat, float lng)
{
    // 检查经纬度字段类型
    if (unlikely( _profile_field->type != DT_UINT64 ||
                  _profile_field->multiValNum == 1 ))
    {
        return false;
    }

    _client_lat = lat;
    _client_lng = lng;
    return true;
}

}
