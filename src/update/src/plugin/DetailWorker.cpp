#include <arpa/inet.h>
#include "DetailWorker.h"
#include "common/compress.h"
#include "update/update_def.h"

namespace update {

DetailWorker::DetailWorker(QueueCache& cache) : _cache(cache)
{
    _level = 0;
}

DetailWorker::~DetailWorker()
{

}

int DetailWorker::init(Config* cfg, ClusterMessage* info)
{
    int ret = Worker::init(cfg, info);
    if(ret < 0) {
        return ret;
    }

    if(_que.open(cfg->_enter_data_path)) {
        TERR("open queue %s error", cfg->_enter_data_path);
        return -1;
    }
    _que.reset(_info.seq);

    return 0;
}

int DetailWorker::process(net_head_t* head, char* message)
{
    int ret = Worker::process(head, message);
    if(ret <= 0) {
        return ret;
    }
    
    ret = _cache.get(_msg.seq, this, message);
    if(ret > 0) { // 命中
        *head = *(net_head_t*)message;
        if(head->size == 0) { // 处理失败的消息
            TLOG("detail cache failed");
            return RET_FAILE;
        }
        head->size += sizeof(net_head_t);
        return RET_SUCCESS;
    } else if( ret == 0) { // 其它线程正在处理
       return  RET_DOING;
    }

    _message.Clear();
    _message.set_nid(_docMessage.nid());
    _message.set_action(_msg.action);
    _message.set_seq(_msg.seq);

    do {
        if (ACTION_DELETE == _docMessage.action() || ACTION_LAST == _docMessage.action()) {
            ret = serial(head, message, 'N');
            break;
        }

        for (int i = 0; i < _docMessage.fields_size(); i++) {
            const std::string &strName = _docMessage.fields(i).field_name();
            const std::string &strValue = _docMessage.fields(i).field_value();
            ksb_auction_insert_field(_pKsbAuction, strName.c_str(), strValue.c_str());
        }
        if (unlikely(ksb_parse(_pKsbAuction, _pKsbConf, _pKsbResult, KSB_FLAG_INPUT_UTF8) != 0)) {
            ksb_reset_auction(_pKsbAuction);
            ksb_reset_result(_pKsbResult);
            head->size = 0;
            *(net_head_t*)message = *head;
            ret = (int)sizeof(net_head_t);
            break;
        }
        for (uint32_t i = 0; i < _pKsbConf->detail_count; i++) {
            DocMessage_Field *pDetail = _message.add_fields();
            pDetail->set_field_name(_pKsbConf->detail[i].name);
            pDetail->set_field_value(_pKsbResult->detail_fields[i].value);
        }
        ksb_reset_auction(_pKsbAuction);
        ksb_reset_result(_pKsbResult);

        ret = serial(head, message, 'Z');
    } while(0);
    
    _cache.put(_msg.seq, message, ret);
    if(ret <= (int32_t)sizeof(net_head_t)) {
        TLOG("detail put cache failed");
        return RET_FAILE;
    }
    return RET_SUCCESS;
}

int DetailWorker::serial(net_head_t* head, char* message, char zip)
{
    head->size = 0;
    uint32_t nBufSize = _message.ByteSize();

    do {
        if(zip == 'Z') {
            char szBuf[nBufSize + 1];
            int ret = _message.SerializeToArray(szBuf, nBufSize);
            if(ret == 0) {
                break;
            }
            uLong nComprSize = compressBound(nBufSize) + 1;
            if(nComprSize >= DEFAULT_MESSAGE_SIZE - sizeof(net_head_t)) {
                break;
            }
            ret = zcompress((Bytef *)(message+sizeof(net_head_t)), &nComprSize, (Bytef *)szBuf, nBufSize);
            if(ret != 0) {
                break;
            }
            head->cmpr = 'Z';
            head->size = nComprSize;
        } else {
            if(nBufSize >= DEFAULT_MESSAGE_SIZE - sizeof(net_head_t)) {
                break;
            }
            int ret = _message.SerializeToArray(message+sizeof(net_head_t), nBufSize);
            if(ret == 0) {
                break;
            }
            head->cmpr = 'N';
            head->size = nBufSize;
        }
    } while(0);

    *(net_head_t*)message = *head;
    head->size += sizeof(net_head_t);
    return head->size;
}

}

