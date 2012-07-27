#include "../include/Logger.h"
#include "StringUtil.h"
#include "LoggingEvent.h"
#include <assert.h>

namespace alog {

//static member init
Mutex Logger::s_rootMutex;
Mutex Logger::s_loggerMapMutex;
Logger* Logger :: s_rootLogger = NULL;
std::map<std::string, Logger *>* Logger::s_loggerMap = NULL;
int Logger::MAX_MESSAGE_LENGTH = 1024;

Logger::Logger(const char *name, uint32_t level, Logger *parent, bool bInherit ) :
    m_loggerName(name), m_loggerLevel(level), m_bLevelSet(false), m_parent(parent), m_bInherit(bInherit)
{}

Logger* Logger::getRootLogger()
{
    ScopedLock lock(s_rootMutex);
    if (s_rootLogger == NULL)
    {
        s_rootLogger = new Logger("", LOG_LEVEL_INFO);
    }
    return s_rootLogger;
}

Logger* Logger::getLogger(const char *loggerName, bool bInherit )
{
    ScopedLock lock(s_loggerMapMutex);
    if (NULL == s_loggerMap) {
        s_loggerMap = new std::map<std::string, Logger *>();
    }
    return _getLogger(loggerName, bInherit);
}

void Logger::flushAll()
{
    if(NULL != s_loggerMap) {
        ScopedLock lock(s_loggerMapMutex);
        for (std::map<std::string, Logger *>::iterator it = s_loggerMap->begin(); 
             it != s_loggerMap->end(); it++)
        {
            it->second->flush();
        }
    }

    {
        ScopedLock lock2(s_rootMutex);
        if(s_rootLogger) {
            s_rootLogger->flush();
        }
    }
}

void Logger::shutdown()
{
    //remove the root logger
    s_rootMutex.lock();
    {
        if (s_rootLogger != NULL)
        {
            delete s_rootLogger;
            s_rootLogger = NULL;
        }
    }
    s_rootMutex.unlock();

    //remove all the none root logger
    if (NULL != s_loggerMap) {
        ScopedLock lock(s_loggerMapMutex);
        for (std::map<std::string, Logger *>::const_iterator i = s_loggerMap->begin(); 
             i != s_loggerMap->end(); i++)
        {
            delete ((*i).second);
        }
        s_loggerMap->clear();
        delete s_loggerMap;
        s_loggerMap = NULL;
    }

    //release all the appenders
    Appender::release();
}

Logger::~Logger()
{
    removeAllAppenders();
    m_children.clear();
}

const std::string& Logger::getName() const
{
    return m_loggerName;
}

uint32_t Logger::getLevel() const
{
    return m_loggerLevel;
}

void Logger::setLevel(uint32_t level)
{
    if(level >= LOG_LEVEL_COUNT)
        return;
    m_loggerLevel = (level == LOG_LEVEL_NOTSET && m_bInherit && m_parent)? m_parent->getLevel() : level;
    if(m_loggerLevel == LOG_LEVEL_NOTSET)
        m_loggerLevel = LOG_LEVEL_INFO;
    m_bLevelSet = (level == LOG_LEVEL_NOTSET)? false : true;
    size_t childCnt = m_children.size();
    for(size_t i = 0; i < childCnt; i++) {
        m_children[i]->setLevelByParent(m_loggerLevel);
    }
}

void Logger::setLevelByParent(uint32_t level)
{
    if(level >= LOG_LEVEL_COUNT)
        return;
    if(!m_bInherit || m_bLevelSet)
        return;
    m_loggerLevel = (level == LOG_LEVEL_NOTSET)? LOG_LEVEL_INFO : level;
    size_t childCnt = m_children.size();
    for(size_t i = 0; i < childCnt; i++) {
        m_children[i]->setLevelByParent(m_loggerLevel);
    }
}

bool Logger::getInheritFlag() const
{
    return m_bInherit;
}

void Logger::setInheritFlag(bool bInherit)
{
    m_bInherit = bInherit;
    if(!m_bLevelSet && !m_bInherit)
    {
        m_loggerLevel = LOG_LEVEL_INFO;
        size_t childCnt = m_children.size();
        for(size_t i = 0; i < childCnt; i++) {
            m_children[i]->setLevelByParent(m_loggerLevel);
        }
    }
    if(!m_bLevelSet && m_bInherit)
    {
        m_loggerLevel = m_parent->getLevel();
        size_t childCnt = m_children.size();
        for(size_t i = 0; i < childCnt; i++) {
            m_children[i]->setLevelByParent(m_loggerLevel);
        }
    }
}

void Logger::setAppender(Appender* appender)
{
    if (appender)
    {
        removeAllAppenders() ;
        addAppender(appender);
    }
}

void Logger::addAppender(Appender* appender)
{
    if (appender)
    {
        {
            ScopedLock lock(m_appenderMutex);
            std::set<Appender *>::iterator i = m_appenderSet.find(appender);
            if (m_appenderSet.end() == i)
            {
                m_appenderSet.insert(appender);
            }
        }
    }
}

void Logger::removeAllAppenders()
{
    {
        ScopedLock lock(m_appenderMutex);
        m_appenderSet.clear();
    }
}

void Logger::log(uint32_t level, const char * file, int line, const char * func, const char* fmt, ...)
{
    if (__builtin_expect((!isLevelEnabled(level)),1))
        return;

    char buffer[MAX_MESSAGE_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, MAX_MESSAGE_LENGTH, fmt, ap);
    va_end(ap);
    std::string msg(buffer);
    LoggingEvent event(m_loggerName, msg, level, std::string(file), line, std::string(func));
    
    _log(event);
}

void Logger::log(uint32_t level, const char* fmt, ...)
{
    if (__builtin_expect((!isLevelEnabled(level)),1))
        return;

    char buffer[MAX_MESSAGE_LENGTH];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, MAX_MESSAGE_LENGTH, fmt, ap);
    va_end(ap);
    std::string msg(buffer);
    LoggingEvent event(m_loggerName, msg, level);

    _log(event);
}

void Logger::logPureMessage(uint32_t level, const char *message)
{
    if (__builtin_expect((!isLevelEnabled(level)),1))
        return;	
    std::string msg(message);
    LoggingEvent event(m_loggerName, msg, level);

    _log(event);
}

void Logger::flush()
{
    {
        ScopedLock lock(m_appenderMutex);
        if (!m_appenderSet.empty())
        {
            for (std::set<Appender*>::const_iterator i = m_appenderSet.begin();
                 i != m_appenderSet.end(); i++)
            {
                (*i)->flush();
            }
        }
    }
}

/////////////////////private funtion///////////////////////////////////////////////////////////
void Logger::_log(LoggingEvent& event)
{
    if (m_bInherit && m_parent != NULL)
        m_parent->_log(event);
    
    if(event.loggerName.size() == 0)
        event.loggerName = m_loggerName;
    {
        ScopedLock lock(m_appenderMutex);
        if (!m_appenderSet.empty())
        {
            for (std::set<Appender*>::const_iterator i = m_appenderSet.begin();
                 i != m_appenderSet.end(); i++)
            {
                (*i)->append(event);
            }
        }
    }

}

Logger* Logger::_getLogger(const char *loggerName, bool bInherit)
{
    Logger* result = NULL;
    std::string name = loggerName;
    assert(s_loggerMap);
    std::map<std::string, Logger *>::iterator i = s_loggerMap->find(name);
    if (s_loggerMap->end() != i)
    {
        result = (*i).second;
    }
    if (NULL == result)
    {
        std::string parentName;
        size_t dotIndex = name.find_last_of('.');
        Logger* parent = NULL;
        if (dotIndex == std::string::npos)
        {
            //the topest logger
            parent = getRootLogger();
        }
        else
        {
            //recursive get parent logger
            parentName = name.substr(0, dotIndex);
            parent = _getLogger(parentName.c_str());
        }
        result = new Logger(name.c_str(), parent->getLevel(), parent, bInherit);
        parent->m_children.push_back(result);
        s_loggerMap->insert(make_pair(name, result));
    }
    return result;
}
    
}

