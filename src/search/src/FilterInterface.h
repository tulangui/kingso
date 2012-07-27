/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 430 $
 *
 * $LastChangedDate: 2011-04-07 18:11:49 +0800 (四, 07  4月 2011) $
 *
 * $Id: FilterInterface.h 430 2011-04-07 10:11:49Z boluo.ybb $
 *
 * $Brief: search过程中需要处理的插件基类定义，包含filter、static、sort等 $
 ********************************************************************
 */

#ifndef _SE_OBJ_H_
#define _SE_OBJ_H_

#include <stdint.h>

namespace search {
//虚基类，提供过滤的接口
class FilterInterface
{
public:
    
    FilterInterface();

    virtual ~FilterInterface();

    /*
     * 插件处理函数
     * @param [in] doc_id docid值
     * @return true 成功 false 失败
     */
    virtual bool process(uint32_t doc_id) = 0;

};

}

#endif
