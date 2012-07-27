/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 274 $
 *
 * $LastChangedDate: 2011-04-01 15:28:30 +0800 (Fri, 01 Apr 2011) $
 *
 * $Id: Session.h 274 2011-04-01 07:28:30Z taiyi.zjh $
 *
 * $Brief: stored session information $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_SESSION_H_
#define _FRAMEWORK_SESSION_H_
#include "util/namespace.h"
#include "util/common.h"
#include "framework/Query.h"
#include "framework/ParallelCounter.h"
#include "util/Log.h"
#include "util/Timer.h"

namespace anet {
    class Connection;
}

FRAMEWORK_BEGIN;
class Service;
class Request;

//session的处理阶段
typedef enum {
    //新到的会话请求
    ss_arrive,
    //phase1请求已发出
    ss_phase1_send,
    //获取UNIQII信息的请求已发出
    ss_uniqinfo_send,
    //phase1应答返回
    ss_phase1_return,
    //phase1超时
    ss_phase1_timeout,
    //phase2请求已发出
    ss_phase2_send,
    //phase2应答返回
    ss_phase2_return,
    //phase2超时
    ss_phase2_timeout,
    //以下为预留位
    ss_phase3_send,
    ss_phase3_return,
    ss_phase3_timeout,
    //会话超时
    ss_timeout,
    //会话错误
    ss_error,
    ss_notexist
} session_status_t;

//session的类型
typedef enum {
    st_unknown,
    //完整的查询请求
    st_full,
    //phase1查询请求
    st_phase1_only,
    //phase2查询请求
    st_phase2_only,
    //预留位
    st_phase3_only
} session_type_t;

//纪录级别
typedef enum {
    sr_simple,
    sr_full
} session_role_t;

class LogInfo {
    public:
        LogInfo()
        :_eRole(sr_simple),
        _unCost(0),
        _inFound(0),
        _inReturn(0),
        _inLen(0),
        _query(NULL)
        {_addr[0] = '\0';}
        ~LogInfo(){};
    public:
        void print()
        {
            if (_query == NULL) {
                return;
            }
            if (_eRole==sr_full) {
                TACCESS("%d\t%d\t%d\t%d\t%s\t%s", 
                        _unCost, 
                        _inFound, 
                        _inReturn,
                        _inLen,
                        _addr, 
                        SAFE_STRING(_query));
            }
            else if (_eRole==sr_simple) {
                TACCESS("%d\t%d\t%s",
                        _unCost,
                        _inLen,
                        SAFE_STRING(_query));
            }
        }
    public:
        session_role_t _eRole;
        int32_t _unCost;
        int32_t _inFound;
        int32_t _inReturn;
        int32_t _inLen;
        char _addr[32];
        const char *_query;
};

class Session {
    public:
        Session(ParallelCounter& counter);
        ~Session();
    public:
        /**
         *@brief set session status
         *@param status session status
         */
        void setStatus(session_status_t status, const char *msg = NULL);
        /**
         *@brief set error message
         *@param msgno error number
         *@param msg message infomation
         */
        void setMessage(uint32_t msgno, const char *msg);
        /**
         *@brief get status
         *@return status
         */
        session_status_t getStatus() const { return _status; }
        /**
         *@brief set type
         *@param type session type
         */
        void setType(session_type_t type) { _type = type; }
        /**
         *@brief get session type
         *@return session type
         */
        session_type_t getType() const { return _type; }
        /**
         *@brief set query string
         *@param str query string
         *@param len query string length
         *return 0:success, other:fail
         */
        int32_t setQuery(const char *str, uint32_t len) { 
            return _query.setOrigQuery(str, len); 
        }
        /**
         *@brief get query string
         *@return query string
         */
        Query & getQuery() { return _query; }
        /**
         *@brief set anet channel id
         *@param id channel id
         */
        void setANETChannelID(uint32_t id) { _channelId = id; }
        /**
         *@brief set anet channel id
         *@return channel id
         */
        uint32_t getANETChannelID() const { return _channelId; }
        /**
         *@brief set the value of keepalive
         *@param on keepalive or not
         */
        void setANETKeepAlive(bool on) { _keepalive = on; }
        /**
         *@brief get the value of keepalive
         *@return the value of keepalive
         */
        bool getANETKeepAlive() const { return _keepalive; }
        /**
         *@brief bind this Session to input service object
         *@param service service object
         *@param conn connection object of anet
         */
        void bindService(Service *service, anet::Connection *conn);
        /**
         *@brief release service
         */
        void releaseService();
        /**
         *@brief set response data
         *@param data response data
         *@param size response data size
         */
        void setResponseData(char *data, uint32_t size);
        /**
         *@brief get response data
         *@return response data
         */
        const char * getResponseData() const { return _responseData; }
        /**
         *@brief get response data size
         *@return response data size
         */
        uint32_t getResponseSize() const { return _responseSize; }
        /**
         *@brief set session start time
         *@param t start time
         */
        void setStartTime(uint64_t t) { _startTime = t; }
        /**
         *@brief get session start time
         *@return start time
         */
        uint64_t getStartTime() const { return _startTime; }
        /**
         *@brief set session last time
         *@param last time
         */
        void setLastTime(uint64_t t) { _lastTime = t; }
        /**
         *@brief get session last time
         *@return last time
         */
        uint64_t getLastTime() const { return _lastTime; }
        /**
         *@brief set duration time of schdule
         *@param duration time of schdule
         */
        void setSchduleTime(float t) { _scheduleTime = t; }
        /**
         *@brief get duration time of schdule
         *@return duration time of schdule
         */
        float getSchduleTime() const { return _scheduleTime; }

        /**
         *@brief get duration time of session
         *@return duration time of session
         */
        uint64_t getLatencyTime() const { 
            return (UTIL::Timer::getMicroTime() - _startTime); }
        /**
         *@brief get Session id
         *@return session id
         */
        uint64_t getSessionId() const { return _id; }
        /**
         *@brief set current request
         *@param req request object
         */
        void setCurrentRequest(Request *req);
        /**
         *@brief get current request
         *@return request object
         */
        Request * getCurrentRequest() const { return _curRequest; }
        /**
         *@brief release current request object
         */
        Request * releaseCurrentRequest();
        /**
         *@brief set the start number of query
         *@param n start number
         */
        void setStartNum(uint32_t n) { _startNum = n; }
        /**
         *@brief set the request number of query
         *@param n request number
         */
        void setRequestNum(uint32_t n) { _reqNum = n; }
        /**
         *@brief get start number
         *@return start number
         */
        uint32_t getStartNum() const { return _startNum; }
        /**
         *@brief get request number
         *@return request number
         */
        uint32_t getRequestNum() const { return _reqNum; }
        /**
         *@brief set sub start offset
         *@param sub start offset
         */
        void setSubStart(int iSubStart) { _iSubStart = iSubStart; }
        /**
         *@brief set sub number
         *@param sub number
         */
        void setSubNum(int iSubNum) { _iSubNum = iSubNum; }
        /**
         *@brief get sub start offset
         *@return sub start offset
         */
        int getSubStart() const { return _iSubStart; }
        /**
         *@brief get sub number
         *@return sub number
         */
        int getSubNum() const { return _iSubNum; }
        /**
         *@brief set arguments for session
         *@param arg argument
         */
        void setArgs(void *arg) { _args = arg; }
        /**
         *@brief get argument for session
         *@param return argument for session
         */
        void * getArgs() const { return _args; }
        /**
         *@brief set max level of the query
         *@param n max level of the query
         */
        void setMaxLevel(uint32_t n) { _maxLevel = n; }
        /**
         *@brief get max level of the query
         *@return max level of the query
         */
        uint32_t getMaxLevel() const { return _maxLevel; }
        /**
         *@brief set current level of query
         *@param n current level of query
         */
        void setCurLevel(uint32_t n) { _curLevel = n; }
        /**
         *@brief get current level of query
         *@return current level of query
         */
        uint32_t getCurLevel() const { return _curLevel; }
        /**
         *@brief get next level of query
         *@return next level of query
         */
        uint32_t nextLevel() {
            return (_curLevel > 0 && (_maxLevel == 0 || 
                        _curLevel <= _maxLevel)) ? 
                (_curLevel++) : 0;
        }
        /**
         *@brief set default format of result (V3,XML)
         *@param fmt result format
         */
        void setDflFmt(const char *fmt);
        /**
         *@brief get default format of result (V3,XML)
         *@return result format
         */
        char *getDflFmt(){ return _dflFmt; }
        /**
         *@brief get error message
         *@param errres message string
         *@param errsize message string size
         */
        void getErrorRes(char *&errres, uint32_t &errsize);
        /**
         *@brief return the result to client
         *@param isDec decrease parraller count
         *@return 0:success, other:fail
         */
        int32_t response(bool isDec = true);
        /**
         *@brief judge connect protocal is or not Http
         *@return connect is http protocal return ture, else return false
         */
        bool isHttp(); 
    public:
        /**
         *@brief get current time
         *@return current time
         */
        static uint64_t getCurrentTime();
        /**
         *@brief get sessions count
         *@return sessions count
         */
        static uint32_t getCurrentCount();
        //Source Control 
        void extractSource(); 
        char * getSource() { return _psSource; } 
        char * getApp() { return _psAppName; } 
        char * getSvc() { return _psSvcName; } 
        char * getMachine() { return _psMachineName; } 
    protected:
        session_status_t _status;
        session_type_t _type;
        Query _query;
        uint32_t _channelId;
        bool _keepalive;
        char *_responseData;
        uint32_t _responseSize;
        Service *_service;
        anet::Connection *_conn;
        uint64_t _startTime;
        uint64_t _lastTime;
        float _scheduleTime;
        Request *_curRequest;
        uint32_t _startNum;
        uint32_t _reqNum;
        int _iSubStart;
        int _iSubNum;
        void *_args;
        uint32_t _curLevel;
        uint32_t _maxLevel;
        uint64_t _id;
        ParallelCounter& _counter;
    public:
        LogInfo _logInfo;
        Session *_prev;
        Session *_next;
        //Source Control 
        char *_psSvcName; 
        char *_psAppName; 
        char *_psMachineName; 
        char *_psSource; 
        char *_dflFmt;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_SESSION_H_
