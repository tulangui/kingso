#include <zlib.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <sys/types.h>

#include "VisitStat.h" 
#include "util/MD5.h"
#include "util/iniparser.h"
#include "index_lib/DocIdManager.h"

VisitStat::VisitStat()
{
    _queryNum = 0;
    _maxStatNum = 10<<20;

    _timeSegBegin = 0;
    _timeSegEnd = 0x7fffffff;

    _docNum = 0;
    _logFileNum = 0;

    _visit = NULL;

    _finishNum = 0;
    _hashQuery = NULL;

    _deal_state = 0;    // 状态
    _vist_state = 0;    // 状态
}

VisitStat::~VisitStat()
{
    if(_visit) free(_visit);
    _visit = NULL;

    if(_hashQuery) {
        uint64_t key;
        QueryInfo info;

        _hashQuery->itReset();
        while(_hashQuery->itNext(key, info)) {
            if(info.query) free(info.query);
        }
       delete _hashQuery;
    }
    _hashQuery = NULL;
}


// 统计访问频度
int VisitStat::statFreq(MemPool* pool, const char* query, int len, int freq)
{
    FRAMEWORK::Context context;
    
    pool->reset();
    context.setMemPool(pool);
    
    queryparser::QPResult qpResult(pool);
    int ret = _qp.doParse(&context, &qpResult, query);
    if(ret != KS_SUCCESS) return -1;

    SearchResult seResult;
    ret = _search.doQuery(&context, &qpResult, &_sort, &seResult);
    if(ret != KS_SUCCESS){
        return -1;
    }
   
    _vLock.lock();
    for(uint32_t i = 0; i < seResult.nDocFound; i++) {
        _visit[seResult.nDocIds[i]].st += freq;
    }
    _finishNum++;
    if(_finishNum % 10000 == 0)
        fprintf(stdout, "finish %d / %d\n", _finishNum, _hashQuery->size());
    _vLock.unlock();

    return 0;
}

// 对指定时间段内的日志做处理，根据search/filter 做排重
int VisitStat::dealQueryLog(const char* filename)
{
    gzFile fp = gzopen(filename, "rb");
    if(!fp) {
        fprintf(stderr, "open %s error\n", filename);
        return -1;
    }
   
    char buf[102400];
    char query[102400]; 
        
    util::MD5_CTX ctx;
    QueryInfo info;
    unsigned char queryKey[16];

    info.freq = 0;
    info.query = NULL;

    while(gzgets(fp, buf, 102400)) {
        char* p = strchr(buf, '?');
        if(NULL == p) continue;
        p = strchr(p+1, '?');
        if(NULL == p) continue;
        p++;
        
        if(strstr(p, "&n=0")) continue;

        char* search = NULL;
        char* filter = NULL;
        
        char* key = p;
        while(search == NULL || filter == NULL) {
            p = strchr(key, '#');
            if(p) *p++ = 0;
            
            if(search == NULL && strncasecmp(key, "search=", 7) == 0) {
                search = key;
            } else if(filter == NULL && strncasecmp(key, "filter=", 7) == 0) {
                filter = key;
            }
            if(p == NULL) break;
            key = p;
        }
        
        if(search == NULL) continue;
        // 去掉gid信息
        while(1) {
            p = strstr(search, "gid=");
            if(p == NULL) break;
            
            char* p1 = p + 4;
            while(*p1 && *p1 != '&') p1++;
            if(*p1) p1++;
            while(*p1) *p++ = *p1++;
            if(*(p-1) == '&') {
                *(p-1) = 0;
            } else {
                *p = 0;
            }
        }
        // 去掉旺旺在线过滤
        if(filter) {
            p = strstr(filter, "olu=");
            if(p) {
                char* p1 = p + 4;
                while(*p1 && *p1 != '&') p1++;
                if(*p1) p1++;
                while(*p1) *p++ = *p1++;
                if(*(p-1) == '&') {
                    *(p-1) = 0;
                } else {
                    *p = 0;
                }
            }
            while (1) {
                p = strstr(filter, "gid=");
                if(p == NULL) break;

                char* p1 = p + 4;
                while(*p1 && *p1 != '&') p1++;
                if(*p1) p1++;
                while(*p1) *p++ = *p1++;
                if(*(p-1) == '&') {
                    *(p-1) = 0;
                } else {
                    *p = 0;
                }
            }
        }
        
        unsigned int len = 0;
        if(filter) {
            len = sprintf(query, "%s#%s", search, filter);
        } else {
            len = sprintf(query, "%s", search);
        }
      
        util::MD5Init(&ctx);
        util::MD5Update(&ctx, (const unsigned char*)query, len);
        util::MD5Final(queryKey, &ctx);
    
        _lock.lock();
        _queryNum++;
        if(_queryNum >= _maxStatNum) {
            _lock.unlock();
            break;
        }
        _hashQuery->insert(*(uint64_t*)queryKey, info); 
        util::HashMap<uint64_t, QueryInfo>::iterator it = _hashQuery->find(*(uint64_t*)queryKey);
        _lock.unlock();

        if(it == _hashQuery->end()) {
            fprintf(stderr, "insert error\n");
            return -1;
        }
        it->value.freq++;
        it->value.query = (char*)malloc(len+1);
        if(it->value.query == NULL) {
            fprintf(stderr, "alloc error, len=%d\n", len);
            return -1;
        }
        strcpy(it->value.query, query);
    }
    gzclose(fp);

    return 0;
}

int VisitStat::output(VisitResult& result)
{
    index_lib::DocIdManager* doc = index_lib::DocIdManager::getInstance();
    for(int i = 0; i < _docNum; i++) {
        _visit[i].nid =  doc->getNid(i);
    }
  
    result.setSingleResult(_visit, _docNum);
/*    
    qsort(_visit, _docNum, sizeof(VisitInfo), cmp);

    for(int i = 0; i < _docNum; i++) {
        if(_visit[i].st <= 0)  break;
        fprintf(stdout, "%lu %lu\n", _visit[i].st, _visit[i].nid);
    }
*/    
    return 0;
}

// 获取库容量等等信息
int VisitStat::init(const char* serverCfg, const char* logCfg)
{
    if(initLog(logCfg) != KS_SUCCESS) {
        return -1;
    }

    const char *val = NULL;
    const char *seconf = NULL;

    uint32_t nval = 0;
    int32_t ret = 0;
    
    ini_context_t cfg;
    ret = ini_load_from_file(serverCfg, &cfg);
    if (unlikely(ret != 0)) {
        TERR("initialize SearcherWorkerFactory by `%s' error.", 
                SAFE_STRING(serverCfg));
        return KS_EFAILED;
    }
    const ini_section_t* grp = &cfg.global;
    if (unlikely(!grp)) {
        TERR("invalid config file `%s' for SearcherWorkerFactory.",
                SAFE_STRING(serverCfg));
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
        TERR("search module config serverCfg is null");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    //各个处理模块的初始化工作
    if (index_lib::init(_pXMLTree) != KS_SUCCESS) {
        TERR("init index lib failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_qp.init(_pXMLTree) != KS_SUCCESS){
        TERR("init query parser failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    if (_search.init(_pXMLTree) != KS_SUCCESS){
        TERR("init index searcher failed!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    val = ini_get_str_value1(grp, "sort_config_path");
    if (val && (val[0] != '\0')) {
        if (_sort.init(val) != KS_SUCCESS) {
            TERR("init sort failed! serverCfg = %s\n", val);
            ini_destroy(&cfg);
            return KS_EFAILED;
        }
    }
    else {
        TERR("sort config file is error!");
        ini_destroy(&cfg);
        return KS_EFAILED;
    }
    ini_destroy(&cfg);
   
    index_lib::DocIdManager * doc = index_lib::DocIdManager::getInstance();
    _docNum = doc->getDocIdCount();
    _visit = (VisitInfo*)malloc(_docNum * sizeof(VisitInfo));
    if(NULL == _visit) return KS_EFAILED;
    memset(_visit, 0, _docNum * sizeof(VisitInfo));

    _hashQuery = new util::HashMap<uint64_t, QueryInfo>(_maxStatNum + 1);
    if(NULL == _hashQuery) {
        return KS_EFAILED;
    }

    return KS_SUCCESS;
}

// 根据日志配置文件获取日志路径及压缩格式
int VisitStat::initLog(const char* logPath)
{
    ini_context_t cfg;
    int ret = ini_load_from_file(logPath, &cfg);
    if (unlikely(ret != 0)) {
        TERR("initialize SearcherWorkerFactory by `%s' error.", SAFE_STRING(logPath));
        return KS_EFAILED;
    }
    const ini_section_t* grp = &cfg.global;
    if (unlikely(!grp)) {
        TERR("invalid config file `%s' for SearcherWorkerFactory.", SAFE_STRING(logPath));
        ini_destroy(&cfg);
        return KS_EFAILED;
    }

    const char* val = ini_get_str_value1(grp, "alog.appender.access.fileName");
    if(val == NULL || val[0] == 0) {
        TERR("get acclog path error", SAFE_STRING(logPath));
        return KS_EFAILED;
    }
    strcpy(_logPath, val);
    char* p = strrchr(_logPath, '/');
    if(p == NULL) {
        TERR("get acclog path error");
        return KS_EFAILED;
    }
    *p++ = 0;
    char* logName = p; 
    int nameLen = strlen(logName);

    val = ini_get_str_value1(grp, "alog.appender.access.compress");
    if(val == NULL || val[0] == 0) {
        TERR("get acclog zipflag error", SAFE_STRING(logPath));
        return KS_EFAILED;
    }
    if(strcasecmp(val, "true") == 0 || strcasecmp(val, "yes") == 0) {
        _zipFlag = true;
    } else {
        _zipFlag = false;
    }

    DIR* dir = opendir(_logPath);
    if(NULL == dir) {
        TERR("open %s error", _logPath);
        return -1;
    }
    
    char filename[PATH_MAX];
    struct dirent* ent = NULL;
    while((ent = readdir(dir))) {
        char* p = ent->d_name;
        p = p + strlen(p) - 3;
        if(memcmp(p, ".gz", 3)) continue;
   
        if(memcmp(ent->d_name, logName, nameLen)) continue;

        strcpy(filename, ent->d_name);
        p = strrchr(filename, '.');
        if(p == NULL) continue;
        *p = 0;

        p = strrchr(filename, '.');
        if(p == NULL) continue;
        *p++ = 0;   // 日期
        
        time_t t = str2time(p);
        if(t < _timeSegBegin || t > _timeSegEnd) continue;

        sprintf(_logFileName[_logFileNum], "%s/%s", _logPath, ent->d_name);
        if(++_logFileNum >= MAX_LOG_NUM) break;
    }
    closedir(dir);

    return KS_SUCCESS;
}

// 将字符串格式的时间转成time_t, 格式:年月日时分秒
time_t VisitStat::str2time(const char* strTime)
{
    time_t now = time(NULL);
    struct tm st;
    
    localtime_r(&now, &st);
    int ret = sscanf(strTime, "%4d%2d%2d%2d%2d%2d", &st.tm_year, &st.tm_mon, &st.tm_mday, &st.tm_hour, &st.tm_min, &st.tm_sec);
    if(ret != 6) {
        fprintf(stderr, "error, %s %d\n", strTime, ret);
        return 0;
    }

    st.tm_year -= 1900;
    st.tm_mon--;

    return mktime(&st);
}

int VisitStat::cmp(const void * p1, const void * p2)
{
    if(((VisitInfo*)p1)->st > ((VisitInfo*)p2)->st)
        return -1;
    if(((VisitInfo*)p1)->st < ((VisitInfo*)p2)->st)
        return 1;
    return 0;
}

void * VisitStat::deal_query(void* para)
{
    int file = 0;
    VisitStat* st = (VisitStat*)para;

    while(st->_deal_state == 0 && st->_logDeal < st->_logFileNum) {
        st->_lock.lock();
        file = st->_logDeal++;
        st->_lock.unlock();
        
        if(file >= st->_logFileNum) break;
        
        if(st->dealQueryLog(st->_logFileName[file])) {
            TERR("deal query %s error", st->_logFileName[file]);
            st->_lock.lock();
            st->_deal_state++;
            st->_lock.unlock();
            break;
        } 
        TLOG("ok %s\n", st->_logFileName[file]);
    }

    st->_lock.lock();
    st->_hashQuery->itReset();
    st->_lock.unlock();

    return (void*)st->_deal_state;
}

void* VisitStat::stat_visit(void* para)
{
    int no = 0;
    VisitStat* st = (VisitStat*)para;

    uint64_t key;
    QueryInfo info;
    
    MemPool pool;
    char query[10240];

    while(st->_vist_state == 0) {
        st->_lock.lock();
        bool flag = st->_hashQuery->itNext(key, info);
        st->_lock.unlock();
        
        if(flag == false) break;

        int len = sprintf(query, "/bin/search?auction?%s#other=usetrunc=no", info.query);
        st->statFreq(&pool, query, len, info.freq);
    }

    return (void*)st->_vist_state;
}

