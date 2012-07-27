#include <ifaddrs.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "util/Log.h"
#include "VisitStatArgs.h" 

VisitStatArgs::VisitStatArgs()
{
    _serverPath[0] = 0;
    _logPath[0] = 0;
    _outputFile[0] = 0;
    _statColNum = 0;

    _top = 33;
    _threadNum = 16;
    _maxStatNum = 20000000;
}

VisitStatArgs::~VisitStatArgs()
{

}

int VisitStatArgs::parseArgs(int argc, char** argv)
{
    char c = 0; 
    char logTimeSeg[PATH_MAX];

    bool help = false;

    while ((c = getopt(argc, argv, "c:l:n:t:m:f:r:T:h")) != -1) {
        switch(c) {
            case 'c':
                strcpy(_serverPath, optarg); break;
            case 'l':
                strcpy(_logPath, optarg); break;
            case 'f':
                strcpy(_outputFile, optarg); break;
            case 'r':
                strcpy(_statColCfg, optarg); break;
            case 'n':
                _threadNum = atol(optarg); break;
            case 'm':
                _maxStatNum = atol(optarg); break;
            case 't':
                strcpy(logTimeSeg, optarg); break;
            case 'T':
                _top = atol(optarg); break;
            default:
                help = true; break;
        }
    }
    if(help && _serverPath[0] == 0 || _logPath[0] == 0 ||
            _outputFile[0] == 0 || _statColCfg[0] == 0) {
        showHelp();
        return 0;
    }
    
    if(logTimeSeg[0]) {
        char* p = strchr(logTimeSeg, '-');
        if(p == NULL) {
            TERR("parse log time seg error");
            return -1;
        }
        *p++ = 0;
        _segBegin = str2time(logTimeSeg);
        _segEnd = str2time(p);
    } else {
        _segBegin = day2time(10, 0);
        _segEnd = day2time(23, 0);
    }
    if(_segBegin > _segEnd) {
        TERR("log begin(%d) > end(%d) error", _segBegin, _segEnd);
        return 0;
    }

    if(parseColInfo() < 0) {
        TERR("get col info error");
        return -1;
    }
    if(getLocalPort() < 0) {
        TERR("get local port error");
        return -1;
    }

    return 0;
}
    
int VisitStatArgs::parseColInfo()
{
    FILE* fp = fopen(_statColCfg, "rb");
    if(NULL == fp) {
        TERR("open %s error", _statColCfg);
        return -1;
    }

    char buf[1024];
    _statColNum = 0;

    while(fgets(buf, 1024, fp)) {
        char* p = strchr(buf, ':');
        if(p == NULL) continue;
        
        *p++ = 0;
        strcpy(_statColIp[_statColNum], buf);
        _statColPort[_statColNum++] = atol(p);
    }
    fclose(fp);
    
    return _statColNum;
}

int VisitStatArgs::getLocalPort()
{
    struct ifaddrs *ifc = NULL, *ifc1 = NULL;
    if(getifaddrs(&ifc)) {
        TERR("getifaddrs error");
        return -1;
    }
    
    char ip[16];
    int ret = -1;
    
    _listenPort = 0;

    for(ifc1 = ifc; ifc; ifc = ifc->ifa_next) {
        if(ifc->ifa_addr == NULL || ifc->ifa_netmask == NULL)
            continue;
        inet_ntop(AF_INET, &(((struct sockaddr_in*)(ifc->ifa_addr))->sin_addr), ip, MAX_IP_LEN);

        for(int i = 0; i < _statColNum; i++) {
            if(strcmp(_statColIp[i], ip) == 0) {
                ret = 0;
                _listenPort = _statColPort[i];
                break;
            }
        }
    }
    freeifaddrs(ifc1);

    if(ret < 0) {
        TERR("not find local port");
    }
    return ret;
}

// 将字符串格式的时间转成time_t, 格式:年月日时分秒
time_t VisitStatArgs::str2time(const char* strTime)
{
    time_t now = time(NULL);
    struct tm st;
    
    localtime_r(&now, &st);
    int ret = sscanf(strTime, "%4d%2d%2d%2d%2d%2d", &st.tm_year, &st.tm_mon, &st.tm_mday, &st.tm_hour, &st.tm_min, &st.tm_sec);
    if(ret != 6) {
        TERR(stderr, "error, %s %d\n", strTime, ret);
        return 0;
    }

    st.tm_year -= 1900;
    st.tm_mon--;

    return mktime(&st);
}

time_t VisitStatArgs::day2time(int hour, int min)
{
    time_t now = time(NULL);

    struct tm st;
    localtime_r(&now, &st);

    st.tm_hour = hour;
    st.tm_min  = min;
    st.tm_sec  = 0;

    now = mktime(&st);
    localtime_r(&now, &st);
    printf("%4d%2d%2d%2d%2d%2d\n", st.tm_year+1900, st.tm_mon+1, st.tm_mday, st.tm_hour, st.tm_min, st.tm_sec);
    return now;
}

void VisitStatArgs::showHelp()
{
    fprintf(stdout, "Usage: -c <server_confPath>\n");
    fprintf(stdout, "       -l <log_confPath>\n"); 
    fprintf(stdout, "       -f <stat_result_file>\n"); 
    fprintf(stdout, "       -r <row info (matchine list)>\n"); 
    fprintf(stdout, "      [-n <thread_num>]\n");
    fprintf(stdout, "      [-m <max_stat_query_num>]\n");
    fprintf(stdout, "      [-t <log begin-end time>]\n");
    fprintf(stdout, "      [-T <top Visit doc / all doc num, default 33>]\n");
}

