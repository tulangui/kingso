#include <string.h>

#include "util/Log.h"
#include "util/iniparser.h"
#include "common/compress.h"
#include "libdetail/IncManager.h"
#include "update/update_def.h"
#include "update/DetailUpdater.h"
#include "UpdateProcessor.h"

namespace update {

struct DetailRes : public UpdateRes {
    DocMessage doc_msg;
};

DetailUpdater::DetailUpdater()
{
    _rol = DETAIL;
}
DetailUpdater::~DetailUpdater()
{
}

int DetailUpdater::init(const char* pConf)
{
    detail::IncManager* inc_manager = detail::IncManager::getInstance();
    _incMsgCount = inc_manager->getMsgCount();
    return Updater::init(pConf);
}

UpdateRes* DetailUpdater::allocRes()
{
    DetailRes* ret = new(std::nothrow) DetailRes;
    return ret;
}

void DetailUpdater::freeRes(UpdateRes* res)
{
    if(NULL == res) return;
    delete res;
}

int DetailUpdater::check(UpdateRes* res)
{
    DetailRes* detailRes = (DetailRes*)res;
    net_head_t& head = detailRes->head;

    if(head.cmpr == 'Z' || head.cmpr == 'z') {
        uLong size = uncompressSize((Bytef*)detailRes->orgbuf, head.size);
        if(size >= (uLong)DEFAULT_MESSAGE_SIZE) {
            head.cmd = UPCMD_ACK_EUNKOWN;
            return -1;
        }
        if(zdecompress((Bytef*)detailRes->tmpbuf, &size, (const Bytef*)detailRes->orgbuf, head.size)) {
            detailRes->head.cmd = UPCMD_ACK_EFORMAT;
            return -1;
        }
        detailRes->size = size;
        detailRes->message = detailRes->tmpbuf;
    } else {
        detailRes->size = detailRes->head.size;
        detailRes->message = detailRes->orgbuf;
    }
    
    detailRes->doc_msg.Clear();
    if(detailRes->doc_msg.ParseFromArray(detailRes->message, detailRes->size)) {
        head.cmd = UPCMD_ACK_SUCCESS;
        return 0;
    }

    head.cmd = UPCMD_ACK_EPROCESS;
    return -1;
}

int DetailUpdater::doupdate(UpdateRes* res)
{
    bool ret = false;
    detail::IncManager* inc_manager = detail::IncManager::getInstance();
    DocMessage& msg = ((DetailRes*)res)->doc_msg;

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
            ret = inc_manager->add(&msg);
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

int64_t DetailUpdater::nid(UpdateRes* res)
{
    DetailRes* di = (DetailRes*)res;
    return di->doc_msg.nid();
}
UpdateAction DetailUpdater::action(UpdateRes* res)
{
    DetailRes* di = (DetailRes*)res;
    return (UpdateAction)di->doc_msg.action();
}

}

