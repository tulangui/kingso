/**
*@file Appender.h
*@brief the file to declare Appender class and its children class.
*
*@version 1.0.0
*@date 2008.12.19
*@author jinhui.li
*/
#ifndef _ALOG_APPENDER_H_
#define _ALOG_APPENDER_H_

#include <string>
#include <set>
#include <map>
#include <pthread.h>
#include <stdint.h>
#include "Sync.h"
#include "Layout.h"

namespace alog
{
class Mutex;
class LoggingEvent;
/**
*@class Appender
*@brief the class to represent output destination
*
*@version 1.0.0
*@date 2008.12.19
*@author jinhui.li
*@warning
*/
class Appender
{
public:
    /**
    *@brief the actual output function.
    */
    virtual int append(LoggingEvent& event) = 0;
    /**
    *@brief flush message to destination.
    */
    virtual void flush() = 0;
    /**
    *@brief layout set function.
    */
    virtual void setLayout(Layout* layout = NULL);
    /**
    *@brief release all the static Appender object.
    *@warning can only be called in Logger::shutdown() method
    */
    static void release();
    /**
    *@brief check if this appender auto flush.
    */
    bool isAutoFlush();
    /**
    *@brief set method of m_bAutoFlush.
    *@param bAutoFlush is the set value.
    */
    void setAutoFlush(bool bAutoFlush);
protected:
    Appender();
    Appender(const Appender &app) {}
    virtual ~Appender();

    static std::set<Appender *> s_appenders;
    //the lock for append() function
    Mutex m_appendMutex;
    Layout* m_layout;

    bool m_bAutoFlush;
};


/**
*@class ConsoleAppender
*@brief the appender of console type
*
*@version 1.0.0
*@date 2008.12.19
*@author jinhui.li
*@warning
*/
class ConsoleAppender: public Appender
{
public:
    /**
    *@brief the actual output function.
    *@param level log level
    *@param message user log message
    *@param loggerName the logger's name from caller
    */
    virtual int append(LoggingEvent& event);
    /**
    *@brief flush message to destination.
    */
    virtual void flush();
    /**
    *@brief get the static ConsoleAppender pointer.
    *@return the pointer of static ConsoleAppender object
    */
    static Appender *getAppender(); 
    /**
    *@brief release all the static Appender object.
    *@warning can only be called in Logger::shutdown() method
    */
    static void release();

protected:
    ConsoleAppender() {}
    ConsoleAppender(const ConsoleAppender &app) {}
    ~ConsoleAppender() {}
private:
    static Appender *s_appender;
    static Mutex s_appenderMutex;
};


/**
*@class FileAppender
*@brief the appender of file type
*
*@version 1.0.0
*@date 2008.12.19
*@author jinhui.li
*@warning
*/
#define CHUNK 16384

typedef struct parameter
{
    char *fileName;
    uint64_t cacheLimit;
} param;

class FileAppender: public Appender
{
public:
    /**
    *@brief the actual output function.
    *@param level log level
    *@param message user log message
    *@param loggerName the logger's name from caller
    */
    virtual int append(LoggingEvent& event);
    /**
    *@brief flush message to destination.
    */
    virtual void flush();
    /**
    *@brief get the static FileAppender pointer.
    *@return the pointer of static FileAppender object
    */
    static Appender *getAppender(const char *filename);
    /**
    *@brief release all the static Appender object.
    *@warning can only be called in Logger::shutdown() method
    */
    static void release();
    /**
    *@brief set max file size
    */
    void setMaxSize(uint32_t maxSize);
    /**
    *@brief set log file delay time (hour)
    */
    void setDelayTime(uint32_t hour); 
    /**
    *@brief set rotated log file compress flag
    */
    void setCompress(bool bCompress); 
    /**
    *@brief set log file cache limit size
    */
    void setCacheLimit(uint32_t cacheLimit);
    /**
    *@brief set history log file keep count
    */
    void setHistoryLogKeepCount(uint32_t keepCount);
    /**
    *@brief get history log file keep count
    */
    uint32_t getHistoryLogKeepCount() const;
    /**
    *@brief remove history log file beyond 'm_keepFileCount'.
    */
    size_t removeHistoryLogFile(const char *dir);
public:
    static const uint32_t MAX_KEEP_COUNT = 1024; 
protected:
    FileAppender(const char * filename);
    FileAppender(const FileAppender &app) {}
    virtual ~FileAppender();
private:
    int open();
    int close();
    void rotateLog();
    void computeLastLogFileName(char *lastLogFileName, size_t length);
    void compressLog(char *logFileName);
    static void* compressHook(void *p);
    static std::string getParentDir(const std::string &fileName);
    static std::string getPureFileName(const std::string &fileName);
    static void freeNameList(struct dirent **namelist, int32_t entryCount);
        
    FILE* m_file;
    std::string m_fileName;
    std::string m_patentDir;
    std::string m_pureFileName;
    static std::map<std::string, Appender *> s_fileAppenders;
    static Mutex s_appenderMapMutex;
    time_t m_delayTime;
    uint32_t m_nDelayHour;
    uint32_t m_keepFileCount;
    uint64_t m_nCurSize;
    uint64_t m_nMaxSize;
    uint64_t m_nPos;
    uint64_t m_nCacheLimit;
    bool m_bCompress;
};

/** syslog appender, its destination is unix syslog. */
/**
*@class SyslogAppender
*@brief the appender of unix syslog type
*
*@version 1.0.0
*@date 2008.12.19
*@author jinhui.li
*@warning
*/
class SyslogAppender: public Appender
{
public:
    /**
    *@brief the actual output function.
    *@param level log level
    *@param message user log message
    *@param loggerName the logger's name from caller
    */
    virtual int append(LoggingEvent& event);
    /**
    *@brief flush message to destination.
    */
    virtual void flush();
    /**
    *@brief release the static Appender object.
    *
    *@param ident a string that is prepended to every message
    *@param facility encodes  a  default facility to be assigned to all messages that do not have an explicit facility
    *@n already encoded. The initial default facility is LOG_USER
    *@warning MUST support duplicated release operation.
    */
    static Appender *getAppender(const char *ident, int facility); 
    /**
    *@brief release all the static Appender object.
    *@warning can only be called in Logger::shutdown() method
    */
    static void release();

protected:
    SyslogAppender(const char *ident,  int facility);
    SyslogAppender(const SyslogAppender &app) {}
    virtual ~SyslogAppender();
private:
    int open();
    int close();
    /**
     	* @brief Translates a alog level to a syslog priority
     	* @param level The alog level.
     	* @return the syslog priority.
     	**/
    int toSyslogLevel(uint32_t level);

    std::string m_ident;
    int m_facility;

    static std::map<std::string, Appender *> s_syslogAppenders;
    static Mutex s_appenderMapMutex;
};

/** The roll file appender, it will be implemented in the future.
*TODO: to implement

class RollFileAppender: public FileAppender
{

};
*/
}
#endif

