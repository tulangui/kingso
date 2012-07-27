#include "FlowControl.h"
#include "framework/Request.h"
#include "framework/Session.h"
#include "clustermap/CMClient.h"
#include "util/Log.h"
#include "util/errcode.h"
#include "util/namespace.h"
#include "util/iniparser.h"
#include "RowLocalizer.h"
#include <dlfcn.h>

MERGER_BEGIN;

FlowControl::FlowControl()
{
    _server = 0;
    _mpFactory = 0;
    _connPool = 0;
    _row_localizer = 0;
    _plugin = 0;
    _name = 0;
    _handler = 0;
    _init = 0;
    _dest = 0;
    _make = 0;
    _done = 0;
    _ctrl = 0;
    _rqst = 0;
    _cmbn = 0;
    _frmt = 0;
}

FlowControl::~FlowControl()
{
    dest();
}

static uint32_t get_int_value(const ini_section_t *section, const char *key)
{
    int value = ini_get_int_value1(section, key, 0);
    if (value < 0) {
        return 0;
    }
    return value;
}

int FlowControl::init(const char* conf, FRAMEWORK::Server* server, MemPoolFactory* mpFactory)
{
    ini_context_t cfg;
    const ini_section_t* grp = 0;
    int ret = KS_EFAILED;
    const char* val = 0;
    const char* module_path = 0;

    if(!server || !mpFactory){
        TERR("server or mpFactory is null, return!");
        return KS_EFAILED;
    }
    _server = server;
    _mpFactory = mpFactory;

    // 加载配置merger_server.cfg
    if(unlikely(!conf)){
        TERR("init FlowControl error, conf path is null, return!");
        return KS_EFAILED;
    }
    ret = ini_load_from_file(conf, &cfg);
    if(unlikely(ret != 0)){
        TERR("init FlowControl error, conf path is %s, return!", conf);
        return KS_EFAILED;
    }
    grp = &cfg.global;
    if(unlikely(!grp)){
        TERR("init FlowControl error, conf path is %s, return!", conf);
        ini_destroy(&cfg);
        return KS_EFAILED;
    }

    // 配置connectionpool
    int conn_queue_limit = get_int_value(grp, "conn_queue_limit");
    int conn_queue_timeout = get_int_value(grp, "conn_queue_timeout");
    _connPool = new(std::nothrow) FRAMEWORK::ConnectionPool(*_server->getANETTransport(),
            conn_queue_limit, conn_queue_timeout);
    if (unlikely(_connPool==0)) {
        TERR("out of memory at %s:%d.", __FILE__, __LINE__ - 3);
        ini_destroy(&cfg);
        return KS_ENOMEM;
    }

    // 配置clustermap行选择器
    val = ini_get_str_value1(grp, "row_localizer");
    if(val && val[0]!='\0'){
        _row_localizer = is_assist::RowLocalizerFactory::make(val);
        if(!_row_localizer){
            TERR("invalid rowlocalizer define: %s.", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }

    // module path
    module_path = ini_get_str_value1(grp, "module_config_path");
    if(!module_path || module_path[0]==0){
        TERR("module config path is null, conf path is %s, return!", conf);
        ini_destroy(&cfg);
        return KS_EFAILED;
    }

    // 配置插件
    val = ini_get_str_value1(grp, "plugin_config_path");
    if(val && val[0]!='\0'){
        merger_plugin_name_f* name_func = 0;
        _plugin = dlopen(val, RTLD_NOW);
        if(!_plugin){
            TERR("dlopen error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        name_func = (merger_plugin_name_f*)dlsym(_plugin, "name");
        if(!name_func){
            TERR("dlsym name function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _init = (merger_plugin_init_f*)dlsym(_plugin, "init");
        if(!_init){
            TERR("dlsym init function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _dest = (merger_plugin_dest_f*)dlsym(_plugin, "dest");
        if(!_dest){
            TERR("dlsym dest function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _make = (merger_plugin_make_f*)dlsym(_plugin, "make");
        if(!_make){
            TERR("dlsym make function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _done = (merger_plugin_done_f*)dlsym(_plugin, "done");
        if(!_done){
            TERR("dlsym done function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _ctrl = (merger_plugin_ctrl_f*)dlsym(_plugin, "ctrl");
        if(!_ctrl){
            TERR("dlsym ctrl function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _rqst = (merger_plugin_rqst_f*)dlsym(_plugin, "rqst");
        if(!_rqst){
            TERR("dlsym rqst function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _cmbn = (merger_plugin_cmbn_f*)dlsym(_plugin, "cmbn");
        if(!_cmbn){
            TERR("dlsym cmbn function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _frmt = (merger_plugin_frmt_f*)dlsym(_plugin, "frmt");
        if(!_frmt){
            TERR("dlsym frmt function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _ofmt = (merger_plugin_ofmt_f*)dlsym(_plugin, "ofmt");
        if(!_ofmt){
            TERR("dlsym ofmt function error, error: %s, path: %s.", dlerror(), val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _name = name_func();
        if(!_name){
            TERR("get plugin name error, path: %s.", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _handler = _init(module_path);
        if(!_handler){
            TERR("init function error, path: %s.", module_path);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }
    else{
        TERR("no plugin config.");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }

    ini_destroy(&cfg);

    return KS_SUCCESS;
}

void FlowControl::dest()
{
    if(_connPool){
        delete _connPool;
        _connPool = 0;
    }
    if(_row_localizer){
        is_assist::RowLocalizerFactory::recycle(_row_localizer);
        _row_localizer = 0;
    }
    if(_plugin){
        if(_handler){
            _dest(_handler);
        }
        dlclose(_plugin);
        _plugin = 0;
        _name = 0;
        _handler = 0;
        _init = 0;
        _dest = 0;
        _make = 0;
        _done = 0;
        _ctrl = 0;
        _rqst = 0;
        _cmbn = 0;
        _frmt = 0;       
        _ofmt = 0;
    }
}

const char* FlowControl::name()
{
    return _name;
}

void* FlowControl::make(const char* query, const int qsize, FRAMEWORK::LogInfo *pLogInfo)
{
    return _make(query, qsize, pLogInfo, _handler, _mpFactory);
}

void FlowControl::done(void* mydata)
{
    return _done(mydata);
}

int FlowControl::ctrl(void* mydata)
{
    return _ctrl(mydata);
}

int FlowControl::rqst(void* mydata, FRAMEWORK::Request& req)
{
    clustermap::enodetype nodetype;
    const char* query = _rqst(mydata, &nodetype);
    int qsize = 0;
    uint32_t locating_signal = 0;
    int32_t* ids = 0;
    int32_t cnt = 0;
    int i = 0;
    clustermap::CMISNode** nodes = 0;
    int count = 0;
    FRAMEWORK::ANETRequest* anetreq = 0;
    clustermap::CMClient* cm = _server->getCMClient();

    if(!query){
        TWARN("get request query error, return!");
        return -1;
    }
    qsize = strlen(query);

    if(_row_localizer){
        locating_signal = _row_localizer->getLocatingSignal(query, qsize);
    }

    // 申请节点，返回节点id
    if(cm->allocSubRow(ids, cnt, nodetype, 1, clustermap::GE, locating_signal)){
        TWARN("alloc subrow from clustermap error, return!");
        return -1;
    }    
    if(!ids){
        TWARN("alloc subrow from clustermap error, return!");
        return -1;
    }    
    if(cnt==0){
        cm->freeSubRow(ids);
        return 0;
    }    

    // 填充节点信息
    nodes = (clustermap::CMISNode**)malloc(sizeof(clustermap::CMISNode*)*cnt);
    if(!nodes){
        TWARN("new clustermap nodes error, return!");
        cm->freeSubRow(ids);
        return -1;
    }    
    for(i=0; i<cnt; i++) {
        nodes[i] = cm->getNodeInfo(ids[i]);
    }    
    count = cnt; 
    cm->freeSubRow(ids);

    // 填充每个下游节点的请求到总体请求结构
    for(i=0; i<count; i++){
        if(!nodes[i]){
            continue;
        }
        anetreq = new (std::nothrow) FRAMEWORK::ANETRequest(*_connPool);
        if(!anetreq) {
            free(nodes);
            return -1;
        }
        if(anetreq->setQuery(query, qsize)){
            free(nodes);
            delete anetreq;
            return -1;
        }
        anetreq->setRequestNode(nodes[i], 0);
        if(req.addANETRequest(anetreq)){
            free(nodes);
            delete anetreq;
            return -1;
        }
    }
    free(nodes);    

    return 0;
}

int FlowControl::cmbn(void* mydata, const char* inputs[], const int inlens[], const int incnt)
{
    return _cmbn(mydata, inputs, inlens, incnt);
}

int FlowControl::frmt(void* mydata, char* &output, int &outlen)
{
    return _frmt(mydata, &output, &outlen);
}

const char* FlowControl::ofmt(void* mydata)
{
    return _ofmt(mydata);
}

MERGER_END;
