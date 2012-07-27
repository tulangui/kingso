#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>


//alog include files
#include "Logger.h"
#include "Appender.h"
#include "Configurator.h"

//log4cxx include files
#include <log4cxx/logstring.h>
#include <stdlib.h>
#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <log4cxx/layout.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/patternlayout.h>
#include <log4cxx/fileappender.h>

using namespace std;

using namespace log4cxx;
using namespace log4cxx::helpers;

uint64_t nowTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000LL + t.tv_usec);
}


int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <logCount>" <<  std::endl;
        exit(-1);
    }
    int logCount = atoi(argv[1]);
    std::cerr << " -------- logCount: " << logCount << "  --------" << std::endl;
    const char* testStr = "abcdefg hijklmn opqrst uvwxyz";
    const char* filename = "/tmp/pt.log";
    const char* filename2 = "/tmp/pt.log2";
    const char* filename3 = "/tmp/pt.log3";

    int count;
    uint64_t startTime;

    /** fwrite output.*/
    unlink(filename);
    FILE *logFile = fopen(filename, "a+");
    char tmp[20];
    startTime = nowTime();
    count = 0;
    while (count < logCount)
    {
        //alog::Appender::getCurTime(tmp, 20);//get the current time string
        fprintf(logFile, "%ld (%d/ %d) [INFO] %s\n", time(NULL), getpid(), (int)pthread_self(), testStr);
        count++;
    }
    cerr << "[fwrite time cost]: " << nowTime() - startTime << endl;
    fclose(logFile);

    /** alog output. */
    unlink(filename2);
    alog::Configurator::configureRootLogger();
    alog::Logger *logger = alog::Logger::getRootLogger();
    logger->setAppender(alog::FileAppender::getAppender(filename2));
    startTime = nowTime();
    count = 0;
    while (count < logCount)
    {
        logger->log(alog::LOG_LEVEL_INFO, testStr);
        count++;
    }
    cerr << "[alog time cost]: " << nowTime() - startTime << endl;
    alog::Logger::shutdown();

    /** log4cxx output.*/
    try
    {
        unlink(filename3);
        //log4cxx::LayoutPtr layout(new log4cxx::SimpleLayout());
        log4cxx::LayoutPtr layout(new log4cxx::PatternLayout("%p [%d] %C.%M(%L) | %m%n"));
        log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
        log4cxx::FileAppenderPtr appender(new log4cxx::FileAppender(layout, filename3, true));
        rootLogger->addAppender(appender);

        startTime = nowTime();
        count = 0;
        while (count < logCount)
        {
            LOG4CXX_INFO(rootLogger, testStr);
            count++;
        }

    }
    catch (std::exception&)
    {
        //result = EXIT_FAILURE;
    }

    cerr << "[log4cxx time cost]: " << nowTime() - startTime << endl;
    return 0;
}





