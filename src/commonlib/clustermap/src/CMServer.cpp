/** \file
 *******************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 11379 $
 *
 * $LastChangedDate: 2012-04-18 17:14:23 +0800 (Wed, 18 Apr 2012) $
 *
 * $Id: CMServer.cpp 11379 2012-04-18 09:14:23Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include "CMServer.h"
#include "DGramClient.h"

namespace clustermap {

CMServer::CMServer(): m_tcpserver(m_cmtree)
{
    //printf("CMServer new\n");
    m_stop_flag = 0;
    m_timeOut_thres = 2000000;
    m_getMasterStateTime = 2000000;
    m_maxPeriod_reportStateUpdate = 500000;
    m_maxPeriod_reportAllState = 2000000;
    m_thread_num = 0;
    m_mast_state = false;
    m_mast_flag = true;
    m_pudpserver = NULL;
    m_pip_name[0] = 0;
    pthread_mutex_init(&m_lock, NULL);
}

CMServer::~CMServer()
{
    //printf("CMServer del\n");
    m_stop_flag = 1;
    if(m_pip_name[0]) {
        int fd = open(m_pip_name, O_WRONLY);
        if(fd) {
            write(fd, &fd, 1);
            close(fd);
        }
    }
    while(m_stop_flag != m_thread_num+1) 
        usleep(1);

    if(m_pudpserver)
        delete m_pudpserver;
    pthread_mutex_destroy(&m_lock);
}

int32_t CMServer::initialize(const char *pConf, const char* master_spec)
{
    if(pConf == NULL)
    {
        TLOG("CMapBuilder:the input conf can't be null. \n");
        return -1;
    }	
    strcpy(m_cfg_file_name, pConf);

    m_stop_flag = 0;
    pthread_t id;
    if(master_spec) {
        m_mast_flag = false;
        if(m_cmtree.init(pConf, 0) != 0)
            return -1;
        if(m_tcpClient.setServerAddr(master_spec) != 0)
            return -1;
        if(m_tcpClient.start() != 0)
            return -1;
        if(pthread_create(&id, NULL, thread_slave, this))
            return -1;
        strcpy(m_pip_name, "cm_s.pipe");
        m_thread_num++;
    } else {
        m_mast_flag = true;
        if(m_cmtree.init(pConf, 1) != 0)
            return -1;
        strcpy(m_pip_name, "cm_m.pipe");
    }

    if(pthread_create(&id, NULL, thread_load, this))
        return -1;
    m_thread_num++;

    return 0;
}

int32_t CMServer::startUdpListen(uint16_t port)
{
    m_pudpserver = new CDGramServer();
    if(m_pudpserver == NULL)
        return -1;
    if(m_pudpserver->InitDGramServer(port) != 0)
        return -1;

    pthread_t id;
    if(pthread_create(&id, NULL, thread_udp, this))
        return -1;

    TLOG("begin start check timeout thread");
    if(pthread_create(&id, NULL, thread_timeout, this))
        return -1;

    m_thread_num++;
    if(pthread_create(&id, NULL, thread_reportStateUpdate, this))
        return -1;
    m_thread_num++;

    return 0;
}

int32_t CMServer::startTcpListen(uint16_t port)
{
    char buf[200];
    sprintf(buf, "tcp::%u", port);

    if(m_tcpserver.setServerAddr(buf) != 0)
        return -1;
    m_tcpserver.setTimeout(3000);
    if(m_tcpserver.start() != 0)
        return -1;
    return 0;
}

int32_t CMServer::getDifference(struct timeval begin, struct timeval end)
{
    int32_t dif = (end.tv_sec - begin.tv_sec) * 1000000;
    dif += end.tv_usec - begin.tv_usec;
    return dif;
}

void* CMServer::thread_udp(void* vpPara)
{
    CMServer* cpClass = (CMServer*)vpPara;
    char buffer[MAX_UDP_PACKET_SIZE];

    while(cpClass->m_stop_flag == 0)
    {
        int32_t ret_len = cpClass->m_pudpserver->receive(buffer, MAX_UDP_PACKET_SIZE);
        if(cpClass->m_stop_flag)
            break;

        StatInput in;
        StatOutput out;
        if(in.bufToNode(buffer, ret_len) <= 0) {
            TLOG("getNodeState failed\n");
            continue;
        }
        in.op_time = cpClass->m_cur_time;
        cpClass->m_cmtree.process(&in, &out);
    }

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

void* CMServer::thread_timeout(void* vpPara)
{
    CMServer* cpClass = (CMServer*)vpPara;
    CMTreeProxy *cpTree = &cpClass->m_cmtree;

    checkInput cInput;
    cInput.type = msg_check_timeout;

    while(cpClass->m_stop_flag == 0)
    {
        gettimeofday(&cpClass->m_cur_time, NULL);

        cInput.op_time = cpClass->m_cur_time;
        cInput.minUpdateTime.tv_sec = cpClass->m_cur_time.tv_sec - cpClass->m_timeOut_thres/1000000;
        cInput.minUpdateTime.tv_usec = cpClass->m_cur_time.tv_usec - cpClass->m_timeOut_thres%1000000;
        if(cInput.minUpdateTime.tv_usec < 0) {
            cInput.minUpdateTime.tv_sec -= 1;
            cInput.minUpdateTime.tv_usec += 1000000;
        }

        cpTree->process(&cInput, NULL);
        usleep(cpClass->m_timeOut_thres>>2);
    }

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

void* CMServer::thread_slave(void* vpPara)
{
    CMServer* cpClass = (CMServer*)vpPara;

    char buf[100], *recv_buf, *p;
    int32_t buf_len, succflag = 1, failflag = 1;
    time_t master_time = 0, slave_time = 0;
    char slave_md5[16], master_md5[16];

    while(cpClass->m_stop_flag == 0)
    {
        cpClass->m_cmtree.getversion(slave_md5, slave_time);

        p = buf;
        *p = (char)msg_active;
        p += 1;
        memcpy(p, slave_md5, 16);
        p += 16;
        *(int32_t*)p = htonl(slave_time);
        p += 4;

        if(cpClass->m_tcpClient.send(buf, p-buf) != 0 || 
                cpClass->m_tcpClient.recv(recv_buf, buf_len) != 0) {
            cpClass->m_mast_state = false;
            if(failflag) {
                TLOG("tcp client to server error");
                failflag = 0;
                succflag = 1;
            }
        } else {
            if(succflag) {
                TLOG("tcp client to server success");
                succflag = 0;
                failflag = 1;
            }
            cpClass->m_mast_state = true;
            p = recv_buf;
            char type = *p;
            p += 1;

            if(type == msg_active) {
                memcpy(master_md5, p, 16);
                p += 16;
                master_time = ntohl(*(int32_t*)p);
                p += 4;

                if(memcmp(master_md5, slave_md5, 16)) {
                    setmapInput in;
                    setmapOutput out;
                    in.type = cmd_set_clustermap;
                    if(slave_time > master_time){
                        in.mapbuf = cpClass->m_cfg_file_name;
                        in.op_time = slave_time;
                        if(in.nodeToBuf() <= 0) {
                            TLOG("slave set clustermap to server error");
                            continue;
                        }
                        if(cpClass->m_tcpClient.send(in.buf, in.len) != 0) {
                            TLOG("slave set clustermap to server, send error");
                            continue;
                        }
                        if(cpClass->m_tcpClient.recv(recv_buf, buf_len) != 0) {
                            TLOG("slave set clustermap to server, recv error");
                            continue;
                        }
                        if(out.isSuccess(recv_buf, buf_len)) {
                            TLOG("slave set clustermap to server success");
                        }
                    }
                }
            } else if(type == cmd_get_clustermap) {
                setmapInput in;
                setmapOutput out;
                in.type = cmd_set_clustermap;
                in.op_time = ntohl(*(uint32_t*)p);
                p += 4;
                in.mapbuf = p;
                in.maplen = buf_len - (p - recv_buf);
                if(cpClass->m_cmtree.process(&in, &out) != 0) {
                    TLOG("master set clustermap to slave failed");
                } else {
                    if(cpClass->m_cmtree.reload()) {
                        TLOG("master set clustermap to slave succeed");
                    } else {
                        TLOG("master set clustermap to slave, reload failed");
                    }
                }
            }
        }
        usleep(cpClass->m_getMasterStateTime);
    }
    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

void* CMServer::thread_reportStateUpdate(void* vpPara)
{
    CMServer* cpClass = (CMServer*)vpPara;

    CDGramClient cDGramClient;

    bool send_flag = false;
    struct timeval last_send_time;
    gettimeofday(&last_send_time, NULL);

    while(cpClass->m_stop_flag == 0)
    {
        struct timeval  begin_time;
        gettimeofday(&begin_time, NULL);

        StatInput in;
        StatOutput out;
        in.type = msg_check_update;
        if(cpClass->getDifference(last_send_time, begin_time) > cpClass->m_maxPeriod_reportAllState)
        {
            in.type = msg_update_all;
            last_send_time = begin_time;
        }

        if(cpClass->m_cmtree.process(&in, &out) != 0) {
            printf("getStateUpdate error\n");
            continue;
        }

        StatOutput* pout = out.next;
        while(pout) {
            if( cpClass->m_mast_state == false) {
                if(cDGramClient.InitDGramClient(pout->sub->sub_ip, pout->sub->sub_port) == 0) {
                    cDGramClient.send(pout->send_buf, pout->send_len);
                } else {
                    printf("send error %s:%u\n", pout->sub->sub_ip, pout->sub->sub_port);
                }
            }
            pout = pout->next;
        }

        send_flag = false;
        struct timeval end_time;
        gettimeofday(&end_time, NULL);

        time_t idle_time = cpClass->m_maxPeriod_reportStateUpdate - cpClass->getDifference(begin_time, end_time);
        if(idle_time > 0) usleep(idle_time);
    }

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

void* CMServer::thread_load(void* vpPara)
{
    CMServer* cpClass = (CMServer*)vpPara;

    char c; 
    int fd, n;
    umask(0);
    if((mkfifo(cpClass->m_pip_name, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) && 
            (errno != EEXIST)) {
        TLOG("mkfifo pipe error, name=%s\n", cpClass->m_pip_name);
        return NULL;
    }
    fd = open(cpClass->m_pip_name, O_RDONLY | O_NONBLOCK);
    if(fd < 0) {
        TLOG("open pipe=%s failed", cpClass->m_pip_name);
        return NULL;
    }

    while(cpClass->m_stop_flag == 0)
    {
        if((n=read(fd, &c, 1)) < 0 && errno != EAGAIN) {
            TLOG("sync read error(exit),pip_name=%s", cpClass->m_pip_name);	
            break;
        }
        if(cpClass->m_stop_flag) break;
        if(n != 1) { 
            usleep(1);
            continue;
        }
        if(c == 'a') { 
            cpClass->m_cmtree.loadSyncInfo();
            TLOG("sync make tree load");
        }	else {
            TLOG("error cmd(%c) at pipe=%s", c, cpClass->m_pip_name);
        }
    }
    close(fd);

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

}
