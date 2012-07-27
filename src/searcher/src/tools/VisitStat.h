#ifndef TOOLS_VISITSTAT_H_
#define TOOLS_VISITSTAT_H_

#include "mxml.h"
#include "searcher.h"
#include "util/common.h"
#include "framework/Worker.h"
#include "framework/Server.h"
#include "framework/Session.h"
#include "framework/Command.h"
#include "framework/Context.h"
#include "framework/MemPoolFactory.h"
#include "framework/namespace.h"
#include "commdef/SearchResult.h"
#include "search/Search.h"
#include "queryparser/QueryParser.h"
#include "queryparser/QPResult.h"
#include "index_lib/IndexLib.h"
#include "sort/SearchSortFacade.h"
#include "sort/SortResult.h"
#include "statistic/StatisticResult.h"
#include "statistic/StatisticManager.h"
#include "update/Updater.h"
#include "update/IndexUpdater.h"
#include "util/HashMap.hpp"
#include "util/ThreadLock.h"
#include "VisitResult.h"

#define MAX_THRAD_NUM 16
#define MAX_LOG_NUM   1024

class VisitStat {
public:
    VisitStat();
    ~VisitStat();

    // 获取库容量等等信息
    int init(const char* serverCfg, const char* logCfg);

    // 输出统计结果
    int output(VisitResult& result);

    // 获取query数量信息
    int getQueryNum(int& allNum, int& trueNum) {
        if(_hashQuery == NULL) return -1;
        allNum = _queryNum;
        trueNum = _hashQuery->size();
        return 0;
    }

    // 设置最大query数量
    void setStatMaxNum(int maxStatNum) { _maxStatNum = maxStatNum;}
    // 设置需要统计的日志的时间段,格式: 年月日时分
    void setSegTime(time_t segBegin, time_t segEnd) {
        _timeSegBegin = segBegin;
        _timeSegEnd = segEnd;
    }

private:
public:
    // 将字符串格式的时间转成time_t, 格式:年月日时分秒
    time_t str2time(const char* strTime);
    
    // 根据日志配置文件获取日志路径及压缩格式
    int initLog(const char* logPath);
    
    // 统计访问频度
    int statFreq(MemPool* pool, const char* query, int len, int freq);

    // 对指定时间段内的日志做处理，根据search/filter 做排重
    int dealQueryLog(const char* filename);

private:
    int32_t _timeSegBegin;  // 访问日志的开始时间
    int32_t _timeSegEnd;    // 访问日志的结束时间

    bool _zipFlag;           // 压缩标志
    char _logPath[PATH_MAX]; // 日志文件名

    int32_t _queryNum;       // 排重前query数量
    int32_t _maxStatNum;     // 最多统计的query数量
    util::Mutex _lock;       // 计数锁,map遍历锁

    int32_t _docNum;         // 文档总数量
    VisitInfo* _visit;       // 访问统计
    util::Mutex _vLock;      // 统计结果锁

    int32_t _finishNum;     // 完成数量
    util::HashMap<uint64_t, QueryInfo>* _hashQuery;

    int _logFileNum;        // 日志文件数量
    int _logDeal;           // 处理到的模块
    char _logFileName[MAX_LOG_NUM][PATH_MAX]; 

    mxml_node_t *_pXMLTree;  //

    queryparser::QueryParser _qp;
    search::Search _search;
    SORT::SearchSortFacade _sort;

private:
    int _deal_state;    // 状态
    int _vist_state;    // 状态
    
    static void * deal_query(void* para);
    static void* stat_visit(void* para);
    
    static int cmp(const void * p1, const void * p2);
};

#endif //TOOLS_VISITSTAT_H_
