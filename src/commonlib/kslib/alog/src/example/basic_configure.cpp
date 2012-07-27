#include "../include/Logger.h"
#include "../include/Appender.h"
#include "../include/Configurator.h"

using namespace alog;

int main(int argc, char *argv[])
{
    Configurator::configureRootLogger();
    Logger* root = Logger::getRootLogger();
    root->log(LOG_LEVEL_INFO,"This is a basic configure test.");
    Logger::shutdown();
}
