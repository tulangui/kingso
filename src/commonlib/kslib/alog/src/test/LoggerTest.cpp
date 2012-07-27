#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <string.h>

#include "tut.hpp"
#include "tut_reporter.hpp"

#include "Logger.h"
#include "Appender.h"
#include "Configurator.h"
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
factory tf("------- TEST Logger class -------");

}

/* Design the test cases. */
namespace tut
{
int getFileLineCount(const char * filename)
{
    //to count the line count of given file
    std::ifstream logfile(filename);
    char buffer[2*Logger::MAX_MESSAGE_LENGTH];
    int lineCount = 0;
    while (logfile.getline(buffer, sizeof(buffer)))
        lineCount++;
    return lineCount;
}

/**test  the root Logger.*/
template<>
template<>
void object::test<1>()
{
    Logger *logger = Logger::getRootLogger();
    ensure("## root logger is legal", logger != NULL);
    ensure("## root logger's name is right", logger->getName() == "");
    ensure("## root logger's inherit flag is right", logger->getInheritFlag() == true);

    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.case1.log"; 
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/lt.case1.log2";

    const char* tmpfile = stmpfile.c_str();
    const char* tmpfile2 = stmpfile2.c_str();

    unlink(tmpfile);
    unlink(tmpfile2);

    logger->setAppender(FileAppender::getAppender(tmpfile));
    logger->addAppender(FileAppender::getAppender(tmpfile2));
    logger->setLevel(LOG_LEVEL_ERROR);
    ensure("## level set is right", logger->getLevel() == LOG_LEVEL_ERROR);

    int count = 0;
    while (count < 10000)
    {
        logger->log(LOG_LEVEL_DEBUG, "debug message:%d", count);
        logger->log(LOG_LEVEL_INFO, "info message:%d", count);
        logger->log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, "error message:%d", count);
        logger->log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, "fatal message:%d", count);
        count++;
    }

    int lineCount = getFileLineCount(tmpfile);
   // cout << "lineCount: " << lineCount;
    ensure("## message count is right", lineCount == 20000);
    lineCount = getFileLineCount(tmpfile2);
   // cout << "lineCount: " << lineCount;
    ensure("## message count is right 2", lineCount == 20000);

    logger->setLevel(LOG_LEVEL_DEBUG);
    while (count < 30000)
    {
        logger->log(LOG_LEVEL_DEBUG, "debug message:%d", count);
        logger->log(LOG_LEVEL_INFO, "info message:%d", count);
        logger->log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __FUNCTION__, "error message:%d", count);
        logger->log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __FUNCTION__, "fatal message:%d", count);
        count++;
    }

    lineCount = getFileLineCount(tmpfile);
    //cout << "lineCount: " << lineCount;
    ensure("## message count is right", lineCount == 100000);
    lineCount = getFileLineCount(tmpfile2);
    //cout << "lineCount: " << lineCount;
    ensure("## message count is right 2", lineCount == 100000);

    unlink(tmpfile);
    unlink(tmpfile2);
    Logger::shutdown();
}

//#if 0
/**test  the output logic of none root Logger.*/

template<>
template<>
void object::test<2>()
{ 
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/test2.isearch.log";  
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/test2.isearch.index.log"; 
    string stmpfile3(TOP_BUILD_DIR);
    stmpfile3 += "/test2.isearch.index.segment.log"; 
    const char *tmpfile1 = stmpfile1.c_str();
    const char *tmpfile2 = stmpfile2.c_str();
    const char *tmpfile3 = stmpfile3.c_str();
    unlink(tmpfile1);
    unlink(tmpfile2);
    unlink(tmpfile3);

    Logger *logger = Logger::getLogger("isearch");
    logger->setAppender(FileAppender::getAppender(tmpfile1));
    logger->setLevel(LOG_LEVEL_ERROR);

    Logger *logger_c = Logger::getLogger("isearch.index");
    logger_c->setAppender(FileAppender::getAppender(tmpfile2));
    logger_c->setLevel(LOG_LEVEL_INFO);

    Logger *logger_c_c = Logger::getLogger("isearch.index.segment");
    logger_c_c->setAppender(FileAppender::getAppender(tmpfile3));
    logger_c_c->setLevel(LOG_LEVEL_DEBUG);

    ensure(" ## logger pointer is legal" ,  logger == Logger::getLogger("isearch"));
    ensure(" ## logger pointer is legal 2", logger_c == Logger::getLogger("isearch.index"));
    ensure(" ## logger pointer is legal 3", logger_c_c == Logger::getLogger("isearch.index.segment"));

    int count = 0;
    while (count < 30000)
    {
        logger_c_c->log(LOG_LEVEL_DEBUG, "debug message: %d", count);
        logger_c_c->log(LOG_LEVEL_INFO, "info message: %d", count);
        logger_c_c->log(LOG_LEVEL_ERROR, "error message: %d", count);
        logger_c_c->log(LOG_LEVEL_FATAL, "fatal message: %d", count);

        logger_c->log(LOG_LEVEL_DEBUG, "debug message: %d", count);
        logger_c->log(LOG_LEVEL_INFO, "info message: %d", count);
        logger_c->log(LOG_LEVEL_ERROR, "error message: %d", count);
        logger_c->log(LOG_LEVEL_FATAL, "fatal message: %d", count);

        logger->log(LOG_LEVEL_DEBUG, "debug message: %d", count);
        logger->log(LOG_LEVEL_INFO, "info message: %d", count);
        logger->log(LOG_LEVEL_ERROR, "error message: %d", count);
        logger->log(LOG_LEVEL_FATAL, "fatal message: %d", count);
        count++;
    }

    int lineCount1 = getFileLineCount(tmpfile1);
    int lineCount2 = getFileLineCount(tmpfile2);
    int lineCount3 = getFileLineCount(tmpfile3);

    ensure("lineCount1 is correct", lineCount1 == (30000*4 + 30000*3 + 30000*2));
    ensure("lineCount2 is correct", lineCount2 == (30000*4 + 30000*3));
    ensure("lineCount3 is correct", lineCount3 == (30000*4)); 
 
    unlink(tmpfile1);
    unlink(tmpfile2);
    unlink(tmpfile3);

    Logger::shutdown();
}

//#if 0

/**test  simple test for a logger.*/
template<>
template<>
void object::test<3>()
{ 
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/LoggerTest1-1.log";   
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/LoggerTest1-2.log";  
    const char *tmpfile1 = stmpfile1.c_str();
    const char *tmpfile2 = stmpfile2.c_str();
    unlink(tmpfile1);
    unlink(tmpfile2);
    Logger* logger = Logger::getLogger("simpleLogger");
    uint32_t level = logger->getLevel();
    uint32_t rootLevel = Logger::getRootLogger()->getLevel();
    ensure("## Default log level is INFO", level == LOG_LEVEL_INFO);
    ensure("## Default root log level is INFO", rootLevel == LOG_LEVEL_INFO);

    bool inherit = logger->getInheritFlag();
    ensure("## Default inherit is true", inherit == true);

    std::string name = logger->getName();
    ensure("## Logger is correctly named", name == "simpleLogger");

    bool isEnable = logger->isLevelEnabled(LOG_LEVEL_ERROR);
    ensure("## ERROR is able to log on INFO", isEnable== true);

    isEnable = logger->isLevelEnabled(LOG_LEVEL_DEBUG);
    ensure("## DEBUG is unable to log on INFO", isEnable== false);

    logger->setLevel(LOG_LEVEL_ERROR);
    level = logger->getLevel();
    ensure("## Set log level is correct", level == LOG_LEVEL_ERROR);

    Appender *appender = FileAppender::getAppender(tmpfile1);
    logger->setAppender(appender);
    logger->log(LOG_LEVEL_ERROR, "simpletest");
    logger->log(LOG_LEVEL_FATAL, "simpletest %s", "...");
    logger->logPureMessage(LOG_LEVEL_ERROR, "simpletest pure msg");
    logger->log(LOG_LEVEL_INFO, "should not be output");

    int lineCount = getFileLineCount(tmpfile1);
    ensure("## setAppender : lineCount is equal to logCount", lineCount == 3);

    Appender *appender2 = FileAppender::getAppender(tmpfile2);
    logger->addAppender(appender2);
    logger->log(LOG_LEVEL_ERROR, "simpletest");
    logger->log(LOG_LEVEL_FATAL, "filename", 10, "function", "simpletest %s", "...");
    logger->logPureMessage(LOG_LEVEL_ERROR, "simpletest pure msg");
    logger->log(LOG_LEVEL_INFO, "should not be output");
    std::ifstream logfile2(tmpfile2);

    int lineCount2 = getFileLineCount(tmpfile2);
    ensure("## addAppender : lineCount is equal to logCount", lineCount2 == 3);

    logger->removeAllAppenders();
    logger->log(LOG_LEVEL_ERROR, "not be output");

    int lineCount3 = getFileLineCount(tmpfile1);
    ensure("## removeAllAppenders1 : lineCount is equal to logCount", lineCount3 == 6);

    int lineCount4 = getFileLineCount(tmpfile2);
    ensure("## removeAllAppenders2 : lineCount is equal to logCount", lineCount4 == 3); 
 
    unlink(tmpfile1);
    unlink(tmpfile2);

    Logger::shutdown();
}

/**test  inherit test. */
template<>
template<>
void object::test<4>()
{ 
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/LoggerTest2-1.log";   
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/LoggerTest2-2.log";   
    string stmpfile3(TOP_BUILD_DIR);
    stmpfile3 += "/LoggerTest2-3.log";  
    const char *tmpfile1 = stmpfile1.c_str();
    const char *tmpfile2 = stmpfile2.c_str();
    const char *tmpfile3 = stmpfile3.c_str();
    unlink(tmpfile1);
    unlink(tmpfile2);
    unlink(tmpfile3);

    Appender* appender1 = FileAppender::getAppender(tmpfile1);
    Appender* appender2 = FileAppender::getAppender(tmpfile2);
    Appender* appender3 = FileAppender::getAppender(tmpfile3);
    Logger* sub1 = Logger::getLogger("sub1");
    sub1->addAppender(appender1);
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");
    sub1_sub2->addAppender(appender1);
    Logger* sub1_sub2_sub3 = Logger::getLogger("sub1.sub2.sub3");
    sub1_sub2_sub3->addAppender(appender1);

    sub1_sub2_sub3->log(LOG_LEVEL_INFO, "inherit test1");
    Logger::flushAll();
    int lineCount = getFileLineCount(tmpfile1);
    ensure("## inherit1 : lineCount is equal to logCount", lineCount == 3);

    sub1_sub2->setLevel(LOG_LEVEL_ERROR);
    sub1->setAppender(appender2);
    sub1_sub2->setAppender(appender2);
    sub1_sub2_sub3->setAppender(appender2);
    sub1_sub2_sub3->log(LOG_LEVEL_INFO, "inherit test2");
    Logger::flushAll();
    int lineCount2 = getFileLineCount(tmpfile2);
    ensure("## inherit2 : lineCount is equal to logCount", lineCount2 == 0);

    sub1_sub2->setLevel(LOG_LEVEL_INFO);
    sub1->setAppender(appender3);
    sub1_sub2->setAppender(appender3);
    sub1_sub2_sub3->setAppender(appender3);
    sub1_sub2->setInheritFlag(false);
    sub1_sub2_sub3->log(LOG_LEVEL_INFO, "inherit test3");
    Logger::flushAll();
    int lineCount3  = getFileLineCount(tmpfile3);
    ensure("## inherit3 : lineCount is equal to logCount", lineCount3 == 2);
  
    unlink(tmpfile1);
    unlink(tmpfile2);
    unlink(tmpfile3);

    Logger::shutdown();
}

/**test  getParentLogger(). */
template<>
template<>
void object::test<5>()
{
    cout<<"test 5"<<endl;
    Logger *logger_s_s = alog::Logger::getLogger("isearch.search.filter");
    Logger *logger_s = alog::Logger::getLogger("isearch.search");
    Logger *logger = alog::Logger::getLogger("isearch");

    ensure("parent ensure not ok 1", logger_s == logger_s_s->getParentLogger());
    ensure("parent ensure not ok 2", logger == logger_s->getParentLogger());
    Logger::shutdown();
}

/**test  long message(). */
template<>
template<>
void object::test<6>()
{ 
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.test6.log";   
    const char *filename = stmpfile.c_str();
    unlink(filename);
    Logger *logger = alog::Logger::getLogger("isearch");
    logger->setAppender(alog::FileAppender::getAppender(filename));
    logger->addAppender(alog::ConsoleAppender::getAppender());

    char buffer[Logger::MAX_MESSAGE_LENGTH + 20];
    int buffLen =  sizeof(buffer);
    memset(buffer, 'x', buffLen);
    buffer[buffLen -1] = '\0';
    logger->log(LOG_LEVEL_ERROR, "%s", buffer);
    logger->flush();

    int lineCount = getFileLineCount(filename);
    ensure("test 6 lineCount is right", lineCount == 1);
    unlink(filename);
    Logger::shutdown();
}

template<>
template<>
void object::test<7>()
{ 
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.test7.log";   
    const char *filename = stmpfile.c_str();
    unlink(filename);
    Logger *root = alog::Logger::getRootLogger();
    root->setAppender(alog::FileAppender::getAppender(filename));
    Logger *childA = alog::Logger::getLogger("childA");
    Logger *childB = alog::Logger::getLogger("childB");
    childA->log(alog::LOG_LEVEL_DEBUG, "message 1. should NOT output it");
    childB->log(alog::LOG_LEVEL_DEBUG, "message 1. should NOT output it");
    Logger::flushAll();
    ensure("test 7-1 lineCount is right", getFileLineCount(filename) == 0);
    childA->setLevel(LOG_LEVEL_DEBUG);
    childA->log(alog::LOG_LEVEL_DEBUG, "message 2. SHOULD output it");
    childB->log(alog::LOG_LEVEL_DEBUG, "message 2. should NOT output it");
    childB->log(alog::LOG_LEVEL_DEBUG, "message 2. should NOT output it");
    Logger::flushAll();
    ensure("test 7-2 lineCount is right", getFileLineCount(filename) == 1);
    root->setLevel(LOG_LEVEL_TRACE1);
    childB->setLevel(LOG_LEVEL_TRACE2);
    childA->log(alog::LOG_LEVEL_TRACE1, "message 3. should NOT output it");
    childA->log(alog::LOG_LEVEL_TRACE1, "message 3. should NOT output it");
    childB->log(alog::LOG_LEVEL_TRACE1, "message 3. SHOULD output it");
    Logger::flushAll();
    ensure("test 7-3 lineCount is right", getFileLineCount(filename) == 2);
    unlink(filename);
    Logger::shutdown();
}
/** test large file */
template<>
template<>
void object::test<8>()
{ 
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.test8.log";   
    const char *filename = stmpfile.c_str();
    unlink(filename);
    FILE *file = fopen64(filename, "w");
    fseeko64(file, 3*1024*1024*1024L, SEEK_SET);
    fwrite("end", 1, 3, file);
    fclose(file);
    Logger *root = alog::Logger::getRootLogger();
    root->setAppender(alog::FileAppender::getAppender(filename));
    root->log(LOG_LEVEL_ERROR,"test8");
    Logger::shutdown();
    unlink(filename);
}
/** test setlevel */
template<>
template<>
void object::test<9>()
{ 
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.test9.log";   
    const char *filename = stmpfile.c_str();
    unlink(filename);
    Logger *root = alog::Logger::getRootLogger();
    root->setAppender(alog::FileAppender::getAppender(filename));
    root->setLevel(LOG_LEVEL_ERROR);
    Logger *sub1_sub2_sub3 = alog::Logger::getLogger("sub1.sub2.sub3");
    root->log(LOG_LEVEL_ERROR,"test9");
    sub1_sub2_sub3->log(LOG_LEVEL_ERROR,"test9");
    sub1_sub2_sub3->log(LOG_LEVEL_WARN,"test9");
    ensure("test 9-1 lineCount is right", getFileLineCount(filename) == 2);
    Logger *sub1_sub2 = alog::Logger::getLogger("sub1.sub2");
    sub1_sub2->log(LOG_LEVEL_FATAL,"test9");
    sub1_sub2->log(LOG_LEVEL_INFO,"test9");
    ensure("test 9-1 lineCount is right", getFileLineCount(filename) == 3);
    Logger::shutdown();
    unlink(filename);
}
/** test setlevel */
template<>
template<>
void object::test<10>()
{
    Logger *root = alog::Logger::getRootLogger();
    Logger *sub1_sub2_sub3 = alog::Logger::getLogger("sub1.sub2.sub3");
    Logger *sub1_sub2 = alog::Logger::getLogger("sub1.sub2");
    Logger *sub1 = alog::Logger::getLogger("sub1");
    ensure("test 10-1 sub1 level correct", sub1->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-2 sub1_sub2 level correct", sub1_sub2->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-3 sub1_sub2_sub3 level correct", sub1_sub2_sub3->getLevel() == LOG_LEVEL_INFO);
    root->setLevel(LOG_LEVEL_ERROR);
    Logger *sub2 = alog::Logger::getLogger("sub2");
    Logger *sub2_sub2 = alog::Logger::getLogger("sub2.sub2"); 
    ensure("test 10-4 sub1 level correct", sub1->getLevel() == LOG_LEVEL_ERROR);
    ensure("test 10-5 sub1_sub2 level correct", sub1_sub2->getLevel() == LOG_LEVEL_ERROR);
    ensure("test 10-6 sub1_sub2_sub3 level correct", sub1_sub2_sub3->getLevel() == LOG_LEVEL_ERROR); 
    ensure("test 10-7 sub2 level correct", sub2->getLevel() == LOG_LEVEL_ERROR);
    ensure("test 10-8 sub2_sub2 level correct", sub2_sub2->getLevel() == LOG_LEVEL_ERROR);
    sub1_sub2->setLevel(LOG_LEVEL_INFO);
    ensure("test 10-9 sub1 level correct", sub1->getLevel() == LOG_LEVEL_ERROR);
    ensure("test 10-10 sub1_sub2 level correct", sub1_sub2->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-11 sub1_sub2_sub3 level correct", sub1_sub2->getLevel() == LOG_LEVEL_INFO);
    sub2->setInheritFlag(false); 
    ensure("test 10-12 sub2 level correct", sub2->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-13 sub2_sub2 level correct", sub2_sub2->getLevel() == LOG_LEVEL_INFO);
    root->setLevel(LOG_LEVEL_WARN);
    ensure("test 10-14 sub1 level correct", sub1->getLevel() == LOG_LEVEL_WARN);
    ensure("test 10-15 sub1_sub2 level correct", sub1_sub2->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-16 sub1_sub2_sub3 level correct", sub1_sub2->getLevel() == LOG_LEVEL_INFO); 
    ensure("test 10-17 sub2 level correct", sub2->getLevel() == LOG_LEVEL_INFO);
    ensure("test 10-18 sub2_sub2 level correct", sub2_sub2->getLevel() == LOG_LEVEL_INFO);
    sub2->setLevel(LOG_LEVEL_DEBUG);
    ensure("test 10-19 sub2 level correct", sub2->getLevel() == LOG_LEVEL_DEBUG);
    ensure("test 10-20 sub2_sub2 level correct", sub2_sub2->getLevel() == LOG_LEVEL_DEBUG);

    Logger::shutdown();
}
/**test  long pattern layout message(). */
template<>
template<>
void object::test<11>()
{
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/lt.test11.log";
    const char *filename = stmpfile.c_str();
    unlink(filename);
    Logger *logger = alog::Logger::getLogger("abc");
    PatternLayout *layout = new PatternLayout();
    PatternLayout *layout2 = new PatternLayout();
    layout->setLogPattern("[%%d] [%%F -- LINE(%%n) -- %%f()] [%%m]");
    layout2->setLogPattern("[%%d] [%%F -- LINE(%%n) -- %%f()] [%%m]");
    alog::Appender *appender1 = alog::FileAppender::getAppender(filename);
    appender1->setLayout(layout);
    alog::Appender *appender2 = alog::ConsoleAppender::getAppender();
    cout << " ### appender: " << appender2 << endl;
    appender2->setLayout(layout2);
    logger->setAppender(appender1);
    logger->addAppender(appender2);

    char buffer[Logger::MAX_MESSAGE_LENGTH + 100];                                                                                        
    int buffLen =  sizeof(buffer);
    memset(buffer, 'a', buffLen);
    buffer[buffLen -1] = '\0';
    logger->log(LOG_LEVEL_ERROR, "%s", buffer);
    logger->flush();

    int lineCount = getFileLineCount(filename);
    ensure("test 11 lineCount is right", lineCount == 1);
    unlink(filename);
    Logger::shutdown();
}

//#endif
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


