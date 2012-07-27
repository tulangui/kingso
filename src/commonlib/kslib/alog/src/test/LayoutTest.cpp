#include <stdio.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <linux/unistd.h>

#include "tut.hpp"
#include "tut_reporter.hpp"

#include "Logger.h"
#include "Appender.h"
#include "Configurator.h"
#include "Layout.h"
#include "../cpp/LoggingEvent.h"

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
factory tf("------- TEST Layout class -------");

}

/* Design the test cases. */
namespace tut
{
/**test the BasicLayout.*/
template<>
template<>
void object::test<1>()
{
    Layout *basic = new BasicLayout();
    LoggingEvent event("test1","log content of test1",LOG_LEVEL_WARN);
    snprintf(event.loggingTime, 20, "%04d-%02d-%02d %02d:%02d:%02d", 2009, 3, 17, 10, 36, 41);
    string formattedStr = basic->format(event);
    delete basic;
    ensure("Basic Layout formatted string is correct", formattedStr == "2009-03-17 10:36:41 WARN test1 : log content of test1\n");
}

/**test the SimpleLayout.*/
template<>
template<>
void object::test<2>()
{
    Layout *simple = new SimpleLayout();
    LoggingEvent event("test2","log content of test2",LOG_LEVEL_ERROR);
    snprintf(event.loggingTime, 20, "%04d-%02d-%02d %02d:%02d:%02d", 2009, 3, 17, 10, 36, 41);
    int pid = getpid();
    long tid = (long)syscall(SYS_gettid);
    string formattedStr = simple->format(event);
    char target[256];
    sprintf(target, "2009-03-17 10:36:41 ERROR test2 (%d/%ld) : log content of test2\n", pid, tid);
    delete simple;
    ensure("Simple Layout formatted string is correct", formattedStr == target);
}

/**test the PatternLayout: Default pattern.*/
template<>
template<>
void object::test<3>()
{
    Layout *layout = new PatternLayout();
    LoggingEvent event("test3","log content of test3",LOG_LEVEL_ERROR);
    snprintf(event.loggingTime, 20, "%04d-%02d-%02d %02d:%02d:%02d", 2009, 3, 17, 10, 36, 41);
    int pid = getpid();
    long tid = (long)syscall(SYS_gettid);
    string formattedStr = layout->format(event);
    char target[256];
    sprintf(target, "2009-03-17 10:36:41 ERROR test3 (%d/%ld) : log content of test3\n", pid, tid);
    delete layout;
    ensure("Pattern Layout formatted string is correct", formattedStr == target);
}

/**test the PatternLayout: set pattern.*/
template<>
template<>
void object::test<4>()
{
    PatternLayout *layout = new PatternLayout();
    layout->setLogPattern(string("ALOG -- [%%d] [%%c] [%%l] (%%p/%%t) [%%F:%%n:%%f()] : %%m"));
    LoggingEvent event("test4","log content of test4",LOG_LEVEL_ERROR, "test4file", 100, "case4");
    snprintf(event.loggingTime, 20, "%04d-%02d-%02d %02d:%02d:%02d", 2009, 3, 17, 10, 36, 41);
    int pid = getpid();
    long tid = (long)syscall(SYS_gettid);
    string formattedStr = layout->format(event);
    char target[256];
    sprintf(target, "ALOG -- [2009-03-17 10:36:41] [test4] [ERROR] (%d/%ld) [test4file:100:case4()] : log content of test4\n", pid, tid);
    delete layout;
    ensure("Pattern Layout formatted string is correct", formattedStr == target);
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


