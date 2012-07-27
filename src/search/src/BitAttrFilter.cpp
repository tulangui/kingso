#include "BitAttrFilter.h"
#include <stdlib.h>

using namespace index_lib;

namespace search {

BitAttrFilter::BitAttrFilter(ProfileDocAccessor *profile_accessor, 
                             const char         *field_name, 
                             bool                is_negative) 
    : AttrFilter(profile_accessor, field_name, is_negative)
{

}

BitAttrFilter::~BitAttrFilter()
{

}

void BitAttrFilter::addFiltValue(const char *value)
{
    _values.pushBack(strtoll(value, NULL, 10));
    _value_count++;
}

}
