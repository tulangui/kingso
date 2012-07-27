#include "util/StringUtility.h"

#include "OccWeightCalculator.h"

namespace search{

OccWeightCalculator::OccWeightCalculator(Package *p_package,
                                         char    *sub_field)
{
    _size = 0;
    _subfield = false;
    _separators.clear();
    _scores.clear();

    UTIL::Vector<char *> sub_vec;
    if(sub_field && sub_field[0] != '\0') {
        split(sub_field, "|", sub_vec); 
    }
    char *field_name = NULL;
    //配置文件中指定了package的sub_field和occ范围，
    //如果query指定的sub_field和配置文件中的匹配上,
    //将配置文件中的occ范围存储到_separators中。用于过滤
    //将配置文件中weight存储到_scores中，用于计算权值
    for(int i=0; i<p_package->size; i++) {
        field_name = p_package->elements[i]->field_name;
        int32_t min    = p_package->elements[i]->occ_min;
        int32_t max    = p_package->elements[i]->occ_max;
        int32_t weight = p_package->elements[i]->weight;
        if(sub_field[0] == '\0') {
            OccInterval tmp = {min, max};
            _separators.pushBack(tmp);
            _scores.pushBack(weight);
            _size++;
        } else {
            for(uint32_t j=0; j<sub_vec.size(); j++) {
                if(sub_vec[j] && strcmp(sub_vec[j], field_name) == 0) {
                    if(min > max) {
                        _separators.clear();
                        _scores.clear();
                        _size = 0;
                        return;
                    } else {
                        OccInterval tmp = {min, max};
                        _separators.pushBack(tmp);
                        _scores.pushBack(weight);
                        _size++;
                    }
                }
            }
        }
    }
    //如果配置文件中的sub_field的个数和query中的sub_field中的个数不相等，
    //说明需要occ的信息，进行过滤。所以不能使用bitmap。因为bitmap没有occ信息
    if(_separators.size() != p_package->size) {
        _subfield = true;
    }
}

OccWeightCalculator::~OccWeightCalculator()
{}

}
