#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "UpdateProcessor.h"

#include "update/Updater.h"
#include "update/update_def.h"
#include "util/Log.h"
#include "util/iniparser.h"

namespace update {

Updater::Updater() 
{
    _rol = ROLE_LAST;
    _incMsgCount = 0;
    _processor = NULL;
}

Updater::~Updater()
{
    if(_processor) delete _processor;
    _processor = NULL;
}

void* Updater::start(void* arg)
{
    Updater* updater = (Updater*)arg;
    if(NULL == updater || NULL == updater->_processor) {
        return NULL;
    }
    return updater->_processor->start(arg);
}

void Updater::stop()
{
    if(_processor) {
        _processor->stop();
    }
}

int Updater::init(const char* conf_path)
{
    if(!conf_path){
        TERR("init function argument error, return!");
        return -1;
    }
    _processor = new (std::nothrow) UpdateProcessor;
    if(NULL == _processor) {
        return -1;
    }
 
    if(_processor->init(conf_path, this, _rol) < 0) {
        TERR("init config %s failed", conf_path);
        return -1;
    }

    return 0;
}

}
