/*********************************************************************
 * $Author: taiyi.zjh $
 *
 * $LastChangedBy: taiyi.zjh $
 * 
 * $Revision: 13113 $
 *
 * $LastChangedDate: 2012-06-29 14:29:36 +0800 (Fri, 29 Jun 2012) $
 *
 * $Id: SearcherServer.h 13113 2012-06-29 06:29:36Z taiyi.zjh $
 *
 * $Brief: searcher server 框架类 $
 ********************************************************************/

#ifndef KINGSO_SEARCHER_SEARCHERSERVER_H
#define KINGSO_SEARCHER_SEARCHERSERVER_H

#include "mxml.h"
#include "commdef/SearchResult.h"
#include "util/common.h"
#include "framework/namespace.h"
#include "framework/Worker.h"
#include "framework/Server.h"
#include "framework/Session.h"
#include "framework/Command.h"
#include "framework/Context.h"
#include "framework/MemPoolFactory.h"
#include "index_lib/IndexLib.h"
#include "queryparser/QueryParser.h"
#include "queryparser/QPResult.h"
#include "search/Search.h"
#include "statistic/StatisticManager.h"
#include "statistic/StatisticResult.h"
#include "sort/SearchSortFacade.h"
#include "sort/SortResult.h"
#include "update/Updater.h"
#include "update/IndexUpdater.h"
#include "searcher.h"
#include "queryparser/QueryRewriter.h"
#include "FormatProcessor.h"


namespace kingso_searcher {

class SearcherWorker : public FRAMEWORK::Worker {
    public:
        SearcherWorker(FRAMEWORK::Session &session,
                queryparser::QueryRewriter &qrewriter,
                queryparser::QueryParser &qp,
                search::Search &is,
                SORT::SearchSortFacade &sort,
                statistic::StatisticManager &stat,
                FRAMEWORK::MemPoolFactory &memFactory,
                uint64_t memLimit = 0,
                uint64_t timeout = 0,
                bool detail = false,
                outfmt_type ofmt_type = KS_OFMT_PBP);
        ~SearcherWorker();
        /**
         *@brief SearcherWorker的处理函数
         *@return 0,成功 非0,失败
         */
        virtual int32_t run();
        /**
         *@brief 接收返回信息
         *@param 超时处理
		 */
        void handleTimeout();
        /**
         *@brief 获取nid组成的字符串用来查询detailindex
         *@param  Nid数组头指针
		 *@param  数据元素个数
		 *@param  内存池
		 *@param  输出字符串头指针
		 *@param  输出字符串长度
         *@return 0,成功 非0,失败
         */		
        int32_t get_nid_str(int64_t* pNid, int32_t num, MemPool* memPool, char** output, uint32_t* outlen);
        int32_t getDetailInfo(const char *q_reslut, uint32_t q_size, const char *pureString, char** res, int* res_size);
        void get_outfmt_type(const char *reslut, uint32_t size);
    private:
        queryparser::QueryRewriter &_qrewriter;
        queryparser::QueryParser &_qp;
        search::Search &_is;
        SORT::SearchSortFacade &_sort;
        statistic::StatisticManager &_stat;
        FRAMEWORK::MemPoolFactory &_memFactory;
	    FormatProcessor _formatProcessor;
        uint64_t _memLimit;
        uint64_t _timeout;
        bool _detail;
        outfmt_type _ofmt_type;
};

class SearcherWorkerFactory : public FRAMEWORK::WorkerFactory {
    public:
        SearcherWorkerFactory();
        ~SearcherWorkerFactory();
    public:
        /**
         *@brief 根据配置信息，进行初始化
         *@param path 配置文件路径
         *@return 0,成功 非0,失败
         */
        virtual int32_t initilize(const char *path);
        /**
         *@brief 创建Worker对象
         *@param session 目标session对象
         *@return Worker对象
         */
        virtual FRAMEWORK::Worker * makeWorker(FRAMEWORK::Session &session);
    private:
        outfmt_type get_outfmt_type(mxml_node_t *pXMLTree);
    private:
        bool _ready;
        queryparser::QueryRewriter _qrewiter;
        queryparser::QueryParser _qp;
        search::Search _is;
        statistic::StatisticManager _stat;
        SORT::SearchSortFacade _sort;
        FRAMEWORK::MemPoolFactory _memFactory;		
        mxml_node_t *_pXMLTree;
        mxml_node_t *_pXMLTreeDetail;
        bool _detail;
        outfmt_type _ofmt_type;
        UPDATE::Updater *_pIndexUpdater;
        pthread_t _updateTid;
};

class SearcherServer : public FRAMEWORK::Server {
    public:
        SearcherServer() { }
        ~SearcherServer() { }
    protected:
        virtual FRAMEWORK::WorkerFactory *makeWorkerFactory();
};

class SearcherCommand : public FRAMEWORK::CommandLine {
    public:
        SearcherCommand() : CommandLine("ks_search_server") { }
        ~SearcherCommand() { }
    protected:
        virtual FRAMEWORK::Server * makeServer();
};

}

#endif
