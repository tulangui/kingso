/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 1272 $
 *
 * $LastChangedDate: 2011-05-03 13:34:56 +0800 (Tue, 03 May 2011) $
 *
 * $Id: ClusterServerMain.cpp 1272 2011-05-03 05:34:56Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <iostream>
#include <unistd.h>
#include <string.h>
#include <cstring>
#include <signal.h>
#include <sys/types.h>
#include "util/Log.h"
#include "CMServer.h"

using namespace clustermap;

int g_stop_flag = 0;
void sig_quit(int signum, siginfo_t* info, void* myact) {
    g_stop_flag = 1;
}
const int MAX_ARGC = 20;

int main(int argc, char** argv)
{
    if(argc < 4)
    {
        fprintf(stdout, "%s tcp_port udp_port cfg_file [mast_spec(tcp:ip:tcp_port)] [maxPeriod_reportAllState] [maxPeriod_reportStateUpdate] [timeOut_thres] [-d]\n\n", argv[0]);
        return -1;
    }

    struct sigaction act3;
    sigemptyset(&act3.sa_mask);
    act3.sa_flags=SA_SIGINFO;
    act3.sa_sigaction=sig_quit;

    if(sigaction(SIGQUIT,&act3,NULL)){
        fprintf(stderr, "Sig error\n");
        return -1;
    }

    int daemon_count = 0;
    char *targv[MAX_ARGC];	
    targv[0] = argv[0];
    for (int i = 1; i < argc; i++) {
        if (strcasecmp(argv[i], "-d") != 0) {
            targv[i - daemon_count] = argv[i];
        } else {
            daemon_count++;
        }
    }
    if (daemon_count > 0) {
        if(daemon(1, 1) == -1) {
            fprintf(stderr, "set self process as daemon failed.\n\n");
            return -1;
        }

    }

    int32_t maxPeriod_reportAllState = -1;
    int32_t maxPeriod_reportStateUpdate = -1;
    int32_t timeOut_thres = -1;
    uint16_t tcp_port = 0, udp_port = 0;

    tcp_port = (uint16_t)atol(targv[1]);
    udp_port = (uint16_t)atol(targv[2]);
    char* cfg_file = targv[3];

    CMServer cServer;

    argc = argc-daemon_count;	
    if(argc >= 5 && strcasecmp(targv[4], "null")) {
        char* tmp_cfg = "s_alog.ini";
        FILE* fp = fopen(tmp_cfg, "w");
        if(fp == NULL)
            return -1;
        fprintf(fp, "alog.rootLogger=INFO, A\n");
        fprintf(fp, "alog.appender.A=FileAppender\n");
        fprintf(fp, "alog.appender.A.fileName=%s\n", "s_clustermap.log");
        fprintf(fp, "alog.appender.A.flush=true\n");
        fclose(fp);
        alog::Configurator::configureLogger(tmp_cfg);
        TLOG("clustermap begin init");
        if(cServer.initialize(cfg_file, targv[4]) != 0) {
            alog::Logger::shutdown();
            fprintf(stderr, "clustermap init failed!\n");
            return -1;
        }
    } else {
        char* tmp_cfg = "m_alog.ini";
        FILE* fp = fopen(tmp_cfg, "w");
        if(fp == NULL)
            return -1;
        fprintf(fp, "alog.rootLogger=INFO, A\n");
        fprintf(fp, "alog.appender.A=FileAppender\n");
        fprintf(fp, "alog.appender.A.fileName=%s\n", "m_clustermap.log");
        fprintf(fp, "alog.appender.A.flush=true\n");
        fclose(fp);
        alog::Configurator::configureLogger(tmp_cfg);
        TLOG("clustermap begin init");
        if(cServer.initialize(cfg_file, NULL) != 0) {
            alog::Logger::shutdown();
            fprintf(stderr, "clustermap init failed!\n");
            return -1;
        }
    }

    if(argc > 5)
        maxPeriod_reportAllState = atol(targv[5]);
    if(argc > 6)
        maxPeriod_reportStateUpdate = atol(targv[6]);
    if(argc > 7)
        timeOut_thres = atol(targv[7]);

    if(maxPeriod_reportAllState > 0) cServer.setMaxPeriod_reportAllState(maxPeriod_reportAllState);
    if(maxPeriod_reportStateUpdate > 0) cServer.setMaxPeriod_reportStateUpdate(maxPeriod_reportStateUpdate);
    if(timeOut_thres > 0) cServer.setTimeOut_thres(timeOut_thres);

    if(cServer.startTcpListen(tcp_port) != 0)
    {
        fprintf(stderr, "cServer.startTcpListen Fail!\n\n");
        alog::Logger::shutdown();
        return -1;
    }
    fprintf(stdout, "tcp server, port = %u, start ok\n", tcp_port);

    if(cServer.startUdpListen(udp_port) != 0)
    {
        fprintf(stderr, "cServer.startUdpListen Fail!\n\n");
        alog::Logger::shutdown();
        return -1;
    }
    fprintf(stdout, "udp server, id=%d,port = %u, start ok\n", getpid(), udp_port);
    fprintf(stdout, "clustermap start ok\n\n");
    fflush(stdout);
    fflush(stderr);

    while(g_stop_flag == 0) sleep(10);
    alog::Logger::shutdown();

    return 0;
}
