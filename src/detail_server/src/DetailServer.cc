#include "DetailServer.h"
#include "commdef/commdef.h"
#include "util/iniparser.h"
#include "libdetail/detail.h"
#include <stdlib.h>
#include "util/Log.h"
#include "util/py_url.h"
#include "util/charsetfunc.h"
#include "update/DetailUpdater.h"
#include "framework/Compressor.h"

using namespace std;
using namespace UPDATE;

namespace detail{

#define srv_type_detail srv_type_searcher



/**********************************
 *         DetailWorker           *
 **********************************/

DetailWorker::DetailWorker(FRAMEWORK::Session &session, uint64_t timeout)
	: Worker(session), _timeout(timeout)
{
}

DetailWorker::~DetailWorker()
{
}

void DetailWorker::handleTimeout() 
{
	_session.setStatus(FRAMEWORK::ss_timeout);
  _session.response();
}

int DetailWorker::run()
{
	const char* pureString = NULL;
	int pureSize = 0;
	
	char* psid = 0;
	char* pLightKey = 0;
	char* pUserLoc = 0;
	char* pET = 0;
	char* pLatlng = 0;
	char* pDL = 0;
	
	char* resData = NULL;
	int resSize = 0;
	char* compress = 0;
	uint32_t compsize = 0;
	FRAMEWORK::Compressor* compressor = 0;
	
	char** args = 0;
	int fieldCount = 0;
	int ret = 0;

	//check session status
	FRAMEWORK::session_status_t status = _session.getStatus();
	if(status==FRAMEWORK::ss_timeout){
		TWARN("timeout, return!");
		handleTimeout();
		return KS_SUCCESS;
	}
	
	//get query infomation
	FRAMEWORK::Query &query = _session.getQuery();
	pureSize = query.getPureQuerySize();
	pureString = query.getPureQueryData();
	if (unlikely(!pureString || pureSize==0)) {
		TWARN("query string is null, return!");
		_session.setStatus(FRAMEWORK::ss_error);
		_session.response();
		return KS_EFAILED;
	}
	psid = FRAMEWORK::Query::getParam(pureString, pureSize, SID);
	if(!psid){
		TWARN("psid is null, return![query:%s][SID:%s]", pureString, SID);
		_session.setStatus(FRAMEWORK::ss_error);
		_session.response();
		return KS_EFAILED;
	}
	pLightKey = FRAMEWORK::Query::getParam(pureString, pureSize, LIGHTKEY);
	if(pLightKey && pLightKey[0]!=0){
		int len = strlen(pLightKey);
		ret = decode_url(pLightKey, len, pLightKey, len+1);
		if(ret<0){
			TWARN("decode_url pLightKey error, return!");
			_session.setStatus(FRAMEWORK::ss_error);
			_session.response();
			return KS_EFAILED;
		}
		pLightKey[ret] = 0;
	}
	pUserLoc = FRAMEWORK::Query::getParam(pureString, pureSize, USERLOC);
	if(pUserLoc && pUserLoc[0]!=0){
		int len = strlen(pUserLoc);
		char* buf = (char*)malloc(len*2+1);
		if(!buf){
			TERR("malloc pUserLoc error, return!");
			_session.setStatus(FRAMEWORK::ss_error);
			_session.response();
			return KS_EFAILED;
		}
		ret = decode_url(pUserLoc, len, pUserLoc, len+1);
		if(ret<0){
			TWARN("decode_url pUserLoc error, return!");
			std::free(buf);
			_session.setStatus(FRAMEWORK::ss_error);
			_session.response();
			return KS_EFAILED;
		}
		pUserLoc[ret] = 0;
		len = ret;
		ret = code_gbk_to_utf8(buf, len*2+1, pUserLoc, 0);
		if(ret<0){
			memcpy(buf, pUserLoc, len);
			ret = len;
		}
		buf[ret] = 0;
		std::free(pUserLoc);
		pUserLoc = buf;
	}
	pET = FRAMEWORK::Query::getParam(pureString, pureSize, ET);
	pLatlng = FRAMEWORK::Query::getParam(pureString, pureSize, LATLNG);
	pDL = FRAMEWORK::Query::getParam(pureString, pureSize, DL);
		
	fieldCount = get_di_field_count();
	if(di_detail_arg_alloc(&args)<(fieldCount+1)){
		TERR("di_detail_arg_alloc error, return!");
		_session.setStatus(FRAMEWORK::ss_error);
		_session.response();
		return KS_EFAILED;
	}
	di_detail_arg_set(args, fieldCount+1, "title", 5, pLightKey, pDL);
	di_detail_arg_set(args, fieldCount+1, "real_post_fee", 13, pUserLoc, pDL);
	di_detail_arg_set(args, fieldCount+1, "promotions", 10, pET, pDL);
	di_detail_arg_set(args, fieldCount+1, "ends", 4, pET, pDL);
	di_detail_arg_set(args, fieldCount+1, "latlng", 6, pLatlng, pDL);
	di_detail_arg_set(args, fieldCount+1, "zk_time", 7, pET, pDL);
	di_detail_arg_set(args, fieldCount+1, "dl", 2, pDL, pDL);
	
	//set LogInfo level
	_session._logInfo._eRole = FRAMEWORK::sr_simple;
	
	//Deal with detail
	if(di_detail_get(psid, pDL, args, fieldCount, &resData, &resSize)){
		resData = (char*)malloc(2);
		resData[0] = '0';
		resData[1] = 0;
		resSize = 1;
	}
	if(_timeout>0 && _session.getLatencyTime()>_timeout){
		_session.setStatus(FRAMEWORK::ss_timeout);
	}
	
	if(psid)std::free(psid);
	if(pLightKey)std::free(pLightKey);
	if(pUserLoc)std::free(pUserLoc);
	if(pET)std::free(pET);
	if(pLatlng)std::free(pLatlng);
	if(pDL)std::free(pDL);
	if(args)di_detail_arg_free(args);
	
	compressor = FRAMEWORK::CompressorFactory::make(FRAMEWORK::ct_zlib, NULL);
	if(!compressor){
		TWARN("FRAMEWORK::CompressorFactory::make error, return!");
		std::free(resData);
		_session.setStatus(FRAMEWORK::ss_error);
		_session.response();
		return KS_EFAILED;
	}
	ret = compressor->compress(resData, resSize, compress, compsize);
	FRAMEWORK::CompressorFactory::recycle(compressor);
	if(ret!=KS_SUCCESS || compress==0 || compsize==0){
		TWARN("compress error, return!");
		std::free(resData);
		_session.setStatus(FRAMEWORK::ss_error);
		_session.response();
		return KS_EFAILED;
	}
	
	std::free(resData);
	resData = compress;
	resSize = compsize;

	_session.setResponseData(resData, resSize);
	_session.response();
	return KS_SUCCESS;
}

/**********************************
 *     DetailWorkerFactory        *
 **********************************/

DetailWorkerFactory::DetailWorkerFactory() 
	: 
	_ready(false),
	_pXMLTree(NULL),
	_pDetailUpdater(NULL),
	_updateTid(0)
{
}

DetailWorkerFactory::~DetailWorkerFactory() 
{
	if(_ready){
		_ready = false;
		if(_pDetailUpdater){
            _pDetailUpdater->stop();
			if(_updateTid!=0){
				pthread_join(_updateTid, NULL);
				_updateTid = 0;
			}
			delete _pDetailUpdater;
			_pDetailUpdater = NULL;
		}
	}
	if(_pXMLTree!=NULL){
		mxmlDelete(_pXMLTree);
		_pXMLTree = NULL;
	}
}

int DetailWorkerFactory::initilize(const char *path) 
{
	ini_context_t cfg;
	const ini_section_t *grp = NULL;
	const char *val = NULL;
	int ret = 0;
	
	if (_ready) {
		return KS_SUCCESS;
	}
	if (!_server){
		TERR("initialize _server error.");
		return KS_EFAILED;
	}
	_server->_type = FRAMEWORK::srv_type_detail;
	
	ret = ini_load_from_file(path, &cfg);
	if(unlikely(ret!=0)){
		TERR("initialize DetailWorkerFactory by `%s' error.", SAFE_STRING(path));
		return KS_EFAILED;
	}
	grp = &cfg.global;
	if(unlikely(!grp)){
		TERR("invalid config file `%s' for DetailWorkerFactory.", SAFE_STRING(path));
		ini_destroy(&cfg);
		return KS_EFAILED;
	}

	//获得搜索xml文件句柄
	val = ini_get_str_value1(grp, "module_config_path");
	if(val && (val[0]!='\0')){
		FILE *fp = NULL;
		if((fp=fopen(val, "r"))==NULL){
			TERR("模块配置文件 %s 打开出错, 文件可能不存在.\n", val);
			return KS_EFAILED;
		}
		_pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
		if(_pXMLTree==NULL){
			TERR("模块配置文件 %s 格式有错, 请修正您的配置文件.\n", val);
			fclose(fp);
			return KS_EFAILED;
		}
		fclose(fp);
	}
	else {
		TERR("detail module config path is null");
		return KS_EFAILED;
	}
	
	//各个处理模块的初始化工作
	if(di_detail_init(_pXMLTree)!=KS_SUCCESS){
		TERR("di_detail_init failed!");
		ini_destroy(&cfg);
		return KS_EFAILED;
	}
	
	// 初始化updater
	val = ini_get_str_value1(grp, "update_config_path");
	if(val && (val[0]!='\0')){
		do{
			_pDetailUpdater = new UPDATE::DetailUpdater;
			if(!_pDetailUpdater){
				TWARN("new DetailUpdater failed, running with no updater!");
				break;
			}
			int nRes = -1;
			if((nRes=_pDetailUpdater->init(val))!= 0){
				TWARN("init DetailUpdater failed, running with no updater! errno=", nRes);
				delete _pDetailUpdater;
				_pDetailUpdater = NULL;
				break;
			}
			if(pthread_create(&_updateTid, NULL, Updater::start, _pDetailUpdater) != 0) {
				TWARN("start updater thread failed, running with no updater!");
				delete _pDetailUpdater;
				_pDetailUpdater = NULL;
				break;
			}
		}while(0);
	}
	else{
		TWARN("update module config path is null, running with no updater!");
	}
	
	ini_destroy(&cfg);
	_ready = true;
	return KS_SUCCESS;
}

FRAMEWORK::Worker* DetailWorkerFactory::makeWorker(FRAMEWORK::Session &session) 
{
	return _ready ? 
		new (std::nothrow) DetailWorker(session, _server->getTimeoutLimit()) : NULL;
}

/**********************************
 *         DetailServer           *
 **********************************/

FRAMEWORK::WorkerFactory* DetailServer::makeWorkerFactory() 
{
	return new (std::nothrow) DetailWorkerFactory;
}

/**********************************
 *         DetailCommand          *
 **********************************/

FRAMEWORK::Server * DetailCommand::makeServer() 
{
	return new (std::nothrow) DetailServer;
}


};	
