#include "update/Updater.h"
#include <stdio.h>
#include "util/Log.h"

#define PRINT(fmt, arg...) fprintf(stderr, fmt"\n", ##arg)

static void init_log(const char* log_path)
{
    if(log_path){
        try {
            alog::Configurator::configureLogger(log_path);
        } catch(std::exception &e) {
            PRINT("open alog error, path : %s", log_path);
            alog::Configurator::configureRootLogger();
        }   
    }   
    else{
        alog::Configurator::configureRootLogger();
    }   
}

int main(int argc, char* argv[])
{
    update::Updater* updater = 0;

    if(argc<4){
        PRINT("no enough argument, exit!");
        return -1;
    }

    init_log(argv[3]);

    updater = update::Updater::getInstance(argv[1]);
    if(!updater){
        PRINT("get %s updater instance erorr, exit!", argv[1]);
        return -1;
    }
    PRINT("get %s updater instance done.", argv[1]);
    if(updater->init(argv[2])){
        PRINT("init %s updater instance erorr, exit!", argv[1]);
        return -1;
    }
    PRINT("init %s updater instance done.", argv[1]);
    update::Updater::start(updater);
    PRINT("exiting...");

    return 0;
}
