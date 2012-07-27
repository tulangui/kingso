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

#ifndef _OCC_FILTER_H
#define _OCC_FILTER_H 

#include <stdint.h>

#include "commdef/commdef.h"
#include "util/Vector.h"
#include "util/common.h"
#include "util/Log.h"
#include "CfgManager.h"

namespace search{

class OccFilter
{ 
public:
    OccFilter(Package *p_package, char *sub_field);
    ~OccFilter();

    inline bool doFilter(const uint8_t *p_occ_list, 
                         const int32_t  occ_num) const;

private:
    //根据配置和query形成occ的范围，如果在范围之内不过滤，否则过滤
    UTIL::Vector<OccInterval> _separators;
    int32_t                   _size;
};


bool OccFilter::doFilter(const uint8_t *p_occ_list, 
                         const int32_t  occ_num) const
{
    if(unlikely(p_occ_list == NULL || occ_num <= 0)) {
        return false;
    }
    if(_size == 0) {
        return false;
    }

    int32_t  i     = 0;
    int32_t  j     = 0;
    //传进来的occ_list的值不在_separators的范围之内的话,就过滤掉
    do { 
        if(p_occ_list[i] < _separators[j].min) {
            i++;
        } else if(p_occ_list[i] <= _separators[j].max) {
            return false; 
        } else {
            j++;
        }
    } while(i < occ_num && j < _size);
    
    return true; 
}

}

#endif //SRC_TOKENWEIGHTCALCULATOR_H_
