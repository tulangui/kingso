#ifndef _DI_CLASS_DETAILSERVER_H_
#define _DI_CLASS_DETAILSERVER_H_


#include "mxml.h"
#include "util/common.h"
#include "framework/Worker.h"
#include "framework/Server.h"
#include "framework/Session.h"
#include "framework/Command.h"
#include "framework/Context.h"
#include "framework/MemPoolFactory.h"
#include "framework/namespace.h"
#include "update/DetailUpdater.h"


#define DETAIL detail


using namespace UPDATE;

namespace detail{

class DetailWorker : public FRAMEWORK::Worker
{
	public:
		
		DetailWorker(FRAMEWORK::Session &session, 
		             uint64_t timeout = 0);
		
		~DetailWorker();
		
		/**
		 *@brief DetailWorker的处理函数
		 *@return 0,成功 非0,失败
		 */
		virtual int run();
		
		/**
		 *@brief 接收返回信息
		 *@param req request对象
		 *@return 0,成功 非0,失败
		 */
		void handleTimeout();
		
	private:
		
		uint64_t _timeout;
};

class DetailWorkerFactory : public FRAMEWORK::WorkerFactory
{
	public:
		
		DetailWorkerFactory();
		
		~DetailWorkerFactory();
		
	public:
		
		/**
		 *@brief 根据配置信息，进行初始化
		 *@param path 配置文件路径
		 *@return 0,成功 非0,失败
		 */
		virtual int initilize(const char *path);
		
		/**
		 *@brief 创建Worker对象
		 *@param session 目标session对象
		 *@return Worker对象
		 */
		virtual FRAMEWORK::Worker * makeWorker(FRAMEWORK::Session &session);
		
	private:
		
		bool _ready;
		mxml_node_t *_pXMLTree;
		
		Updater *_pDetailUpdater;
		pthread_t _updateTid;
};

class DetailServer : public FRAMEWORK::Server
{
	public:
	
		DetailServer() { }
		
		~DetailServer() { }
		
	protected:
		
		virtual FRAMEWORK::WorkerFactory* makeWorkerFactory();
		
};

class DetailCommand : public FRAMEWORK::CommandLine
{
	public:
		
		DetailCommand() : CommandLine("ks_search_server") { }
		
		~DetailCommand() { }
		
	protected:
		
		virtual FRAMEWORK::Server * makeServer();
};


};


#endif //_SEARCHERSERVER_H_
