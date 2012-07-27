#include "WorkerFactory.h"
#include "DetailWorker.h"
#include "SearcherWorker.h"
#include "EnterWorker.h"
#include "common/compress.h" 
#include "SignDict.h"

namespace update {

Worker::Worker()
{
    _fd = 0;
    _flux = 0;

    _valid = true;
    _nextRow = NULL;
    _beginTime = time(NULL);

    _msgInit = false;
    _nextWorker = NULL;
  
    _unCompMem = NULL;
    _unCompSize= 0;

    _pKsbConf = NULL;       //libbuild的配置文件
    _pKsbAuction = NULL;    //libbuild中间结果
    _pKsbResult = NULL;     //libbuild输出

    _roleName[DETAIL] = "detail";
    _roleName[SEARCHER] = "searcher";
    _roleName[UPDATE_SERVER] = "updater";
    _roleName[ROLE_LAST] = "unknown";

    _actionName[ACTION_NONE] = "none";
    _actionName[ACTION_ADD] = "add";
    _actionName[ACTION_DELETE] = "del";
    _actionName[ACTION_MODIFY] = "modify";
    _actionName[ACTION_UNDELETE] = "undel";
    _actionName[ACTION_LAST] = "unknown";
}

Worker::~Worker()
{
    if(_fd > 0) {
        close(_fd);
        _fd = 0;
    }
    if(_unCompMem) {
        free(_unCompMem);
        _unCompMem = NULL;
    }

    if(_pKsbConf) {
        ksb_destroy_config(_pKsbConf);
        _pKsbConf = NULL;
    }
    if(_pKsbAuction) {
        ksb_destroy_auction(_pKsbAuction);
        _pKsbAuction = NULL;
    }
    if(_pKsbResult) {
        ksb_destroy_result(_pKsbResult);
        _pKsbResult = NULL;
    }
}
            
int Worker::init(Config* cfg, ClusterMessage* info)
{
    _info = *info;
    _fluxLimit = cfg->_flux_limit;

    _pKsbConf = ksb_load_config(cfg->_build_conf, MODE_ALL);
    if(NULL == _pKsbConf) {
        TERR("ksb_load_config error, return![path:%s]", cfg->_build_conf);
        return -1;
    }
    _pKsbAuction = ksb_create_auction();
    _pKsbResult = ksb_create_result();
    if(NULL == _pKsbAuction || NULL == _pKsbResult) {
        TERR("ksb_create_result error, return!");
        return -1;
    }
    
    _select.colNum = info->colNum;
    _select.colNo  = info->colNo;

    _seq = info->seq;
    if(_seq > 0) _seq--;
    _select.seq = info->seq;

    return 0;
}
    
int Worker::process(net_head_t* head, char* message)
{
    if(_msgInit == false) {
        int ret = nextWork();
        if(ret < 0) {
            return RET_FAILE;
        }
        if(ret == 0) {
            return RET_NO;
        }
    }
    head->seq = _msg.seq;

    return RET_SUCCESS;
}

int Worker::nextWork()
{
    int ret = -1;
    _msgInit = false;

    ret = _que.pop(_msg, _select);
    _seq = _select.seq;
    if(ret <= 0) {
        return ret;
    }

    if(_msg.cmpr=='Z' || _msg.cmpr=='z') { // uncompress
        uLong size = uncompressSize((Bytef*)_msg.ptr, _msg.size);
        if(size > _unCompSize) {
            char* mem = (char*)malloc(size);
            if(NULL == mem) {
                TWARN("malloc msg buffer error, return![seq=%lu org=%u, size:%lu]", _msg.seq, _msg.size, size);
                return -1;
            }
            if(_unCompMem) free(_unCompMem);
            _unCompMem = mem;
            _unCompSize = size;
        }
        if(zdecompress((Bytef*)_unCompMem, &size, (Bytef*)_msg.ptr, _msg.size)){
            TWARN("zdecompress error, return!");
            return UPCMD_ACK_EFORMAT;
        }
        _msg.ptr  = _unCompMem;
        _msg.size = size;
    }

    _docMessage.Clear();
    if(likely(_docMessage.ParseFromArray(_msg.ptr, _msg.size))) {
        _msgInit = true;
        return 1;
    } else {
        TWARN("DocMessage ParseFormArray fail, seq=%lu", _msg.seq);
        return -1;
    }

    return -1;
}

const char* Worker::role()
{
    return _roleName[_info.role];
}

const char* Worker::action()
{
    if(_msgInit == false) {
        return _actionName[ACTION_LAST];
    }
    return _actionName[_msg.action];
}

uint64_t Worker::nid()
{
    if(_msgInit == false) {
        return 0;
    }
    return _msg.nid;
}

uint64_t Worker::seq()
{
    if(_msgInit == false) {
        return 0;
    }
    return _msg.seq;
}

int Worker::checkFlux()
{
    const static int64_t TRAFFIC_CHECK_TIME = 10; //每10s检查下流量
    time_t now = time(NULL);
    time_t use = now - _beginTime;
    if(use <= 0) {
        use = 1;
    }

    int ret = 0;
    uint64_t flux = _flux / use;
    if(flux >= _fluxLimit) {
        ret = 1;
    }

    if(use > TRAFFIC_CHECK_TIME) {
        _flux = 0;
        _beginTime = now;
    }
    return ret;
}

int Worker::statFlux(int size)
{
    _flux += size;
    return 0;
}

/////////////////////////////////////////////////////
WorkerMap::WorkerMap()
{
    _colNum = 0;
    memset(_map, 0, sizeof(WorkerMap*) * MAX_COL_NUM);
}

WorkerMap::~WorkerMap()
{
    
}

/* 检测该序号的detail是否处理完成 */
bool WorkerMap::checkPass(StorableMessage* msg)
{
    _lock.rdlock();
    if(msg->action == ACTION_DELETE) {
        _lock.unlock();
        return true;
    }
    if(_colNum == 0 || _map[msg->distribute % _colNum] == NULL) {
        _lock.unlock();
        TLOG("have no detail, %d", _colNum);
        return false;
    }
    
    int col = msg->distribute % _colNum;
    Worker* head = _map[col];
    uint64_t nMinSeq = (uint64_t)-1;
    do {
        if(head->fseq() < nMinSeq) {
            nMinSeq = head->fseq();
        }
        head = head->_nextRow;
    } while(head);
    
    if(msg->seq <= nMinSeq) {
        _lock.unlock();
        return true;
    }
    _lock.unlock();
    return false;
}

/* 增加、删除一个worker */
bool WorkerMap::addWorker(Worker* worker)
{
    _lock.wrlock();
    int colNum = worker->colNum();
    int colNo = worker->colNo();
    if(colNum <= 0 || colNum >= MAX_COL_NUM || colNo < 0 || colNo >= colNum) {
        _lock.unlock();
        TERR("worker info error");
        return false;
    }
    if(_colNum == 0) {
        _colNum = colNum;
    }
    worker->_nextRow = _map[colNo]; 
    _map[colNo] = worker;
    _lock.unlock();
    return true;
}

bool WorkerMap::delWorker(Worker* worker)
{
    _lock.wrlock();
    uint32_t colNum = worker->colNum();
    uint32_t colNo = worker->colNo();
    if(colNum != _colNum || _map[colNo] == NULL) {
        _lock.unlock();
        TERR("worker info error %d/%d/%x , del failed", colNum, _colNum, _map[colNo]);
        return false;
    }

    Worker* head = _map[colNo];
    if(head == worker) {
        _map[colNo] = head->_nextRow;
        _lock.unlock();
        return true;
    }
    while(head->_nextRow != worker) {
        head = head->_nextRow;
        if(head == NULL) {
            _lock.unlock();
            TERR("not find this worker");
            return false;
        }
    }
    head->_nextRow = worker->_nextRow;
    _lock.unlock();
    return true;
}

/////////////////////////////////////////////////////
WorkerFactory::WorkerFactory()
{
    _workerNum = 0;
    memset(_workerStat, 0, sizeof(int) * MAX_WORKER_NUM);
}

WorkerFactory::~WorkerFactory()
{
    _writeQue.close();
}

int WorkerFactory::init(Config* cfg)
{
    _cfg = cfg;
    SignDict* pdict = SignDict::getInstance();
    
    if(NULL == pdict || pdict->init(_cfg->_dotey_sign_path) < 0) {
        TERR("pdict init failed, path=%s", _cfg->_dotey_sign_path);
        return -1;
    }
    if(_writeQue.open(_cfg->_enter_data_path) < 0) {
        TERR("que open %s failed", _cfg->_enter_data_path);
        return -1;
    }
    
    if(_searchCache.init(512<<20) < 0 || _detailCache.init(512<<20) < 0) {
        TERR("worker cache init failed");
        return -1;
    }

    return 0;
}

Worker* WorkerFactory::make(ClusterMessage* info, int fd)
{
    char key[256];
    sprintf(key, "%s:%d", info->ip, info->port);

    _lock.lock();
    std::map<std::string, Worker*>::iterator it = _workerTable.find(key);
    if(it != _workerTable.end()) {
        it->second->setValid(false);
    }
    _lock.unlock();

    Worker* worker = NULL;
    switch (info->role) {
        case SEARCHER:
            worker = new(std::nothrow) SearcherWorker(_searchCache, _workerMap);
            break;
        case DETAIL:
            worker = new(std::nothrow) DetailWorker(_detailCache);
            break;
        case UPDATE_SERVER:
            worker = new(std::nothrow) EnterWorker(_writeQue);
            break;
        default:
            break;
    };
    
    if(NULL == worker) {
        return NULL;
    }

    if(worker->init(_cfg, info) < 0) {
        delete worker;
        return NULL;
    }

    worker->setSocket(fd);

    _lock.lock();
    it = _workerTable.find(key);
    if(it != _workerTable.end()) {
        it->second = worker;
    } else {
        _workerTable.insert(std::make_pair<std::string,Worker*>(key, worker));
    }
    _workerNum++;
    _lock.unlock();
    
    if(info->role == DETAIL) {
        _workerMap.addWorker(worker);
    }

    return worker;
}

void WorkerFactory::free(Worker* worker)
{
    if(NULL == worker) return;

    char key[256];
    sprintf(key, "%s:%d", worker->ip(), worker->port());

    _lock.lock();
    _workerTable.erase(key);
    _workerNum--;
    _lock.unlock();
    
    if(worker->Role() == DETAIL) {
        _workerMap.delWorker(worker);
    }

    delete worker;
}

}

