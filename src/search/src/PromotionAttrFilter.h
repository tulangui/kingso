/** \file
 ********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 68 $
 *
 * $LastChangedDate: 2011-03-22 10:16:32 +0800 (Tue, 22 Mar 2011) $
 *
 * $Id: IntAttrFilter.h 68 2011-03-22 02:16:32Z boluo.ybb $
 *
 * $Brief: int数值profile类型过滤器 $
 ********************************************************************
 */

#ifndef _PROMOTION_ATTR_FILTER_H
#define _PROMOTION_ATTR_FILTER_H

#include "AttrFilter.h"
#include "util/Vector.h"

using namespace index_lib;
using namespace util;

namespace search {

class PromotionAttrFilter : public AttrFilter
{
public:
    /**
     * 构造函数
     * @param pProfileDoc profile读写器
     * @param szFieldName profile字段名
     * @param bNegative 是（非）过滤
     */
    PromotionAttrFilter(ProfileDocAccessor *pProfileDoc, const char *szFieldName, bool bNegative);

    /**
     * 析构函数
     */
    virtual ~PromotionAttrFilter();

    /**
     * 添加需要过滤的值
     * @param value 过滤值
     */
    virtual void addFiltValue(const char *value);

    /**
     * 添加范围过滤
     * @param value 范围结构值
     */
    virtual void addFiltRange(const Range<char*> value);

    /**
     * 过滤处理函数
     * @param nDocId docid值
     * @return true（不过滤），false（过滤）
     */
    inline virtual bool process(uint32_t nDocId);

protected:
    Vector<int64_t> _values;                                    /* 需要过滤的值 */
    Vector< Range<int64_t> > _ranges;                             /* 值范围 */

};

bool PromotionAttrFilter::process(uint32_t nDocId)
{
    int16_t nDiscount = getDiscount(nDocId);
    bool bFound = false;
    do {
        for(uint32_t i = 0; i < _value_count; i++)
        {
            if(bFound = (nDiscount == _values[i]))
                break;
        }
        if(bFound)
            break;
        if(_range_count > 0)
        {
            for(uint32_t i = 0; i < _range_count; i++)
            {
                if(bFound = _ranges[i].in(nDiscount))
                    break;
            }
        }
        if(bFound)
            break;
    } while(0);
    if(_is_negative)
        bFound = !bFound;

    return bFound;
}

}

#endif
