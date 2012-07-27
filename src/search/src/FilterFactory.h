/*********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 491 $
 *
 * $LastChangedDate: 2011-04-08 16:06:30 +0800 (五, 08  4月 2011) $
 *
 * $Id: FilterFactory.h 491 2011-04-08 08:06:30Z boluo.ybb $
 *
 * $Brief: search object 工厂 $
 ********************************************************************/

#ifndef _SEOBJ_FACTORY_H_
#define _SEOBJ_FACTORY_H_

#include "FilterInterface.h"
#include "index_lib/ProfileDocAccessor.h"
#include "framework/Context.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProvCityManager.h"

using namespace index_lib;
using namespace framework;

namespace search {

typedef enum FilterInterfaceType
{
    DT_DELMAP,
    DT_WANGWANG,
    DT_BITMAP,
    DT_PROFILE
} FilterInterfaceType;


class FilterFactory
{ 
public:
    FilterFactory();

    ~FilterFactory();

    /**
     * 初始化函数
     * @param [in]  profile_accessor profile读写器
     * @param [in]  provcity_manager 行政区编码管理器 
     * @param [in]  p_delete_map     deletemap读取器
     * @param [in]  p_online_reader  旺旺online读取器
     * @return true 成功，false 失败
     */
    bool init(ProfileDocAccessor      *profile_accessor, 
              ProvCityManager         *provcity_manager, 
              DocIdManager::DeleteMap *p_delete_map); 

    /**
     * FilterInterface对象创建函数，从mempool中分配，无须释放
     * @param [in]  p_context 框架提供的资源，mempool等 
     * @param [in]  filter_type 过滤类型
     * @param [out] p_filter_interface对象, NULL创建失败
     * @param [in]  field_name profile字段名
     * @param [in]  is_negative 是（非）过滤
     * @return 0 正常 非0 错误码
     */
    int create(Context            *p_context, 
               FilterInterfaceType filter_type, 
               FilterInterface*   &p_filter_interface, 
               const char         *field_name = NULL, 
               bool                is_negative = false);

private:
    ProfileDocAccessor      *_profile_accessor;      /* ProfileManager管理器，用于创建profile filter */
    ProvCityManager         *_provcity_manager;      /* 行政区编码管理器 */
    DocIdManager::DeleteMap *_p_detele_map;          /*deletemap管理器，用于创建deletemap filter */
};

}


#endif //FILTER_FILTERFACTORY_H_
