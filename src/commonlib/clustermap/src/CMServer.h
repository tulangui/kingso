/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 516 $
 *
 * $LastChangedDate: 2011-04-08 23:43:26 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: CMServer.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_SERVER_H_
#define _CM_SERVER_H_

#include <stdio.h>
#include "CMTree.h"
#include "CMTCPServer.h"
#include "DGramServer.h"
#include "CMTCPClient.h"

namespace clustermap {

class CMServer
{
    public:
        CMServer();
        ~CMServer();

        int32_t initialize(const char *pconf, const char* master_spec = NULL);	
        void setTimeOut_thres(time_t t) {m_timeOut_thres = t * 1000;}
        void setMaxPeriod_reportStateUpdate(time_t t) {m_maxPeriod_reportStateUpdate = t * 1000;}
        void setMaxPeriod_reportAllState(time_t t) {m_maxPeriod_reportAllState = t * 1000;}

        int32_t startUdpListen(uint16_t port);
        int32_t startTcpListen(uint16_t port);

    private:
        CMTCPServer m_tcpserver;
        CDGramServer* m_pudpserver;

        CMTCPClient m_tcpClient;

        char m_pip_name[128];
        struct timeval m_cur_time;
        char m_cfg_file_name[128];

        time_t m_timeOut_thres;
        time_t m_maxPeriod_reportStateUpdate;
        time_t m_maxPeriod_reportAllState;
        time_t m_getMasterStateTime;

        bool m_mast_state;
        bool m_mast_flag;

        int m_stop_flag;
        int m_thread_num;
        pthread_mutex_t m_lock;

        CMTreeProxy m_cmtree;

        int32_t getDifference(struct timeval begin, struct timeval end);
        int32_t readFiletoSysMap(std::map<std::string, std::string> &sys, const char *filename);

        static void* thread_udp(void* vpPara);
        static void* thread_tcp(void* vpPara);
        static void* thread_timeout(void* vpPara);
        static void* thread_reportStateUpdate(void* vpPara);
        static void* thread_slave(void* vpPara);
        static void* thread_load(void* vpPara);
};

}
#endif
