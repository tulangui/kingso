#include "framework/ConnectionPool.h"
#include "util/ScopedLock.h"
#include "util/Log.h"
#include <new>

FRAMEWORK_BEGIN;

static anet::HTTPPacketFactory g_http_pf;
static anet::HTTPStreamer g_http_ps(&g_http_pf);
static anet::DefaultPacketFactory g_df_pf;
static anet::DefaultPacketStreamer g_df_ps(&g_df_pf);

ConnectionPool::ConnectionPool(anet::Transport &tran,
        uint32_t connQueueLimit, uint32_t timeout)
    : _transport(tran),
    _connQueueLimit(connQueueLimit),
    _timeout(timeout) 
{ }

ConnectionPool::~ConnectionPool() 
{ 
    std::map<std::string, anet::Connection*>::iterator it;
    MUTEX_LOCK(_lock);
    for (it = _mp.begin(); it != _mp.end(); it++) {
        if (it->second) {
            it->second->subRef();
        }
    }
    _mp.clear();
    MUTEX_UNLOCK(_lock);
}

anet::Connection * ConnectionPool::makeConnection(const char *protocol,
        const char *server,
        uint16_t port) 
{
    char key[64];
    if (unlikely(!protocol || !server)) {
        return NULL;
    }
    if (strcmp(protocol, "http") == 0) {
        snprintf(key, 64, "tcp:%s:%u", server, port);
        return makeHttpConnection(key);
    } 
    else if (strcmp(protocol, "tcp") == 0) {
        snprintf(key, 64, "tcp:%s:%u", server, port);
        return makeTcpConnection(key);
    } 
    else if (strcmp(protocol, "udp") == 0) {
        TWARN("upd protocol is not supportted now.");
        return NULL;
    }
    TWARN("unknown protocol '%s'.", protocol);
    return NULL;
}

anet::Connection * ConnectionPool::makeHttpConnection(const char *spec) 
{
    anet::Connection *conn = _transport.connect(spec, &g_http_ps, false);
    if (conn) {
        conn->setWriteFinishClose(true);
    }
    return conn;
}

anet::Connection * ConnectionPool::makeTcpConnection(const char *spec) 
{
    anet::Connection *conn = NULL;
    std::map<std::string, anet::Connection*>::iterator it;
    if (!spec) {
        return NULL;
    }
    MUTEX_LOCK(_lock);
    it = _mp.find(std::string(spec));
    if (it != _mp.end()) {
        conn = it->second;
    }
    if (unlikely(conn && conn->isClosed())) {
        conn->subRef();
        _mp.erase(std::string(spec));
        conn = NULL;
    }
    if (!conn) {
        conn = _transport.connect(spec, &g_df_ps, false);
        if (likely(conn)) {
            if (_connQueueLimit > 0) {
                conn->setQueueLimit(_connQueueLimit);
            }
            if (_timeout > 0) {
                conn->setQueueTimeout(_timeout);
            }
            _mp.insert(std::make_pair(std::string(spec), conn));
        }
    }
    if (likely(conn)) {
        conn->addRef();
    }
    MUTEX_UNLOCK(_lock);
    return conn;
}

FRAMEWORK_END;

