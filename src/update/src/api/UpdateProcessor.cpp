#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "netfunc.h"
#include "UpdateProcessor.h"
#include "update/DetailUpdater.h"
#include "update/IndexUpdater.h"
#include "libdetail/IncManager.h"
#include "index_lib/IncManager.h"
#include "update/update_def.h"
#include "common/compress.h"
#include "util/Log.h"
#include "util/Timer.h"
#include "util/iniparser.h"

namespace update {

#define IDX_FILE_EXNAME "idx"
#define DAT_FILE_EXNAME "dat"
#define MAX_DATA_SIZE (1<<30)

UpdateProcessor::UpdateProcessor() 
{
    _bStartSimon = false;
    _incMsgCount = 0;
    _running = 0;
    _threadNum = 0;
    _updater = NULL;
    _fdSeq = 0;
    memset(_thread_list, 0, sizeof(ThreadInfo) * MAX_UPDATE_SERVER_NUM); 
}

UpdateProcessor::~UpdateProcessor()
{
    for(int i = 0; i < MAX_UPDATE_SERVER_NUM; i++) {
        if(_thread_list[i].fdIdx) close(_thread_list[i].fdIdx);
        if(_thread_list[i].fdDat) close(_thread_list[i].fdDat);
        _thread_list[i].fdIdx = 0;
        _thread_list[i].fdDat = 0;
    }

    if(_fdSeq > 0) close(_fdSeq);
    _fdSeq = 0;
}

void* UpdateProcessor::start(void* arg)
{
    if(NULL == _updater) {
        return NULL;
    }
     
    Config& cfg = _cfg;
    ThreadInfo* list = _thread_list;

    _running = 1;
    for(int i = 0; i < cfg._updater_server_num; i++) {
        list[i].isActive = 0;
    }

    for(int i = 0; i < cfg._updater_server_num; i++) {
        if(pthread_create(&list[i].id, NULL, update_work, this)) {
            _running = 0;
            TWARN("create thread error, return!");
            return NULL;
        }
        list[i].isActive = 1;
    }

    return NULL;
}

void* UpdateProcessor::stop()
{
    _running = 0;
    for(int i = 0; i < _cfg._updater_server_num; i++) {
        if(_thread_list[i].isActive == 0) continue;
        pthread_join(_thread_list[i].id, NULL);
        
        close(_thread_list[i].fdIdx);
        _thread_list[i].fdIdx = 0;
        
        close(_thread_list[i].fdDat);
        _thread_list[i].fdDat = 0;
    }
    return NULL;
}

int UpdateProcessor::init(const char* conf_path, Updater* up, ROLE role)
{
    if(!conf_path){
        TERR("init function argument error, return!");
        return -1;
    }
    
    if(_cfg.init(conf_path, role) < 0) {
        TERR("init config %s failed", conf_path);
        return -1;
    }
    _updater = up;
    
    int flags = 0, fd = -1;
    char fileName[PATH_MAX];

    snprintf(fileName, PATH_MAX, "%s/seq.txt", _cfg._data_path);
    _fdSeq = open(fileName, O_RDWR | O_CREAT,  S_IRWXU | S_IRWXG | S_IRWXO);
    if(_fdSeq <= 0) {
        TERR("open sequence fail! path=%s", fileName);
        return -1;
    }

    for(int i = 0; i < _cfg._updater_server_num; i++) {
        _thread_list[i].msg.colNum = _cfg._colNum;
        _thread_list[i].msg.colNo = _cfg._colNo;
        _thread_list[i].msg.role = role;
        _thread_list[i].msg.port = _cfg._selfPort;
        strcpy(_thread_list[i].msg.ip, _cfg._selfIp);

        _thread_list[i].port = _cfg._updater_server_port[i];
        strcpy(_thread_list[i].ip, _cfg._updater_server_ip[i]);

        flags = O_RDWR;
        idxFileName(fileName, i);
        if(access(fileName, F_OK)) {// 不存在或者不可读
            flags |= O_CREAT;
        }
        fd = open(fileName, flags, S_IRWXU|S_IRWXG|S_IRWXO);
        if(fd <= 0) {
            TERR("open file %s failed", fileName);
            return -1;
        }
        _thread_list[i].fdIdx = fd;
   
        struct stat st;
        if(fstat(fd, &st)) {
            TERR("fstat %s failed", fileName);
            break;
        }
        if(st.st_size % sizeof(MessageIdx)) { // 字节数不完整
            int size = (st.st_size / sizeof(MessageIdx)) * sizeof(MessageIdx);
            if(ftruncate(fd, size)) {
                TERR("ftruncate file %s failed. error=%s", fileName, strerror(errno));
                return -1;
            }
            st.st_size = size;
        }
        if(st.st_size > 0) {
            MessageIdx idx;
            lseek(fd, st.st_size - sizeof(MessageIdx), SEEK_SET);
            if(read(fd, &idx, sizeof(MessageIdx)) != sizeof(MessageIdx)) {
                TERR("read file %s error", fileName);
                return -1;
            }
            _thread_list[i].datOffset = idx.offset + idx.size;
            _thread_list[i].datFileNo = idx.fileno;
            if(_incMsgCount <= idx.lseq) _incMsgCount = idx.lseq + 1;
            _thread_list[i].msg.seq = idx.seq + 1;
        } else {
            _thread_list[i].datOffset = 0;
            _thread_list[i].datFileNo = 0;
            _thread_list[i].msg.seq = 0;
        }
        
        flags = O_RDWR;
        datFileName(fileName, i, _thread_list[i].datFileNo);
        if(access(fileName, F_OK)) {// 不存在或者不可读
            flags |= O_CREAT;
        }
        fd = open(fileName, flags, S_IRWXU|S_IRWXG|S_IRWXO);
        if(fd <= 0) {
            TERR("open file %s failed", fileName);
            return -1;
        }
        if(_thread_list[i].datOffset && ftruncate(fd, _thread_list[i].datOffset)) {
            TERR("ftruncate file %s failed. error=%s", fileName, strerror(errno));
            return -1;
        }
        _thread_list[i].fdDat = fd;
        lseek(fd, _thread_list[i].datOffset, SEEK_SET);
    }
    _running = 0;

    return 0;
}

void UpdateProcessor::idxFileName(char* name, int updaterSeq)
{
    snprintf(name, PATH_MAX, "%s/%s_%d.%s", _cfg._data_path,
            _cfg._updater_server_ip[updaterSeq], _cfg._updater_server_port[updaterSeq], IDX_FILE_EXNAME); 
}
void UpdateProcessor::datFileName(char* name, int updaterSeq, int fileNo)
{
    snprintf(name, PATH_MAX, "%s/%s_%d_%.3d.%s", _cfg._data_path,
            _cfg._updater_server_ip[updaterSeq], _cfg._updater_server_port[updaterSeq], fileNo, DAT_FILE_EXNAME); 
}

int UpdateProcessor::bakMessage(ThreadInfo* info, UpdateRes* res, int threadNo)
{
    MessageIdx idx;
    idx.nid = _updater->nid(res);
    idx.offset = info->datOffset;
    idx.size = 0;
    idx.action = _updater->action(res);
    idx.fileno = info->datFileNo;
    idx.cmpr   = res->head.cmpr;
    idx.seq    = res->head.seq; 

    char szSeqBuf[128];

    _lock.lock();
    idx.lseq   = _incMsgCount++;
    int nSeqBufSize = sprintf(szSeqBuf, "%lu", idx.lseq);
    lseek(_fdSeq, 0, SEEK_SET);
    write(_fdSeq, szSeqBuf, nSeqBufSize);
    _lock.unlock();

    if(write(info->fdDat, res->orgbuf, res->head.size) != res->head.size) {
        TERR("write dat fail, id=%lu, error=%s(%d)", idx.nid, strerror(errno), errno);
        return -1;
    }
    idx.size = res->head.size;
    info->datOffset += res->head.size;
    if(info->datOffset > MAX_DATA_SIZE) {
        close(info->fdDat);
        info->datFileNo++;
        
        char datName[PATH_MAX];
        datFileName(datName, threadNo, info->datFileNo);
        info->fdDat = open(datName, O_RDWR|O_CREAT, S_IRWXU|S_IRWXG|S_IRWXO);
        if(info->fdDat <= 0) {
            TERR("write dat fail, id=%lu, error=%s(%d)", idx.nid, strerror(errno), errno);
            return -1;
        }
        info->datOffset = 0;
    }
    if(write(info->fdIdx, &idx, sizeof(MessageIdx)) != sizeof(MessageIdx)) {
        TERR("write idx fail, id=%lu, error=%s(%d)", idx.nid, strerror(errno), errno);
        return -1;
    }
    return 0; 
}

const uint32_t _unValidIdx = 0xffffffffl;
int UpdateProcessor::unionMessage(UnionInfo* messageList, int msgCount)
{
    UnionInfo value;
    int orgundel = 0, orgdel = 0, orgadd = 0;
    HashMap<int64_t, UnionInfo> st(msgCount+1);
    
    for(int i = 0; i < msgCount; i++) {
        util::HashMap<int64_t, UnionInfo>::iterator it = st.find(messageList[i].idx.nid);
        switch (messageList[i].idx.action) {
            case ACTION_DELETE:
                orgdel++;
                if(it == st.end()) {// 新发现的删除消息，是有效消息(删全量)
                    st.insert(messageList[i].idx.nid, messageList[i]);
                } else { // 更新消息中已经存在该nid
                    uint32_t bakAdd = it->value.bak;
                    it->value = messageList[i];
                    it->value.bak = bakAdd;
                }
                break;
            case ACTION_UNDELETE:
                orgundel++;
                if(it == st.end()) {// 新发现的恢复消息
                    st.insert(messageList[i].idx.nid, messageList[i]);
                } else { // 更新消息中已经存在该nid
                    // 上一个消息不是add消息, 并且有备份的add消息
                    if(it->value.bak != _unValidIdx) {
                        uint32_t bakAdd = it->value.bak;
                        it->value = messageList[it->value.bak];
                        it->value.bak = bakAdd;
                    } else {
                        it->value = messageList[i];
                    }
                }
                break;
            default:
                orgadd++;
                if(it == st.end()) {
                    value = messageList[i];
                    value.bak = i;
                    st.insert(messageList[i].idx.nid, value);
                } else {
                    it->value = messageList[i];
                    it->value.bak = i;
                }
        }
    }
    
    int64_t key;
    int32_t add = 0, del = 0, undel = 0;
    st.itReset();

    while(st.itNext(key, value)) {
        if(value.idx.action == ACTION_DELETE)  {
            del++;
        } else if(value.idx.action == ACTION_UNDELETE) {
            undel++;
            TWARN("can't find this");
        } else {
            add++;
        }
        messageList[value.idx.lseq].bak = 0;
    }   
    TLOG("all = %d, orgdel=%d, orgundel=%d, orgadd=%d", msgCount, orgdel, orgundel, orgadd);
    TLOG("all = %d, valid=%d, del=%d, undel=%d, add=%d", msgCount, st.size(), del, undel, add);

    return msgCount;
}

void* UpdateProcessor::update_work(void* para)
{
    UpdateProcessor* processor = (UpdateProcessor*)para;
    if(NULL == processor) {
        return NULL;
    }

    processor->_lock.lock();
    int threadNo = processor->_threadNum++;
    processor->_lock.unlock();

    Updater* updater = processor->_updater;
    ThreadInfo* threadInfo = processor->_thread_list + threadNo;

    int fd = 0;
    int keepalive = 1;
    struct sockaddr_in addr;

    UpdateRes* res = (UpdateRes*)updater->allocRes();
    if(NULL == res) {
        TERR("alloc updater res failed");
        return NULL;
    }
    net_head_t& head = res->head;
    char* recvBuf = res->orgbuf;

    while(processor->_running) {
        fd = socket(AF_INET, SOCK_STREAM, 0); 
        if(fd < 0) {
            TWARN("processor create socket error, %s, continue!", strerror(errno));
            sleep(1);
            continue;
        }  

        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));

        bzero(&addr, sizeof(struct sockaddr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(threadInfo->port);
        inet_aton(threadInfo->ip, &(addr.sin_addr));

        TLOG("processor wait connect to %s:%d ...", threadInfo->ip, threadInfo->port);
        if(connect_wait(fd, (struct sockaddr*)(&addr), sizeof(addr), &processor->_running)) {
            close(fd);
            TWARN("processor wait connect to %s:%d, continue!", threadInfo->ip, threadInfo->port);
            continue;
        }
        if(write_check(fd, &threadInfo->msg, sizeof(threadInfo->msg))!=sizeof(threadInfo->msg)) {
            TWARN("write first ack error, continue!");
            close(fd);
            continue;
        }
        TLOG("processor connect to %s:%d, seq=%lu begin!", threadInfo->ip, threadInfo->port, threadInfo->msg.seq);
        
        while(processor->_running) {
            int ret = read_wait(fd, &head, sizeof(net_head_t), &processor->_running);
            if(ret != sizeof(net_head_t)){
                TWARN("read client head error, need=%lu,true=%d, return!", sizeof(net_head_t), ret);
                break;
            }
            if(processor->_running==0){
                break;
            }
            if(head.size > (uint32_t)DEFAULT_MESSAGE_SIZE) {
                TWARN("message seq=%lu, cmd=%c, size overlimit(%d/%d), abandon", head.seq, head.cmd, head.size, DEFAULT_MESSAGE_SIZE);
                uint32_t size = 0;
                do {
                    uint32_t need = head.size - size > (uint32_t)DEFAULT_MESSAGE_SIZE ? DEFAULT_MESSAGE_SIZE : head.size - size; 
                    if(read_wait(fd, recvBuf, need, &processor->_running) != need) {
                        break;
                    }
                    size += need;
                } while(size < head.size);
                if(size < head.size) {
                    TWARN("read client data error, return!");
                    break;
                }
                // 返回异常错误
                head.cmd = UPCMD_ACK_EUNKOWN;
                if(write_check(fd, &head, sizeof(net_head_t)) != sizeof(net_head_t)) {
                    TWARN("write ack error, return!");
                    break;
                }
            } else {
                if(read_wait(fd, recvBuf, head.size, &processor->_running) != head.size) {
                    TWARN("read client data error, return!");
                    break;
                }
                ret = updater->check(res);
                if(ret == 0) {
                    processor->bakMessage(threadInfo, res, threadNo);
                    if(updater->doupdate(res) < 0) {
                        head.cmd = UPCMD_ACK_EPROCESS;
                    } else {
                        head.cmd = UPCMD_ACK_SUCCESS;
                    }
                }
                threadInfo->msg.seq = head.seq + 1;
                if(write_check(fd, &head, sizeof(net_head_t)) != sizeof(net_head_t)) {
                    TWARN("write ack error, return!");
                    break;
                }
            }
        }
        close(fd);
        TLOG("connection %s:%d done.", threadInfo->ip, threadInfo->port);
    }
    updater->freeRes(res);
    
    return NULL;
}


int UpdateProcessor::recovery(uint32_t msgCount)
{
    struct stat st;
    char idxName[PATH_MAX];
    char datName[PATH_MAX];

    bool success = true;
    uint32_t messageNum = 0, readNum = 0;
    int32_t serverNum = _cfg._updater_server_num;
    
    for(int i = 0; i < serverNum; i++) {
        success = false;
        idxFileName(idxName, i);
        if(stat(idxName, &st)) {
            TERR("fstat %s failed", idxName);
            break;
        }
        success = true;
        messageNum += st.st_size / sizeof(MessageIdx);
    }
    if(success == false) {
        return -1;
    }
    if(messageNum <= msgCount) {
        return 0;
    }

    UpdateRes* res = _updater->allocRes();
    UnionInfo* pInfo = new(std::nothrow) UnionInfo[messageNum];
    if(NULL == res || NULL == pInfo) {
        if(pInfo) delete[] pInfo;
        if(res) _updater->freeRes(res);
        TERR("recovery alloc res failed");
        return -1;
    }
    for(uint32_t i = 0; i < messageNum; i++) {
        pInfo[i].no = serverNum;
        pInfo[i].bak = _unValidIdx;
    }

    // 读取索引信息
    MessageIdx idx;
    for(int i = 0; i < serverNum; i++) {
        idxFileName(idxName, i);
        int fdIdx = open(idxName, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
        if(fdIdx <= 0) {
            TERR("open file %s failed", idxName);
            break;
        }
        if(fstat(fdIdx, &st)) {
            close(fdIdx);
            TERR("fstat %s failed", idxName);
            break;
        }

        int num = st.st_size / sizeof(MessageIdx);
        for(int j = 0; j < num; j++, readNum++) {
            if(read(fdIdx, &idx, sizeof(MessageIdx)) != sizeof(MessageIdx)) {
                TERR("read idx %d, all=%d failed", j, num);
                break;
            }
            pInfo[idx.lseq].no = i;
            pInfo[idx.lseq].idx = idx;
        }
        close(fdIdx);
    }
    if(readNum != messageNum) {
        delete[] pInfo;
        _updater->freeRes(res);
        TERR("readNum %d != messageNum %d", readNum, messageNum);
        return -1;
    }

    uint64_t nBeginTime = UTIL::Timer::getMicroTime();
    int num = unionMessage(pInfo + msgCount, messageNum - msgCount);
    if(num < 0) {
        delete[] pInfo;
        _updater->freeRes(res);
        TERR("unionMessage failed");
        return -1;
    }
    
    int nCnt = 0, len = 0, failNum = 0;
    UnionInfo* msg = pInfo + msgCount;
    int fdDat[MAX_UPDATE_SERVER_NUM];
    int fileNo[MAX_UPDATE_SERVER_NUM];

    success = true;
    memset(fdDat, 0, sizeof(int) * MAX_UPDATE_SERVER_NUM);
    memset(fileNo, 0xffff, sizeof(int) * MAX_UPDATE_SERVER_NUM);

    for(int i = 0; i < num; i++) {
        if(msg[i].bak) continue;    // 确认为重复消息，过滤掉
        success = false;
        int no = msg[i].no;
        if(fileNo[no] != msg[i].idx.fileno) {
            if(fdDat[no]) close(fdDat[no]);
            fileNo[no] = msg[i].idx.fileno;
            datFileName(datName, no, fileNo[no]);
            fdDat[no] = open(datName, O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
            if(fdDat[no] <= 0) {
                TERR("open file %s failed", datName);
                break;
            }
        }
        lseek(fdDat[no], msg[i].idx.offset, SEEK_SET);
        res->head.size = msg[i].idx.size;
        res->head.cmpr = msg[i].idx.cmpr;
        len = read(fdDat[no], res->orgbuf, msg[i].idx.size);
        if(len != (int)msg[i].idx.size) {
            TERR("read message error, need=%u, true=%d", msg[i].idx.size, len);
            break;
        }
        if(_updater->check(res) == 0) {
            if(_updater->doupdate(res) == 0) {
                if (++nCnt % 100000 == 0) {
                    TLOG("recover %d / %d message", nCnt, num);
                }
            } else {
                failNum++;
            }
        } else {
            failNum++;
        }
    }
    for(int i = 0; i < serverNum; i++) {
        if(fdDat[i]) close(fdDat[i]);
    }
    delete[] pInfo;
    _updater->freeRes(res);
    
    uint64_t nEndTime = UTIL::Timer::getMicroTime();
    TLOG("recover %d message, begin_seq=%u, end_seq=%u, cost=%lu", 
          nCnt, msgCount, messageNum, nEndTime - nBeginTime); 

    return 0;
}

}
