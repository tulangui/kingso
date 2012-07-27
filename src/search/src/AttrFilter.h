/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 494 $
 *
 * $LastChangedDate: 2011-04-08 16:08:21 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: AttrFilter.h 494 2011-04-08 08:08:21Z boluo.ybb $
 *
 * $Brief: profile类型过滤器 $
 ********************************************************************
 */

#ifndef _ATTR_FILTER_H_
#define _ATTR_FILTER_H_

#include "FilterInterface.h"
#include "SearchDef.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/ProfileMultiValueIterator.h"

using namespace search;

namespace search {

template <typename T>
inline bool compareLR(T min, T max, T value)
{
    return ((value > min) && (value < max));
}

template <typename T>
inline bool compareLeR(T min, T max, T value)
{
    return ((value >= min) && (value < max));
}

template <typename T>
inline bool compareLRe(T min, T max, T value)
{
    return ((value > min) && (value <= max));
}

template <typename T>
inline bool compareLeRe(T min, T max, T value)
{
    return ((value >= min) && (value <= max));
}

/* 范围类型存储结构类 */
template <typename T>
class Range
{
    typedef bool(*FuncPtr)(T min, T max, T value);
public:
    Range() : _min(0), _max(0), _is_Lequal(false), _is_Requal(false)
    {
    }

    Range(T min, T max, bool is_Lequal, bool is_Requal) : _min(min), 
                                                          _max(max), 
                                                          _is_Lequal(is_Lequal), 
                                                          _is_Requal(is_Requal)
    {
        if(is_Lequal && is_Requal) {
            _cmp = compareLeRe<T>;
        }
        else if(is_Lequal && !is_Requal) {
            _cmp = compareLeR<T>;
        }
        else if(!is_Lequal && _is_Requal) {
            _cmp = compareLRe<T>;
        }
        else {
            _cmp = compareLR<T>;
        }
    }

    bool in(T value)
    {
        return (*_cmp)(_min, _max, value);
    }

public:
    T       _min;           //左边界
    T       _max;           //右边界
    bool    _is_Lequal;     //左边界是否是闭区间
    bool    _is_Requal;     //右边界是否是闭区间
    FuncPtr _cmp;           //函数指针，判断值是否在区间内
};

class AttrFilter : public FilterInterface
{
public:
    
    AttrFilter(index_lib::ProfileDocAccessor *profile_accessor, //profile 读写器
               const char                    *field_name,       //profile 字段名
               bool                           is_negative);     //是(非)  过滤

    virtual ~AttrFilter();

    /**
     * 添加需要过滤的值
     * @param [in] value 过滤值
     */
    virtual void addFiltValue(const char *value);

    /**
     * 添加范围过滤
     * @param [in] value 范围结构值
     */
    virtual void addFiltRange(const Range<char*> value);

    /**
     * 设置折扣字段
     * @param [in] discount_field 折扣字段名
     * @param [in] discount_time 折扣持续时间
     */
    virtual void setDiscount(const char *discount_field, time_t discount_time);

    /**
     * 获取对应docid的折扣基数
     * @param [in] docid
     * @return 折扣基数
     */
    virtual int16_t getDiscount(uint32_t doc_id);

    virtual int16_t getZKGroup(uint32_t doc_id);

    /**
     * 过滤处理函数
     * @param [in]  docid值
     * @return true（不过滤），false（过滤）
     */
    virtual bool process(uint32_t doc_id) = 0;

    bool isPromotionType();

    /**
     * 判断目标字段是否为LATLNG类型
     */
    bool isLatLngType();


protected:
    uint32_t                             _value_count;            /* 需要过滤的值个数 */
    uint32_t                             _range_count;            /* 值范围个数 */
    index_lib::ProfileDocAccessor       *_profile_accessor;       /* profile管理器 */
    const index_lib::ProfileField       *_profile_field;          /* profile字段类型 */
    const index_lib::ProfileField       *_zk_rate_pf;              /* 折扣字段 */
    const index_lib::ProfileField       *_zk_time_pf;              /* 折扣字段 */
    time_t                               _discount_time;          /* et或当前时间，用于匹配打折时间区间 */
    bool                                 _is_negative;            /* 非逻辑过滤 */
    index_lib::ProfileMultiValueIterator _iterator;           /* profile多值字段迭代器对象 */
    index_lib::ProfileMultiValueIterator _zk_time_iter;           /* profile多值字段迭代器对象 */
    index_lib::ProfileMultiValueIterator _zk_rate_iter;           /* profile多值字段迭代器对象 */

    bool _is_promotion;
};

}

#endif
