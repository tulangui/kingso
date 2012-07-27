#include "DoubleAttrFilter.h"
#include <stdlib.h>
#include <values.h>

using namespace index_lib;

namespace search {

DoubleAttrFilter::DoubleAttrFilter(ProfileDocAccessor *profile_accessor,
                                   const char         *field_name, 
                                   bool                is_negative) 
    : AttrFilter(profile_accessor, field_name, is_negative)
{

}

DoubleAttrFilter::~DoubleAttrFilter()
{

}

void DoubleAttrFilter::addFiltRange(const Range<char*> value)
{
    double min = (value._min == NULL)?  -MAXDOUBLE : strtod(value._min, NULL);
    double max = (value._max == NULL)?  MAXDOUBLE : strtod(value._max, NULL);
    _ranges.pushBack(Range<double>(min, max, value._is_Lequal, value._is_Requal));
    _range_count++;
}

void DoubleAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack(strtod(value, NULL));
    _value_count++;
}


}
