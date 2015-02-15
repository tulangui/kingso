/**
*@file LoggingEvent.h
*@brief the file to declare LoggingEvent struct.
*
*@version 1.0.4
*@date 2009.03.05
*@author bingbing.yang
*/
#ifndef _ALOG_LOGGINGEVENT_H_
#define _ALOG_LOGGINGEVENT_H_

//#include <linux/unistd.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <sys/syscall.h>

namespace alog
{
static const char* s_gLevelName[] = {"DISABLE", "FATAL", "ERROR", "WARN",  "INFO", "DEBUG", "TRACE1", "TRACE2", "TRACE3", "NOTSET"};

struct LoggingEvent 
{
public:
    /**
    *@brief to get formatted current time stored in param cur as a return.
    *
    *@param cur formatted time as a return value.
    *@param length the cur buffer's length
    */
    void getCurTime(char cur[], int length)
    {
        time_t n = time(NULL);
        struct tm * p = localtime(&n);
        snprintf(cur, length, "%04d-%02d-%02d %02d:%02d:%02d",
             p->tm_year + 1900, p->tm_mon + 1, p->tm_mday,
             p->tm_hour, p->tm_min, p->tm_sec);
    }
    /**
    *@brief constructor.
    *
    *@param name logger name.
    *@param msg user's logger message.
    *@param level logging level.
    */
    LoggingEvent(const std::string& name, const std::string& msg, const uint32_t level, const std::string& file = "", const int& line = 0, const std::string& func = "") : 
        loggerName(name), message(msg), level(level),levelStr(s_gLevelName[level]),
        file(file), line(line), function(func), pid(getpid()), tid((long)syscall(SYS_gettid))
    {
        getCurTime(loggingTime, 20);
    }
    /** The logger name. */
    std::string loggerName;
    /** The application supplied message of logging event. */
    const std::string message;
    /** Level of logging event. */
    const uint32_t level; 
    /** Level string of logging event. */
    const std::string levelStr;
    /** File name. */
    const std::string file;
    /** Line count. */
    const int line;
    /** Function name. */
    const std::string function;
    /** Process id */
    const int pid;
    /** Thread id */
    const long tid;
    /** Time yyyy-mm-dd hh:mm:ss*/
    char loggingTime[20];
};
}
#endif

