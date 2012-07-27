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

//the output log count set by user.
int logCount = 0;
const char* TEST_STR = "abcdefg hijklmn opqrst uvwxyz";
std::map<pthread_t, uint64_t> g_times;

uint64_t nowTime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return (t.tv_sec * 1000000LL + t.tv_usec);
}

void *output_fwrite(void *param)
{
    FILE *logFile = (FILE *)param;
    int count = 0;
    char tmp[20];
    while (count < logCount)
    {
        //alog::Appender::getCurTime(tmp, 20);//get the current time string
        fprintf(logFile, "%d (%u/%u) [INFO] %s\n", time(NULL), getpid(), (uint32_t)pthread_self(), TEST_STR);
        count++;
    }
    return NULL;
}

void *output_alog(void *param)
{
    int count = 0;
    alog::Logger *logger = alog::Logger::getRootLogger();
    while (count < logCount)
    {
        logger->log(alog::LOG_LEVEL_INFO, TEST_STR);
        count++;
    }
    return NULL;
}

void *output_log4cxx(void *param)
{
    int count = 0;
    try {
        log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
        while (count < logCount)
        {
            LOG4CXX_INFO(rootLogger, TEST_STR);
            count++;
        }
    } catch (...)
    {
        std::cerr <<"exception occurred!" << std::endl;
    }
    return NULL;
}

uint32_t printTimeStat()
{
    uint32_t totalTime = 0L;
    for (map<pthread_t, uint64_t>::iterator it = g_times.begin(); it != g_times.end(); it++)
    {
        //cerr << "pthread id: " << it->first << "  time cost: " << it->second <<  " us" << endl;
        totalTime += it->second;
    }
    return totalTime;
}


int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <logCount> <threadCount>" <<  std::endl;
        exit(-1);
    }
    logCount = atoi(argv[1]);
    int threadCount = atoi(argv[2]);
    std::cerr << " -------- logCount: " << logCount << "  --------" << std::endl;
    std::cerr << " -------- threadCount: " << threadCount << "  --------" << std::endl;

    const char *filename = "/tmp/mpt.log";
    const char *filename2 = "/tmp/mpt.log2";
    const char *filename3 = "/tmp/mpt.log3";

    vector<pthread_t> tids;
    uint64_t  totalTime = 0L;
    uint64_t startTime = 0L;
    unlink(filename);

    /** fwrite output test.*/
    tids.clear();
    g_times.clear();
    FILE *logFile = fopen(filename, "a+");
    if (!logFile)
    {
        cerr << "File open error" << endl;
        exit(-2);
    }
    //create thread.
    startTime = nowTime();
    for (int i = 0; i <threadCount; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, output_fwrite, logFile);
        tids.push_back(tid);
    }
    cerr << "## create " << tids.size() <<" threads" << endl;
    //join child thread.
    for (int i = 0; i <tids.size(); i++)
    {
        pthread_t tid = tids[i];
        pthread_join(tid, NULL);
    }
    totalTime = printTimeStat();
    cerr << "[fwrite time cost]: " << nowTime() - startTime  << " us" << endl;
    cerr << "---------------------------------------------------" << endl;
    fclose(logFile);

    /** alog output test. */
    unlink(filename2);
    tids.clear();
    g_times.clear();
    alog::Configurator::configureRootLogger();
    alog::Logger *logger = alog::Logger::getRootLogger();
    logger->setAppender(alog::FileAppender::getAppender(filename2));
    //create thread.
    startTime = nowTime();
    for (int i = 0; i <threadCount; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, output_alog, NULL);
        tids.push_back(tid);
    }
    cerr << "## create " << tids.size() <<" threads" << endl;
    //join child thread.
    for (int i = 0; i <tids.size(); i++)
    {
        pthread_t tid = tids[i];
        pthread_join(tid, NULL);
    }
    totalTime = printTimeStat();
    cerr << "[alog time cost]: " << nowTime() - startTime  <<  " us" << endl;
    cerr << "---------------------------------------------------" << endl;
    alog::Logger::shutdown();

    /** log4cxx output.*/
    unlink(filename3);
    tids.clear();
    g_times.clear();
    try {
        //log4cxx::LayoutPtr layout(new log4cxx::SimpleLayout());
        log4cxx::LayoutPtr layout(new log4cxx::PatternLayout("%p [%d] %C.%M(%L) | %m%n"));
        log4cxx::LoggerPtr rootLogger = log4cxx::Logger::getRootLogger();
        log4cxx::FileAppenderPtr appender(new log4cxx::FileAppender(layout, filename3, true));
        rootLogger->addAppender(appender);

    } catch (...) {
        cerr <<"## log4cxx configure test" << endl;
        exit(-3);
    }
    //create thread.
    startTime = nowTime();
    for (int i = 0; i <threadCount; i++)
    {
        pthread_t tid;
        pthread_create(&tid, NULL, output_log4cxx, NULL);
        tids.push_back(tid);
    }
    cerr << "## create " << tids.size() <<" threads" << endl;
    //join child thread.
    for (int i = 0; i <tids.size(); i++)
    {
        pthread_t tid = tids[i];
        pthread_join(tid, NULL);
    }
    totalTime = printTimeStat();
    cerr << "[log4cxx time cost]: " << nowTime() - startTime  <<  " us" << endl;
    cerr << "---------------------------------------------------" << endl;

    return 0;
}





