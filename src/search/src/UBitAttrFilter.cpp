#include "UBitAttrFilter.h"
#include <stdlib.h>

using namespace index_lib;

namespace search {

UBitAttrFilter::UBitAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative)
{

}

UBitAttrFilter::~UBitAttrFilter()
{

}

void UBitAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack((uint64_t)(strtoll(value, NULL, 10)));
    _value_count++;
}

}
