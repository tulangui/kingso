/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 510 $
 *
 * $LastChangedDate: 2011-04-08 19:13:35 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: ServerWatchPoint.h 510 2011-04-08 11:13:35Z santai.ww $
 *
 * $Breif: the monitor of server$
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_SERVER_WATCH_POINT_H_
#define _FRAMEWORK_SERVER_WATCH_POINT_H_
#include "util/common.h"
#include "clustermap/CMWatchPoint.h"
#include "framework/ParallelCounter.h"

FRAMEWORK_BEGIN;

class ServerWatchPoint : public clustermap::ServerWatchPoint {
    public:
        ServerWatchPoint(const ParallelCounter & pc) : _pc(pc) { }
        ~ServerWatchPoint() { }
    public:
        /**
         *@brief 判断服务是否可用
         *@return true:可用, flase:不可用
         **/
        virtual bool isOK() { return _pc.isOK(); }
        /**
         *@brief 判断服务是否可用
         *@return true:可用, flase:不可用
         **/
        virtual bool getCurrentResource() {
            _qps = _pc._lastQps;
            _latency = _pc._lastLatency;
            return true;
        }
        /**
         *@brief 输出信息
         *@return 输出信息
         **/
        virtual std::string printMessage() {
            return "ServerWatchPoint ParallelCounter";
        }
    private:
        const ParallelCounter &_pc;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_SERVER_WATCH_POINT_H_

