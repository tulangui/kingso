#include <stdio.h>
#include "../include/Logger.h"
#include "../include/Appender.h"
#include "../include/Configurator.h"

using namespace alog;

int main(int argc, char *argv[])
{
	try
	{
        Configurator::configureLogger("alog.template.conf");
	}
	catch (std::exception &e)
	{
	    printf("%s\n",e.what());
	    Logger::shutdown();
	    return 0;
	}
    Logger* root = Logger::getRootLogger();
    root->log(LOG_LEVEL_INFO, __FILE__, __LINE__, __FUNCTION__, "This is a configure test of root.");

    Logger* sub1 = Logger::getLogger("sub1");
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");
    Logger* sub1_sub2_sub3 = Logger::getLogger("sub1.sub2.sub3");
    sub1->log(LOG_LEVEL_INFO,__FILE__, __LINE__, __FUNCTION__, "This is a configure test of sub1.");
    sub1_sub2->log(LOG_LEVEL_ERROR,__FILE__, __LINE__, __FUNCTION__, "This is a configure test of sub1.sub2.");
    sub1_sub2_sub3->log(LOG_LEVEL_FATAL,__FILE__, __LINE__, __FUNCTION__, "This is a configure test of sub1.sub2.sub3.");
   
    Logger::shutdown();
	return 1;
}
