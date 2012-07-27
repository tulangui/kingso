#include "MergerServer.h"
#include "framework/Request.h"
#include "util/Log.h"
#include "util/errcode.h"
#include "util/namespace.h"
#include "util/iniparser.h"
#include <stdlib.h>
#include <string.h>
#include <map>
#include <new>
#include <dlfcn.h>


using namespace std;


MERGER_BEGIN;

MergerWorker::MergerWorker(
    // 会话对象类
    FRAMEWORK::Session& session,
    // 流程控制插件管理类
    FlowControl& flowControl,
    // 内存池工
    MemPoolFactory& mpFactory,
    // 任务队列
    FRAMEWORK::TaskQueue& taskQueue
):
    Worker(session), 
    _flowControl(flowControl),
    _mpFactory(mpFactory),
    _taskQueue(taskQueue)
{
}

MergerWorker::~MergerWorker()
{
}

// 主处理流程
int32_t MergerWorker::run()
{
    FRAMEWORK::session_status_t status = _session.getStatus();

    // 处理超时
    if(status==FRAMEWORK::ss_phase1_timeout ||
       status==FRAMEWORK::ss_phase2_timeout ||
       status==FRAMEWORK::ss_timeout){
        handleTimeout();
        return KS_SUCCESS;
    }
    // 处理收到的新请求
    else if(status==FRAMEWORK::ss_arrive){
        if(handleRequest()==KS_SUCCESS){
            return KS_SUCCESS;
        }
    }
    // 处理收到的searcher返回结果
    else if(status==FRAMEWORK::ss_phase1_send){
        if(handleResponse()==KS_SUCCESS){
            return KS_SUCCESS;
        }
    }
    
    _session.setStatus(FRAMEWORK::ss_error);
    _session.response();
    
    return KS_EFAILED;
}

// 处理超时
void MergerWorker::handleTimeout()
{
    FRAMEWORK::Query& query = _session.getQuery();
    const char* pureString = query.getOrigQueryData();
    int pureSize = query.getOrigQuerySize();

    do{
        void* session_data = 0;
        char* output = 0;
        int outlen = 0;
        char* tmp = 0;
        int len = 0;
        if(!pureString || pureSize==0){
            break;
        }
        session_data = _session.getArgs();
        if(session_data == NULL){
            session_data = _flowControl.make(pureString, pureSize, &_session._logInfo);
            if(session_data == NULL){
                break;
            }
        }
        if(_flowControl.frmt(session_data, output, outlen)){
            _flowControl.done(session_data);
            break;
        }
        // 结果一定要是malloc出来的
        tmp = (char*)malloc(outlen);
        if(!tmp){
            _flowControl.done(session_data);
            break;
        }
        memcpy(tmp, output, outlen);
        output = tmp;
        // 返回查询结果给上游
        _session.setResponseData(output, outlen);
        _flowControl.done(session_data);
    }while(0);

    _session.setStatus(FRAMEWORK::ss_error);
    _session.response();
}

// 处理查询请求
int32_t MergerWorker::handleRequest()
{
    FRAMEWORK::Query& query = _session.getQuery();
    const char* pureString = query.getOrigQueryData();
    int pureSize = query.getOrigQuerySize();
    void* session_data = 0;
    FRAMEWORK::Request* req = 0;
    int ret = KS_EFAILED;

    if(!pureString || pureSize==0){
        makeResponse();
        return KS_EFAILED;
    }
    _session.setType(FRAMEWORK::st_phase1_only);
    _session._logInfo._eRole = FRAMEWORK::sr_full;       
    _session.extractSource();
    session_data = _flowControl.make(pureString, pureSize, &_session._logInfo);
    if(!session_data){
        TWARN("make FlowControl data error, return!");
        makeResponse();
        return KS_EFAILED;
    }
    _session.setArgs(session_data);

    // 设置结果输出格式
    //_session.setDflFmt(_flowControl.ofmt(session_data));

    _flowControl.ctrl(session_data);

    // 制作下游查询请求
    req = new (std::nothrow) FRAMEWORK::Request(_session, _taskQueue);
    if(!req){
        TWARN("new request error, return!");
        makeResponse();
        _flowControl.done(session_data);
        return KS_EFAILED;
    }
    _session.setCurrentRequest(req);
    if(_flowControl.rqst(session_data, *req)){
        TWARN("fill request error, return!");
        makeResponse();
        _flowControl.done(session_data);
        return KS_EFAILED;
    }
    if(req->size()==0){
        TWARN("request server size is 0, return!");
        makeResponse();
        _flowControl.done(session_data);
        return KS_EFAILED;
    }

    // 发送查询请求到下游
    _session.setStatus(FRAMEWORK::ss_phase1_send);
    ret = req->request();
    if(ret!=KS_SUCCESS){
        TWARN("send request error, return!");
        makeResponse();
        _flowControl.done(session_data);
        return ret;
    }
    return KS_SUCCESS;
}

// 处理返回结果
int32_t MergerWorker::handleResponse()
{
    FRAMEWORK::Request* req = _session.getCurrentRequest();
    void* session_data = _session.getArgs();
    MemPool *heap = 0;
    FRAMEWORK::Compressor* compr = 0;
    char** inputs = 0;
    int* inlens = 0;
    int32_t ret = KS_EFAILED;
    FRAMEWORK::ANETResponse *res = 0;
    FRAMEWORK::ANETRequest *r = 0;
    int i = 0;
 
    // 检查请求状态
    if (!req || req->getStatus()!=FRAMEWORK::rt_responsed) {
        TWARN("servers not response, return!");
        makeResponse();
        _flowControl.done(session_data);
        return KS_EFAILED;
    }
 
    // 获取内存池
    heap = _mpFactory.make();
    if (unlikely(!heap)) {
        TWARN("get MemPool from MemPoolFactory error, return!");
        makeResponse();
        _flowControl.done(session_data);
        return KS_EFAILED;
    }
 
    // 获取压缩器
    compr = FRAMEWORK::CompressorFactory::make(FRAMEWORK::ct_zlib, heap);
    if(!compr){
        TWARN("get Compressor from CompressorFactory error, return!");
        makeResponse();
        _flowControl.done(session_data);
        _mpFactory.recycle(heap);
        return KS_EFAILED;
    }
 
    // 申请下游节点返回数据数组
    inputs = NEW_VEC(heap, char*, req->size());
    if(!inputs){
        TWARN("NEW_VEC inputs error, return!");
        makeResponse();
        _flowControl.done(session_data);
        FRAMEWORK::CompressorFactory::recycle(compr);
        _mpFactory.recycle(heap);
        return KS_ENOMEM;
    }
    inlens = NEW_VEC(heap, int, req->size());
    if(!inlens){
        TWARN("NEW_VEC inlens error, return!");
        makeResponse();
        _flowControl.done(session_data);
        FRAMEWORK::CompressorFactory::recycle(compr);
        _mpFactory.recycle(heap);
        return KS_ENOMEM;
    }
 
    // 顺序获取每个下游节点的数据
    for (i=0; i<req->size(); i++) {
        r = req->getANETRequest(i);
        if(!r || !(res=r->getResponse()) || !res->getData() || res->getSize()==0){
            inputs[i] = 0;
            inlens[i] = 0;
            continue;
        }
        ret = compr->uncompress(res->getData(), res->getSize(), inputs[i], (uint32_t&)inlens[i]);
        if(ret!=KS_SUCCESS || inputs[i]==0 || inlens[i]==0){
            TWARN("uncompress data error, return!");
            makeResponse();
            _flowControl.done(session_data);
            FRAMEWORK::CompressorFactory::recycle(compr);
            _mpFactory.recycle(heap);
            return KS_EFAILED;
        }
    }
    FRAMEWORK::CompressorFactory::recycle(compr);

    // 检查是否有数据返回
    for(i=0; i<req->size(); i++){
        if(inputs[i]!=0 && inlens[i]!=0){
            break;
        }
    }
    if(i==req->size()){
        TWARN("all server no response, return!");
        makeResponse();
        _flowControl.done(session_data);
        _mpFactory.recycle(heap);
        return KS_EFAILED;
    }

    // 调用插件提供的结果合并函数
    if(_flowControl.cmbn(session_data, (const char**)inputs, inlens, req->size())){
        TWARN("combine result error, return!");
        makeResponse();
        _flowControl.done(session_data);
        _mpFactory.recycle(heap);
        return KS_EFAILED;
    }

    ret = _flowControl.ctrl(session_data);
    if(ret>0){
        _mpFactory.recycle(heap);
        // 制作下游查询请求
        req = new (std::nothrow) FRAMEWORK::Request(_session, _taskQueue);
        if(!req){
            TWARN("new request error, return!");
            makeResponse();
            _flowControl.done(session_data);
            return KS_EFAILED;
        }
        _session.setCurrentRequest(req);
        if(_flowControl.rqst(session_data, *req)){
            TWARN("fill request error, return!");
            makeResponse();
            _flowControl.done(session_data);
            return KS_EFAILED;
        }
        if(req->size()==0){
            TWARN("request server size is 0, return!");
            makeResponse();
            _flowControl.done(session_data);
            return KS_EFAILED;
        }
        // 发送查询请求到下游
        ret = req->request();
        if(ret!=KS_SUCCESS){
            TWARN("send request error, return!");
            makeResponse();
            _flowControl.done(session_data);
            return ret;
        }
    }
    else if(ret==0){
        makeResponse();
        _session.response();
        _mpFactory.recycle(heap);
        _flowControl.done(session_data);
    } 
    else{
        TWARN("flow error, return!");
        makeResponse();
        _flowControl.done(session_data);
        _mpFactory.recycle(heap);
        return KS_EFAILED;
    }

    return KS_SUCCESS;
}

void MergerWorker::makeResponse()
{
    void* session_data = _session.getArgs();
    char* output = 0;
    int outlen = 0;
    char* tmp = 0;
    int len = 0;

    if(!session_data){
        return;
    }

    if(_flowControl.frmt(session_data, output, outlen)){
        return;
    }
    // 结果一定要是malloc出来的
    tmp = (char*)malloc(outlen);
    if(!tmp){
        return;
    }
    memcpy(tmp, output, outlen);
    output = tmp;
    // 返回查询结果给上游
    _session.setResponseData(output, outlen);
}


MergerWorkerFactory::MergerWorkerFactory() 
    :
    _ready(false)
{
}

MergerWorkerFactory::~MergerWorkerFactory()
{
    _ready = false;
}

// Worker工厂资源初始化
int32_t MergerWorkerFactory::initilize(const char* path)
{
    ini_context_t cfg;
    const ini_section_t* grp = 0;
    const char* key = NULL;
    const char* val = NULL;
    const char* val2 = NULL;
    const char* helper_val = 0;
    int32_t ret = KS_EFAILED;

    // 状态检查
    if(_ready){
        TLOG("MergerWorkerFactory has been initialized yet.");
        return KS_SUCCESS;
    }
    if(unlikely(!_server || !_server->getANETTransport() || !_server->getTaskQueue())){
        TERR("initialize MergerWorkerFactory error.");
        return KS_EFAILED;
    }
    _server->_type = FRAMEWORK::srv_type_merger;
    
    // 加载配置merger_server.cfg
    if(unlikely(!path)){
        TERR("initialize MergerWorkerFactory by `%s' error.", SAFE_STRING(path));
        return KS_EFAILED;
    }
    ret = ini_load_from_file(path, &cfg);
    if(unlikely(ret != 0)){
        TERR("initialize MergerWorkerFactory by `%s' error.", SAFE_STRING(path));
        return KS_EFAILED;
    }
    grp = &cfg.global;
    if(unlikely(!grp)){
        TERR("invalid config file `%s' for MergerWorkerFactory.", SAFE_STRING(path));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }

    // 配置流程控制插件
    if(_flowControl.init(path, _server, &_mpFactory)){
        TERR("init FlowControl by %s error, return!", SAFE_STRING(path));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    
    ini_destroy(&cfg);
    _ready = true;
    
    return KS_SUCCESS;
}

// 生成Worker对象
FRAMEWORK::Worker* MergerWorkerFactory::makeWorker(FRAMEWORK::Session & session)
{
    return (_ready && _server && _server->getCMClient()) ? 
        new (std::nothrow) MergerWorker(session, 
                _flowControl,
                _mpFactory,
                *(_server->getTaskQueue())) :
        NULL;
    return NULL;
}

FRAMEWORK::WorkerFactory* MergerServer::makeWorkerFactory()
{
    return new (std::nothrow) MergerWorkerFactory;
}

FRAMEWORK::Server* MergerCommand::makeServer()
{
    return new (std::nothrow) MergerServer;
}

MERGER_END



