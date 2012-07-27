#include "framework/Request.h"
#include "framework/Session.h"
#include "framework/TaskQueue.h"
#include "framework/ConnectionPool.h"
#include "util/ScopedLock.h"

//=============================ANETRequest===========================
FRAMEWORK_BEGIN;

ANETRequest::ANETRequest(ConnectionPool &connPool) 
    : _status(rt_unknown), 
    _res(NULL), 
    _owner(NULL),
    _query(NULL), 
    _querySize(0),
    _node(NULL), 
    _standby(NULL),
    _args(NULL),
    _connPool(connPool) { }

ANETRequest::~ANETRequest() 
{
    if (_res) {
        delete _res;
    }
    if (_query) {
        ::free(_query);
    }
}

anet::IPacketHandler::HPRetCode 
ANETRequest::handlePacket(anet::Packet *packet, void *args) 
{
    const char *body = NULL;
    size_t size = 0;
    struct _packet_info_t *pi = (struct _packet_info_t *)args;
    if (unlikely(packet==NULL) || unlikely(args==NULL)) {
        if (_res) {
            delete _res;
        }
        _res = new (std::nothrow) ANETResponse(body, size);
        _status = rt_responsed;
        _owner->response(this);
        if (packet) {
            packet->free();
        }
        if (args) {
            delete (struct _packet_info_t *)args;
        }
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    //判断接收包packet的有效性
    if (packet->isRegularPacket()) {
        if (_node) {
            if (_node->m_protocol == clustermap::http ||
                    _node->m_protocol == clustermap::tcp) 
            {
                anet::DefaultPacket *pkg;
                pkg = dynamic_cast<anet::DefaultPacket*>(packet);
                if (likely(pkg)) {
                    body = pkg->getBody(size);
                }
            } 
            else if (_node->m_protocol == clustermap::udp) {
                //TODO: parse udp packet.
            }
        }
    }
    else {
        TWARN("ANETClient: recived a contrl pkg[%d] ip=%s.", 
                ((anet::ControlPacket*)packet)->getCommand(), pi->addr);
    }
    if (args != NULL) {
        delete (struct _packet_info_t *)args;
    }
    if (!body || size == 0) {
     	//query error, try standby node. for doc sep
        if (_standby && _standby != _node) {
            //alloc from clustermap, not new, donot need free
            _node = _standby;
            if (request() == KS_SUCCESS) {
             	//resend successful
                if (packet) {
                    packet->free();
                }
                return anet::IPacketHandler::FREE_CHANNEL;
            }
        }
    }
    if (_res != NULL) {
        delete _res;
    }
    _res = new (std::nothrow) ANETResponse(body, size);
    _status = rt_responsed;
    _owner->response(this);
    if (packet != NULL) {
        packet->free();
    }
    return anet::IPacketHandler::FREE_CHANNEL;
}

int32_t ANETRequest::setQuery(const char *data, uint32_t size) 
{
    if (_query != NULL) {
        ::free(_query);
    }
    if (!data || size == 0) {
        _query = NULL;
        _querySize = 0;
    } 
    else {
        _query = (char*)malloc(size + 1);
        if (unlikely(_query == NULL)) {
            _querySize = 0;
            return ENOMEM;
        }
        memcpy(_query, data, size);
        _query[size] = '\0';
        _querySize = size;
    }
    return KS_SUCCESS;
}

int32_t ANETRequest::request() 
{
    anet::Connection *conn = NULL;
    anet::Packet *packet = NULL;
    struct _packet_info_t *pi = NULL;
    if (unlikely(!_node)) {
        _node = _standby;
    }
    if (unlikely(!_node)) {
        return KS_EFAILED;
    }
    //根据不同的通讯协议，设定报文头
    if (_node->m_protocol == clustermap::http) {
        anet::HTTPPacket *p = new (std::nothrow) anet::HTTPPacket;
        if (unlikely(p == NULL)) {
            return ENOMEM;
        }
        p->setMethod(anet::HTTPPacket::HM_GET);
        if (_query && _querySize > 0) {
            p->setURI(_query);
        }
        else {
            p->setURI("");
        }
        p->addHeader("Accept", "*/*");
        p->addHeader("Connection", "Keep-Alive");
        packet = p;
    } 
    else if (_node->m_protocol == clustermap::tcp) {
        anet::DefaultPacket *p = new (std::nothrow) anet::DefaultPacket;
        if (unlikely(p == NULL)) {
            return ENOMEM;
        }
        if (_query && _querySize > 0) {
            p->setBody(_query, _querySize);
        }
        else {
            p->setBody("", 0);
        }
        packet = p;
    } 
    else if (_node->m_protocol == clustermap::udp) {
        //TODO: create udp packet
    }
    if (unlikely(packet == NULL)) {
        return KS_EFAILED;
    }
    conn = _connPool.makeConnection(
            (_node->m_protocol == clustermap::http ? "http" :
             (_node->m_protocol == clustermap::tcp ? "tcp" :
              (_node->m_protocol == clustermap::udp ? "udp" : ""))),
            _node->m_ip, 
            _node->m_port);
    if (unlikely(!conn)) {
        packet->free();
        if (_standby && _node != _standby) {
            _node = _standby;
            return request();
        }
        return KS_EFAILED;
    }

    pi = new (std::nothrow) (struct _packet_info_t);
    if (unlikely(pi==NULL)) {
        return ENOMEM;
    }
    conn->getIOComponent()->getSocket()->getAddr(pi->addr, 32);
    // end			 

    if (unlikely(!conn->postPacket(packet, this, (void *)pi))) {
        packet->free();
        delete pi;
        conn->subRef();
        if (_standby && _node != _standby) {
            _node = _standby;
            return request();
        }
        return KS_EFAILED;
    }
    conn->subRef();
    _status = rt_sended;
    return KS_SUCCESS;
}

FRAMEWORK_END;

//==========================ANETResponse=============================
FRAMEWORK_BEGIN;

ANETResponse::ANETResponse(const char *data, uint32_t size) 
    : _data(NULL), 
    _size(0) 
{
    if (data && size > 0) {
        _data = (char *)malloc(size);
        if (likely(_data != NULL)) {
            memcpy(_data, data, size);
            _size = size;
        }
    }
}

ANETResponse::~ANETResponse() 
{
    if (_data) {
        ::free(_data);
    }
}

FRAMEWORK_END;

//===============================Request=============================
FRAMEWORK_BEGIN;

Request::Request(Session &owner, TaskQueue &taskqueue) 
    : _status(rt_unknown),
    _owner(owner), 
    _taskQueue(taskqueue),
    _reqList(NULL), 
    _reqSize(0),
    _sendFail(0), 
    _return(0) {
    }

Request::~Request() 
{
    ANETRequest *p = NULL;
    while (_reqList) {
        p = _reqList;
        _reqList = _reqList->_next;
        delete p;
    }
}

int32_t Request::request() 
{
    int32_t ret = KS_EFAILED;
    MUTEX_LOCK(_lock);
    if (unlikely(!_reqList || _status != rt_unknown)) {
        return ret;
    }
    _status = rt_sended;
    _sendFail = 0;
    _return = 0;
    for (ANETRequest *p=_reqList; p!=NULL; p=p->_next) {
        if (p->request() != KS_SUCCESS) {
            _sendFail++;
        }
        else {
            ret = KS_SUCCESS;
        }
    }
    MUTEX_UNLOCK(_lock);
    //TODO: maybe
    //	response(NULL);
    return ret;
}

void Request::response(ANETRequest *req) 
{
    MUTEX_LOCK(_lock);
    if (_status != rt_sended) {
        return;
    }
    if (req) {
        _return++;
    }
    if (_return + _sendFail < _reqSize) {
        return;
    }
    _status = rt_responsed;
    MUTEX_UNLOCK(_lock);
    if (unlikely(_taskQueue.enqueue(&_owner) != KS_SUCCESS)) {
        _owner.setStatus(ss_error);
        _owner.response();
    }
}

int32_t Request::addANETRequest(ANETRequest *req) 
{
    if (unlikely(!req)) {
        return EINVAL;
    }
    MUTEX_LOCK(_lock);
    if (_status != rt_unknown) {
        return KS_EFAILED;
    }
    req->setOwner(this);
    req->_next = _reqList;
    _reqList = req;
    _reqSize++;
    MUTEX_UNLOCK(_lock);
    return KS_SUCCESS;
}

int32_t Request::add(Request &req) 
{
    ANETRequest *anetreq = NULL;
    ANETRequest *p = NULL;
    uint32_t size = 0;
    req._lock.lock();
    size = req._reqSize;
    if (size > 0) {
        anetreq = req._reqList;
        req._reqList = NULL;
        req._reqSize = 0;
    }
    req._lock.unlock();
    if (size > 0) {
        _lock.lock();
        for (p = _reqList; p && p->_next; p = p->_next);
        if (p == NULL) {
            _reqList = anetreq;
            _reqSize = size;
        } 
        else {
            p->_next = anetreq;
            _reqSize += size;
        }
        _lock.unlock();
    }
    return KS_SUCCESS;
}

ANETRequest * Request::getANETRequest(uint32_t idx) 
{
    ANETRequest *r = NULL;
    MUTEX_LOCK(_lock);
    if (idx > _reqSize) {
        return NULL;
    }
    for (r = _reqList; r && idx > 0; idx--, r = r->_next);
    MUTEX_UNLOCK(_lock);
    return r;
}

FRAMEWORK_END;

