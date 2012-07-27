#include "framework/Server.h"
#include "framework/Service.h"
#include "framework/TaskQueue.h"
#include "framework/Dispatcher.h"
#include "framework/Worker.h"
#include "framework/ServerWatchPoint.h"
#include "framework/Cache.h"
#include "util/ThreadPool.h"
#include "util/iniparser.h"
#include "util/Log.h"
#include "util/ScopedLock.h"
#include "clustermap/CMClient.h"
#include <anet/anet.h>

FRAMEWORK_BEGIN;

Server::Server() 
    : _timeout(0),  
    _services(NULL),
    _taskQueue(NULL),
    _thrPool(NULL),
    _disp(NULL), 
    _workerFactory(NULL),
    _transport(NULL),
    _counter(NULL),
    _cmClient(NULL),
    _status(srv_unknown),
    _type(srv_type_unknown),
    _cache(NULL),
    _webProbe(NULL) { 
    }

Server::~Server() 
{
    release();
}

int32_t Server::addService(const char *protocol,
        const char *server, uint16_t port) 
{
    Service *service = NULL;
    int32_t ret = 0;
    if (unlikely(!_taskQueue || !_counter || !_transport)) {
        return KS_EFAILED;
    }
    service = ServiceFactory::makeService(protocol, *_taskQueue, 
            *_counter, _webProbe);
    if (service == NULL) {
        TERR("invalid protocol[%s].", SAFE_STRING(protocol));
        return KS_EFAILED;
    }
    ret = service->start(*_transport, server, port);
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("startting service on [%s:%s:%d] failed(%d).",
                SAFE_STRING(protocol), SAFE_STRING(server), port, 
                ret);
        delete service;
        return ret;
    }
    service->next = _services;
    _services = service;
    return KS_SUCCESS;
}

int32_t Server::startThreads(uint32_t maxThrCount, uint32_t stackSize) 
{
    int32_t ret = 0;
    if (unlikely(!_taskQueue || !_workerFactory)) {
        return KS_EFAILED;
    }
    if (_thrPool) {
        delete _thrPool;
    }
    _thrPool = new UTIL::ThreadPool(maxThrCount, stackSize);
    if (unlikely(_thrPool == NULL)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
        return ENOMEM;
    }
    if (_disp == NULL) {
        _disp = new (std::nothrow) 
            Dispatcher(*_thrPool, *_taskQueue, *_workerFactory);
        if (unlikely(_disp == NULL)) {
            TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
            delete _thrPool;
            _thrPool = NULL;
            return ENOMEM;
        }
    }
    ret = _disp->start();
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("startting dispatcher thread failed(%d).", ret);
        delete _thrPool;
        _thrPool = NULL;
        delete _disp;
        _disp = NULL;
        return ret;
    }
    return KS_SUCCESS;
}

int32_t Server::initParallelLimitter(uint32_t max_parallel,
        uint64_t session_timeout, 
        uint32_t seriate_timeout_count_limit,
        uint32_t seriate_error_count_limit) 
{
    if (_counter != NULL) {
        delete _counter;
    }
    _counter = new (std::nothrow) ParallelCounter(max_parallel);
    if (unlikely(_counter == NULL)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
        return ENOMEM;
    }
    _counter->setTimeoutLimit(session_timeout);
    _counter->setSeriateTimeoutCountLimit(seriate_timeout_count_limit);
    _counter->setSeriateErrorCountLimit(seriate_error_count_limit);
    return KS_SUCCESS;
}

int32_t Server::initWorkerFactory(const char *path) 
{
    int32_t ret = 0;
    if (_workerFactory) {
        delete _workerFactory;
    }
    _workerFactory = makeWorkerFactory();
    if (unlikely(_workerFactory == NULL)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
        return ENOMEM;
    }
    _workerFactory->setOwner(this);
    ret = _workerFactory->initilize(path);
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("init worker factory by `%s' failed(%d).", 
                SAFE_STRING(path), ret);
        delete _workerFactory;
        _workerFactory = NULL;
        return ret;
    }
    return KS_SUCCESS;
}

static uint32_t get_int_value(const ini_section_t *section, 
        const char *key) 
{
    int value = ini_get_int_value1(section, key, 0);
    if (value < 0) {
        return 0;
    }
    return value;
}

static bool parse_service_config(const char *src,
        char *protocol, uint32_t protocol_size,
        char *server, uint32_t server_size,
        uint16_t &port) 
{
    uint32_t len = 0;
    int32_t v = 0;
    if (unlikely(!src || !protocol || !server)) {
        return false;
    }
    while (isspace(*src)) {
        src++;
    }
    len = 0;
    while (!isspace(src[len]) && src[len] != '\0') {
        len++;
    }
    if (src[len] == '\0' || len >= protocol_size) {
        return false;
    }
    memcpy(protocol, src, len);
    protocol[len] = '\0';
    src += len;
    while (isspace(*src)) {
        src++;
    }
    len = 0;
    while (!isspace(src[len]) && src[len] != '\0') {
        len++;
    }
    if (len >= server_size) {
        return false;
    }
    memcpy(server, src, len);
    server[len] = '\0';
    src += len;
    while (isspace(*src)) {
        src++;
    }
    if (*src == '\0') {
        v = atoi(server);
        if (v <= 0) {
            return false;
        }
        port = v;
        server[0] = '\0';
    } 
    else {
        v = atoi(src);
        if (v <= 0) {
            return false;
        }
        port = v;
    }
    return true;
}

int32_t Server::initCMClient(const char *local_cfg, const char *servers) 
{
    const char *val = NULL;
    const char *key = NULL;
    int nval = 0;;
    std::vector<std::string> cm_servers;
    if (_services == NULL) {
        TWARN("Service should be startted before Clustermap.");
        return KS_EFAILED;
    }
    if (servers != NULL) {
        val = servers;
        while(true) {
            while (isspace(*val) || *val == ',') {
                val++;
            }
            if (*val== '\0') {
                break;
            }
            key = val;
            while (*key != '\0' && *key != ',') {
                key++;
            }
            nval = key - val;
            while (nval > 0 && isspace(val[nval - 1])) {
                nval--;
            }
            if (nval > 0) {
                cm_servers.push_back(std::string(val, nval));
            }
            if (*key == '\0') {
                break;
            }
            val = key + 1;
        }
    }
    if ((cm_servers.size() ==  0) && !local_cfg) {
        TWARN("no invalid clustermap configure.");
        return KS_EFAILED;
    }
    if (_cmClient) {
        delete _cmClient;
    }
    _cmClient = new (std::nothrow) clustermap::CMClient();
    if (unlikely(_cmClient == NULL)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
        return ENOMEM;
    }
    if (_cmClient->Initialize(_services->getSpec(), 
                cm_servers, local_cfg) != 0) 
    {
        TWARN("configure clustermap client (\"%s\", %d, \"%s\") failed.",
                SAFE_STRING(_services->getSpec()), 
                SAFE_STRING(servers),
                SAFE_STRING(local_cfg));
        delete _cmClient;
        _cmClient = NULL;
        return KS_EFAILED;
    }
    TLOG("Initialize clulstermap client succeed.");
    return KS_SUCCESS;
}

int32_t Server::regCMClient() 
{
    if (!_cmClient) {
        return KS_EFAILED;
    }
    if (_cmClient->Register() != 0) {
        return KS_EFAILED;
    }
    if (_counter) {
        ServerWatchPoint *wp = new (std::nothrow) ServerWatchPoint(*_counter);
        if (unlikely(wp == NULL)) {
            TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
            return ENOMEM;
        }
        if (_cmClient->addWatchPoint(wp) != 0) {
            TWARN("add WatchPoint to clustermap failed.");
            delete wp;
        }
    }
    return KS_SUCCESS;
}

int32_t Server::subCMClient() 
{
    if (!_cmClient) {
        return KS_EFAILED;
    }
    if (_cmClient->Subscriber(0) != 0) {
        return KS_EFAILED;
    }
    return KS_SUCCESS;
}

int32_t Server::start(const char *path) 
{
    int32_t ret = 0;
    uint64_t session_timeout = 0;
    uint32_t max_parallel = 0;
    uint32_t timeout_cnt_limit = 0;
    uint32_t err_cnt_limit = 0;
    uint32_t maxThrCount = 0;
    uint32_t stackSize = 0;
    uint32_t max_requst_per_connection = 0;
    uint32_t max_request_overflow = 0;
    uint16_t port = 0;
    int service_count = 0;
    const char *worker_init_cfg = NULL;
    const char *cacheCfg;
    const char *prefix = NULL;
    ini_context_t cfg;
    const ini_section_t *grp = NULL;
    const ini_section_t *cm_grp = NULL;
    const ini_item_t *item;
    const ini_item_t *item_end;
    const char *httproot = NULL;
    char protocol[64];
    char server[64];
    COND_LOCK(_cond);
    if (_status == srv_startting) {
        TWARN("maybe another thread is startting this server now.");
        return KS_EFAILED;
    } 
    else if (_status == srv_startted) {
        TWARN("this server has been startted yet.");
        return KS_EFAILED;
    } 
    else if (_status == srv_stopping) {
        TWARN("maybe another thread is stopping this server now.");
        return KS_EFAILED;
    } 
    _status = srv_startting;
    TLOG("startting server(%p) by `%s'...", this, SAFE_STRING(path));
    ret = ini_load_from_file(path, &cfg);
    if (unlikely(ret != 0)) {
        TERR("parse config file `%s' error.", SAFE_STRING(path));
        return KS_EFAILED;
    }

    grp = &cfg.global;
    if (unlikely(grp == NULL)) {
        TERR("bad config file `%s'.", SAFE_STRING(path));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    max_parallel = get_int_value(grp, "max_parallel");
    session_timeout = (uint64_t)get_int_value(grp,
            "session_timeout")*(uint64_t)1000; //微秒
    _memLimit = (uint64_t)get_int_value(grp, "max_mempool_size");
    timeout_cnt_limit = get_int_value(grp, "seriate_timeout_count_limit");
    err_cnt_limit = get_int_value(grp, "seriate_error_count_limit");
    maxThrCount = get_int_value(grp, "max_thread_count");
    stackSize = get_int_value(grp, "thread_stackSize") * 1024L;
    max_requst_per_connection = get_int_value(grp, 
            "max_requst_per_connection");
    max_request_overflow = get_int_value(grp, "max_request_overflow");
    worker_init_cfg = ini_get_str_value1(grp, "worker_init_conf");
    cacheCfg = ini_get_str_value1(grp, "cache_init_conf");
    prefix = ini_get_str_value1(grp, "prefix");
    httproot = ini_get_str_value1(grp, "httproot");
    cm_grp = ini_get_section("clustermap", &cfg);

    if (_taskQueue == NULL) {
        _taskQueue = new (std::nothrow) TaskQueue;
        if (unlikely(_taskQueue == NULL)) {
            TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
            ini_destroy(&cfg);
            return ENOMEM;
        }
    }
    if (max_parallel > 0) {
        TLOG("setting parallel limit as %d.", max_parallel);
    } 
    else {
        TLOG("closing parallel limit.");
    }
    ret = initParallelLimitter(max_parallel, 
            session_timeout, timeout_cnt_limit, err_cnt_limit);
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("init parallel limitter[%d] error.", max_parallel);
        release();
        ini_destroy(&cfg);
        return ret;
    }
    _transport = new (std::nothrow) anet::Transport;
    if (unlikely(!_transport)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 2);
        release();
        ini_destroy(&cfg);
        return ENOMEM;
    }
    TLOG("startting cache handle...");
    if (_cache) {
        delete _cache;
        _cache = NULL;
    }
    if (!cacheCfg || strlen(cacheCfg) == 0) {
        TLOG("set no cache.");
    } 
    else {
        _cache = new (std::nothrow) MemcacheClient;
        if (unlikely(!_cache)) {
            TERR("out of memory at %s:%d.");
            release();
            ini_destroy(&cfg);
            return KS_ENOMEM;
        }
        if (_cache->initialize(cacheCfg) != KS_SUCCESS) {
            TERR("init cache handle by `%s' failed.", cacheCfg);
            delete _cache;
            _cache = NULL;
        }
        TLOG("startted cache handle by `%s'.", cacheCfg);
    }
    //start web probe
    if (httproot && httproot[0]) {
        _webProbe = new WebProbe();
        if (_webProbe->initialize(httproot) != KS_SUCCESS) {
            TERR("Initialize Web Probe error.");
            release();
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }

    if (!worker_init_cfg || strlen(worker_init_cfg) == 0) {
        worker_init_cfg = path;
    }
    TLOG("initilizing worker factory by `%s'...", 
            SAFE_STRING(worker_init_cfg));
    ret = initWorkerFactory(worker_init_cfg);
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("init worker by `%s' error.", SAFE_STRING(worker_init_cfg));
        release();
        ini_destroy(&cfg);
        return ret;
    }
    TLOG("startting work threads[%d] and dispatcher...", maxThrCount);
    ret = startThreads(maxThrCount, stackSize);
    if (unlikely(ret != KS_SUCCESS)) {
        TERR("start threads[%d] and dispatcher error.", maxThrCount);
        release();
        ini_destroy(&cfg);
        return ret;
    }
    if (_disp) {
        _disp->setTimeoutLimit(session_timeout);
    }
    
    TLOG("startting services...");
    grp = ini_get_section("services", &cfg);
    if (unlikely(grp == NULL)) {
        TERR("no service define in `%s'.", SAFE_STRING(path));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    item = ini_get_str_values_ex1(grp, "service", &service_count);
    if (service_count == 0) {
        TERR("no valid service define.");
        release();
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    item_end = item + service_count;
    service_count = 0;
    for (; item<item_end; item++) {
        if (!parse_service_config(item->value, protocol, sizeof(protocol), 
                    server, sizeof(server), port)) 
        {
            TERR("bad define of service[%s]", SAFE_STRING(item->value));
            continue;
        }
        ret = addService(protocol, server, port);
        if (ret != KS_SUCCESS) {
            TERR("create service[%s:%s:%d] error.", protocol, server, port);
            continue;
        }
        _services->setMaxRequest(max_requst_per_connection);
        _services->setMaxOverFlow(max_request_overflow);
        TLOG("startting service[%s:%s:%d], maxrequest=%d, maxoverflow=%d ...",
                protocol, server, port, 
                _services->getMaxRequest(), _services->getMaxOverFlow());
        service_count++;
    }
    if (unlikely(service_count == 0)) {
        TERR("no valid service define.");
        release();
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    for (Service *p = _services; p; p = p->next) {
        ret = p->setPrefix(prefix);
        if (ret != KS_SUCCESS) {
            TERR("set prefix[%s] of service error(%d).", 
                    SAFE_STRING(prefix), ret);
            release();
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        p->setCacheHandle(_cache);
    }
    TLOG("%d services startted.", service_count);
    
    if (cm_grp) {
        TLOG("initializing clustermap client...");
        ret = initCMClient(ini_get_str_value1(cm_grp, "local_config_path"),
                ini_get_str_value1(cm_grp, "cm_server"));
        if (ret != KS_SUCCESS) {
            TERR("initialize clustermap client failed[%d].", ret);
            release();
            ini_destroy(&cfg);
            return KS_EFAILED;
        } 
        else {
            TLOG("initialized clustermap client.");
        }
    }
    
    TLOG("startting ANET...");
    if (unlikely(!_transport->start())) {
        TERR("startting ANET error.");
        release();
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    _timeout = session_timeout;
    
    if (_cmClient) {
        ret = regCMClient();
        if (_type==srv_type_unknown || _type==srv_type_searcher) {
            if (ret != KS_SUCCESS) {
                TWARN("register clustermap client error.");
            } 
            else {
                TLOG("register clustermap client success.");
            }
        }
        else {
            if (ret != KS_SUCCESS) {
                TERR("register clustermap client error.");
                release();
                ini_destroy(&cfg);
                return KS_EFAILED;
            } 
            else {
                TLOG("register clustermap client success.");
                ret = subCMClient();
                if (ret != KS_SUCCESS) {
                    TERR("subscribe clustermap client error.");
                    release();
                    ini_destroy(&cfg);
                    return KS_EFAILED;
                }
                TLOG("subscribe clustermap client success.");
            }
        }
    }
    ini_destroy(&cfg);
    TLOG("server(%p) has been startted!", this);
    _status = srv_startted;
    COND_UNLOCK(_cond);
    return KS_SUCCESS;
}

void Server::release() 
{
    Service *p = NULL;
    if (_transport) {
        _transport->stop();
        _transport->wait();
        delete _transport;
        _transport = NULL;
    }
    if (_disp) {
        _disp->terminate();
    }
    if (_taskQueue) {
        _taskQueue->terminate();
    }
    if (_webProbe) {
        _webProbe->terminate();
    }
    if (_thrPool) {
        _thrPool->terminate();
    }
    if (_disp) {
        _disp->join();
    }
    if (_thrPool) {
        _thrPool->join();
    }
    if (_disp) {
        delete _disp;
        _disp = NULL;
    }
    if (_thrPool) {
        delete _thrPool;
        _thrPool = NULL;
    }
    if (_workerFactory) {
        delete _workerFactory;
        _workerFactory = NULL;
    }
    if (_counter) {
        delete _counter;
        _counter = NULL;
    }
    if (_taskQueue) {
        delete _taskQueue;
        _taskQueue = NULL;
    }
    while (_services) {
        p = _services->next;
        delete _services;
        _services = p;
    }
    /*
       if(_cache) {
       delete _cache;
       _cache = NULL;
       }
       */
    if (_cmClient) {	    
        delete _cmClient;		
        _cmClient = NULL;	
    }
    if (_webProbe) {
        delete _webProbe;
        _webProbe = NULL;
    }
}

int32_t Server::stop() 
{
    COND_LOCK(_cond);
    if (_status == srv_startting) {
        TWARN("this server is been startting now, stop latter.");
        return KS_EFAILED;
    } 
    else if (_status == srv_stopping) {
        TWARN("maybe another thread is stopping this server now.");
        return KS_EFAILED;
    } 
    else if (_status == srv_stopped) {
        TWARN("this server has been stopped yet.");
        return KS_SUCCESS;
    } 
    else if (_status != srv_startted) {
        TWARN("this server is not been startted yet.");
        return KS_EFAILED;
    }
    _status = srv_stopping;
    release();
    _status = srv_stopped;
    _cond.broadcast();
    COND_UNLOCK(_cond);
    TLOG("server(%p) has been stopped.", this);
    return KS_SUCCESS;
}

int32_t Server::wait() 
{
    COND_LOCK(_cond);
    while (_status == srv_startting || _status == srv_startted ||
            _status == srv_stopping) 
    {
        _cond.wait();
    }
    if (_status != srv_stopped) {
        return KS_EFAILED;
    }
    COND_UNLOCK(_cond);
    return KS_SUCCESS;
}

FRAMEWORK_END;

