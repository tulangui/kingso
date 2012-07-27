#include "framework/Session.h"
#include "framework/Service.h"
#include "framework/Request.h"
#include "framework/Compressor.h"
#include "util/Log.h"
#include <anet/atomic.h>
#include <sys/time.h>

FRAMEWORK_BEGIN;

#define XMLTMPL "<?xml version=\"1.0\" encoding=\"utf-8\"?> \
    <errorno>%d</errorno><errormsg>%s</errormsg>"

static atomic_t SESSION_COUNTER;
static atomic_t SESSION_BUSY;

uint64_t Session::getCurrentTime() 
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (static_cast<uint64_t>(
                t.tv_sec) * static_cast<int64_t>(1000000) + 
            static_cast<int64_t>(t.tv_usec));
}


uint32_t Session::getCurrentCount() 
{
    return SESSION_BUSY.counter;
}

Session::Session(ParallelCounter& counter) 
    : _counter(counter),
    _status(ss_arrive), 
    _type(st_unknown),
    _channelId(0), 
    _keepalive(true),
    _responseData(NULL), 
    _responseSize(0),
    _service(NULL), 
    _conn(NULL), 
    _startTime(getCurrentTime()), 
    _lastTime(0), 
    _scheduleTime(0.0),
    _curRequest(NULL),
    _curLevel(0), 
    _maxLevel(0),
    _startNum(0), 
    _reqNum(0),
    _iSubStart(0),
    _iSubNum(0),
    _args(NULL), 
    _prev(NULL), 
    _next(NULL), 
    _psSvcName(NULL),
    _psAppName(NULL), 
    _psMachineName(NULL), 
    _psSource(NULL), 
    _dflFmt(NULL),
    _id(atomic_inc_return(&SESSION_COUNTER))
{
        _lastTime = _startTime;
        atomic_inc(&SESSION_BUSY);
}

Session::~Session() 
{
    atomic_dec(&SESSION_BUSY);
    if (_responseData) {
        ::free(_responseData);
    }
    releaseService();
    if (_curRequest) {
        delete _curRequest;
    }
    if (_psAppName) {
        delete [] _psAppName;
    }
    if (_psMachineName) {
        delete [] _psMachineName;
    }
    if (_psSource) {
        delete [] _psSource;
    }
    if (_psSvcName) {
        delete [] _psSvcName;
    }
    if (_dflFmt) {
        ::free(_dflFmt);
    }
}

void Session::setResponseData(char *data, uint32_t size) 
{
    if (_responseData) {
        ::free(_responseData);
    }
    _responseData = data;
    _responseSize = (data ? size : 0);
    return;
}

void Session::setMessage(uint32_t msgno, const char *msg) 
{
    if ((msg) && (msg[0])) {
        int32_t len = strlen(msg)+ strlen(XMLTMPL) + 13;
        char *data = (char *)malloc(len+1);
        memset(data, 0x0, len+1);
        snprintf(data, len, XMLTMPL, msgno, msg);
        setResponseData(data, strlen(data)+1);
    }
}

void Session::setStatus(session_status_t status, const char *msg) 
{
    if ((msg) && (msg[0])) {
        int32_t len = strlen(msg);
        char *data = (char *)malloc(len+1);
        memcpy(data, msg, len);
        snprintf(data, len, XMLTMPL, msg);
        data[len] = '\0';
        setResponseData(data, len+1);
    }
    _status = status;
}

void Session::setCurrentRequest(Request *req) 
{ 
    if (_curRequest) {
        delete _curRequest; 
    }
    _curRequest = req; 
}

void Session::bindService(Service *service, anet::Connection *conn) 
{
    _service = service;
    _conn = conn;
    if (_conn) {
        _conn->addRef();
    }
}

void Session::releaseService() 
{
    if (_conn) {
        _conn->subRef();
        _conn = NULL;
    }
    _service = NULL;
}

Request * Session::releaseCurrentRequest() 
{
    Request *r = _curRequest;
    _curRequest = NULL;
    return r;
}

int32_t Session::response(bool isDec) 
{
    int32_t ret = 0;
    int32_t finalret = 0;
    char *res = NULL;
    uint32_t resSize = 0;
    uint64_t cost = 0;
    if (_query.useCache() && _service && _status != ss_error &&
            _responseData && _responseSize > 0) {
        _service->putCache(_query.getPureQueryData(),
                _query.getPureQuerySize(),
                _responseData, _responseSize, 
                _logInfo._inFound);
    }
    if (unlikely(!_service || !_conn)) {
        delete this;
        return KS_EFAILED;
    }
    res = _responseData;
    resSize = _responseSize;
    _responseData = NULL;
    _responseSize = 0;
    cost = getCurrentTime() - _startTime;
    char *outfmt = NULL;
    if (_service && strncmp(_service->getSpec(), "http", 4)==0)
        outfmt = FRAMEWORK::Query::getParam(getQuery().getPureQueryData(),
                getQuery().getPureQuerySize(), "outfmt");
    ret = _service->response(_conn, 
            res, resSize, _channelId, _keepalive, 
            cost, (outfmt && strlen(outfmt) > 0) ? outfmt : _dflFmt,
            (_status==ss_error), (_status==ss_notexist), 
            _query.getCompress(), isDec);
    if (outfmt) ::free(outfmt);
    if (_logInfo._eRole==sr_full) {
        _logInfo._inLen = resSize;
        if (_logInfo._unCost == 0)
            _logInfo._unCost = (int32_t)cost;
        _logInfo._query = _query.getOrigQueryData();
        _conn->getIOComponent()->getSocket()->getAddr(_logInfo._addr, 32);
        if ((_status==ss_error)||(_status==ss_notexist)) {
            _logInfo._unCost = -1;
            _logInfo._inFound = -1;
            _logInfo._inReturn = -1;
        }
    }
    else if (_logInfo._eRole == sr_simple) {
        _logInfo._inLen = resSize;
        if (_logInfo._unCost == 0)
            _logInfo._unCost = (int32_t)cost;
        _logInfo._query = _query.getOrigQueryData();
        if ((_status==ss_error)||(_status==ss_notexist)) {
            _logInfo._unCost = -1;
        }
    }
    _logInfo.print();
    /*
       TACCESS("[%lu] %s %u cost=%d", 
       _id, 
       SAFE_STRING(_query.getOrigQueryData()), 
       resSize, 
       cost);
       */
    uint64_t curTime = getCurrentTime();
    _counter.stat(getStartTime(), curTime);
    delete this;
    return finalret;
}
bool Session::isHttp()
{ 
    if(_service && 0 == strncmp(_service->getSpec(), "http", 4))
    {
        return true;
    }
    return false; 
}
void Session::setDflFmt(const char *fmt) 
{
    if (fmt && *fmt) {
        if (_dflFmt) ::free(_dflFmt);
        _dflFmt = strdup(fmt);
    }
    return;
}


//Source Control 
void Session::extractSource() 
{ 
    const static int MAX_SRC_PARAM_LEN = 256; 
    char *ptr = NULL; 
    char *val = NULL; 
    char *valSvc = NULL; 
    const char *qTmp = NULL;
    int len = 0; 
    int qLen = 0;
    qLen = _query.getPureQuerySize();
    if (qLen <= 0)
        return;
    qTmp = _query.getPureQueryData(); 
    if ((qTmp==NULL)&&(*qTmp=='\0')) {
        return;
    }
    char *psMsg = new (std::nothrow) char[qLen + 1];
    if (NULL == psMsg) {
        return;
    }
    memcpy(psMsg, qTmp, qLen);
    psMsg[qLen] = '\0';
    _psSource  = NULL; 
    _psSvcName = NULL; 
    _psAppName = NULL; 
    _psMachineName = NULL; 
    while ( true ) { 
        ptr = strstr(psMsg, "&src="); 
        if ( ptr == NULL ) {
            ptr = strstr(psMsg, "&SRC="); 
        }
        if ( ptr == NULL ) {
            ptr = strstr(psMsg, "?src="); 
        }
        if ( ptr == NULL ) {
            ptr = strstr(psMsg, "?SRC="); 
        }
        if ( ptr == NULL ) {
            break; 
        }
        ptr += 5; 
        val = ptr; 
        while (*ptr != '&' && *ptr != 0) {
            ptr++;
        }
        *ptr = 0; 
        len = ptr - val; 
        if (len <= 0 || len>MAX_SRC_PARAM_LEN) {
            if (psMsg) {
                delete [] psMsg;
            }
            return; 
        }
        _psSource = new (std::nothrow) char[len + 1]; 
        memcpy(_psSource, val, len); 
        _psSource[len] = 0; 
        *ptr = 0; 
        //get source info 
        ptr = val; 
        while (*ptr != '-' && *ptr != 0) {
            ptr++;
        }
        valSvc = val; 
        len = ptr - val; 
        _psSvcName = new (std::nothrow) char[len + 1]; 
        memcpy(_psSvcName, val, len); 
        _psSvcName[len] = 0; 
        if (*ptr == 0) {
            if (psMsg) {
                delete [] psMsg;
            }
            return;
        }
        val = ptr+1; 
        ptr = val; 
        while (*ptr != '-' && *ptr != 0) {
            ptr++;
        }
        len = ptr - valSvc; 
        _psAppName = new (std::nothrow) char[len + 1]; 
        memcpy(_psAppName, valSvc, len); 
        _psAppName[len] = 0; 
        if (*ptr == 0) {
            if (psMsg) {
                delete [] psMsg;
            }
            return;
        }
        val = ptr+1; 
        ptr = val; 
        while (*ptr != '-' && *ptr != 0) {
            ptr++; 
        }
        len = ptr - val; 
        _psMachineName = new (std::nothrow) char[len + 1]; 
        memcpy(_psMachineName, val, len); 
        _psMachineName[len] = 0; 
        if (psMsg) {
            delete [] psMsg;
        }
        return; 
    } 
    if (psMsg) {
        delete [] psMsg;
    }
    return; 
} 

FRAMEWORK_END;

