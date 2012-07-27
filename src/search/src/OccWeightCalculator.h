/*********************************************************************
 * $Author: shituo.lyc $
 *
 * $LastChangedBy: shituo.lyc $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: OccWeightCalculator.h 2577 2011-03-09 01:50:55Z shituo.lyc $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef _TOKENWEIGHTCALCULATOR_H_
#define _TOKENWEIGHTCALCULATOR_H_

#include <stdint.h>

#include "commdef/commdef.h"
#include "util/Vector.h"
#include "util/common.h"
#include "util/Log.h"
#include "CfgManager.h"

namespace search{



class OccWeightCalculator
{ 
public:
    OccWeightCalculator(Package *p_package, char *sub_field);
    ~OccWeightCalculator();

    inline int32_t getScore(const uint8_t *p_occ_list, 
                             const int32_t occ_num) const;

    inline bool     isSubFieldSearch();

private:
    UTIL::Vector<OccInterval> _separators;  //occ的范围
    UTIL::Vector<uint32_t>    _scores;      //指定occ范围的权重
    int32_t                   _size;
    bool                      _subfield;    //是否是sub_field查询
};

bool OccWeightCalculator::isSubFieldSearch()
{
    return _subfield;
}

int32_t OccWeightCalculator::getScore(const uint8_t *p_occ_list, 
                                      const int32_t  occ_num) const
{
    if(unlikely(p_occ_list == NULL || occ_num <= 0)) {
        return 0;
    }
    if(_size == 0) {
        return 0;
    }
    int32_t  i     = 0;
    int32_t  j     = 0;
    bool     is_in = false;
    uint32_t ret   = 0;
    //如果参数中的occ_list的occ在_separators中的范围之内的话，
    //累加_scores中的权值。
    do {
        if(p_occ_list[i] < _separators[j].min) {
            i++;
        } else if(p_occ_list[i] <= _separators[j].max) {
            is_in = true;
            i++;
        } else {
            ret += is_in ? _scores[j] : 0;
            is_in = false;
            j++;
        }
    } while(i < occ_num && j < _size);
    ret += is_in ? _scores[j] : 0;
    
    return is_in ? ret : -1;
}

}

#endif //SRC_TOKENWEIGHTCALCULATOR_H_
