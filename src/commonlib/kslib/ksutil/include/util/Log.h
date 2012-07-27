#ifndef _UTIL_LOG_H_
#define _UTIL_LOG_H_

#include <stdio.h>
#include "alog/Configurator.h"
#include "alog/Logger.h"
#include "alog/Layout.h"
    
#ifdef __cplusplus
extern "C" {
    
#endif                            //__cplusplus
    typedef void (*alog_fun) (const char *, const uint32_t, const char *,
                              ...);
    void alog_log(const char *logger, const uint32_t level,
                     const char *fmt, ...);
    extern alog_fun G_LOG;
     
#ifdef __cplusplus
} 
#endif                            //__cplusplus
 
//#define TLOG_SETUP(file) alog::Configurator::configureLogger(file)
#define TLOG(fmt, args...) \
    alog::Logger::getRootLogger()->log(alog::LOG_LEVEL_INFO, \
            __FILE__, __LINE__, __func__, fmt, ##args); \
    alog::Logger::getRootLogger()->flush()

#define TNOTE(fmt, args...) \
    alog::Logger::getRootLogger()->log(alog::LOG_LEVEL_INFO, \
            __FILE__, __LINE__, __func__, fmt, ##args); \
    alog::Logger::getRootLogger()->flush()

#define TERR(fmt, args...) \
    alog::Logger::getRootLogger()->log(alog::LOG_LEVEL_ERROR, \
            __FILE__, __LINE__, __func__, fmt, ##args)

#define TWARN(fmt, args...) \
    alog::Logger::getRootLogger()->log(alog::LOG_LEVEL_WARN, \
            __FILE__, __LINE__, __func__, fmt, ##args)

#define TACCESS(fmt, args...) \
    alog::Logger::getLogger("access")->log(alog::LOG_LEVEL_INFO, \
            fmt, ##args)

#define TACCESS_NOISY(fmt, args...) \
    alog::Logger::getLogger("access")->log(alog::LOG_LEVEL_TRACE1, \
            fmt, ##args)
 
#ifdef DEBUG
#define TDEBUG(fmt, args...) \
    alog::Logger::getRootLogger()->log(alog::LOG_LEVEL_DEBUG, \
            __FILE__, __LINE__, __func__, fmt, ##args)
 
#else    /*  */
 
#define TDEBUG(fmt, args...) void(0)
 
#endif    /*  */
 
#endif                            //_UTIL_LOG_H_
