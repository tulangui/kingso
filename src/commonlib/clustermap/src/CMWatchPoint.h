/** \file
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 11573 $
 *
 * $LastChangedDate: 2012-04-25 15:55:14 +0800 (Wed, 25 Apr 2012) $
 *
 * $Id: CMWatchPoint.h 11573 2012-04-25 07:55:14Z pujian $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_WATCH_POINT_H_
#define _CM_WATCH_POINT_H_

#include "CMClientInterface.h"

namespace clustermap {

enum WatchPointType {
    COMMON_WATCH = 0,
    SERVER_WATCH = 1
};

class WatchPoint {
    public:
        WatchPoint() { type = COMMON_WATCH; }
        virtual ~WatchPoint() { }
    public:
        virtual bool isOK() = 0;
        virtual std::string printMessage() = 0;
    public:
        WatchPoint * next;
        WatchPointType type;
};

class ServerWatchPoint : public WatchPoint {
    public:
        ServerWatchPoint() : _qps(0), _latency(0) {type = SERVER_WATCH;}
        virtual ~ServerWatchPoint() {}
    public:
        virtual bool isOK() = 0;
        virtual bool getCurrentResource() = 0;
        virtual std::string printMessage() = 0;
    public:
        uint32_t _qps;
        uint32_t _latency;
};

class ResourceWatchPoint : public WatchPoint {
    public:
        ResourceWatchPoint(uint32_t limit, uint32_t n) 
            : _limit(limit),
            _overload_count_limit((n == 0 ? 1 : n)), 
            _overload_count(0) {
            }
        ResourceWatchPoint() { }
        virtual ~ResourceWatchPoint() { }
    public:
        virtual bool isOK();
        virtual uint32_t getCurrentResource() = 0;
    protected:
        uint32_t _limit;
        uint32_t _overload_count_limit;
        uint32_t _overload_count;
};

typedef struct _cpu_info
{
    uint16_t cpu_busy;
    uint16_t io_wait;
}CPUWatchInfo;

union CPUWatchResult
{
    CPUWatchInfo sVal;
    uint32_t     value;
};

class CPUWatchPoint : public ResourceWatchPoint {
    public:
        CPUWatchPoint(uint32_t limit, uint32_t n) 
            : ResourceWatchPoint(limit, n) { 
            }
        CPUWatchPoint() { }
        virtual ~CPUWatchPoint() { }
        virtual uint32_t getCurrentResource(); //get cur cpu rate
        virtual std::string printMessage() {
            return "CPUWatchPoint";}
};

class SysloadWatchPoint : public ResourceWatchPoint {
    public:
        SysloadWatchPoint(uint32_t limit, uint32_t n = 1) 
            : ResourceWatchPoint(limit, n) { 
            }
        SysloadWatchPoint() { }
        virtual ~SysloadWatchPoint() { }
        virtual uint32_t getCurrentResource(); //get cur sys load
        virtual std::string printMessage() {
            return "SysloadWatchPoint";}
};

}
#endif
