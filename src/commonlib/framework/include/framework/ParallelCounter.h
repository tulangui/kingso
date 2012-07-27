/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 189 $
 *
 * $LastChangedDate: 2011-03-25 15:24:17 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: ParallelCounter.h 189 2011-03-25 07:24:17Z taiyi.zjh $
 *
 * $Brief: record and control the parallel of service $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_PARALLEL_COUNTER_H_
#define _FRAMEWORK_PARALLEL_COUNTER_H_
#include "util/namespace.h"
#include "util/ThreadLock.h"
#include "framework/namespace.h"

FRAMEWORK_BEGIN;

class ParallelCounter {
    public:
        /**
         *@brief constructor
         *@param maxParallel max parallel number
         *@return
         */
        ParallelCounter(uint32_t maxParallel) 
            : _max(maxParallel), 
            _cur(0), 
            _total(0),
            _timeout(0), 
            _timeoutCount(0), 
            _seriateTimeoutCount(0),
            _errorCount(0), 
            _seriateErrorCount(0),
            _seriateTimeoutLimit(0), 
            _seriateErrorLimit(0),
            _startStatTime(0),
            _allStatTime(0),
            _allStatCount(0),
            _lastQps(0),
            _lastLatency(0) {}
        ~ParallelCounter() {
        }
    public:
        /**
         *@brief 增加当前服务数量计数
         *@return true,成功 false,失败
         */
        bool inc();

        /**
         *@brief 减少服务数量计数
         *@param cost 减少服务数量
         *@param error 是否出错
         *@return true,成功 false,失败
         */
        bool dec(uint64_t cost, bool error = false);

        /**
         *@brief 统计最近一分钟的qps和latency 
         *@param start 开始时间
         *@param end   结束时间
         *@return true,成功 false,失败
         */
        bool stat(uint64_t start, uint64_t end);
    public:
        /**
         *@brief 设置超时时间
         *@param t 超时时间
         */
        void setTimeoutLimit(uint64_t t) { _timeout = t; }
        /**
         *@brief 设置连续超时的最大次数，超过限制将触发异常恢复
         *@param n 连续超时的最大次数
         */
        void setSeriateTimeoutCountLimit(uint32_t n) { 
            _seriateTimeoutLimit = n; 
        }
        /**
         *@brief 设置连续出错的最大次数，超过限制将触发异常恢复
         *@param n 连续出错的最大次数
         */
        void setSeriateErrorCountLimit(uint32_t n) {
            _seriateErrorLimit = n;
        }
        /**
         *@brief 获取超时计数
         *@return 返回超时计数
         */
        uint32_t getTimeoutCount() const { return _timeoutCount; }
        /**
         *@brief 获取连续超时计数
         *@return 返回连续超时计数
         */
        uint32_t getSeriateTimeoutCount() const { 
            return _seriateTimeoutCount; 
        }
        /**
         *@brief 获取出错计数
         *@return 返回出错计数
         */
        uint32_t getErrorCount() const { return _errorCount; }
        /**
         *@brief 获取连续出错计数
         *@return 返回连续出错计数
         */
        uint32_t getSeriateErrorCount() const { 
            return _seriateErrorCount;
        }
        /**
         *@brief 获取当前服务数量计数
         *@return 当前服务数量计数
         */
        uint32_t current() const { return _cur; }
        /**
         *@brief 获取总服务数量计数
         *@return 总服务数量计数
         */
        uint64_t getTotalCount() const { return _total; }
    public:
        /**
         *@brief 判断是否处于异常状态
         *@return true,正常状态 false,异常状态
         */
        bool isOK() const {
            return (_seriateTimeoutLimit==0 || 
                    _seriateTimeoutCount < _seriateTimeoutLimit) &&
                   (_seriateErrorLimit == 0 ||
                    _seriateErrorCount < _seriateErrorLimit);
        }
    public:
        uint64_t _lastServiceTime;
        uint64_t _startStatTime;    // 开始统计时间
        uint64_t _allStatTime;      // 总耗时
        uint64_t _allStatCount;     // 总请求数量
        uint32_t _lastQps;          // 最近的qps
        uint32_t _lastLatency;      // 最近的latency

    private:
        uint32_t _max;
        uint32_t _cur;
        uint64_t _total;
        uint64_t _timeout;
        uint32_t _timeoutCount;
        uint32_t _seriateTimeoutCount;
        uint32_t _errorCount;
        uint32_t _seriateErrorCount;
        uint32_t _seriateTimeoutLimit;
        uint32_t _seriateErrorLimit;
        UTIL::Mutex _lock;
        UTIL::Mutex _statLock;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_PARALLEL_COUNTER_H_

