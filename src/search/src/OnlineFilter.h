/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 393 $
 *
 * $LastChangedDate: 2011-04-07 11:13:23 +0800 (Thu, 07 Apr 2011) $
 *
 * $Id: OnlineFilter.h 393 2011-04-07 03:13:23Z boluo.ybb $
 *
 * $Brief: 旺旺online过滤器 $
 ********************************************************************
 */

#ifndef _ONLINE_FILTER_H_
#define _ONLINE_FILTER_H_

#include "FilterInterface.h"
#include "online_reader/OnlineReader.h"

using namespace search;
using namespace online_reader;

namespace search {
/**
  **  用户在线状态
  **/
/*  enum WWOnlineStatus
  {
    WWOS_ONLINE = 0,    // 0: 在线
    WWOS_OFFLINE = 1,    // 1：  不在线
    WWOS_MOBILE_ONLINE = 2     // 2: 手机在线
  };
  */

class OnlineFilter : public FilterInterface
{
public:
    /**
     * 构造函数
     * @param [in] p_online_reader 旺旺online读取器
     * @param [in[ is_negative 是（非）过滤
     */
    OnlineFilter(OnlineReader *p_online_reader, bool is_negative)
                : _p_online_reader(p_online_reader), _is_negative(is_negative)
    {
   
    }

    virtual ~OnlineFilter(){};

    /**
     * 旺旺过滤处理函数
     * @param [in] doc_id docid值
     * @return true 成功 false 失败
     */
    inline bool process(uint32_t doc_id);

private:
    OnlineReader *_p_online_reader;          // 旺旺online读取器
    bool          _is_negative;              // 是非
};

bool OnlineFilter::process(uint32_t doc_id)
{
    bool          is_found = false;
    unsigned char ww_state = 0;                             // 旺旺在线状态
    if (false == _p_online_reader->query(doc_id, ww_state)){
        return true;
    }
    if (online::CUserStatus::OFF_LINE != ww_state){
        is_found = true;
    }
    if(_is_negative) {
        is_found = !is_found;
    }

    return is_found;
}

}
#endif
