#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "util/Log.h"



void alog_log(const char *logger, const uint32_t level, const char* fmt, ...)
{
    char buffer[alog::Logger::MAX_MESSAGE_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, alog::Logger::MAX_MESSAGE_LENGTH, fmt, ap);
    va_end(ap);

    if(strcmp(logger, "") == 0)
    {
        alog::Logger::getRootLogger()->log(level, buffer);
    }
    else
    {
        alog::Logger::getLogger(logger)->log(level, buffer);
    } 
}

alog_fun G_LOG = alog_log;

