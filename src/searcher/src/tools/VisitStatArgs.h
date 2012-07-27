#ifndef TOOLS_VISITSTATARGS_H_
#define TOOLS_VISITSTATARGS_H_

#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_STAT_COL_NUM 128  // 集群最大列数
#define MAX_IP_LEN       16   // ip长度
#define MAX_THRAD_NUM    16   // 最大线程个数
#define MAX_LOG_NUM      1024 // 最多处理日志文件数量

class VisitStatArgs
{ 
public:
    VisitStatArgs();
    ~VisitStatArgs();

    int parseArgs(int argc, char** argv);
    
    char* getServerCfg() {return _serverPath;}
    char* getLogCfg()    {return _logPath;}
    char* getOutPath()   {return _outputFile;}
    
    time_t getSegBegin() {return _segBegin; }
    time_t getSegEnd()   {return _segEnd;}

    int getMaxStatNum()  {return _maxStatNum;}
    int getListenPort()  {return _listenPort;}
    
    int getColNum()      {return _statColNum;}
    int getTop()         {return _top;}
    int getThreadNum()   {return _threadNum;}

    int getColInfo(int col, char*& ip, int& port) {
        if(col < 0 || col >= _statColNum) {
            return -1;
        }
        ip = _statColIp[col];
        port = _statColPort[col];
        return 0;
    }

private:

    char _serverPath[PATH_MAX];
    char _logPath[PATH_MAX];
    char _outputFile[PATH_MAX];
    char _statColCfg[PATH_MAX];
    
    time_t _segBegin;
    time_t _segEnd;

    int _statColNum;
    int _statColPort[MAX_STAT_COL_NUM];
    char _statColIp[MAX_STAT_COL_NUM][MAX_IP_LEN];

    int _top;
    int _listenPort;
    int _threadNum;
    int _maxStatNum;

private:

    // 将字符串格式的时间转成time_t, 格式:年月日时分秒
    time_t str2time(const char* strTime);
    time_t day2time(int hour, int min);

    void showHelp();
    int parseColInfo();
    int getLocalPort();
};

#endif //TOOLS_VISITSTATARGS_H_
