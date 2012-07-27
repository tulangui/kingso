#include "FloatAttrFilter.h"
#include <stdlib.h>
#include <values.h>

using namespace index_lib;

namespace search {

FloatAttrFilter::FloatAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative)
{

}

FloatAttrFilter::~FloatAttrFilter()
{

}

void FloatAttrFilter::addFiltRange(const Range<char*> value)
{
    float min = (value._min == NULL)?  -MAXFLOAT : strtof(value._min, NULL);
    float max = (value._max == NULL)?  MAXFLOAT : strtof(value._max, NULL);
    _ranges.pushBack(Range<float>(min, max, value._is_Lequal, value._is_Requal));
    _range_count++;
}

void FloatAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack(strtof(value, NULL));
    _value_count++;
}

}
