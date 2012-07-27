#include "StringAttrFilter.h"

using namespace index_lib;

namespace search {

StringAttrFilter::StringAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative) 
    : AttrFilter(pProfileDoc, szFieldName, bNegative)
{

}

StringAttrFilter::~StringAttrFilter()
{
    
}

void StringAttrFilter::addFiltValue(const char* value)
{
    _values.pushBack(value);
    _value_count++;
}

}
