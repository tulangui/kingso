#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <list>
#include <syslog.h>
#include "../include/Configurator.h"
#include "StringUtil.h"
#include "Properties.h"

namespace alog {

typedef std::map<std::string, Appender*> AppenderMap;

void Configurator::configureLogger(const char *initFileName) throw (std::exception)
{
    std::ifstream initFile(initFileName);
    if (!initFile)
    {
        throw std::runtime_error(std::string("Config File ") + initFileName + " does not exist or is unreadable");
    }

    Properties properties;
    // parse the file to get all of the configuration
    properties.load(initFile);
    
    // global init
    globalInit(properties);
    
    AppenderMap allAppenders;
    initAllAppenders(allAppenders, properties);

    // get loggers
    std::vector<std::string> loggerList;
    initLoggers(loggerList, properties);

    // configure each logger
    for (std::vector<std::string>::const_iterator iter = loggerList.begin(); iter != loggerList.end(); ++iter)
    {
        configureLogger(*iter, allAppenders, properties);
    }
}


void Configurator::configureRootLogger()
{
    Logger* root = Logger::getRootLogger();
    root->setLevel(LOG_LEVEL_INFO);
    root->setAppender(ConsoleAppender::getAppender());
}

void Configurator::globalInit(Properties &properties)
{
    int value = properties.getInt(std::string("max_msg_len"), 1024);
    if(value > 0)
        Logger::MAX_MESSAGE_LENGTH = value;
}

void Configurator::initAllAppenders(AppenderMap &allAppenders, Properties &properties) throw(std::exception)
{
    std::string currentAppender;

    //find all the "appender.XXX" key in properties map.
    std::string prefix("appender");
    Properties::const_iterator from = properties.lower_bound(prefix + '.');
    Properties::const_iterator to = properties.lower_bound(prefix + (char)('.' + 1));
    for (Properties::const_iterator i = from; i != to; ++i)
    {
        const std::string& key = (*i).first;
        std::list<std::string> propNameParts;
        std::back_insert_iterator<std::list<std::string> > pnpIt(propNameParts);
        StringUtil::split(pnpIt, key, '.');
        std::list<std::string>::const_iterator i2 = propNameParts.begin();
        std::list<std::string>::const_iterator iEnd = propNameParts.end();
        if (++i2 == iEnd)
        {
            throw std::runtime_error(std::string("missing appender name"));
        }

        const std::string appenderName = *i2++;

        /* WARNING, approaching lame code section:
           skipping of the Appenders properties only to get them
           again in instantiateAppender.
           */
        if (appenderName == currentAppender)
        {
            // simply skip properties for the current appender
        }
        else
        {
            if (i2 == iEnd)
            {
                // a new appender
                currentAppender = appenderName;
                allAppenders[currentAppender] = instantiateAppender(currentAppender,properties);
            }
            else
            {
                throw std::runtime_error(std::string("partial appender definition : ") + key);
            }
        }
    }
}

void Configurator::configureLogger(const std::string& loggerName, AppenderMap &allAppenders, Properties &properties) throw (std::exception)
{
    // start by reading the "rootLogger" key
    std::string tempLoggerName =
        (loggerName == "rootLogger") ? loggerName : "logger." + loggerName;

    Properties::iterator iter = properties.find(tempLoggerName);

    if (iter == properties.end())
    {
        if (tempLoggerName == "rootLogger")
        {
            Logger::getRootLogger();
            return;
        }
        throw std::runtime_error(std::string("Unable to find logger: ") + tempLoggerName);
    }

    // need to get the root instance of the logger
    Logger* logger = (loggerName == "rootLogger") ?
                     Logger::getRootLogger() : Logger::getLogger(loggerName.c_str());


    std::list<std::string> tokens;
    std::back_insert_iterator<std::list<std::string> > tokIt(tokens);
    StringUtil::split(tokIt, (*iter).second, ',');
    std::list<std::string>::const_iterator i = tokens.begin();
    std::list<std::string>::const_iterator iEnd = tokens.end();

    uint32_t level = LOG_LEVEL_NOTSET;
    if (i != iEnd)
    {
        std::string  levelStr = StringUtil::trim(*i++);
        if (levelStr != "")
        {
            level = getLevelByString(levelStr);
            if (level >= LOG_LEVEL_NOTSET)
            {
                //level = LOG_LEVEL_INFO;
                i--;
                throw std::runtime_error("Unknow level  for logger '" + loggerName + "'");
            }
        }
    }
    logger->setLevel(level);

    bool inherit = properties.getBool("inherit." + loggerName, true);
    logger->setInheritFlag(inherit);

    logger->removeAllAppenders();
    for (/**/; i != iEnd; ++i)
    {
        std::string appenderName = StringUtil::trim(*i);
        AppenderMap::const_iterator appIt = allAppenders.find(appenderName);
        if (appIt == allAppenders.end())
        {
            // appender not found;
            throw std::runtime_error(std::string("Appender '") +
                                     appenderName + "' not found for logger '" + loggerName + "'");
        }
        else
        {
            /* pass by reference, i.e. don't transfer ownership
            */
            logger->addAppender(((*appIt).second));
        }
    }
}

Appender* Configurator::instantiateAppender(const std::string& appenderName, Properties &properties) throw (std::exception)
{
    Appender* appender = NULL;
    std::string appenderPrefix = std::string("appender.") + appenderName;

    // determine the type by the appenderName
    Properties::iterator key = properties.find(appenderPrefix);
    if (key == properties.end())
        throw std::runtime_error(std::string("Appender '") + appenderName + "' not defined");

    std::string::size_type length = (*key).second.find_last_of(".");
    std::string appenderType = (length == std::string::npos) ?
                               (*key).second : (*key).second.substr(length+1);

    // and instantiate the appropriate object
    if (appenderType == "ConsoleAppender") {
        appender = ConsoleAppender::getAppender();
        bool isFlush = properties.getBool(appenderPrefix + ".flush", false);
        appender->setAutoFlush(isFlush);
    }
    else if (appenderType == "FileAppender") {
        std::string fileName = properties.getString(appenderPrefix + ".fileName", "default.log");
        appender = FileAppender::getAppender(fileName.c_str());
        bool isFlush = properties.getBool(appenderPrefix + ".flush", false);
        appender->setAutoFlush(isFlush);
	FileAppender *fileAppender = (FileAppender *)appender;
        uint32_t maxFileSize = properties.getInt(appenderPrefix + ".max_file_size", 0);
        fileAppender->setMaxSize(maxFileSize); 
        uint32_t delayHour = properties.getInt(appenderPrefix + ".delay_time", 0);
        fileAppender->setDelayTime(delayHour); 
        bool isCompress = properties.getBool(appenderPrefix + ".compress", false);
        fileAppender->setCompress(isCompress); 
        uint32_t cacheLimit = properties.getInt(appenderPrefix + ".cache_limit", 0);
        fileAppender->setCacheLimit(cacheLimit); 
        uint32_t keepCount = properties.getInt(appenderPrefix + ".log_keep_count", 0);
        fileAppender->setHistoryLogKeepCount(keepCount); 
    }
    else if (appenderType == "SyslogAppender") {
        std::string ident = properties.getString(appenderPrefix + ".ident", "syslog");
        int facility = properties.getInt(appenderPrefix + ".facility", LOG_USER >> 3) << 3;
        appender = SyslogAppender::getAppender(ident.c_str(), facility);
    }
    else {
        throw std::runtime_error(std::string("Appender '") + appenderName +
                                 "' has unknown type '" + appenderType + "'");
    }
    setLayout(appender, appenderName, properties);

    return appender;
}

void Configurator::setLayout(Appender* appender, const std::string& appenderName, Properties &properties) 
{
    // determine the type by appenderName
    std::string tempString;
    Properties::iterator key = properties.find(std::string("appender.") + appenderName + ".layout");

    if (key == properties.end())
        return;
		
    std::string::size_type length = (*key).second.find_last_of(".");
    std::string layoutType = (length == std::string::npos) ? (*key).second : (*key).second.substr(length+1);
 
    Layout* layout;
    // and instantiate the appropriate object
    if (layoutType == "BasicLayout") 
    {
        layout = new BasicLayout();
    }
    else if (layoutType == "SimpleLayout") 
    {
        layout = new SimpleLayout();
    }
    else if (layoutType == "PatternLayout") 
    {
        PatternLayout* patternLayout = new PatternLayout();
        key = properties.find(std::string("appender.") + appenderName + ".layout.LogPattern");
        if (key == properties.end()) 
        {
            // leave default pattern
        } 
        else 
        {
            patternLayout->setLogPattern((*key).second);
        }
        layout = patternLayout;
    }
    else 
    {
        throw std::runtime_error(std::string("Unknown layout type '" + layoutType + "' for appender '") + appenderName + "'");
    }

    appender->setLayout(layout);
}

/**
 * Get the categories contained within the map of properties.  Since
 * the logger looks something like "logger.xxxxx.yyy.zzz", we need
 * to search the entire map to figure out which properties are logger
 * listings.  Seems like there might be a more elegant solution.
 */
void Configurator::initLoggers(std::vector<std::string>& loggers, Properties &properties)
{
    loggers.clear();

    // add the root logger first
    loggers.push_back(std::string("rootLogger"));


    // then look for "logger."
    std::string prefix("logger");
    Properties::const_iterator from = properties.lower_bound(prefix + '.');
    Properties::const_iterator to = properties.lower_bound(prefix + (char)('.' + 1));
    for (Properties::const_iterator iter = from; iter != to; iter++) {
        loggers.push_back((*iter).first.substr(prefix.size() + 1));
    }
}

uint32_t Configurator::getLevelByString(const std::string &levelStr)
{
    uint32_t level;
    if (levelStr == "DISABLE")
        level = LOG_LEVEL_DISABLE;
	else if (levelStr == "FATAL")
        level = LOG_LEVEL_FATAL;
    else if (levelStr == "ERROR")
        level = LOG_LEVEL_ERROR; 
    else if (levelStr == "WARN")
        level = LOG_LEVEL_WARN;
    else if (levelStr == "INFO")
        level = LOG_LEVEL_INFO;
    else if (levelStr == "DEBUG")
        level = LOG_LEVEL_DEBUG;
    else if (levelStr == "TRACE1")
        level = LOG_LEVEL_TRACE1;
    else if (levelStr == "TRACE2")
        level = LOG_LEVEL_TRACE2;
    else if (levelStr == "TRACE3")
        level = LOG_LEVEL_TRACE3;
    else
        level = LOG_LEVEL_NOTSET;

    return level;

}
}



