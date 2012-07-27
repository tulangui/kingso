#include "ProvCityAttrFilter.h"
#include <stdlib.h>

using namespace index_lib;

namespace search {

ProvCityAttrFilter::ProvCityAttrFilter(ProfileDocAccessor *profile_accessor, 
                                       ProvCityManager    *provcity_manager,
                                       const char         *field_name,
                                       bool                is_negative) 
    : AttrFilter(profile_accessor, field_name, is_negative), 
      _provcity_manager(provcity_manager)
{

}

ProvCityAttrFilter::~ProvCityAttrFilter()
{

}

void ProvCityAttrFilter::addFiltValue(const char* value)
{
    ProvCityID prov_id;
    if(_provcity_manager->seek(value, &prov_id) == 0)
    {
        _values.pushBack(prov_id);
        _value_count++;
    }
}

}
