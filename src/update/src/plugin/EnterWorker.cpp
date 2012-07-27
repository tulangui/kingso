#include "util/Log.h"
#include "SignDict.h"
#include "EnterWorker.h"
#include "common/compress.h"

namespace update {

EnterWorker::EnterWorker(FileQueue& que) : _enterQueue(que)
{
    _dispFieldAdd = -1;
    _dispFieldDel = -1;
    _reader = NULL;
    _fdSeq = 0;
}

EnterWorker::~EnterWorker()
{
    if(_fdSeq > 0) close(_fdSeq);
    if(_reader) delete _reader;
    _fdSeq = 0;
    _reader = NULL;
}
    
int EnterWorker::init(Config* cfg, ClusterMessage* info)
{
    int ret = Worker::init(cfg, info);
    if(ret < 0) {
        return ret;
    }
    _cfg = cfg;
    _reader = new(std::nothrow) XmlReader(_pKsbConf);
    if(NULL == _reader || _reader->init(NULL, false) < 0) {
        TERR("init xmlreader failed");
        return -1;
    }

    char fileName[PATH_MAX];
    snprintf(fileName, PATH_MAX, "%s/seq.txt", _cfg->_enter_data_path);
    _fdSeq = open(fileName, O_RDWR | O_CREAT,  S_IRWXU | S_IRWXG | S_IRWXO);
    if(_fdSeq <= 0) {
        TERR("open sequence fail! path=%s", fileName);
        return -1;
    }

    return 0;
}

int EnterWorker::process(net_head_t* head, char* message)
{
    _msgInit = false;
    _docMessage.Clear();

    if(head->cmpr == 'Z' || head->cmpr== 'z') { // uncompress
        uLong size = uncompressSize((Bytef*)message, head->size);
        char unCompMem[size+1];
        if(zdecompress((Bytef*)unCompMem, &size, (Bytef*)message, head->size)) {
            TWARN("zdecompress error, return!");
            return UPCMD_ACK_EFORMAT;
        }
        if((_docMessage.ParseFromArray(unCompMem, size)) == false) {
            TWARN("ParseFromArray error, size=%lu, return!", size);
            return -1;
        }
        if(getDispatcherId(&_docMessage, _msg.distribute) < 0) {
            TWARN("get dispatcher field failed, nid=%lu", _docMessage.nid());
            return -1;
        }
    } else {
        if((_docMessage.ParseFromArray(message, head->size)) == false) {
            TWARN("ParseFromArray error, size=%d, return!", head->size);
            return -1;
        }
        if(getDispatcherId(&_docMessage, _msg.distribute) < 0) {
            TWARN("get dispatcher field failed, nid=%lu", _docMessage.nid());
            return -1;
        }
    }
    
    _msg.nid = _docMessage.nid();
    _msg.action = _docMessage.action();
    _msg.ptr = message;
    _msg.max = head->size;
    _msg.size = head->size;
    _msg.cmpr = head->cmpr;

    // add or modify message 检测指纹重复
    if(_docMessage.action() == ACTION_ADD || _docMessage.action() == ACTION_MODIFY) {
        unsigned char finger[16];  
        if(_reader->sign(&_docMessage, finger) < 0) {
            TWARN("calsign failed, nid=%lu", _docMessage.nid());
            return -1;
        }
        SignDict* pdict = SignDict::getInstance();
        if(pdict->check(_docMessage.nid(), finger) == 0) { // 无变化 
            _msg.action = ACTION_UNDELETE;
        }
    }

    _msgInit = true;
    if(_enterQueue.push(_msg) <= 0) {
        TWARN("put in filequeue error, fuck!");
        return -1;
    }

    char szSeqBuf[128];
    int nSeqBufSize = sprintf(szSeqBuf, "%lu", _msg.seq);
    lseek(_fdSeq, 0, SEEK_SET);
    write(_fdSeq, szSeqBuf, nSeqBufSize);

    return 0;
}

int EnterWorker::getDispatcherId(DocMessage* msg, uint64_t& id)
{
    int* no = &_dispFieldAdd;
    if(msg->action() != ACTION_ADD) no = &_dispFieldDel;

    if(*no >= 0 && *no < msg->fields_size()) {
        const ::DocMessage_Field& field = msg->fields(*no);
        if(strcmp(field.field_name().c_str(), _cfg->_dispatcher_field) == 0) {
            const std::string& value = field.field_value();
            if(value.empty()) {
                TWARN("DispatchWoker: %lu dispatcher field error", msg->nid());
                return -1;
            }
            char* end = NULL;
            const char* need = value.c_str() + value.length();
            id = strtoull(value.c_str(), &end, 10);
            while(end && *end && isspace(*end)) end++;
            if(end != need) {
                TWARN("DispatchWoker: %lu dispatcher field error, len=%lu, v=%s", msg->nid(), value.length(), value.c_str());
                return -1;
            }
            return 0;
        }
    }

    //
    for(*no = 0; *no < msg->fields_size(); (*no)++) {
        const ::DocMessage_Field& field = msg->fields(*no);
        if(strcmp(field.field_name().c_str(), _cfg->_dispatcher_field) == 0) {
            const std::string& value = field.field_value();
            if(value.empty()) {
                TWARN("DispatchWoker: %lu dispatcher field error", msg->nid());
                return -1;
            }
            char* end = NULL;
            const char* need = value.c_str() + value.length();
            id = strtoull(value.c_str(), &end, 10);
            while(end && *end && isspace(*end)) end++;
            if(end != need) {
                TWARN("DispatchWoker: %lu dispatcher field error, len=%lu, v=%s", msg->nid(), value.length(), value.c_str());
                return -1;
            }
            return 0;
        }
    }

    id = 0;
    return -1;
}

}

