#include <time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <syslog.h>
#include <stdlib.h>
#include <algorithm>
#include <zlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <string>
#include <dirent.h>
#include <assert.h>
#include "../include/Appender.h"
#include "../include/Logger.h"
#include "LoggingEvent.h"

namespace alog {

using namespace std;

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// Appender
///////////////////////////////////////////////////////////////////////////////////////////////////////////
std::set<Appender *> Appender :: s_appenders;

Appender::Appender()
{
    m_layout = new BasicLayout();
    m_bAutoFlush = false;
}

Appender::~Appender()
{
    if(m_layout)
        delete m_layout;
}

void Appender::setLayout(Layout* layout)
{
    if (layout != m_layout) 
    {
        Layout *oldLayout = m_layout;
        m_layout = (layout == NULL) ? new BasicLayout() : layout;
        if(oldLayout)
            delete oldLayout;
    }
}

void Appender::release()
{
    for (std::set<Appender *>::iterator i = s_appenders.begin(); i != s_appenders.end(); i++)
    {
        if (NULL != *i)
            delete (*i);
    }
    s_appenders.clear();
    ConsoleAppender::release();
    FileAppender::release();
    SyslogAppender::release();
}

bool Appender::isAutoFlush()
{
    return m_bAutoFlush;
}

void Appender::setAutoFlush(bool bAutoFlush)
{
    m_bAutoFlush = bAutoFlush;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
// ConsoleAppender
///////////////////////////////////////////////////////////////////////////////////////////////////////////
Appender* ConsoleAppender::s_appender = NULL;
Mutex ConsoleAppender::s_appenderMutex;

Appender* ConsoleAppender::getAppender()
{
    ScopedLock lock(s_appenderMutex);
    if (!s_appender)
    {
        s_appender = new ConsoleAppender();
        s_appenders.insert(s_appender);
    }
    return s_appender;
}

int ConsoleAppender::append(LoggingEvent& event)
{
    if(m_layout == NULL)
    	m_layout = new BasicLayout();
    std::string formatedStr = m_layout->format(event);
    size_t wCount = 0;
    {
        ScopedLock lock(m_appendMutex);
        wCount = fwrite(formatedStr.data(), 1, formatedStr.length(), stdout);
        if (m_bAutoFlush || event.level < LOG_LEVEL_INFO)
            fflush(stdout);
    }
    return (wCount == formatedStr.length())? formatedStr.length() : 0;
}

void ConsoleAppender::flush()
{
    fflush(stdout);
}

void ConsoleAppender::release()
{
    s_appender = NULL;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
// FileAppender
///////////////////////////////////////////////////////////////////////////////////////////////////////////
std::map<std::string, Appender *>  FileAppender::s_fileAppenders;
Mutex FileAppender::s_appenderMapMutex;
const int32_t FILE_NAME_MAX_SIZE = 511;
const char DIR_DELIM = '/';

FileAppender::FileAppender(const char * fileName) : m_fileName(fileName)
{
    m_delayTime = 0;
    m_nDelayHour = 0;
    m_keepFileCount = 0;
    m_nMaxSize = 0;
    m_nCurSize = 0;
    m_bCompress = false;
    m_file = NULL;
    m_nPos = 0;
    m_nCacheLimit = 0;
    m_patentDir = getParentDir(m_fileName);
    m_pureFileName = getPureFileName(m_fileName);
    open();
}

FileAppender::~FileAppender()
{
    if (m_file)
        close();
    m_file = NULL;
}

Appender* FileAppender::getAppender(const char *filename)
{
    std::string file = filename;
    ScopedLock lock(s_appenderMapMutex);
    std::map<std::string, Appender *>::iterator i = s_fileAppenders.find(file);
    if (s_fileAppenders.end() == i)
    {
        Appender *newAppender = new FileAppender(filename);
        s_fileAppenders[file] = newAppender;
        s_appenders.insert(newAppender);
        return newAppender;
    }
    else
    {
        return (*i).second;
    }
}

int FileAppender::append(LoggingEvent& event)
{
    if(!m_file)
        return 0;
    if(m_layout == NULL)
        m_layout = new BasicLayout();
    std::string formatedStr = m_layout->format(event); 
 
    size_t wCount = 0;
    { 
        ScopedLock lock(m_appendMutex);
        if((m_nMaxSize > 0 && m_nCurSize + formatedStr.length() >= m_nMaxSize) 
	   || (m_delayTime > 0 && time(NULL) >= m_delayTime))
        {
            rotateLog();
        }
        wCount = fwrite(formatedStr.data(), 1, formatedStr.length(), m_file);
        if (m_bAutoFlush || m_nMaxSize > 0 || event.level < LOG_LEVEL_INFO)
            fflush(m_file);
        m_nCurSize += wCount;
        /* fix bug #169781
         * don't evict pagecache until compress log
         */
        if (!m_bCompress)
        {
            if(m_nCacheLimit > 0 && (m_nCurSize - m_nPos) > m_nCacheLimit)
            {
                posix_fadvise(fileno(m_file), 0, 0, POSIX_FADV_DONTNEED);
                m_nPos = m_nCurSize;
            }
        }
    }
    return (wCount == formatedStr.length())? formatedStr.length() : 0;
}

void FileAppender::flush()
{
    if(m_file)
        fflush(m_file);
}

int FileAppender :: open()
{
    m_file = fopen64(m_fileName.c_str(), "a+");
    if (m_file)
    {
        struct stat64 stbuf;
        if (fstat64(fileno(m_file), &stbuf) == 0)
        {
            m_nCurSize = stbuf.st_size;
            m_nPos = 0;
        }
        return 1;
    }
    else
    {
        fprintf(stderr, "Open log file error : %s\n",  m_fileName.c_str());
        return 0;
    }
}

void FileAppender::release()
{
    s_fileAppenders.clear();
}

int FileAppender :: close()
{
    int ret = -1;
    if(m_file)
        ret = fclose(m_file);
    m_file = NULL;
    return ret;
}

void FileAppender :: rotateLog()
{
    /* fix bug #169781
     * dont' evict page cache until rotateLog complete to avoid disk read
    if(m_nCacheLimit > 0)
        posix_fadvise(fileno(m_file), 0, 0, POSIX_FADV_DONTNEED);
    */
    close();

    char *lastLogFileName = new char[FILE_NAME_MAX_SIZE + 1];
    computeLastLogFileName(lastLogFileName, FILE_NAME_MAX_SIZE);
    rename(m_fileName.c_str(), lastLogFileName);

    if(m_delayTime > 0 && time(NULL) >= m_delayTime) {
        setDelayTime(m_nDelayHour);
    }
    if (m_keepFileCount > 0) {
        removeHistoryLogFile(m_patentDir.c_str());
    }
    if(m_bCompress) {
        compressLog(lastLogFileName);
    } 
    else {
        if(m_nCacheLimit > 0) {
            int fd = ::open(lastLogFileName, O_LARGEFILE | O_RDONLY, 0644);
            if(fd == -1)
            {
                fprintf(stderr, "Open %s error: %s!\n", lastLogFileName, strerror(errno));
            }
            else
            {
                if(posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) != 0)
                    fprintf(stderr, "Clear cache %s error: %s!\n", lastLogFileName, strerror(errno));
                ::close(fd);
            }
        }
        delete lastLogFileName;
    }
    open();
}

void FileAppender :: computeLastLogFileName(char *lastLogFileName, 
					    size_t length) 
{
    char oldLogFile[FILE_NAME_MAX_SIZE + 1];
    oldLogFile[FILE_NAME_MAX_SIZE] = '\0';
    char oldZipFile[FILE_NAME_MAX_SIZE + 1];
    oldZipFile[FILE_NAME_MAX_SIZE] = '\0';

    time_t t; 
    time(&t);		
    struct tm tim;
    ::localtime_r((const time_t*)&t, &tim);		

    snprintf(oldLogFile, FILE_NAME_MAX_SIZE, "%s.%04d%02d%02d%02d%02d%02d", 
	     m_fileName.c_str(), tim.tm_year+1900, tim.tm_mon+1, tim.tm_mday, 
	     tim.tm_hour, tim.tm_min, tim.tm_sec);
    int32_t suffixStartPos = strlen(oldLogFile);
    int32_t suffixCount = 1;
    do {
        snprintf(oldLogFile + suffixStartPos, FILE_NAME_MAX_SIZE - suffixStartPos, 
		 "-%03d", suffixCount);
        snprintf(oldZipFile, FILE_NAME_MAX_SIZE, "%s.gz", oldLogFile);
        suffixCount++;
    } while(access(oldLogFile, R_OK) == 0 || access(oldZipFile, R_OK) == 0);
    
    snprintf(lastLogFileName, length, "%s", oldLogFile);
}

size_t FileAppender :: removeHistoryLogFile(const char *dirName) 
{
    size_t removedFileCount = 0;
    struct dirent **namelist = NULL;
    int32_t entryCount = scandir(dirName, &namelist, NULL, alphasort);
    if (entryCount < 0) {
        fprintf(stderr, "scandir error, errno:%d!\n", errno);
	return 0 ;
    } else if (entryCount <= (int32_t)m_keepFileCount) {
        freeNameList(namelist, entryCount);
	return 0;
    }

    //filter by entry's name and entry's st_mode, remain all history log file
    std::vector<std::string> filteredEntrys; 
    size_t prefixLen = m_pureFileName.size();
    for (int entryIndex = 0; entryIndex < entryCount; entryIndex++) {
      const struct dirent *logEntry = namelist[entryIndex];
      assert(logEntry);
      if (0 == strncmp(m_pureFileName.c_str(), logEntry->d_name, prefixLen)
	  && (size_t)strlen(logEntry->d_name) > prefixLen)
      {
	    std::string logFileName = std::string(dirName) + std::string("/") + std::string(logEntry->d_name);
	    struct stat st;
	    errno = 0;
	    int statRet = lstat(logFileName.c_str(), &st);
	    //	    fprintf(stdout, "statRet:%d, S_ISREG(st.st_mode):%d, errno:%d\n", 
	    //		    statRet, S_ISREG(st.st_mode), errno);
	    if (!statRet && S_ISREG(st.st_mode)) {
	        //correct log file
	        filteredEntrys.push_back(logEntry->d_name);
	    }
	}
    }

    //fprintf(stdout, "filteredEntrys.size():%zd\n", filteredEntrys.size());
    if (filteredEntrys.size() > m_keepFileCount) 
    {
        sort(filteredEntrys.begin(), filteredEntrys.end()); //sort by string's lexical order
	removedFileCount = filteredEntrys.size() - m_keepFileCount;
	filteredEntrys.erase(filteredEntrys.begin() + removedFileCount, filteredEntrys.end());

        //actually remove the history log file beyond the limit of 'm_keepFileCount'
	for (std::vector<std::string>::const_iterator it = filteredEntrys.begin();
	     it != filteredEntrys.end(); ++it) 
	{
	    std::string fileName = std::string(dirName) + "/" + it->c_str();
	    unlink(fileName.c_str());
	}
    }
    
    freeNameList(namelist, entryCount);
    return removedFileCount;
}

void FileAppender::freeNameList(struct dirent **namelist, int32_t entryCount)
{
    if(namelist == NULL) {
      return;
    }
    //delete the dirent
    for (int32_t i = 0; i < entryCount; i++) {
        free(namelist[i]);
    }
    free(namelist);
}


void FileAppender :: setMaxSize(uint32_t maxSize)
{
    m_nMaxSize = (uint64_t)maxSize * 1024 * 1024;
}

void FileAppender :: setCacheLimit(uint32_t cacheLimit)
{
    m_nCacheLimit = (uint64_t)cacheLimit * 1024 * 1024;
}

void FileAppender :: setHistoryLogKeepCount(uint32_t keepCount)
{
  if (keepCount > MAX_KEEP_COUNT) {
      m_keepFileCount = MAX_KEEP_COUNT;
      fprintf(stderr, "keepCount[%d] is beyond MAX_KEEP_COUNT:%d\n", 
	      keepCount, MAX_KEEP_COUNT);
  } else {
      m_keepFileCount = keepCount;
  }
}

uint32_t FileAppender :: getHistoryLogKeepCount() const 
{
    return m_keepFileCount;
}

string FileAppender::getParentDir(const string &fileName)
{
  if(fileName.empty()) {
    return std::string(".");
  }     

  size_t delimPos = fileName.rfind(DIR_DELIM);
  if(std::string::npos == delimPos) {
    return std::string(".");
  } else {
    return fileName.substr(0, delimPos + 1);
  }
}

string FileAppender :: getPureFileName(const string &fileName)
{
  size_t delimPos = fileName.rfind(DIR_DELIM);
  if(std::string::npos == delimPos) {
    return fileName;
  } else {
    return fileName.substr(delimPos + 1);
  }  
}

void FileAppender :: setDelayTime(uint32_t hour)
{
    m_nDelayHour = hour;
    if(hour > 0)
        m_delayTime = time(NULL) + hour * 3600;
    else
        m_delayTime = 0;
}

void FileAppender :: setCompress(bool bCompress)
{
    m_bCompress = bCompress;
}

void FileAppender :: compressLog(char *logFileName)
{
    param *p = new param();
    p->fileName = logFileName;
    p->cacheLimit = m_nCacheLimit;

    pthread_t pThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&pThread, &attr, FileAppender::compressHook, (void*)p);  
}

void* FileAppender :: compressHook(void *p)
{
    char* fileName = ((param*)p)->fileName;
    uint64_t cacheLimit = ((param*)p)->cacheLimit;
    int source;
    gzFile gzFile;
    struct stat64 stbuf;
    char *in = NULL;
    void *tmp = NULL;
    char gzFileName[512];
    ssize_t leftSize = 0;
    sprintf(gzFileName, "%s.gz",fileName);
    int flags = O_LARGEFILE;
    flags = flags | O_RDONLY;
	/* fix bug #169781
	 * dont't do direct io because it affects latency
    if(cacheLimit > 0)
        flags = flags | O_DIRECT;
	*/
    source = ::open(fileName, flags, 0644);
    if(source == -1)
    {
        fprintf(stderr, "Compress log file %s error: open file failed!\n", fileName);
        delete fileName;
        delete (param*)p;
        return NULL;
    }
    if (fstat64(source, &stbuf) != 0)
    {
        fprintf(stderr, "Compress log file %s error: access file info error!\n", fileName);
        ::close(source);
        delete fileName;
        delete (param*)p;
        return NULL;
    }
    leftSize = stbuf.st_size;
    gzFile = gzopen(gzFileName, "wb"); 
    if(!gzFile)
    {
        fprintf(stderr, "Compress log file %s error: open compressed file failed!\n", gzFileName); 
        ::close(source);
        delete fileName;
        delete (param*)p;
        return NULL;
    }
    if(cacheLimit > 0)
    {
        if(posix_memalign(&tmp, 512, CHUNK) != 0)
        {
            fprintf(stderr, "posix_memalignes error: %s\n", strerror(errno));
            ::close(source);
            delete fileName;
            delete (param*)p;
            return NULL;
        }
        in = static_cast<char *>(tmp);
    }
    else
    {
        tmp = (void *)malloc(CHUNK);
        if(tmp == NULL)
        {
            fprintf(stderr, "malloc mem error: %s\n", strerror(errno));
            ::close(source);
            delete fileName;
            delete (param*)p;
            return NULL;
        }
        in = static_cast<char *>(tmp);
    }
    while(leftSize > 0)
    {
        ssize_t readLen = ::read(source, in, CHUNK);
        if(readLen < 0)
        {
            fprintf(stderr, "Read log file %s error: %s\n", fileName, strerror(errno));
            break;
        }
        ssize_t writeLen = gzwrite(gzFile, in, readLen);
        if(readLen != writeLen)
        {
            fprintf(stderr, "Compress log file %s error: gzwrite error!\n", gzFileName);
            break;
        }
        if(leftSize < writeLen)
            leftSize = 0;
        else
            leftSize -= writeLen;
    }
    ::close(source);
    gzclose(gzFile);
    unlink(fileName);
    delete fileName;
    delete (param*)p;
    if(tmp)
        free(tmp); 
    if(cacheLimit > 0)
    {
    int fd = ::open(gzFileName, O_LARGEFILE | O_RDONLY, 0644);
        if(fd == -1)
        {
            fprintf(stderr, "Open %s error: %s!\n", gzFileName, strerror(errno));
        }
        else
        {
            if(posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED) != 0)
                fprintf(stderr, "Clear cache %s error: %s!\n", gzFileName, strerror(errno));
            ::close(fd);
        }
    }
    return NULL;
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////
// SyslogAppender
///////////////////////////////////////////////////////////////////////////////////////////////////////////
std::map<std::string, Appender *>  SyslogAppender::s_syslogAppenders;
Mutex SyslogAppender::s_appenderMapMutex;

SyslogAppender :: SyslogAppender(const char *ident,  int facility) :
        m_ident(ident), m_facility(facility)
{
    open();
}

SyslogAppender :: ~SyslogAppender()
{
    close();
}

Appender* SyslogAppender :: getAppender(const char *ident, int facility)
{
    std::string name = ident;
    ScopedLock lock(s_appenderMapMutex);
    std::map<std::string, Appender *>::iterator i = s_syslogAppenders.find(name);
    if (s_syslogAppenders.end() == i)
    {
        Appender *newAppender = new SyslogAppender(ident, facility);
        s_syslogAppenders[name] = newAppender;
        s_appenders.insert(newAppender);
        return newAppender;
    }
    else
    {
        return (*i).second;
    }
}

int SyslogAppender :: append(LoggingEvent& event)
{
    if (event.level >= alog::LOG_LEVEL_COUNT)
        return -1;
    if(m_layout == NULL)
    	m_layout = new BasicLayout();
    std::string formatedStr = m_layout->format(event);
    ScopedLock lock(m_appendMutex);
    int priority = toSyslogLevel(event.level);
    syslog(priority | m_facility, "%s", formatedStr.c_str());
    return 0;
}

void SyslogAppender::flush()
{
    return;
}

int SyslogAppender :: open()
{
    openlog(m_ident.c_str(), LOG_PID, m_facility);
    return 0;
}

int SyslogAppender :: close()
{
    closelog();
    return 0;
}

void SyslogAppender::release()
{
    s_syslogAppenders.clear();
}

int SyslogAppender :: toSyslogLevel(uint32_t level)
{
    int result;

    if (level <= 1)
    {
        result = LOG_EMERG;
    }
    else if (level == 2)
    {
        result = LOG_ERR;
    }
    else if (level == 3)
	{
        result = LOG_WARNING;
	}
    else if (level == 4)
    {
        result = LOG_INFO;
    }
    else
    {
        result = LOG_DEBUG;
    }

    return result;
}

}
