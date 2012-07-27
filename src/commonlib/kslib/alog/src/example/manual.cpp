#include "../include/Logger.h"
#include "../include/Appender.h"
#include "../include/Configurator.h"

using namespace alog;

int main(int argc, char *argv[])
{
    //Init loggers
    Logger* root = Logger::getRootLogger();
    Logger* sub1 = Logger::getLogger("sub1");
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");
    Logger* sub1_sub2_sub3 = Logger::getLogger("sub1.sub2.sub3");
    //Init appenders
    Appender* rootAppender = ConsoleAppender::getAppender();
    Appender* A1 = ConsoleAppender::getAppender();
    Appender* A2 = FileAppender::getAppender("/tmp/alog.template.log1");
    Appender* A3 = FileAppender::getAppender("/tmp/alog.template.log2");
    //add appenders to loggers
    root->setAppender(rootAppender);
    sub1->setAppender(A1);
    sub1_sub2->setAppender(A1);
    sub1_sub2->addAppender(A2);
    sub1_sub2_sub3->setAppender(A1);
    sub1_sub2_sub3->addAppender(A3);
    //set logger's inherit
    sub1_sub2_sub3->setInheritFlag(false);

    root->log(LOG_LEVEL_INFO,"This is a manual test of root.");
    sub1->log(LOG_LEVEL_INFO,"This is a manual test of sub1.");
    sub1_sub2->log(LOG_LEVEL_ERROR,"This is a manual test of sub1.sub2.");
    sub1_sub2_sub3->log(LOG_LEVEL_FATAL,"This is a manual test of sub1.sub2.sub3.");

    Logger::shutdown();

}
