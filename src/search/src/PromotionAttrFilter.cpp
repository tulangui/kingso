#include "PromotionAttrFilter.h"
#include "SearchDef.h"
#include <stdlib.h>

using namespace index_lib;

namespace search {

PromotionAttrFilter::PromotionAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative)
{
    _is_promotion = true;
}

PromotionAttrFilter::~PromotionAttrFilter()
{

}

void PromotionAttrFilter::addFiltRange(const Range<char*> value)
{
    int64_t min = (value._min == NULL)?  MIN_INT64 : strtoll(value._min, NULL, 10);
    int64_t max = (value._max == NULL)?  MAX_INT64 : strtoll(value._max, NULL, 10);
    _ranges.pushBack(Range<int64_t>(min, max, value._is_Lequal, value._is_Requal));
    _range_count++;
}

void PromotionAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack(strtoll(value, NULL, 10));
    _value_count++;
}

}
