#include "SearcherServer.h"
#include <stdlib.h>
#include <string.h>
#include <new>
#include "commdef/ClusterResult.h"
#include "commdef/SearchResult.h"
#include "util/Log.h"
#include "util/Thread.h"
#include "util/iniparser.h"
#include "util/charsetfunc.h"
#include "util/py_url.h"
#include "pool/MemMonitor.h"
#include "framework/namespace.h"
#include "clustermap/CMClient.h"
#include "index_lib/IndexLib.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"
#include "ResultSerializer.h"
#include "libdetail/detail.h"

const static int nidStrMaxLen = 21;
const static int timeoutCheck = 1;
extern bool translateV3(const char *pV3Str, Document ** &ppDocument,
        uint32_t &docsCount, MemPool *heap);
namespace kingso_searcher {

#define OUTPUTINFO "output"
#define OUTPUTFILE "outputfile"

//Worker的子类，用于处理searcher server上的逻辑
SearcherWorker::SearcherWorker(FRAMEWORK::Session &session,
        queryparser::QueryRewriter &qrewriter,
        queryparser::QueryParser &qp,
        search::Search &is,
        SORT::SearchSortFacade &sort,
        statistic::StatisticManager &stat,
        FRAMEWORK::MemPoolFactory &memFactory,
        uint64_t memLimit,
        uint64_t timeout,
        bool detail,
        outfmt_type ofmt_type)
    :Worker(session), 
    _qrewriter(qrewriter),
    _qp(qp),
    _is(is),
    _sort(sort),
    _stat(stat),
    _memFactory(memFactory),
    _memLimit(memLimit),
    _timeout(timeout),
    _detail(detail),
    _ofmt_type(ofmt_type){
    }

SearcherWorker::~SearcherWorker() 
{
}

void SearcherWorker::handleTimeout() 
{
    _session.setStatus(FRAMEWORK::ss_timeout);
    _session.response();
    return;
}

bool isOutputInfo(const FRAMEWORK::Query &query)
{
    if (FRAMEWORK::Query::getParam(query.getPureQueryData(), 
                query.getPureQuerySize(), OUTPUTINFO) != NULL)
        return true;
    return false;
}

void showSearchFilterResult(SearchResult *pSearchResult)
{
    if (pSearchResult == NULL)
        return ;
    FILE *pOutput = NULL;
    pOutput = fopen(OUTPUTFILE, "a+");
    if (pOutput == NULL) {
        fprintf(stderr, "open file %s error\n", OUTPUTFILE);
        return ;
    }
    index_lib::ProfileDocAccessor *pProfileDoc = index_lib::ProfileManager::getDocAccessor();
    const index_lib::ProfileField *pField = pProfileDoc->getProfileField("nid");
    fprintf(pOutput, "\nSearch Result Begin:\n");
    fprintf(pOutput, "DocsFound = %u\n", pSearchResult->nDocFound);
    for(uint32_t i=0; i < pSearchResult->nDocFound; i++)
    {
        fprintf(pOutput, "%d\t%lu\n", i, 
                pProfileDoc->getInt(pSearchResult->nDocIds[i], pField));
    }
    fprintf(pOutput, "Search Result End\n");
    fflush(pOutput);
    fclose(pOutput);
    return ;
}

void showSortResult(sort_framework::SortResult *pSortResult)
{
    if (pSortResult == NULL)
        return ;
    FILE *pOutput = NULL;
    int32_t num = 0;
    int32_t DocsReturn = 0;
    int64_t *pNID = NULL;
    int64_t *szFirstRanks = NULL;
    int64_t *szSecondRanks = NULL;
    pOutput = fopen(OUTPUTFILE, "a+");
    if (pOutput == NULL) {
        fprintf(stderr, "open file %s error\n", OUTPUTFILE);
        return ;
    }
    //Get Nid List
    DocsReturn =
        pSortResult->getNidList(pNID);
    //Get FirstRank List
    num = pSortResult->getFirstRankList(szFirstRanks);
    if (num < 0) {
        TWARN("Get FirstRank List error.");
        return ;
    }
    //Get SecondRank List
    num = pSortResult->getSecondRankList(szSecondRanks);
    if (num < 0) {
        TWARN("Get SecondRank List error.");
        return ;
    }
    fprintf(pOutput, "\nSort Result Begin:\nDocsFound = %d\n", pSortResult->getDocsFound());
    for (uint32_t i=0; i<DocsReturn; i++)
    {
        fprintf(pOutput, "%d\tnid=%lu\tfirst=%lld\tsecond=%lld\n", 
                i, pNID[i], szFirstRanks[i], szSecondRanks[i]);
    }
    fprintf(pOutput, "Sort Result End");
    fflush(pOutput);
    fclose(pOutput);
    return ;
}

int32_t SearcherWorker::run() {
    const char *pureString = NULL;
    uint32_t pureSize = 0;
    char *resData = NULL;
    uint32_t resSize = 0;
    char *resDetailData = NULL;
    int32_t resDetailSize = 0;
    int32_t ret = 0;
    MemPool *memPool = NULL;
    int64_t *pNid = NULL;
    commdef::ClusterResult *pClusterResult = NULL;
    queryparser::QueryRewriterResult *pQRWResult = NULL;
    queryparser::QPResult *pQueryResult = NULL;
    SearchResult *pSearchResult = NULL;
    statistic::StatisticResult *pStatisticResult = NULL;
    sort_framework::SortResult *pSortResult = NULL;
    ResultSerializer resultSerial;
    FRAMEWORK::Context context;
    FILE *pOutput = NULL;
    //check session status
    FRAMEWORK::session_status_t status = _session.getStatus();
    if (status == FRAMEWORK::ss_timeout) {
        handleTimeout();
        return KS_SUCCESS;
    }
    //get query infomation
    FRAMEWORK::Query &query = _session.getQuery();
    pureSize = query.getPureQuerySize();
    pureString = query.getPureQueryData();
    if (!pureString || pureSize == 0) {
        _session.setStatus(FRAMEWORK::ss_error);
        _session.response();
        return KS_EFAILED;
    }
    //set LogInfo level
    _session._logInfo._eRole = FRAMEWORK::sr_simple;
    //get MemPool from factory
    memPool = _memFactory.make((uint64_t)(getOwner()));
    if (memPool == NULL) {
        TWARN("Make mem pool failed!");
        return KS_EFAILED;
    }
    //create memory pool monitor
    MemMonitor memMonitor(memPool, _memLimit);
    memPool->setMonitor(&memMonitor);
    memMonitor.enableException();
    //initialize context class
    context.setMemPool(memPool);
	//initialize format processor
    _formatProcessor.init(memPool);
    //Deal with search proccess
    do{  
        if(_session.isHttp()){
            pQRWResult = NEW(memPool, queryparser::QueryRewriterResult)();
            if (pQRWResult == NULL) {
                TWARN("SEARCHER: new Result no mem");
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
        }
        pQueryResult = NEW(memPool, queryparser::QPResult)(memPool);
        pSearchResult = NEW(memPool, SearchResult);
        pStatisticResult = NEW(memPool, statistic::StatisticResult); 
        pSortResult = NEW(memPool, sort_framework::SortResult)(memPool);
        if(unlikely(!pQueryResult || !pSearchResult || !pStatisticResult
                    || !pSortResult)) {
            TWARN("SEARCHER: new Result no mem");
            _session.setStatus(FRAMEWORK::ss_error);
            break;
        }
        //add queryrewrite process
        if(_session.isHttp()){     
            ret = _qrewriter.doRewrite(&context, pureString, pureSize, pQRWResult);
            if (timeoutCheck && (_timeout > 0) && (_session.getLatencyTime() > _timeout)) {
                _session.setStatus(FRAMEWORK::ss_timeout);
                TWARN("SEARCHER: qrewriter.doRewrite function over time. query is %s", pureString);
                break;
            }
            if (unlikely(ret != KS_SUCCESS)) {
                _session.setStatus(FRAMEWORK::ss_error);
                TWARN("qrewriter.doRewrite function error. query is %s", pureString);                
                break;
            }
            pureString = pQRWResult->getRewriteQuery();
        }
        //end add
        ret = _qp.doParse(&context, pQueryResult, pureString);
        if (timeoutCheck && (_timeout > 0) && 
                (_session.getLatencyTime() > _timeout)) 
        {
            _session.setStatus(FRAMEWORK::ss_timeout);
            TWARN("SEARCHER: qp.doParse function over time. query is %s", pureString);
            break;
        }
        if (unlikely(ret != KS_SUCCESS)){
            TWARN("SEARCHER: queryparser doParse function error. query is %s", pureString);
            _session.setStatus(FRAMEWORK::ss_error);
            break;
        }

        // cache命中情况下, mempool reset时调用CacheResult析构函数,释放命中的节点，
        // 否则sortResult对节点内存的引用失效，变为野指针(bug#118631)
        {
            ret = _is.doQuery(&context, pQueryResult, &_sort, pSearchResult);
            if (timeoutCheck && (_timeout > 0) &&
                    (_session.getLatencyTime() > _timeout)) 
            {
                _session.setStatus(FRAMEWORK::ss_timeout);
                TWARN("SEARCHER: is.doQuery function over time. query is %s", pureString);
                break;
            }
            if (unlikely(ret != KS_SUCCESS)){
                TWARN("SEARCHER: search doQuery function error. query is %s", pureString);
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
            ret = _stat.doStatisticOnSearcher(&context, 
                    *(pQueryResult->getParam()),
                    pSearchResult, pStatisticResult);
            if (timeoutCheck && (_timeout > 0) &&
                    (_session.getLatencyTime() > _timeout)) 
            {
                _session.setStatus(FRAMEWORK::ss_timeout);
                TWARN("SEARCHER: doStatisticOnSearcher function over time. query is %s", pureString);
                break;
            }
            if (unlikely(ret != KS_SUCCESS)){
                TWARN("SEARCHER: statistic doStatisticOnSearcher function error. query is %s", pureString);
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
            ret = _sort.doProcess(context, *(pQueryResult->getParam()), 
                    *pSearchResult, *pSortResult);
            if (timeoutCheck && (_timeout > 0) &&
                    (_session.getLatencyTime() > _timeout))
            {
                _session.setStatus(FRAMEWORK::ss_timeout);
                TWARN("SEARCHER: sort.doProcess function over time. query is %s", pureString);
                break;
            }
            if (unlikely(ret)) {
                TWARN("SEARCHER: sort doProcess function error. query is %s", pureString);
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
        }


        pNid = NULL;
        //get docs return number
        int32_t num = pSortResult->getNidList(pNid);
        pClusterResult = NEW(memPool, commdef::ClusterResult)();

        if ( NULL == pClusterResult ){
            TWARN("SEARCHER: malloc from memPool failed ");
            _session.setStatus(FRAMEWORK::ss_error);
            break;            
        }

        memset(pClusterResult, 0x0, sizeof(commdef::ClusterResult));
        pClusterResult->_nDocsFound = pSortResult->getDocsFound();//pSearchResult->nDocFound;
        pClusterResult->_nEstimateDocsFound = pSortResult->getEstimateDocsFound();//pSearchResult->nEstimateDocFound;
        pClusterResult->_nDocsSearch = pSearchResult->nDocSearch;
        pClusterResult->_nDocsReturn = num;
        pClusterResult->_nDocsRestrict = pSearchResult->nEstimateDocFound;//pSearchResult->nDocFound;
        pClusterResult->_pStatisticResult = pStatisticResult;
        pClusterResult->_pSortResult = pSortResult;
        pClusterResult->_pQPResult = pQueryResult;
        pClusterResult->_ppDocuments = NULL;

        if(_session.isHttp() && _detail){

            if(!pClusterResult->_nDocsReturn){
                break;
            }  
            char* pid = NULL;
            uint32_t pid_size = 0;
            ret = get_nid_str(pNid, num, memPool, &pid, &pid_size);
            if(KS_SUCCESS != ret){
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }            
            ret = getDetailInfo(pid, pid_size, pureString, &resDetailData, &resDetailSize);
            if(KS_SUCCESS != ret){
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
            if(timeoutCheck && _timeout>0 && _session.getLatencyTime()>_timeout){
        		_session.setStatus(FRAMEWORK::ss_timeout); 
                break;
        	} 

            if (!translateV3(resDetailData, pClusterResult->_ppDocuments, (uint32_t &)pClusterResult->_nDocsReturn, memPool)) {
                _session.setStatus(FRAMEWORK::ss_error);
                break;
            }
        }
    } while (0);

    if(resDetailData)std::free(resDetailData);

    if(_session.isHttp() && _detail){
        //fromat result
        get_outfmt_type(pureString, pureSize);
        result_container_t container;
        container.pClusterResult = pClusterResult;
        container.cost = _session.getLatencyTime();
        if(_formatProcessor.frmt(_ofmt_type, container, &resData , &resSize) < 0){
            TERR("FORMATRESULT: format_processor process error!"); 
            _session.setStatus(FRAMEWORK::ss_error);
        }
    }
    else{     
        //serialize search result
        ret = resultSerial.serialClusterResult(pClusterResult,
                resData, resSize, "Z");
        if (ret != KS_SUCCESS) {
            _session.setStatus(FRAMEWORK::ss_error);
        }
    }
    //recycle mem pool
    context.getMemPool()->reset();
    _session.setResponseData(resData, resSize);
    _session.response();
    return KS_SUCCESS;
}
int32_t SearcherWorker::get_nid_str(int64_t* pNid, int32_t num, MemPool* memPool, char** output, uint32_t* outlen)
{
    if(!pNid || 0 == num){
        return KS_EFAILED;
    }
    if(*output != NULL || *outlen != 0){
        TERR("memory leak error possiblely ");
        *outlen = 0;
    }
    int len = nidStrMaxLen*num;
    int print_len = 0;
    *output = NEW_VEC(memPool, char, len);
    if(!*output){
        return KS_ENOMEM;
    }
    print_len = snprintf(*output+*outlen, len, "%lld", pNid[0]);
    if(print_len < 0){
        return KS_EFAILED;
    }
    *outlen += print_len;
    len -= print_len;
    
    for(int32_t i = 1; i < num; i++)
    {
        print_len = snprintf(*output+*outlen, len, ",%lld", pNid[i]);
        if(print_len < 0){
            return KS_EFAILED;
        }
        *outlen += print_len;
        len -= print_len;

        if(len <= 0){
            TERR("nid num more huge, get mem not enough!!");
            return KS_EFAILED;
        }
    }
    return KS_SUCCESS;
}
int32_t SearcherWorker::getDetailInfo(const char *q_reslut, uint32_t q_size, const char *pureString, char** res, int* res_size)
{
    //for detail
    char** args = NULL;
    int fieldCount = 0;
    char* pLightKey = 0;
    char* pDL = 0;
    if(!q_reslut || 0 == q_size){
        return KS_EFAILED;
    }

    pLightKey = FRAMEWORK::Query::getParam(pureString, q_size, LIGHTKEY);
    if(pLightKey && pLightKey[0]!=0){
        int len = strlen(pLightKey);
        int ret = decode_url(pLightKey, len, pLightKey, len+1);
        if(ret<0){
            TWARN("decode_url pLightKey error, return!");
            _session.setStatus(FRAMEWORK::ss_error);
            _session.response();
            return KS_EFAILED;
        }
        pLightKey[ret] = 0;
    }
    pDL = FRAMEWORK::Query::getParam(pureString, q_size, DL);

    fieldCount = get_di_field_count();
    if(di_detail_arg_alloc(&args)<(fieldCount+1)){
        TERR("di_detail_arg_alloc error, return!");
        _session.setStatus(FRAMEWORK::ss_error);
        _session.response();
        return KS_EFAILED;
    }
    di_detail_arg_set(args, fieldCount+1, "title", 5, pLightKey, pDL);
    //Deal with detail
    if(di_detail_get_v3(q_reslut, pDL, args, fieldCount, res, res_size)){
        *res = (char*)malloc(2);
        (*res)[0] = '0';
        (*res)[1] = 0;
        *res_size = 1;
    }

    if(pLightKey)std::free(pLightKey);
    if(pDL)std::free(pDL);
    if(args)di_detail_arg_free(args);

    return KS_SUCCESS;
}
void SearcherWorker::get_outfmt_type(const char *reslut, uint32_t size)
{
    char *outfmt = FRAMEWORK::Query::getParam(reslut, size, "outfmt");
    if(outfmt){
        if(strcasecmp(outfmt, "pb")==0){
             _ofmt_type = KS_OFMT_PBP;
        }
        else if(strcasecmp(outfmt, "xml")==0){
            _ofmt_type = KS_OFMT_XML;
        }
        else if(strcasecmp(outfmt, "v30")==0 || strcasecmp(outfmt, "v3")==0){
            _ofmt_type = KS_OFMT_V30;
        }
        else if(strcasecmp(outfmt, "kspack")==0){
             _ofmt_type = KS_OFMT_KSP;
        }
        std::free(outfmt);
    }
}

SearcherWorkerFactory::SearcherWorkerFactory() 
    :   _ready(false),
        _detail(false),
        _ofmt_type(KS_OFMT_PBP),
        _pIndexUpdater(NULL),
        _updateTid(0),
        _pXMLTree(NULL), 
        _pXMLTreeDetail(NULL) {
        }

SearcherWorkerFactory::~SearcherWorkerFactory() 
{
    if (_ready) {
        _ready = false;
        if (_pIndexUpdater != NULL) {
            _pIndexUpdater->stop();
            if (_updateTid != 0) {
                pthread_join(_updateTid, NULL);
            }
            delete _pIndexUpdater;
        }
        index_lib::destroy();
    }
    if (_pXMLTree != NULL) {
        mxmlDelete(_pXMLTree);
        _pXMLTree = NULL;
    }
    if (_pXMLTreeDetail != NULL) {
        mxmlDelete(_pXMLTreeDetail);
        _pXMLTreeDetail = NULL;
    }
}

int32_t SearcherWorkerFactory::initilize(const char *path) 
{
    ini_context_t cfg;
    const ini_section_t *grp = NULL;
    const char *val = NULL;
    const char *seconf = NULL;
    uint32_t nval = 0;
    int32_t ret = 0;
    if (_ready) {
        return KS_SUCCESS;
    }
    if (!_server)
    {
        TERR("initialize _server error.");
        return KS_EFAILED;
    }
    _server->_type = FRAMEWORK::srv_type_searcher;
    ret = ini_load_from_file(path, &cfg);
    if (unlikely(ret != 0)) {
        TERR("initialize SearcherWorkerFactory by `%s' error.", 
                SAFE_STRING(path));
        return KS_EFAILED;
    }
    grp = &cfg.global;
    if (unlikely(!grp)) {
        TERR("invalid config file `%s' for SearcherWorkerFactory.",
                SAFE_STRING(path));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    //获得搜索xml文件句柄
    val = ini_get_str_value1(grp, "module_config_path");
    if (val && (val[0] != '\0')) {
        FILE *fp = NULL;
        if ((fp = fopen(val, "r")) == NULL) {
            TERR("模块配置文件 %s 打开出错, 文件可能不存在.\n", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
        if (_pXMLTree == NULL) {
            TERR("模块配置文件 %s 格式有错, 请修正您的配置文件.\n", val);
            fclose(fp);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        fclose(fp);
    }
    else {
        TERR("search module config path is null");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    //各个处理模块的初始化工作
    if (index_lib::init(_pXMLTree) != KS_SUCCESS) {
        TERR("init index lib failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_qrewiter.init(_pXMLTree) != KS_SUCCESS){
        TERR("init query rewriter failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_qp.init(_pXMLTree) != KS_SUCCESS){
        TERR("init query parser failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_is.init(_pXMLTree) != KS_SUCCESS){
        TERR("init index searcher failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_stat.init(_pXMLTree) != KS_SUCCESS){
        TWARN("init statistic failed!");
    }
    //get default value of format type
    _ofmt_type = get_outfmt_type(_pXMLTree);
    //get sort config
    val = ini_get_str_value1(grp, "sort_config_path");
    if (val && (val[0] != '\0')) {
        if (_sort.init(val) != KS_SUCCESS) {
            TERR("init sort failed! path = %s\n", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }
    else {
        TERR("sort config file is error!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    val = ini_get_str_value1(grp, "update_config_path");
    if (val && (val[0] != '\0')) {
        _pIndexUpdater = new UPDATE::IndexUpdater;
        int32_t nRes = -1;
        if ((nRes = _pIndexUpdater->init(val)) != 0) {
            TERR("init IndexUpdater failed! errno=%d", nRes);
        }
        else if (pthread_create(&_updateTid, NULL, UPDATE::Updater::start, _pIndexUpdater) != 0) {
            TERR("start updater thread failed!");
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }
    else {
        TERR("update config path is null");
    }

    //获得detail_module.xml文件句柄
    val = ini_get_str_value1(grp, "detail_module_config_path");
    if (val && (val[0] != '\0')) {
        FILE *fp = NULL;
        if ((fp = fopen(val, "r")) == NULL) {
            TERR("模块配置文件 %s 打开出错, 文件可能不存在.\n", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        _pXMLTreeDetail= mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
        if (_pXMLTreeDetail == NULL) {
            TERR("模块配置文件 %s 格式有错, 请修正您的配置文件.\n", val);
            fclose(fp);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
        fclose(fp);
        _detail = true;
    }
    else {
        _detail = false;
    }

    if(_detail)
    {
		//初始化detail各模块
    	if(di_detail_init(_pXMLTreeDetail)!=KS_SUCCESS){
    		TERR("di_detail_init failed!");
    		ini_destroy(&cfg);
    		return KS_EFAILED;
    	}
    }

    ini_destroy(&cfg);

    _ready = true;
    return KS_SUCCESS;
}

outfmt_type SearcherWorkerFactory::get_outfmt_type(mxml_node_t *pXMLTree)
{
    mxml_node_t* ofmt_conf = 0;
    const char * ofmt_type = NULL;
    ofmt_conf = mxmlFindElement(pXMLTree, pXMLTree, "outfmt", 0, 0, MXML_DESCEND);
    if(!ofmt_conf){
        TNOTE("default outfmt is pb.");
        return KS_OFMT_PBP;
    }
    else{
        ofmt_type = mxmlElementGetAttr(ofmt_conf, "type");
        if(ofmt_type){
            if(strcasecmp(ofmt_type, "pb")==0){
                TNOTE("default outfmt is pb for HTTP server.");
                return KS_OFMT_PBP;
            }
            else if(strcasecmp(ofmt_type, "xml")==0){
                TNOTE("default outfmt is xml for HTTP server.");
                return KS_OFMT_XML;
            }
            else if(strcasecmp(ofmt_type, "v30")==0 || strcasecmp(ofmt_type, "v3")==0){
                TNOTE("default outfmt is v3 for HTTP server.");
                return KS_OFMT_V30;
            }
            else if(strcasecmp(ofmt_type, "kspack")==0){
                TNOTE("default outfmt is kspack for HTTP server.");
                return KS_OFMT_KSP;
            }
            else{
                return KS_OFMT_PBP;
            }
        }
        else{
            TNOTE("default outfmt is pb for HTTP server.");
            return KS_OFMT_PBP;
        }
    }
}

FRAMEWORK::Worker * SearcherWorkerFactory::makeWorker(FRAMEWORK::Session &session) 
{
    return _ready ? 
        new (std::nothrow) SearcherWorker(session,
                _qrewiter,
                _qp,
                _is,
                _sort,
                _stat,
                _memFactory,
                _server->getMemLimit(),
                _server->getTimeoutLimit(),
                _detail,
                _ofmt_type) :
        NULL;
}

FRAMEWORK::WorkerFactory * SearcherServer::makeWorkerFactory() 
{
    return new (std::nothrow) SearcherWorkerFactory;
}

FRAMEWORK::Server * SearcherCommand::makeServer() 
{
    return new (std::nothrow) SearcherServer;
}

}
