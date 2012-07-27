#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>

#include "tut.hpp"
#include "tut_reporter.hpp"

#include "Logger.h"
#include "Appender.h"
#include "Configurator.h"
#include "../cpp/LoggingEvent.h"
#include "test.h"

using namespace std;
using namespace alog;

// Define the struct used in the test cases.
namespace tut
{
struct test_basic
{
    // Define the variable used in test
};

typedef test_group<test_basic> factory;
typedef factory::object object;
factory tf("------- TEST Appender class -------");

}

/* Design the test cases. */
namespace tut
{
/**test  the ConsoleAppender.*/
template<>
template<>
void object::test<1>()
{
    Appender *pAppender = ConsoleAppender::getAppender();
    ensure("## ConsoleAppender constructor success", pAppender != NULL);

    Appender *pAppender2 = ConsoleAppender::getAppender();
    ensure("## the same Appender pointer", pAppender == pAppender2);

    LoggingEvent event1("", "Test ConsoleAppender append", LOG_LEVEL_INFO);
    LoggingEvent event2("", "Test ConsoleAppender append 2", LOG_LEVEL_DEBUG);
    LoggingEvent event3("", "Test ConsoleAppender append 3", LOG_LEVEL_ERROR);
    pAppender->append(event1);
    pAppender2->append(event2);
    pAppender2->append(event3);

    Appender::release();
}

/**test  the FileAppender.*/
template<>
template<>
void object::test<2>()
{
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/AppenderTest.log";
    const char *filename = stmpfile.c_str();
    unlink(filename);

    Appender *pAppender = FileAppender::getAppender(filename);
    ensure("## FileAppender constructor success", pAppender != NULL);

    Appender *pAppender2 = FileAppender::getAppender(filename);
    ensure("## The same Appender pointer", pAppender == pAppender2);

    LoggingEvent event1("", "Test FileAppender append", LOG_LEVEL_INFO);
    LoggingEvent event2("", "Test FileAppender append 2", LOG_LEVEL_DEBUG);
    LoggingEvent event3("", "Test FileAppender append 3", LOG_LEVEL_ERROR);
    LoggingEvent event4("", "Test FileAppender append 4", LOG_LEVEL_FATAL);

    int logCount = 0;
    pAppender->append(event1);
    logCount++;
    pAppender2->append(event2);
    logCount++;
    pAppender2->append(event3);
    logCount++;
    pAppender2->append(event4);
    logCount++;

    Appender::release();

    //to check the log file
    std::ifstream logfile(filename);
    char buffer[256];
    int lineCount = 0;
    while (logfile.getline(buffer, sizeof(buffer)))
    {
        lineCount++;
    }
    cout <<"lineCount: " << lineCount << " logCount: " << logCount << endl;
    ensure("## lineCount is equal to logCount", lineCount == logCount);
    unlink(filename);
}

/**test  the SyslogAppender.*/
template<>
template<>
void object::test<3>()
{
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/syslog";
    const char *filename = stmpfile.c_str();
   
    Appender *pAppender = SyslogAppender::getAppender(filename, 2<<3);
    ensure("## SyslogAppender constructor success", pAppender != NULL);

    Appender *pAppender2 = SyslogAppender::getAppender(filename, 2<<3);
    ensure("## the same Appender pointer", pAppender == pAppender2);
 
    LoggingEvent event1("", "Test SyslogAppender append", LOG_LEVEL_INFO);
    LoggingEvent event2("", "Test SyslogAppender append 2", LOG_LEVEL_DEBUG);
    LoggingEvent event3("", "Test SyslogAppender append 3", LOG_LEVEL_ERROR);

    pAppender->append(event1);
    pAppender2->append(event2);
    pAppender2->append(event3);

    Appender::release();
}

bool createFile(const string &filename) {
    FILE* file = fopen(filename.c_str(), "a+");
    if (!file) {
      cerr << "create file error, filename: " << filename << endl;
    }
    fclose(file);
    return true;
}

bool fileExist(const string &filename) {
    struct stat st;
    errno = 0;
    int statRet = lstat(filename.c_str(), &st);
    if (statRet != 0 && errno == ENOENT) {
        return false;
    }
    return true;
}

/**test main procedure of FileAppender::removeHistoryLogFile().*/
template<>
template<>
void object::test<4>()
{
    //remove old exist dir and create dir again 
    string logDir = string(TOP_BUILD_DIR) + "/removeHistoryLogFile";
    string rmdircmd = string("rm -rf ") + logDir;
    system(rmdircmd.c_str());
    int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    mkdir(logDir.c_str(), mode);

    //create multi history log file
    string filename = "test.log";
    string fullFileName = logDir + "/" + filename;
    createFile(fullFileName);
    createFile(fullFileName + string(".20110714160656-001"));
    createFile(fullFileName + string(".20110714160656-002.gz"));
    createFile(fullFileName + string(".20110714160657-001"));
    createFile(fullFileName + string(".20110714160657-002.gz"));
    createFile(fullFileName + string(".20110714160658-001"));
    createFile(fullFileName + string(".20110714160658-002.gz"));

    //init appender and call 'removeHistoryLogFile' to recycle log file
    Appender *pAppender = FileAppender::getAppender(filename.c_str());
    ensure("## FileAppender constructor success", pAppender != NULL);
    FileAppender *fileAppender = (FileAppender *)pAppender;
    ensure("## cast to fileAppender", fileAppender != NULL);
    ensure("## check keep count", 0 == fileAppender->getHistoryLogKeepCount());
    fileAppender->setHistoryLogKeepCount(-1);
    ensure("## check keep count", 
	   FileAppender::MAX_KEEP_COUNT == fileAppender->getHistoryLogKeepCount());
    fileAppender->setHistoryLogKeepCount(2);
    fileAppender->removeHistoryLogFile(logDir.c_str());

    //to check the result of 'removeHistoryLogFile'
    bool existRet = false;
    existRet = fileExist(fullFileName);
    ensure("exist 1", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160656-001"));
    ensure("exist 2", existRet == false);//removed
    existRet = fileExist(fullFileName + string(".20110714160656-002.gz"));
    ensure("exist 3", existRet == false);//removed
    existRet = fileExist(fullFileName + string(".20110714160657-001"));
    ensure("exist 4", existRet == false);//removed
    existRet = fileExist(fullFileName + string(".20110714160657-002.gz"));
    ensure("exist 5", existRet == false);
    existRet = fileExist(fullFileName + string(".20110714160658-001"));
    ensure("exist 6", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160658-002.gz"));
    ensure("exist 7", existRet == true);

    Appender::release();
}

/**test FileAppender::removeHistoryLogFile() when keepCount <= actualCount.*/
template<>
template<>
void object::test<5>()
{
    //remove old exist dir and create dir again 
    string logDir = string(TOP_BUILD_DIR) + "/removeHistoryLogFile2";
    string rmdircmd = string("rm -rf ") + logDir;
    system(rmdircmd.c_str());
    int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
    mkdir(logDir.c_str(), mode);

    //create multiple history log file
    string fullFileName = logDir + "/test.log";
    createFile(fullFileName);
    createFile(fullFileName + string(".20110714160656-002"));
    createFile(fullFileName + string(".20110714160656-003.gz"));
    createFile(fullFileName + string(".20110714160657-001.gz"));
    createFile(fullFileName + string(".20110714160657-002"));

    //init appender and call 'removeHistoryLogFile' to recycle log file
    Appender *pAppender = FileAppender::getAppender(fullFileName.c_str());
    ensure("## FileAppender constructor success", pAppender != NULL);
    FileAppender *fileAppender = (FileAppender *)pAppender;
    ensure("## cast to fileAppender", fileAppender != NULL);
    ensure("## check keep count", 0 == fileAppender->getHistoryLogKeepCount());
    fileAppender->setHistoryLogKeepCount(4);
    fileAppender->removeHistoryLogFile(logDir.c_str());

    //to check the result of 'removeHistoryLogFile'
    bool existRet = false;
    existRet = fileExist(fullFileName);
    ensure("exist 1", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160656-002"));
    ensure("exist 2", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160656-003.gz"));
    ensure("exist 3", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160657-001.gz"));
    ensure("exist 4", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160657-002"));
    ensure("exist 5", existRet == true);
    
    //set history log keepCount < actualCount 
    fileAppender->setHistoryLogKeepCount(3);
    fileAppender->removeHistoryLogFile(logDir.c_str());

    //to check the result of 'removeHistoryLogFile'
    existRet = fileExist(fullFileName);
    ensure("exist 21", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160656-002"));
    ensure("exist 22", existRet == false);
    existRet = fileExist(fullFileName + string(".20110714160656-003.gz"));
    ensure("exist 23", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160657-001.gz"));
    ensure("exist 24", existRet == true);
    existRet = fileExist(fullFileName + string(".20110714160657-002"));
    ensure("exist 25", existRet == true);

    Appender::release();
}

}


/* Run the test cases. */
using tut::reporter;
using tut::groupnames;
namespace tut
{
test_runner_singleton runner;
}

int main(int argc, char ** argv)
{
    reporter visi;
    tut::runner.get().set_callback(&visi);
    tut::runner.get().run_tests();
    if(visi.all_ok())
        return 0;
    else
        return -1;
}


