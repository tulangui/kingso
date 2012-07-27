#include <string.h>

#include "util/Log.h"
#include "util/iniparser.h"
#include "common/compress.h"
#include "update/update_def.h"
#include "update/IndexUpdater.h"
#include "index_lib/IncManager.h"
#include "UpdateProcessor.h"

namespace update {

struct IndexRes : public UpdateRes {
    DocIndexMessage idx_msg;
};

IndexUpdater::IndexUpdater()
{
    _rol = SEARCHER;
}

IndexUpdater::~IndexUpdater()
{
}

int IndexUpdater::init(const char* pConf)
{
    index_lib::IncManager* inc_manager = index_lib::IncManager::getInstance();
    _incMsgCount = inc_manager->getMsgCount();

    int ret = Updater::init(pConf);
    if(ret < 0) {
        TERR("init failed");
        return ret;
    }
    if(_processor->recovery(_incMsgCount) < 0) {
        TERR("recover failed");
        return -1;
    }
    return ret;
}

UpdateRes* IndexUpdater::allocRes()
{
    return new(std::nothrow) IndexRes;
}

void IndexUpdater::freeRes(UpdateRes* res)
{
    if(NULL == res) return;
    delete res;
}

int IndexUpdater::check(UpdateRes* res)
{
    IndexRes* indexRes = (IndexRes*)res;
    net_head_t& head = indexRes->head;

    if(head.cmpr == 'Z' || head.cmpr == 'z') {
        uLong size = uncompressSize((Bytef*)indexRes->orgbuf, head.size);
        if(size >= (uLong)DEFAULT_MESSAGE_SIZE) {
            head.cmd = UPCMD_ACK_EUNKOWN;
            return -1;
        }
        if(zdecompress((Bytef*)indexRes->tmpbuf, &size, (const Bytef*)indexRes->orgbuf, head.size)) {
            head.cmd = UPCMD_ACK_EFORMAT;
            return -1;
        }
        indexRes->size = size;
        indexRes->message = indexRes->tmpbuf;
    } else {
        indexRes->size = indexRes->head.size;
        indexRes->message = indexRes->orgbuf;
    }
    
    indexRes->idx_msg.Clear();
    if(indexRes->idx_msg.ParseFromArray(indexRes->message, indexRes->size)) {
        head.cmd = UPCMD_ACK_SUCCESS;
        return 0;
    }

    head.cmd = UPCMD_ACK_EPROCESS;
    return -1;
}

int IndexUpdater::doupdate(UpdateRes* res)
{
    bool ret = false;
    index_lib::IncManager* inc_manager = index_lib::IncManager::getInstance();
    DocIndexMessage& msg = ((IndexRes*)res)->idx_msg;
    
    _lock.lock();
    switch(msg.action()){
        case ACTION_ADD :
            ret = inc_manager->add(&msg);
            break;
        case ACTION_DELETE : 
            ret = inc_manager->del(&msg);
            break;
        case ACTION_MODIFY :  
            ret = inc_manager->update(&msg);
            break;
        case ACTION_UNDELETE:
            ret = inc_manager->undel(&msg);
            break;
        default :  
            ret = false;
    }
    _lock.unlock();

    if(ret) {
        return 0;
    }

    return -1;
}

int64_t IndexUpdater::nid(UpdateRes* res)
{
    IndexRes* ir = (IndexRes*)res;
    return ir->idx_msg.nid();
}
UpdateAction IndexUpdater::action(UpdateRes* res)
{
    IndexRes* ir = (IndexRes*)res;
    return (UpdateAction)ir->idx_msg.action();
}

}
