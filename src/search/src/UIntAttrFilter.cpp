#include "UIntAttrFilter.h"
#include "SearchDef.h"
#include <stdlib.h>

using namespace index_lib;

namespace search {

UIntAttrFilter::UIntAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative)
{

}

UIntAttrFilter::~UIntAttrFilter()
{

}

void UIntAttrFilter::addFiltRange(const Range<char*> value)
{
    uint64_t min = (value._min == NULL)?  MIN_UINT64 : (uint64_t)strtoll(value._min, NULL, 10);
    uint64_t max = (value._max == NULL)?  MAX_UINT64 : (uint64_t)strtoll(value._max, NULL, 10);
    _ranges.pushBack(Range<uint64_t>(min, max, value._is_Lequal, value._is_Requal));
    _range_count++;
}

void UIntAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack((uint64_t)(strtoll(value, NULL, 10)));
    _value_count++;
}


}
