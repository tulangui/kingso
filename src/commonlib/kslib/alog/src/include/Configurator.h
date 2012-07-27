/**
 *@file Configurator.h
 *@brief the file to declare Configurator class.
 *
 *@version 1.0.0
 *@date 2008.12.22
 *@author Bingbing Yang
 */
#ifndef _ALOG_CONFIGURATOR_H
#define _ALOG_CONFIGURATOR_H

#include <iostream>
#include <string>
#include <stdexcept>
#include <map>
#include <vector>
#include "Logger.h"
#include "Appender.h"

namespace alog {
class Properties;
/**
 *@class Configurator
 *@brief the class to configure loggers by file and rootLogger basicly.
 *
 *@version 1.0.0
 *@date 2008.12.22
 *@author Bingbing Yang
 *@warning
 */
class Configurator
{
public:
    typedef std::map<std::string, Appender*> AppenderMap;
    /**
     * @brief configure loggers by file.
     * @param conf file path.
     **/
    static void configureLogger(const char* initFileName) throw (std::exception);
    /**
     * @brief configure root logger basicly.
     **/
    static void configureRootLogger();

private:
    static void globalInit(Properties &properties);
    static void initLoggers(std::vector<std::string>& loggers, Properties &properties);
    static void configureLogger(const std::string& loggerName, AppenderMap &allAppenders, Properties &properties) throw (std::exception);
    static void initAllAppenders(AppenderMap &allAppenders, Properties &properties) throw(std::exception);
    static Appender* instantiateAppender(const std::string& name, Properties &properties) throw(std::exception);
    static void setLayout(Appender* appender, const std::string& appenderName, Properties &properties);
    static uint32_t getLevelByString(const std::string &levelStr);
};
}

#endif
