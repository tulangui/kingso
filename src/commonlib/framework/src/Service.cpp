#include "framework/Service.h"
#include "framework/TaskQueue.h"
#include "framework/Session.h"
#include "framework/Cache.h"
#include "util/Log.h"
#include <new>

FRAMEWORK_BEGIN;

static const char *SYSTEM_BUSY = "system is too busy!";
static const uint32_t SYSTEM_BUSY_LEN = strlen(SYSTEM_BUSY);
static anet::DefaultPacketFactory g_anet_dfl_pf;
static anet::DefaultPacketStreamer g_anet_dfl_ps(&g_anet_dfl_pf);

Service::Service(TaskQueue &tq, ParallelCounter &pc, WebProbe *wp) 
    : _listener(NULL), 
    _counter(pc), 
    _taskQueue(tq), 
    _prefix(NULL),
    next(NULL),
    _maxRequestPerConnection(0),
    _maxRequestOverflow(0),
    _webProbe(wp) {
    }

Service::~Service() 
{
    if (_listener) {
        _listener->subRef();
    }
    if (_prefix) {
        ::free(_prefix);
    }
}

int32_t Service::setPrefix(const char *prefix) 
{
    if (_prefix != NULL) {
        ::free(_prefix);
    }
    if (prefix == NULL) {
        _prefix = NULL;
    }
    else {
        _prefix = strdup(prefix);
        if (unlikely(_prefix == NULL)) {
            return ENOMEM;
        }
    }
    return KS_SUCCESS;
}

anet::IPacketHandler::HPRetCode 
Service::handlePacket(anet::Connection *connection, anet::Packet *packet) 
{
    Session *session = NULL;
    //检查接收到的包是否有效
    if (unlikely(!packet->isRegularPacket())) {
        packet->free();
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    session = new (std::nothrow) Session(_counter);
    if (unlikely(session == NULL)) {
        //TODO: reponse system resource limit.
        packet->free();
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    session->bindService(this, connection);
    session->setANETChannelID(packet->getChannelId());
    //解析接收到的包，并放到session中
    if (unlikely(parseQueryString(packet, *session) != KS_SUCCESS)) {
        packet->free();
        session->setStatus(ss_error);
        session->response();
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    //释放packet中的内存
    packet->free();
    //判断前缀是否正确_prefix
    if (session->getQuery().purge(_prefix) != KS_SUCCESS) {
        //是否进行文件访问服务
        if (_webProbe && 
            _webProbe->isProbeRequest(session->getQuery().getOrigQueryData()))
        {
            if (unlikely(_webProbe->enQueue(session) != KS_SUCCESS)) {
                session->setStatus(ss_error);
                session->response(false);
                return anet::IPacketHandler::FREE_CHANNEL;
            }
            return anet::IPacketHandler::FREE_CHANNEL;
        }
        else {
            session->setStatus(ss_error);
            session->response(false);
            return anet::IPacketHandler::FREE_CHANNEL;
        }
    }
    //是否已经达到最大并发，或异常管理的限制	
    if (unlikely(!_counter.inc())) {
        session->setStatus(ss_error);
        session->response(false);
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    //如果使用cache，则进入cache处理流程
    if(_cache && session->getQuery().useCache()) {
        struct iovec key, val;
        key.iov_base = (void*)session->getQuery().getPureQueryData();
        key.iov_len = session->getQuery().getPureQuerySize();
        if(_cache->get(key, val)) {
            session->setResponseData((char*)val.iov_base, val.iov_len);
            session->getQuery().setUseCache(false);
            session->response();
            return anet::IPacketHandler::FREE_CHANNEL;
        }
    }
    //把session放入调度队列
    if (unlikely(_taskQueue.enqueue(session) != KS_SUCCESS)) {
        session->setStatus(ss_error);
        session->response();
        return anet::IPacketHandler::FREE_CHANNEL;
    }
    return anet::IPacketHandler::FREE_CHANNEL;
}

int32_t Service::response(anet::Connection *conn, char *str, uint32_t size,
        uint32_t channelId, bool bKeepalive, uint64_t cost, char *outfmt,
        bool error, bool notExist, compress_type_t compressType, bool isDec) 
{
    bool bWritefinishclose = !bKeepalive;
    bool isHttp = false;
    anet::HTTPPacket *packet = NULL;
    if (strncmp(_spec, "http:", 5) == 0) {
        isHttp = true;
    }
    if (isDec) {
        _counter.dec(cost, error);
    }
    if (unlikely(!conn)) {
        if (str != NULL) {
            ::free(str);
        }
        return EINVAL;
    }
    //判断是否需要重新建立连接
    if (isHttp && bKeepalive && _maxRequestPerConnection>0) {
        int32_t count = conn->getPacketHandledCount();
        if (count < _maxRequestPerConnection) {
        }
        else if (count < (_maxRequestPerConnection+_maxRequestOverflow)) {
            bKeepalive = false;
        }
        else if (count < (_maxRequestPerConnection+_maxRequestOverflow+10)) {
            bKeepalive = false;
            bWritefinishclose = true;
        }
        else {
            conn->close();
            conn->subRef();
            TWARN("request count exceed, force close. %d >= %d" ,
                    count , _maxRequestPerConnection+_maxRequestOverflow+10);
            if (str) {
                ::free(str);
            }
            return KS_EFAILED;
        }
    }
    //根据str，创建报文信息
    packet = (anet::HTTPPacket *)createResponsePacket(str,
            size, channelId, bKeepalive, notExist);
    if (unlikely(packet == NULL)) {
        if (str != NULL) {
            ::free(str);
        }
        return KS_EFAILED;
    }
    //添加http信息类型
    if (isHttp) {
        if (outfmt && (strcmp("xml", outfmt) == 0
                    || (strlen(outfmt) > 4 && 
                        memcmp("xml/", outfmt, 4) == 0))) 
        {
            packet->addHeader("Content-Type", "text/xml");
        }
        else {
            packet->addHeader("Content-Type", "text/html");
        }
    }
    //发送packet
    if (unlikely(!conn->postPacket(packet))) {
        packet->free();
        return KS_EFAILED;
    }
    if (isHttp && bWritefinishclose) {
        conn->setWriteFinishClose(true);
    }
    return KS_SUCCESS;
}

bool Service::putCache(const char * key_data, uint32_t key_size,
        const char * val_data, uint32_t val_size, int nDocsFound) 
{
    if (_cache)  {
        int nMinDocsFound = _cache->getMinDocsFound();
        if (nMinDocsFound != -1 && nDocsFound < nMinDocsFound) {
            return true;
        }
        struct iovec key, val;
        key.iov_base = (void*)key_data;
        key.iov_len = key_size;
        val.iov_base = (void*)val_data;
        val.iov_len = val_size;
        return _cache->put(key, val);
    }
    return false;
}

FRAMEWORK_END;

/************************* TCPService *******************************/
FRAMEWORK_BEGIN;

int32_t TCPService::parseQueryString(anet::Packet *packet,
        Session &session) 
{
    anet::DefaultPacket *pkg = NULL;
    const char *body = NULL;
    size_t size = 0;
    int32_t ret = 0;
    if (unlikely(!packet)) {
        return EINVAL;
    }
    pkg = dynamic_cast<anet::DefaultPacket*>(packet);
    if (unlikely(!packet)) {
        return KS_EFAILED;
    }
    body = pkg->getBody(size);
    ret = session.setQuery(body, size);
    if (unlikely(ret != KS_SUCCESS)) {
        return ret;
    }
    session.setANETKeepAlive(true);
    return KS_SUCCESS;
}

class TCPResponsePacketWrapper : public anet::DefaultPacket 
{
    public:
        TCPResponsePacketWrapper(char *str) : _point(str) { }
        ~TCPResponsePacketWrapper();
    private:
        char *_point;
};

TCPResponsePacketWrapper::~TCPResponsePacketWrapper() 
{
    if (_point) {
        ::free(_point);
    }
}

anet::Packet * TCPService::createResponsePacket(char *str, uint32_t size,
        uint32_t channelId, bool keepalive, bool error) 
{
    TCPResponsePacketWrapper *packet = 
        new (std::nothrow) TCPResponsePacketWrapper(str);
    if (unlikely(packet == NULL)) {
        return NULL;
    }
    if (str && size > 0) {
        packet->setBody(str, size);
    }
    packet->setChannelId(channelId);
    return packet;
}

int32_t TCPService::start(anet::Transport &transport, 
        const char *server, uint16_t port) 
{
    char spec[64];
    snprintf(spec, 64, "tcp:%s:%d", (server ? server : ""), port);
    _listener = transport.listen(spec, &g_anet_dfl_ps, this);
    if (!_listener) {
        _spec[0] = '\0';
        return KS_EFAILED;
    }
    snprintf(_spec, 64, "tcp:%s:%d", (server ? server : ""), port);
    return KS_SUCCESS;
}

FRAMEWORK_END;

/************************* HTTPService ******************************/
FRAMEWORK_BEGIN;

int32_t HTTPService::parseQueryString(anet::Packet *packet,
        Session &session) 
{
    anet::HTTPPacket *pkg = NULL;
    const char *body = NULL;
    size_t size = 0;
    int32_t ret = 0;
    if (unlikely(!packet)) {
        return EINVAL;
    }
    pkg = dynamic_cast<anet::HTTPPacket*>(packet);
    if (unlikely(packet == NULL)) {
        return KS_EFAILED;
    }
    body = pkg->getURI();
    if (body) {
        size = strlen(body);
    }
    else {
        size = 0;
    }
    ret = session.setQuery(body, size);
    if (unlikely(ret != KS_SUCCESS)) {
        return ret;
    }
    session.setANETKeepAlive(pkg->isKeepAlive());
    return KS_SUCCESS;
}

class HTTPResponsePacketWrapper : public anet::HTTPPacket 
{
    public:
        HTTPResponsePacketWrapper(char *str) : _point(str) { }
        ~HTTPResponsePacketWrapper();
    private:
        char *_point;
};

HTTPResponsePacketWrapper::~HTTPResponsePacketWrapper() 
{
    if (_point) {
        ::free(_point);
    }
}

anet::Packet * HTTPService::createResponsePacket(char *str, uint32_t size,
        uint32_t channelId, bool keepalive, bool error) 
{
    HTTPResponsePacketWrapper *packet = 
        new (std::nothrow) HTTPResponsePacketWrapper(str);
    if (unlikely(!packet)) {
        return NULL;
    }
    packet->setPacketType(anet::HTTPPacket::PT_RESPONSE);
    if (str && size > 0) {
        packet->setBody(str, size);
    }
    packet->setChannelId(channelId);
    packet->setKeepAlive(keepalive);
    //	packet->setStatusCode(200);
    packet->setStatusCode(error ? 404 : 200);
    return packet;
}

static anet::HTTPPacketFactory G_ANET_HTTP_PF;
static anet::HTTPStreamer G_ANET_HTTP_PS(&G_ANET_HTTP_PF);

int32_t HTTPService::start(anet::Transport &transport, 
        const char *server, uint16_t port) 
{
    char spec[64];
    snprintf(spec, 64, "tcp:%s:%d", (server ? server : ""), port);
    _listener = transport.listen(spec, &G_ANET_HTTP_PS, this);
    if (!_listener) {
        _spec[0] = '\0';
        return KS_EFAILED;
    }
    snprintf(_spec, 64, "http:%s:%d", (server ? server : ""), port);
    return KS_SUCCESS;
}

FRAMEWORK_END;

/************************* ServiceFactory **************************/
FRAMEWORK_BEGIN;

Service * ServiceFactory::makeService(const char *protocol, 
        TaskQueue &tq, ParallelCounter &pc, WebProbe *wp) 
{
    if (!protocol) {
        return NULL;
    }
    if (strcmp(protocol, "http") == 0) {
        return new (std::nothrow) HTTPService(tq, pc, wp);
    } 
    else if (strcmp(protocol, "tcp") == 0) {
        return new (std::nothrow) TCPService(tq, pc, wp);
    }
    return NULL;
}

FRAMEWORK_END;

