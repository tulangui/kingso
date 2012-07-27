#include "util/Log.h"
#include "queryparser/QueryRewriterResult.h"
#include "queryparser/QueryRewriter.h"
#include "queryparser/QPResult.h"
#include "mxml.h"
#include "util/kspack.h"
#include "util/py_url.h"
#include "sort/SortResult.h"
#include "commdef/ClusterResult.h"
#include "statistic/StatisticResult.h"
#include "util/py_code.h"
#include "sort/MergerSortFacade.h"
#include "statistic/StatisticManager.h"
#include "framework/Request.h"
#include "framework/Session.h"
#include "clustermap/CMClient.h"
#include "KSQuery.h"
#include "merger.h"
#include "MemPoolFactory.h"
#include "ResultFormat.h"
#include "RowLocalizer.h"
#include "DebugInfo.h"
#include "Document.h"
#include "util/strfunc.h"
#include "dlmodule/DlModuleInterface.h"
#include "dlmodule/DlModuleManager.h"
#include "plugin/DocumentRewrite.h"
#include "plugin/string_split.h"


using namespace MERGER;
using namespace blender_interface;

#define FIELDBUFFCOUNT 10


// query类型
enum
{
    KS_TYPE_NORMAL,
    KS_TYPE_UNIQ1,
    KS_TYPE_UNIQ2,
    KS_TYPE_UNIQ2_SEP,
};

// 执行步骤定义
enum
{
    KS_STEP_NEW, 
    KS_STEP_SEARCH,
    KS_STEP_UNIQ,
    KS_STEP_DETAIL,
    KS_STEP_FINISH,
};

// 结果输出格式
enum
{
    KS_OFMT_PBP,
    KS_OFMT_XML,
    KS_OFMT_V30,
    KS_OFMT_KSP,
};

// 插件资源
typedef struct
{
    queryparser::QueryRewriter* qprewriter;
    SORT::MergerSortFacade* msort;
    statistic::StatisticManager* mstat;
    ResultFormatFactory* rffactory;
    int ofmt;
    mxml_node_t* xmlroot;
}
handler_t;

// 插件数据
typedef struct
{
    MemPoolFactory* memfactory;
    MemPool* heap;
    handler_t* handler;
    char* query;
    int qsize;
    KSQuery* ksquery;
    commdef::ClusterResult* result;
    FRAMEWORK::Context* context;
    FRAMEWORK::LogInfo* pLogInfo;

    int type;
    int step;
    int ofmt;

    char* step_query;
    int step_qsize;
    int orig_s;
    int orig_n;
    char* uniqinfo;
    uint64_t time_start;
    uint64_t time_cost;
    commdef::ClusterResult* frmt_result;
}
mydata_t;


#ifdef __cplusplus
extern "C" {
#endif

#define MAXLATLNGSIZE 100

// 插件名获取方法
extern const char* name()
{
    static const char myname[] = "mainse";
    return myname;
}

// 插件资源初始化方法
extern void dest(void* handler);
extern void* init(const char* path)
{
    handler_t* handler = 0;
    FILE* conf_file = 0;
    mxml_node_t* root = 0;
    mxml_node_t* ofmt_conf = 0;
    const char* ofmt_type = 0;
    mxml_node_t* sort_root = 0;
    mxml_node_t* sort_conf = 0;
    const char* sort_conf_path = 0;
    mxml_node_t* module_plugin_conf = NULL;
    const char* module_plugin = NULL;

    if(!path){
        TERR("module config path is null, return!");
        return 0;
    }

    handler = (handler_t*)malloc(sizeof(handler_t));
    if(!handler){
        TERR("malloc handler error, return!");
        return 0;
    }
    handler->xmlroot = NULL;
    handler->qprewriter = new queryparser::QueryRewriter();
    handler->msort = new SORT::MergerSortFacade();
    handler->mstat = new statistic::StatisticManager();
    handler->rffactory = new ResultFormatFactory();
    if(!handler->qprewriter || !handler->msort || !handler->mstat || !handler->rffactory){
        TERR("new handler member error, return!");
        if(handler->qprewriter) delete handler->qprewriter;
        if(handler->msort) delete handler->msort;
        if(handler->mstat) delete handler->mstat;
        if(handler->rffactory) delete handler->rffactory;
        free(handler);
        return 0;
    }

    // 读取module配置文件
    conf_file = fopen(path, "r");
    if(!conf_file){
        TERR("open module config fill error, return![path:%s]", path);
        dest(handler);
        return 0;
    }
    root = mxmlLoadFile(0, conf_file, MXML_NO_CALLBACK);
    if(!root){
        TERR("load module config fill error, return![path:%s]", path);
        dest(handler);
        return 0;
    }
    fclose(conf_file);

    // 配置outfmt
    ofmt_conf = mxmlFindElement(root, root, "outfmt", 0, 0, MXML_DESCEND);
    if(!ofmt_conf){
        TNOTE("default outfmt is pb.");
        handler->ofmt = KS_OFMT_PBP;
    }
    else{
        ofmt_type = mxmlElementGetAttr(ofmt_conf, "type");
        if(ofmt_type){
            if(strcasecmp(ofmt_type, "pb")==0){
                TNOTE("default outfmt is pb.");
                handler->ofmt = KS_OFMT_PBP;
            }
            else if(strcasecmp(ofmt_type, "xml")==0){
                TNOTE("default outfmt is xml.");
                handler->ofmt = KS_OFMT_XML;
            }
            else if(strcasecmp(ofmt_type, "v30")==0 || strcasecmp(ofmt_type, "v3")==0){
                TNOTE("default outfmt is v3.");
                handler->ofmt = KS_OFMT_V30;
            }
            else if(strcasecmp(ofmt_type, "kspack")==0){
                TNOTE("default outfmt is kspack.");
                handler->ofmt = KS_OFMT_KSP;
            }
            else{
                handler->ofmt = KS_OFMT_PBP;
            }
        }
        else{
            TNOTE("default outfmt is pb.");
            handler->ofmt = KS_OFMT_PBP;
        }
    }

    // 配置queryparser
    if(handler->qprewriter->init(root)){
        TERR("init queryparser error, return![path:%s]", path);
        dest(handler);
        mxmlDelete(root);
        return 0;
    }

    // 配置sort
    sort_root = mxmlFindElement(root, root, "sort", 0, 0, MXML_DESCEND);
    if(!sort_root){
        TERR("find sort node in module config error, return![path:%s]", path);
        dest(handler);
        mxmlDelete(root);
        return 0;
    }
    sort_conf = mxmlFindElement(sort_root, sort_root, "sort_config", 0, 0, MXML_DESCEND_FIRST);
    if(!sort_conf){
        TERR("find sort conf in module config error, return![path:%s]", path);
        dest(handler);
        mxmlDelete(root);
        return 0;
    }
    sort_conf_path = mxmlElementGetAttr(sort_conf, "path");
    if(!sort_conf_path){
        TERR("get sort conf path in module config error, return![path:%s]", path);
        dest(handler);
        mxmlDelete(root);
        return 0;
    }
    if(handler->msort->init(sort_conf_path)){
        TERR("init sort error, return![path:%s]", path);
        dest(handler);
        mxmlDelete(root);
        return 0;
    }
    //读取插件配置
    module_plugin_conf = mxmlFindElement(root, root, "module_plugin_conf", 0, 0, MXML_DESCEND);
    if(!module_plugin_conf){
        TNOTE("module_plugin is null.");
    }
    else{
        module_plugin = mxmlElementGetAttr(module_plugin_conf, "path");
        if(module_plugin && (module_plugin[0]!='\0')) {
            dlmodule_manager::CDlModuleManager::getModuleManager()->parse(module_plugin);
            DocumentRewrite::getInstance().parse(module_plugin);
        }
        else {
            TWARN("Dynamic module conf is not set. If you need it, check %s",
                    SAFE_STRING(path));
        }
    }
    handler->xmlroot = root;

    return handler;
}

// 插件资源释放方法
extern void dest(void* handler)
{
    if(handler){
        handler_t* tmphandler = (handler_t*)handler;
        delete tmphandler->qprewriter;
        delete tmphandler->msort;
        delete tmphandler->mstat;
        delete tmphandler->rffactory;
        if (tmphandler->xmlroot) mxmlDelete(tmphandler->xmlroot);
        free(handler);
    }
}

// 插件数据初始化方法
static uint64_t time_now();
extern void* make(const char* query, const int qsize, FRAMEWORK::LogInfo *ploginfo, void* handler, MemPoolFactory* memfactory)
{
    mydata_t* mydata = 0;
    MemPool* heap = 0;
    char* query_buf = 0;
    KSQuery* ksquery = 0;
    commdef::ClusterResult* result = 0;
    FRAMEWORK::Context* context = 0;

    const char* uniq_field = 0;
    const char* uniq_info = 0;
    const char* separate = 0;
    const char* outfmt = 0;

    // 检查参数
    if(!query || qsize==0){
        TWARN("query is null, return!");
        return 0;
    }
    if(!handler){
        TWARN("handler is null, return!");
        return 0;
    }
    if(!memfactory){
        TWARN("MemPoolFactory is null, return!");
        return 0;
    }

    heap = memfactory->make();
    if(!heap){
        TWARN("alloc MemPool error, return!");
        return 0;
    }

    // 申请mydata结构体资源
    mydata = NEW(heap, mydata_t);
    if(!mydata){
        TWARN("new mydata error, return!");
        memfactory->recycle(heap);
        return 0;
    }
    memset(mydata, 0x0, sizeof(mydata_t));
    query_buf = NEW_VEC(heap, char, qsize+1);
    if(!query_buf){
        TWARN("new query buffer error, return!");
        memfactory->recycle(heap);
        return 0;
    }
    memcpy(query_buf, query, qsize);
    query_buf[qsize] = 0;
    ksquery = NEW(heap, KSQuery)(query, heap);
    if(!ksquery){
        TWARN("new KSQuery error, return!");
        memfactory->recycle(heap);
        return 0;
    }
    result = NEW(heap, commdef::ClusterResult);
    if(!result){
        TWARN("new ClusterResult error, return!");
        memfactory->recycle(heap);
        return 0;
    }
    memset(result, 0x0, sizeof(commdef::ClusterResult));
    context = NEW(heap, FRAMEWORK::Context);
    if(!context){
        TWARN("new Context error, return!");
        memfactory->recycle(heap);
        return 0;
    }
    context->setMemPool(heap);

    // 填充结构体 
    mydata->memfactory = memfactory;
    mydata->heap = heap;
    mydata->handler = (handler_t*)handler;
    mydata->query = query_buf;
    mydata->qsize = qsize;
    mydata->ksquery = ksquery;
    mydata->result = result;
    mydata->context = context;
    mydata->pLogInfo = ploginfo;
    uniq_field = mydata->ksquery->getParam("uniqfield");
    uniq_info = mydata->ksquery->getParam("uniqinfo");
    separate = mydata->ksquery->getParam("separate");
    if(uniq_field){
        if(uniq_info){
            if(separate && (strcasecmp(separate, "yes")==0 || strcasecmp(separate, "true")==0)){
                mydata->type = KS_TYPE_UNIQ2_SEP;
            }
            else{
                mydata->type = KS_TYPE_UNIQ2;
            }
        }
        else{
            mydata->type = KS_TYPE_UNIQ1;
        }
    }
    else{
        mydata->type = KS_TYPE_NORMAL;
    }
    mydata->step = KS_STEP_NEW;
    outfmt = mydata->ksquery->getParam("outfmt");
    if(outfmt){
        if(strcasecmp(outfmt, "pb")==0){
            mydata->ofmt = KS_OFMT_PBP;
        }
        else if(strcasecmp(outfmt, "xml")==0){
            mydata->ofmt = KS_OFMT_XML;
        }
        else if(strcasecmp(outfmt, "v30")==0 || strcasecmp(outfmt, "v3")==0){
            mydata->ofmt = KS_OFMT_V30;
        }
        else if(strcasecmp(outfmt, "kspack")==0){
            mydata->ofmt = KS_OFMT_KSP;
        }
        else{
            mydata->ofmt = mydata->handler->ofmt;
        }
    }
    else{
        mydata->ofmt = mydata->handler->ofmt;
    }

    mydata->time_start = time_now();
    mydata->frmt_result = 0;

    return mydata;
}

// 插件数据释放方法
extern void done(void* mydata)
{
    if(mydata){
        MemPoolFactory* memfactory = ((mydata_t*)mydata)->memfactory;
        MemPool* heap = ((mydata_t*)mydata)->heap;
        memfactory->recycle(heap);
    }
}

// 插件流程控制方法
extern int ctrl(void* mydata)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    if(!mydata){
        TWARN("mydata is null, return!");
        return -1;
    }
    commdef::ClusterResult* cr = mydata_p->result;
    switch(mydata_p->step){
        case KS_STEP_NEW :
            TDEBUG("session step KS_STEP_NEW, query=%s", mydata_p->query);
            mydata_p->step = KS_STEP_SEARCH;
            TDEBUG("session next step KS_STEP_SEARCH, query=%s", mydata_p->query);
            return 1;
        case KS_STEP_SEARCH :
            TDEBUG("session step KS_STEP_SEARCH, query=%s", mydata_p->query);
            if(cr->_nDocsReturn==0){
                mydata_p->time_cost = time_now()-mydata_p->time_start;
                mydata_p->frmt_result = mydata_p->result;
                return 0;
            }
            if(mydata_p->type==KS_TYPE_UNIQ2_SEP){
                mydata_p->step = KS_STEP_UNIQ;
                TDEBUG("session next step KS_STEP_UNIQ, query=%s", mydata_p->query);
            }
            else{
                mydata_p->step = KS_STEP_DETAIL;
                TDEBUG("session next step KS_STEP_DETAIL, query=%s", mydata_p->query);
            }
            return 1;
        case KS_STEP_UNIQ :
            TDEBUG("session step KS_STEP_UNIQ, query=%s", mydata_p->query);
            if(cr->_nDocsReturn==0){
                mydata_p->time_cost = time_now()-mydata_p->time_start;
                mydata_p->frmt_result = mydata_p->result;
                return 0;
            }
            mydata_p->step = KS_STEP_DETAIL;
            TDEBUG("session next step KS_STEP_DETAIL, query=%s", mydata_p->query);
            return 1;
        case KS_STEP_DETAIL :
            TDEBUG("session step KS_STEP_DETAIL, query=%s", mydata_p->query);
            mydata_p->step = KS_STEP_FINISH;
            TDEBUG("session next step KS_STEP_FINISH, query=%s", mydata_p->query);
        case KS_STEP_FINISH :
        default :
            mydata_p->time_cost = time_now()-mydata_p->time_start;
            mydata_p->frmt_result = mydata_p->result;
            return 0;
    }
}

// 插件请求申请方法
static const char* rqst_search(void* mydata, clustermap::enodetype* nodetype);
static const char* rqst_detail(void* mydata, clustermap::enodetype* nodetype);
extern const char* rqst(void* mydata, clustermap::enodetype* nodetype)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    if(!mydata){
        TWARN("mydata is null, return!");
        return 0;
    }
    switch(mydata_p->step){
        case KS_STEP_SEARCH :
            return rqst_search(mydata, nodetype);
        case KS_STEP_DETAIL :
            return rqst_detail(mydata, nodetype);
        default :
            return 0;
    }
}

// 结果集合并方法
static int cmbn_search(void* mydata, const char* inputs[], const int inlens[], const int incnt);
static int cmbn_detail(void* mydata, const char* inputs[], const int inlens[], const int incnt);
extern int cmbn(void* mydata, const char* inputs[], const int inlens[], const int incnt)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    if(!mydata){
        TWARN("mydata is null, return!");
        return -1;
    }
    switch(mydata_p->step){
        case KS_STEP_SEARCH :
            return cmbn_search(mydata, inputs, inlens, incnt);
        case KS_STEP_DETAIL :
            return cmbn_detail(mydata, inputs, inlens, incnt);
        default :
            return -1;
    }
}

// 插件结果格式化方法
static int frmt_pbp(void* mydata, char** output, int* outlen);
static int frmt_xml(void* mydata, char** output, int* outlen);
static int frmt_v30(void* mydata, char** output, int* outlen);
static int frmt_ksp(void* mydata, char** output, int* outlen);
extern int frmt(void* mydata, char** output, int* outlen)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    if(!mydata){
        TWARN("mydata is null, return!");
        return -1;
    }
    uint32_t cnt = mydata_p->result->_nDocsReturn;
    Document **ppdocs = mydata_p->result->_ppDocuments;
    //DocumentRewrite
    if ((DocumentRewrite::getInstance().size()>0)&&(cnt>0)&&(ppdocs!=NULL)) {
        DocumentRewrite& rewirter = DocumentRewrite::getInstance();
        blender_interface::DRWParam param;
        param.query_string = mydata_p->query;
        param.query_size = mydata_p->qsize;
        param.m_iSubStart = mydata_p->orig_s;
        param.m_iSubNum = mydata_p->orig_n;
        int32_t lightkey_size = 0;
        int32_t cost_size = 0;
        int32_t len = 0;
        param.match_keywords = NULL;
        param.matchkeyword_size = 0;
        if (cnt > 0) {
            if (DocumentRewrite::getInstance().rewrite(
                        ppdocs, cnt, param) != KS_SUCCESS)
            {
                return -1;
            }
            mydata_p->result->_nDocsReturn = cnt;
        } else {
            ppdocs = NULL;
            DocumentRewrite::getInstance().rewrite(ppdocs, cnt, param);
        }
    }
    //格式化输出结果
    switch(mydata_p->ofmt){
        case KS_OFMT_PBP :
            TDEBUG("outfmt is pb.");
            return frmt_pbp(mydata, output, outlen);
        case KS_OFMT_XML :
            TDEBUG("outfmt is xml.");
            return frmt_xml(mydata, output, outlen);
        case KS_OFMT_V30 :
            TDEBUG("outfmt is v3.");
            return frmt_v30(mydata, output, outlen);
        case KS_OFMT_KSP :
            TDEBUG("outfmt is kspack.");
            return frmt_ksp(mydata, output, outlen);
        default :
            TDEBUG("outfmt is pb.");
            return frmt_pbp(mydata, output, outlen);
    }
}

extern const char* ofmt(void* mydata)
{
    static const char pbp[] = "pb";
    static const char xml[] = "xml";
    static const char v30[] = "v3";
    static const char ksp[] = "kspack";
    mydata_t* mydata_p = (mydata_t*)mydata;
    if(!mydata){
        TWARN("mydata is null, return!");
        return pbp;
    }
    switch(mydata_p->ofmt){
        case KS_OFMT_PBP :
            return pbp;
        case KS_OFMT_XML :
            return xml;
        case KS_OFMT_V30 :
            return v30;
        case KS_OFMT_KSP :
            return ksp;
        default :
            return pbp;
    }
}

// 生成发往searcher的请求
static const char* rqst_search(void* mydata, clustermap::enodetype* nodetype)
{
    static const int MS_DEFAULT_S = 0;
    static const int MS_DEFAULT_N = 20;
    static const int MS_DEFAULT_MAXN = 10240;

    mydata_t* mydata_p = (mydata_t*)mydata;
    handler_t* handler = mydata_p->handler;
    MemPool* heap = mydata_p->heap;
    queryparser::QueryRewriter* qprewriter = handler->qprewriter;
    queryparser::QueryRewriterResult *pQRWResult = NULL;
    FRAMEWORK::Context* context = mydata_p->context;
    const char* value = 0;
    int maxn = MS_DEFAULT_MAXN;
    char nbuf[21];
    char* query_tmp = 0;

    if(!nodetype){
        TWARN("argument nodetype is null, return!");
        return 0;
    }

    // 检查并修改s和n的值
    value = mydata_p->ksquery->getParam("s");
    if(!value){
        mydata_p->orig_s = MS_DEFAULT_S;
    }
    else{
        mydata_p->orig_s = strtoul(value, 0, 10);
        if(mydata_p->orig_s<0){
            mydata_p->orig_s = MS_DEFAULT_S;
        }
    }
    value = mydata_p->ksquery->getParam("n");
    if(!value){
        mydata_p->orig_n = MS_DEFAULT_N;
    }
    else{
        mydata_p->orig_n = strtoul(value, 0, 10);
        if(mydata_p->orig_n<0){
            mydata_p->orig_n = MS_DEFAULT_N;
        }
    }
    value = mydata_p->ksquery->getParam("maxn");
    if(value){
        maxn = strtoul(value, 0, 10);
    }
    if(mydata_p->orig_s+mydata_p->orig_n>maxn){
        TWARN("s+n > maxn, return!");
        return 0;
    }
    snprintf(nbuf, 21, "%d", mydata_p->orig_s+mydata_p->orig_n);
    mydata_p->ksquery->setParam("s", "0");
    mydata_p->ksquery->setParam("n", nbuf);

    // 生成query
    query_tmp = mydata_p->ksquery->toString();
    if(!query_tmp){
        TWARN("KSQuery toString error, return!");
        return 0;
    }

    // 生成query rewriter结果类
    pQRWResult = NEW(heap, queryparser::QueryRewriterResult)();
    if (pQRWResult == NULL) {
        TWARN("create QueryRewriterResult instance failed.");
        return 0;
    }
    int ret = qprewriter->doRewrite(context, query_tmp, strlen(query_tmp), pQRWResult);
    if (ret != KS_SUCCESS) {
        TWARN("queryparser doRewrite function error. query is %s", query_tmp);
        return 0;
    }
    mydata_p->step_query = (char *)pQRWResult->getRewriteQuery();
    mydata_p->step_qsize = pQRWResult->getRewriteQueryLen();

    *nodetype = clustermap::search_mix;
    TDEBUG("step_query=%s", mydata_p->step_query);
    return mydata_p->step_query;
}

static const char* rqst_detail(void* mydata, clustermap::enodetype* nodetype)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    MemPool* heap = mydata_p->heap;
    commdef::ClusterResult* cr = mydata_p->result;
    sort_framework::UniqIIResult* ur = cr->_pUniqIIResult;
    char* detail_query_str = 0;
    int detail_query_len = 0;
    const char* lightkey_str = 0;
    char* userloc_str = 0;
    char* et_str = 0;
    char* dl_str = 0;
    char* pLatLng = 0;
    bool hasLatLng = false;
    float fLatitude = 0.0;
    float fLngitude = 0.0;
    int uniq2_content = 0;
    int i = 0;
    int j = 0;
    int nid_cnt = 0;
    char* detail_query_ptr = 0;
    int tmpLength = 0;
    int ret = 0;

    if(!nodetype){
        TWARN("argument nodetype is null, return!");
        return 0;
    }

    // 获取需要的参数
    if(mydata_p->result->_pQPResult){
        lightkey_str = mydata_p->result->_pQPResult->getParam()->getParam(HLKEY_FLAG);
    }
    userloc_str = mydata_p->ksquery->getParam("userloc");
    et_str = mydata_p->ksquery->getParam("!et");
    dl_str = mydata_p->ksquery->getParam("dl");

    // 获取要读取的nid数量
    nid_cnt = cr->_nDocsReturn;
    if(nid_cnt==0){
        TWARN("no nid, return!");
        return 0;
    }

    // 制作detail query
    detail_query_len += sizeof("/bin/search?auction?")-1;
    detail_query_len += sizeof("_sid_=&")-1+21*nid_cnt;
    detail_query_len += lightkey_str?(sizeof("_LightKey_=&")-1+strlen(lightkey_str)):0;
    detail_query_len += dl_str?(sizeof("dl=&")-1+strlen(dl_str)):0;
    detail_query_str = NEW_VEC(heap, char, detail_query_len+1);
    if(!detail_query_str){
        TWARN("new detail query buffer error, return!");
        return 0;
    }
    detail_query_ptr = detail_query_str;
    memcpy(detail_query_ptr, "/bin/search?auction?", sizeof("/bin/search?auction?")-1);
    detail_query_ptr += sizeof("/bin/search?auction?")-1;
    memcpy(detail_query_ptr, "_sid_=", sizeof("_sid_=")-1);
    detail_query_ptr += sizeof("_sid_=")-1;
    for(i=0; i<cr->_nDocsReturn; i++){
        ret = sprintf(detail_query_ptr, "%lld,", cr->_pNID[i]);
        detail_query_ptr += ret;
    }
    detail_query_ptr[-1] = '&';
    if(lightkey_str){
        memcpy(detail_query_ptr, "_LightKey_=", sizeof("_LightKey_=")-1);
        detail_query_ptr += sizeof("_LightKey_=")-1;
        memcpy(detail_query_ptr, lightkey_str, strlen(lightkey_str));
        detail_query_ptr += strlen(lightkey_str)+1;
        detail_query_ptr[-1] = '&';
    }
    if(userloc_str){
        memcpy(detail_query_ptr, "userloc=", sizeof("userloc=")-1);
        detail_query_ptr += sizeof("userloc=")-1;
        memcpy(detail_query_ptr, userloc_str, strlen(userloc_str));
        detail_query_ptr += strlen(userloc_str)+1;
        detail_query_ptr[-1] = '&';
    }
    if(dl_str){
        memcpy(detail_query_ptr, "dl=", sizeof("dl=")-1);
        detail_query_ptr += sizeof("dl=")-1;
        memcpy(detail_query_ptr, dl_str, strlen(dl_str));
        detail_query_ptr += strlen(dl_str)+1;
        detail_query_ptr[-1] = '&';
    }
    detail_query_ptr[-1] = 0;

    mydata_p->step_query = detail_query_str;
    mydata_p->step_qsize = strlen(mydata_p->step_query);

    *nodetype = clustermap::search_doc;
    TDEBUG("step_query=%s", mydata_p->step_query);
    return mydata_p->step_query;
}

static int deserialSearcherResult(MemPool* heap, const char* inputs[], const int inlens[], const int incnt,
        commdef::ClusterResult* datas[], uint32_t& nDocsSearch, uint32_t& nDocsFound, uint32_t& nEstimateDocsFound, uint32_t& nDocsRestrict);
static int cmbn_search(void* mydata, const char* inputs[], const int inlens[], const int incnt)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    MemPool* heap = mydata_p->heap;
    FRAMEWORK::Context* context = mydata_p->context;
    commdef::ClusterResult* cr = mydata_p->result;
    SORT::MergerSortFacade* msort = mydata_p->handler->msort;
    statistic::StatisticManager* mstat = mydata_p->handler->mstat;
    commdef::ClusterResult** datas = 0;
    queryparser::QPResult* qpresult = 0;
    statistic::StatisticResult* stat_result = 0;
    SORT::SortResult* sort_result = 0;
    uint32_t nDocsSearch = 0;
    uint32_t nDocsFound = 0;
    uint32_t nEstimateDocsFound = 0;
    uint32_t nDocsRestrict = 0;
    int ret = 0;
    int i = 0;
    char snbuf[11];

    if(!inputs || !inlens || incnt==0){
        TWARN("argument error, return!");
        return -1;
    }

    // 反序列化各个search的结果
    datas = (commdef::ClusterResult**)NEW_VEC(heap, commdef::ClusterResult*, incnt);
    if(!datas){
        TWARN("new ClusterResult error, return!");
        return -1;
    }
    if(deserialSearcherResult(heap, inputs, inlens, incnt, datas,
                nDocsSearch, nDocsFound, nEstimateDocsFound, nDocsRestrict)){
        TWARN("deserialSearcherResult error, return!");
        return -1;
    }

    // 获取queryparser结果
    for(i=0; i<incnt; i++){
        if(datas[i] && datas[i]->_pQPResult){
            qpresult = datas[i]->_pQPResult;
            break;
        }
    }
    if(!qpresult){
        TWARN("QPResult is empty, return!");
        return -1;
    }
    snprintf(snbuf, 11, "%d", mydata_p->orig_s);
    qpresult->getParam()->setParam("s", snbuf);
    snprintf(snbuf, 11, "%d", mydata_p->orig_n);
    qpresult->getParam()->setParam("n", snbuf);

    // 获取统计结果
    stat_result = NEW(heap, statistic::StatisticResult)();
    if(!stat_result){
        TWARN("new StatisticResult error, return!");
        return -1;
    }
    if(mstat->doStatisticOnMerger(context, datas, incnt, stat_result)){
        TWARN("doStatisticOnMerger error, return!");
        return -1;
    }

    // 获取排序结果
    sort_result = NEW(heap, SORT::SortResult)(heap);
    if(!sort_result){
        TWARN("new StatisticResult error, return!");
        return -1;
    }
    if(msort->doProcess(*context, *(qpresult->getParam()), datas, (uint32_t)incnt, *sort_result)){
        TWARN("Sort doProcess error, return!");
        return -1;
    }

    // 整理结果
    cr->_nDocsFound = sort_result->getDocsFound();
    cr->_nEstimateDocsFound = sort_result->getEstimateDocsFound();
    cr->_nDocsSearch = nDocsSearch;
    cr->_nDocsRestrict = nDocsRestrict;
    cr->_pStatisticResult = stat_result;
    cr->_pSortResult = sort_result;
    cr->_pQPResult = qpresult;
    cr->_nDocsReturn = sort_result->getNidList(cr->_pNID);
    if (mydata_p->pLogInfo != NULL) {
        mydata_p->pLogInfo->_inFound = cr->_nDocsFound;
        mydata_p->pLogInfo->_inReturn = cr->_nDocsReturn;
    }
    if(sort_result->getSidList(cr->_pServerID)<0){
        TWARN("getSidList error, return!");
        return -1;
    }
    if(sort_result->getFirstRankList(cr->_szFirstRanks)<0){
        TWARN("getFirstRankList error, return!");
        return -1;
    }
    if(sort_result->getSecondRankList(cr->_szSecondRanks)<0){
        TWARN("getSecondRankList error, return!");
        return -1;
    }
    return 0;
}

/* 保存一条detail信息的结构 */
typedef struct
{
    char nid[32];   // detail信息对应的nid
    int nid_size;   // nid长度
    char* data;     // detail信息
    int data_size;  // detail信息长度
    char data_type; // detail信息类型
}
detail_node_t;
/* 全部列detail信息的合并结构 */
typedef struct
{
    int field_count;        // detail域数量
    char* field_name;       // detail域名信息
    int field_name_size;    // detial域名信息长度
    int node_count;         // detail信息数量
    detail_node_t* nodes;   // detail信息数组
}
detail_list_t;
static detail_list_t* query_to_list(const char* query_string, const int query_length);
static void free_list(detail_list_t* list);
static int kspack_to_list(const char* input, const int inlen, detail_list_t* list);
static int cmbn_detail(void* mydata, const char* inputs[], const int inlens[], const int incnt)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    MemPool* heap = mydata_p->heap;
    commdef::ClusterResult* cr = mydata_p->result;
    char* query_string = mydata_p->step_query;
    int query_length = mydata_p->step_qsize;
    detail_list_t* list = 0;
    int i = 0;
    int j = 0;
    int k = 0;
    char** field_name = 0;
    char* start = 0;
    char* end = 0;
    Document** docs = 0;
    char* field_value = 0;

    if(!inputs || !inlens || incnt==0){
        TWARN("argument error, return!");
        return -1;
    }

    // 读取query中的nid，形成结果合并列表
    list = query_to_list(query_string, query_length);
    if(!list){
        TWARN("query_to_list error, return!");
        if(list) {
            free(list);
        }
        return -1;
    }

    // 将每个节点返回的结果合并
    for(i=0; i<incnt; i++){
        if(kspack_to_list(inputs[i], inlens[i], list)){
            TWARN("kspack_to_list error, continue!");
            continue;
        }
    }

    // 分解field name
    field_name = NEW_VEC(heap, char*, list->field_count);
    if(!field_name){
        TWARN("new field name array, return!");
        if(list) {
            free(list);
        }
        return -1;
    }
    start = list->field_name;
    if (start == NULL) {
        TWARN("field name is null");
        if(list) {
            free(list);
        }
        return -1;
    }
    for(i=0; i<list->field_count-1; i++){
        end = strchr(start, '\001');
        if(!end){
            TWARN("no enough field name, return!");
            if(list) {
                free(list);
            }
            return -1;
        }
        field_name[i] = NEW_VEC(heap, char, end-start+1);
        if(!field_name[i]){
            TWARN("new field name buffer, return!");
            if(list) {
                free(list);
            }
            return -1;
        }
        memcpy(field_name[i], start, end-start);
        field_name[i][end-start] = 0;
        start = end+1;
    }
    if(*start==0){
        TWARN("no enough field name, return!");
        if(list) {
            free(list);
        }
        return -1;
    }
    end = start+strlen(start);
    if(end[-1]=='\001'){
        end --;
    }
    field_name[i] = NEW_VEC(heap, char, end-start+1);
    if(!field_name[i]){
        TWARN("new field name buffer, return!");
        if(list) {
            free(list);
        }
        return -1;
    }
    memcpy(field_name[i], start, end-start);
    field_name[i][end-start] = 0;

    // 分解detail数据
    docs = NEW_VEC(heap, Document*, list->node_count);
    if(!docs){
        TWARN("new Document array error, return!");
        if(list) {
            free(list);
        }
        return -1;
    }
    for(i=0; i<list->node_count; i++){
        if(!list->nodes[i].data || list->nodes[i].data_size<=0 || list->nodes[i].data_type==KS_NONE_VALUE){
            docs[i] = 0;
            continue;
        }
        docs[i] = NEW(heap, Document)(heap);
        if(!docs[i]){
            TWARN("new Document error, return!");
            if(list) {
                free(list);
            }
            return -1;
        }
        docs[i]->init(list->field_count+FIELDBUFFCOUNT);
        start = list->nodes[i].data;
        if (start == NULL) {
            TWARN("field value is null");
            if(list) {
                free(list);
            }
            return -1;
        }
        for(j=0; j<list->field_count-1; j++){
            end = strchr(start, '\001');
            if(!end){
                TWARN("no enough field value, return!");
                if(list) {
                    free(list);
                }
                return -1;
            }
            field_value = NEW_VEC(heap, char, end-start+1);
            if(!field_value){
                TWARN("new field value buffer, return!");
                if(list) {
                    free(list);
                }
                return -1;
            }
            memcpy(field_value, start, end-start);
            field_value[end-start] = 0;
            docs[i]->addField(field_name[j], field_value);
            start = end+1;
        }
        if(*start==0){
            TWARN("no enough field value, return!");
            if(list) {
                free(list);
            }
            return -1;
        }
        end = start+strlen(start);
        if(end[-1]=='\001'){
            end --;
        }
        field_value = NEW_VEC(heap, char, end-start+1);
        if(!field_value){
            TWARN("new field value buffer, return!");
            if(list) {
                free(list);
            }
            return -1;
        }
        memcpy(field_value, start, end-start);
        field_value[end-start] = 0;
        docs[i]->addField(field_name[j], field_value);
    }
    if(list) {
        free(list);
    }

    // 填充ClusterResult
    cr->_ppDocuments= docs;
    k = cr->_nDocsReturn;
    return 0;
}

static int frmt_pbp(void* mydata, char** output, int* outlen)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    handler_t* handler = mydata_p->handler;
    MemPool* heap = mydata_p->heap;
    ResultFormat* rf = handler->rffactory->make("pb", heap);
    result_container_t container;
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成pb
    container.pClusterResult = mydata_p->frmt_result;
    container.cost = mydata_p->time_cost;
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        handler->rffactory->recycle(rf);
        return -1;
    }

    *output = fmt_out;
    *outlen = fmt_len;
    handler->rffactory->recycle(rf);

    return 0;
}

static int frmt_xml(void* mydata, char** output, int* outlen)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    handler_t* handler = mydata_p->handler;
    MemPool* heap = mydata_p->heap;
    ResultFormat* rf = handler->rffactory->make("xml", heap);
    result_container_t container;
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成xml
    container.pClusterResult = mydata_p->frmt_result;
    container.cost = mydata_p->time_cost;
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        handler->rffactory->recycle(rf);
        return -1;
    }

    fmt_out[fmt_len] = 0;    
    TDEBUG("utf8_format=%s", fmt_out);

    // 转码成gbk
    *output = NEW_VEC(heap, char, fmt_len+1);
    if(!*output){
        TWARN("new output buffer error, return!");
        handler->rffactory->recycle(rf);
        return -1;
    }
    *outlen = py_utf8_to_gbk(fmt_out, fmt_len, *output, fmt_len+1);
    TDEBUG("gbk_outlen=%d, gbk_strlen=%d", *outlen, strlen(*output));
    TDEBUG("gbk_output=%s", *output);

    handler->rffactory->recycle(rf);

    return 0;
}

static int frmt_v30(void* mydata, char** output, int* outlen)
{
    mydata_t* mydata_p = (mydata_t*)mydata;
    handler_t* handler = mydata_p->handler;
    MemPool* heap = mydata_p->heap;
    ResultFormat* rf = handler->rffactory->make("v30", heap);
    result_container_t container;
    char* fmt_out = 0;
    int fmt_len = 0;
    int ret = 0;
    
    if(!output || !outlen){
        TWARN("argument error, return!");
        return -1;
    }
    if(!rf){
        TWARN("make ResultFormat error, return!");
        return -1;
    }

    // 格式化输出成v3
    container.pClusterResult = mydata_p->frmt_result;
    container.cost = mydata_p->time_cost;
    ret = rf->format(container, fmt_out, (uint32_t&)fmt_len);
    if(ret<0 || !fmt_out || fmt_len<=0){
        TWARN("format output error, return!");
        handler->rffactory->recycle(rf);
        return -1;
    }

    fmt_out[fmt_len] = 0;    
    TDEBUG("utf8_format=%s", fmt_out);

    // 转码成gbk
    *output = NEW_VEC(heap, char, fmt_len+1);
    if(!*output){
        TWARN("new output buffer error, return!");
        handler->rffactory->recycle(rf);
        return -1;
    }
    *outlen = py_utf8_to_gbk(fmt_out, fmt_len, *output, fmt_len+1);
    TDEBUG("gbk_outlen=%d, gbk_strlen=%d", *outlen, strlen(*output));
    TDEBUG("gbk_output=%s", *output);

    handler->rffactory->recycle(rf);

    return 0;
}

static int frmt_ksp(void* mydata, char** output, int* outlen)
{
    return -1;
}

// 获取当前时间(ms)
static uint64_t time_now()
{
    uint64_t now = 0;
    struct timeval tv;
    gettimeofday(&tv, 0);
    now = tv.tv_sec;
    now = now*1000+tv.tv_usec/1000;
    return now;
}

// 反序列化所有search的结果
static int deserialClusterResult(MemPool *pHeap, const char*data, uint32_t size, commdef::ClusterResult &out);
static int deserialSearcherResult(MemPool* heap, const char* inputs[], const int inlens[], const int incnt,
        commdef::ClusterResult* datas[], uint32_t& nDocsSearch, uint32_t& nDocsFound, uint32_t& nEstimateDocsFound, uint32_t& nDocsRestrict) 
{
    int i = 0;

    // 先将返回值清零
    memset(datas, 0x0, sizeof(commdef::ClusterResult*)*incnt);
    nDocsSearch = 0;
    nDocsFound = 0;
    nEstimateDocsFound = 0;
    nDocsRestrict = 0;

    // 反序列化每一个search的结果
    for(i=0; i<incnt; i++){
        if(inputs[i]==0 || inlens[i]==0){
            datas[i] = 0;
            continue;
        }
        datas[i] = (commdef::ClusterResult*)NEW(heap, commdef::ClusterResult)();
        if(!datas[i]){
            TWARN("NEW ClusterResult error");
            return -1;
        }   
        memset(datas[i], 0x0, sizeof(commdef::ClusterResult));
        if (deserialClusterResult(heap, inputs[i], inlens[i], *(datas[i]))!=KS_SUCCESS){   
            TWARN("Deserial ClusterResult from searcher[%d] error", i); 
            datas[i] = 0;
            continue;
        }   
        datas[i]->nServerId = i;
        nDocsSearch += datas[i]->_nDocsSearch;
        nDocsFound += datas[i]->_nDocsFound;
        nEstimateDocsFound += datas[i]->_nEstimateDocsFound;
        nDocsRestrict += datas[i]->_nDocsRestrict;
    }

    return 0;
}

/* 反序列化一个search的结果 */
static int deserialClusterResult(MemPool *pHeap, const char*data, uint32_t size, commdef::ClusterResult &out)
{
    uint32_t paraSize = 0;
    uint32_t sortSize = 0;

    if(!pHeap || !data || size==0){
        return -1;
    }   

    // 反序列化数值信息
    out._nDocsSearch = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nDocsFound = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nEstimateDocsFound = *(uint32_t*)data;
    data += sizeof(uint32_t); 
    out._nDocsRestrict = *(uint32_t*)data;
    data += sizeof(uint32_t);
    paraSize = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nStatBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    sortSize = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nUniqBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nSrcBufLen = *(uint32_t*)data;
    data += sizeof(uint32_t);
    out._nOrsBufferLen = *(uint32_t*)data;
    data += sizeof(uint32_t);

    // 检查数据
    if(sizeof(uint32_t)*10+paraSize+out._nStatBufLen+sortSize+out._nUniqBufLen+out._nSrcBufLen+out._nOrsBufferLen!=size){
        return -1;
    }

    // 反序列化queryparser结果
    if(paraSize>0){
        out._pQPResult = NEW(pHeap, queryparser::QPResult)(pHeap);
        if(out._pQPResult==0){
            TWARN("out of Memory");
            return -1;
        }   
        if(out._pQPResult->deserialize((char*)data, paraSize, pHeap)!=KS_SUCCESS){
            TWARN("deserialize QPResult error, paraSize=%d.", paraSize);
            return -1;
        }
        data += paraSize;
    }
    else{
        out._pQPResult = 0;
    }

    // 反序列化统计结果
    if(out._nStatBufLen>0){
        out._szStatBuffer = (char*)data;
        out._pStatisticResult = NEW(pHeap, statistic::StatisticResult)();
        if(out._pStatisticResult==0){
            TWARN("out of Memory");
            return -1;
        }
        if(out._pStatisticResult->deserialize(out._szStatBuffer, out._nStatBufLen, pHeap)!=KS_SUCCESS){
            TWARN("deserialize Statistic Result error.");
            return -1;
        }
        data += out._nStatBufLen;
    }
    else{
        out._szStatBuffer = NULL;
    }

    // 反序列化排序结果
    if(sortSize>0){
        out._pSortResult = NEW(pHeap, sort_framework::SortResult)(pHeap);
        if(out._pSortResult==0){
            TWARN("out of Memory");
            return -1;
        }
        if(!out._pSortResult->deserialize((char *)data, sortSize)){
            TWARN("deserialize SortResult error.");
            return -1;
        }
        data += sortSize;
    }
    else{
        out._pSortResult = NULL;
    }

    // 反序列化其它结果
    if(out._nUniqBufLen>0){
        out._szUniqBuffer = (char*)data;
        data += out._nUniqBufLen;
    }
    else{
        out._szUniqBuffer = NULL;
    }
    if(out._nSrcBufLen>0){
        out._szSrcBuffer = (char*)data;
        data += out._nSrcBufLen;
    }
    else{
        out._szSrcBuffer = NULL;
    }
    if(out._nOrsBufferLen>0){
        out._szOrsBuffer = (char*)data;
        data += out._nOrsBufferLen;
    }
    else{
        out._szOrsBuffer = NULL;
    }

    return 0;
}

/* 读取query中的nid，形成nid与数据的对应表 */
static detail_list_t* query_to_list(const char* query_string, const int query_length)
{
    static const char* NID_LIST_ARG_STR = "_sid_=";
    static const int NID_LIST_ARG_LEN = 6;

    detail_list_t* list = 0;
    char* query_tmp = 0;
    char* nid_list_str = 0;
    int nid_list_len = 0;
    char** nids = 0;
    int nid_count = 0;
    char* ptr = 0;
    int len = 0;
    int i = 0;

    query_tmp = strdup(query_string);
    if(!query_tmp){
        TWARN("strdup query error, return!");
        return 0;
    }

    // 找到_sid_
    ptr = strstr(query_tmp, NID_LIST_ARG_STR);
    if(!ptr){
        TWARN("there is no _sid_ argument, return!");
        free(query_tmp);
        return 0;
    }
    nid_list_str = ptr+NID_LIST_ARG_LEN;

    // 找到nid列表的结尾
    ptr = strchr(nid_list_str, '&');
    if(ptr){
        *ptr = 0;
    }
    nid_list_str = str_trim(nid_list_str);
    nid_list_len = strlen(nid_list_str);
    if(nid_list_len<=0){
        TWARN("nid count is 0, return!");
        free(query_tmp);
        return 0;
    }

    // 分割nid列表
    nids = (char**)malloc(sizeof(char*)*nid_list_len);
    if(!nids){
        TWARN("malloc nids error, return!");
        free(query_tmp);
        return 0;
    }   
    nid_count = str_split_ex(nid_list_str, ',', nids, nid_list_len);

    // 申请对应表空间
    list = (detail_list_t*)malloc(sizeof(detail_list_t)+sizeof(detail_node_t)*nid_count);
    if(!list){
        TWARN("malloc list error, return!");
        free(query_tmp);
        free(nids);
        return 0;
    }
    list->nodes = (detail_node_t*)(list+1);
    list->node_count = 0;
    list->field_count = 0;
    list->field_name = 0;
    list->field_name_size = 0;

    // 填充对应表的nid
    for(i=0; i<nid_count; i++){
        nids[i] = str_trim(nids[i]);
        len = strlen(nids[i]);
        if(len<=0 || len>31){
            continue;
        }
        memcpy(list->nodes[list->node_count].nid, nids[i], len);
        list->nodes[list->node_count].nid[len] = 0;
        list->nodes[list->node_count].nid_size = len;
        list->nodes[list->node_count].data = 0;
        list->nodes[list->node_count].data_size = 0;
        list->nodes[list->node_count].data_type = KS_NONE_VALUE;
        list->node_count ++;
        TDEBUG("nid[%d] : %s, node_count : %d", i, nids[i], list->node_count);
    }

    free(query_tmp);
    free(nids);

    return list;
}

/* 清理对应表内存空间 */
static void free_list(detail_list_t* list)
{
    free(list);
}

/* 将一个kspack填充到对应表中 */
static int kspack_to_list(const char* input, const int inlen, detail_list_t* list)
{
    int i = 0;
    void* data = 0;
    int data_size = 0;
    char data_type = 0;
    kspack_t* pack = 0;

    // 打开kspack
    pack = kspack_open('r', (void*)input, inlen);
    if(!pack){
        TWARN("kspack_open error, return!");
        return -1;
    }

    if(list->field_count<=0){
        if(kspack_get(pack, "field_count", 11, &data, &data_size, &data_type)==0 && data && data_size==sizeof(int) && data_type==KS_BYTE_VALUE){
            list->field_count = *((int*)data);
        }
    }
    if(list->field_name==0 || list->field_name_size==0){
        if(kspack_get(pack, "field_name", 10, &data, &data_size, &data_type)==0 && data && data_size>0 && data_type==KS_BYTE_VALUE){
            list->field_name = (char*)data;
            list->field_name_size = data_size-1;
        }
    }

    // 将kspack中的每个有效数据填充到对应表中
    for(i=0; i<list->node_count; i++){
        TDEBUG("nid=%s, nid_size=%d", list->nodes[i].nid, list->nodes[i].nid_size);
        if(kspack_get(pack, list->nodes[i].nid, list->nodes[i].nid_size, &data, &data_size, &data_type)){
            continue;
        }
        TDEBUG("nid=%s, nid_size=%d, data=%s, data_size=%d, data_type=%d", list->nodes[i].nid, list->nodes[i].nid_size, data, data_size, data_type);
        if(!list->nodes[i].data && data && data_size>0 && data_type!=KS_NONE_VALUE){
            list->nodes[i].data = (char*)data;
            list->nodes[i].data_size = data_size;
            list->nodes[i].data_type = data_type;
        }
    }

    kspack_close(pack, 0);

    return 0;
}

#ifdef __cplusplus
}
#endif
