#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>

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
factory tf("------- TEST Configure class -------");

}

/* Design the test cases. */
namespace tut
{
//get the line count of given file
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

/**test  basic configureRootLogger.*/
template<>
template<>
void object::test<1>()
{
    Configurator::configureRootLogger();
    Logger* root = Logger::getRootLogger();

    ensure("## Root logger instant by configureRootLogger success", root != NULL);

    ensure("## Default root logger level is INFO", root->getLevel() == LOG_LEVEL_INFO);

    Logger::shutdown();
}

/**test  root logger configure and log only.*/
template<>
template<>
void object::test<2>()
{
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/configuretest1.log";
    const char *filename = stmpfile.c_str();
    unlink(filename);

    string sconf(CONF_FILE_DIR);
    sconf += "/t1.conf";
    const char *conffile = sconf.c_str();
    Configurator::configureLogger(conffile);
    Logger* root = Logger::getRootLogger();

    ensure("## Configure root logger level is DEBUG", root->getLevel() == LOG_LEVEL_DEBUG);

    root->log(LOG_LEVEL_INFO, "configure root test1");
    root->log(LOG_LEVEL_ERROR, "configure root test2");
    Logger::flushAll();

    int lineCount = getFileLineCount(filename);
    ensure("## lineCount is equal to logCount", lineCount == 2);
    Logger::shutdown();
    unlink(filename);
}

/**test  two loggers inherited from root, and test the log output focusing on inherit level relationship.*/
template<>
template<>
void object::test<3>()
{ 
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/configuretest2.log";
    const char *filename1 = stmpfile1.c_str(); 
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/configuretest2-1.log";
    const char *filename2 = stmpfile2.c_str();
    unlink(filename1);
    unlink(filename2);
    string sconf(CONF_FILE_DIR);
    sconf += "/t2.conf";
    const char *conffile = sconf.c_str();
        
    Configurator::configureLogger(conffile);
    Logger* root = Logger::getRootLogger();
    Logger* sub1 = Logger::getLogger("sub1");
    Logger* sub2 = Logger::getLogger("sub2");

    root->log(LOG_LEVEL_INFO, "root  log test1");
    sub1->log(LOG_LEVEL_ERROR, "sub1 log test2");
    sub2->log(LOG_LEVEL_DEBUG, "sub2 log test3");
    Logger::flushAll();

    int lineCount = getFileLineCount(filename1);
    ensure("## configuretest2.log is equal to logCount", lineCount == 4);

    int lineCount2 = getFileLineCount(filename2);
    ensure("## configuretest2-1.log is equal to logCount", lineCount2 == 1);

    Logger::shutdown(); 
    unlink(filename1);
    unlink(filename2);
}

/**test three inherit level logger.
     test muti appenders.
     test inherit false.
     test logger with no specified appender.
*/
template<>
template<>
void object::test<4>()
{
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/configuretest3-1.log";
    const char *filename1 = stmpfile1.c_str(); 
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/configuretest3-2.log";
    const char *filename2 = stmpfile2.c_str();
    unlink(filename1);
    unlink(filename2);
    string sconf(CONF_FILE_DIR);
    sconf += "/t3.conf";
    const char *conffile = sconf.c_str();
        
    Configurator::configureLogger(conffile);
    Logger::getLogger("sub1");
    Logger* sub2 = Logger::getLogger("sub2");
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");

    sub1_sub2->log(LOG_LEVEL_ERROR, "sub1.sub2 log test");
    sub2->log(LOG_LEVEL_DEBUG, "sub2 log test");
    //Logger::flushAll();

    int lineCount = getFileLineCount(filename1);
    ensure("## configuretest3-1.log is equal to logCount", lineCount == 2);

    int lineCount2 = getFileLineCount(filename2);
    ensure("## configuretest3-2.log is equal to logCount", lineCount2 == 1);

    Logger::shutdown(); 
    unlink(filename1);
    unlink(filename2);
}

/**test configure with no root logger.
*/
template<>
template<>
void object::test<5>()
{
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/configuretest4-1.log";
    const char *filename1 = stmpfile1.c_str(); 
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/configuretest4-2.log";
    const char *filename2 = stmpfile2.c_str();
    unlink(filename1);
    unlink(filename2);
    string sconf(CONF_FILE_DIR);
    sconf += "/t4.conf";
    const char *conffile = sconf.c_str();
        
    Configurator::configureLogger(conffile);

    Logger::getLogger("sub1");
    Logger* sub2 = Logger::getLogger("sub2");
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");

    sub1_sub2->log(LOG_LEVEL_ERROR, "sub1.sub2 log test");
    sub2->log(LOG_LEVEL_DEBUG, "sub2 log test");
    Logger::flushAll();

    int lineCount = getFileLineCount(filename1);
    ensure("## configuretest4-1.log is equal to logCount", lineCount == 2);

    int lineCount2 = getFileLineCount(filename2);
    ensure("## configuretest4-2.log is equal to logCount", lineCount2 == 1);

    Logger::shutdown(); 
    unlink(filename1);
    unlink(filename2);
}

/**test configure exception when an appender is not well defined.
*/
template<>
template<>
void object::test<6>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/t5.conf";
    const char *conffile = sconf.c_str();

    try
    {
        Configurator::configureLogger(conffile);
    }
    catch (std::exception e)
    {
        return;
    }
    ensure("## exception fail ", 0 == 1);

    Logger::shutdown();
}

/**test configure exception when define an incorrect appender type.
*/
template<>
template<>
void object::test<7>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/t6.conf";
    const char *conffile = sconf.c_str();

    try
    {
        Configurator::configureLogger(conffile);
    }
    catch (std::exception e)
    {
        return;
    }
    ensure("## exception fail ", 0 == 1);

    Logger::shutdown();
}

/**test configure exception when define an incorrect appender name.
*/
template<>
template<>
void object::test<8>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/t7.conf";
    const char *conffile = sconf.c_str();

    try
    {
        Configurator::configureLogger(conffile);
    }
    catch (std::exception e)
    {
        return;
    }
    ensure("## exception fail ", 0 == 1);

    Logger::shutdown();
}

/**test configure exception when configure file path is incorrect.
*/
template<>
template<>
void object::test<9>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/t8.conf";
    const char *conffile = sconf.c_str();

    try
    {
        Configurator::configureLogger(conffile);
    }
    catch (std::exception e)
    {
        return;
    }
    ensure("## exception fail ", 0 == 1);

    Logger::shutdown();
}

/**test configure exception when configure file path is incorrect.
*/
template<>
template<>
void object::test<10>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/tt.conf";
    const char *conffile = sconf.c_str();

    try
    {
        Configurator::configureLogger(conffile);
    }
    catch (std::exception e)
    {
        return;
    }
    ensure("## exception fail ", 0 == 1);

    Logger::shutdown();
}

/**test  inherited logger's level output.*/
template<>
template<>
void object::test<11>()
{ 
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/configuretest9.log";
    const char *filename = stmpfile.c_str();
    unlink(filename);
    string sconf(CONF_FILE_DIR);
    sconf += "/t9.conf";
    const char *conffile = sconf.c_str();

    Configurator::configureLogger(conffile);
    Logger* sub1_sub2 = Logger::getLogger("sub1.sub2");

    sub1_sub2->log(LOG_LEVEL_INFO, "sub1.sub2 log test");
    Logger::flushAll();

    int lineCount = getFileLineCount(filename);
    ensure("## configuretest9.log is equal to logCount", lineCount == 3);

    Logger::shutdown();
    unlink(filename);
}

/**test  set max message length.*/
template<>
template<>
void object::test<12>()
{ 
    string sconf(CONF_FILE_DIR);
    sconf += "/t10.conf";
    const char *conffile = sconf.c_str();

    Configurator::configureLogger(conffile);
    ensure("## set max message length correctly", Logger::MAX_MESSAGE_LENGTH == 2048);

    Logger::shutdown();
}

/**test  auto flush.*/
template<>
template<>
void object::test<13>()
{
    string stmpfile(TOP_BUILD_DIR);
    stmpfile += "/configuretest11.log";
    const char *filename = stmpfile.c_str();
    unlink(filename);
    string sconf(CONF_FILE_DIR);
    sconf += "/t11.conf";
    const char *conffile = sconf.c_str();

    Configurator::configureLogger(conffile);

    Logger *root = Logger::getRootLogger();
    Appender *appender = FileAppender::getAppender(filename);
    ensure("## set auto flush correctly", appender->isAutoFlush());
    
    root->log(LOG_LEVEL_DEBUG, "test12");
    int lineCount = getFileLineCount(filename);
    ensure("## flush line count correctly", lineCount == 1);

    Logger::shutdown();
    unlink(filename);
}
/**test  set level.*/
template<>
template<>
void object::test<14>()
{
    string stmpfile1(TOP_BUILD_DIR);
    stmpfile1 += "/configuretest12-1.log";
    const char *filename1 = stmpfile1.c_str(); 
    string stmpfile2(TOP_BUILD_DIR);
    stmpfile2 += "/configuretest12-2.log";
    const char *filename2 = stmpfile2.c_str(); 
    string stmpfile3(TOP_BUILD_DIR);
    stmpfile2 += "/configuretest12-3.log";
    const char *filename3 = stmpfile3.c_str();
    unlink(filename1);
    unlink(filename2);
    unlink(filename3);
    string sconf(CONF_FILE_DIR);
    sconf += "/t12.conf";
    const char *conffile = sconf.c_str();
        
    Configurator::configureLogger(conffile);

    Logger *root = Logger::getRootLogger();
    ensure("## 1 root logger level is correct", root->getLevel()==LOG_LEVEL_INFO);
    root->setLevel(LOG_LEVEL_NOTSET);
    ensure("## 2 root logger level is correct", root->getLevel()==LOG_LEVEL_INFO);
    
    Logger *sub1 = Logger::getLogger("sub1"); 
    Logger *sub1_sub1 = Logger::getLogger("sub1.sub1");
    Logger *sub1_sub1_sub1 = Logger::getLogger("sub1.sub1.sub1");

    Logger *sub2 = Logger::getLogger("sub2"); 
    Logger *sub2_sub1 = Logger::getLogger("sub2.sub1");
    Logger *sub2_sub1_sub1 = Logger::getLogger("sub2.sub1.sub1");

    Logger *sub3 = Logger::getLogger("sub3"); 
    Logger *sub3_sub1 = Logger::getLogger("sub3.sub1");
    Logger *sub3_sub1_sub1 = Logger::getLogger("sub3.sub1.sub1");

    ensure("## 1 sub1 logger level is correct", sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 1 sub1_sub1 logger level is correct", sub1_sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 1 sub1_sub1_sub1 logger level is correct", sub1_sub1_sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 1 sub2 logger level is correct", sub2->getLevel()==LOG_LEVEL_WARN);
    ensure("## 1 sub2_sub1 logger level is correct", sub2_sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 1 sub2_sub1_sub1 logger level is correct", sub2_sub1_sub1->getLevel()==LOG_LEVEL_INFO); 
    ensure("## 1 sub3 logger level is correct", sub3->getLevel()==LOG_LEVEL_ERROR);
    ensure("## 1 sub3_sub1 logger level is correct", sub3_sub1->getLevel()==LOG_LEVEL_DEBUG);
    ensure("## 1 sub3_sub1_sub1 logger level is correct", sub3_sub1_sub1->getLevel()==LOG_LEVEL_DEBUG);

    sub1_sub1->setLevel(LOG_LEVEL_WARN);
    sub1_sub1->setInheritFlag(false); 
    ensure("## 2 sub1 logger level is correct", sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 2 sub1_sub1 logger level is correct", sub1_sub1->getLevel()==LOG_LEVEL_WARN);
    ensure("## 2 sub1_sub1_sub1 logger level is correct", sub1_sub1_sub1->getLevel()==LOG_LEVEL_WARN);

    sub2_sub1->setInheritFlag(true); 
    ensure("## 2 sub2 logger level is correct", sub2->getLevel()==LOG_LEVEL_WARN);
    ensure("## 2 sub2_sub1 logger level is correct", sub2_sub1->getLevel()==LOG_LEVEL_WARN);
    ensure("## 2 sub2_sub1_sub1 logger level is correct", sub2_sub1_sub1->getLevel()==LOG_LEVEL_WARN); 

    sub3_sub1->setLevel(LOG_LEVEL_NOTSET);
    ensure("## 2 sub3 logger level is correct", sub3->getLevel()==LOG_LEVEL_ERROR);
    ensure("## 2 sub3_sub1 logger level is correct", sub3_sub1->getLevel()==LOG_LEVEL_ERROR);
    ensure("## 2 sub3_sub1_sub1 logger level is correct", sub3_sub1_sub1->getLevel()==LOG_LEVEL_ERROR);

    sub1_sub1->setLevel(LOG_LEVEL_NOTSET); 
    ensure("## 3 sub1 logger level is correct", sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 3 sub1_sub1 logger level is correct", sub1_sub1->getLevel()==LOG_LEVEL_INFO);
    ensure("## 3 sub1_sub1_sub1 logger level is correct", sub1_sub1_sub1->getLevel()==LOG_LEVEL_INFO);
    
    sub2_sub1->setLevel(LOG_LEVEL_NOTSET);
    ensure("## 3 sub2 logger level is correct", sub2->getLevel()==LOG_LEVEL_WARN);
    ensure("## 3 sub2_sub1 logger level is correct", sub2_sub1->getLevel()==LOG_LEVEL_WARN);
    ensure("## 3 sub2_sub1_sub1 logger level is correct", sub2_sub1_sub1->getLevel()==LOG_LEVEL_WARN); 

    Logger::shutdown();  
    unlink(filename1);
    unlink(filename2);
    unlink(filename3);
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


