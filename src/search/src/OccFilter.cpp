#include "util/StringUtility.h"

#include "OccFilter.h"

namespace search{

OccFilter::OccFilter(Package *p_package, char *sub_field)
{
    _size = 0; 
    _separators.clear();
    if(!p_package) {
        return;
    }
    UTIL::Vector<char *> sub_vec;
    if(sub_field && sub_field[0] != '\0') {
        split(sub_field, "|", sub_vec); 
    }
    char *field_name = NULL;
    //配置文件中指定了package的sub_field和occ范围，
    //如果query指定的sub_field和配置文件中的匹配上,
    //将配置文件中的occ范围存储到_separators中。用于过滤
    for(int i=0; i<p_package->size; i++) {
        field_name = p_package->elements[i]->field_name;
        for(uint32_t j=0; j<sub_vec.size(); j++) {
            if(sub_vec[j] && strcmp(sub_vec[j], field_name) == 0) {
                int32_t min    = p_package->elements[i]->occ_min;
                int32_t max    = p_package->elements[i]->occ_max;
                if(min > max) {
                    _separators.clear();
                    _size = 0; 
                    return;
                } else {
                    OccInterval tmp = {min, max};
                    _separators.pushBack(tmp);
                    _size++;
                }
            }
        }
    }
}

OccFilter::~OccFilter()
{}

}
