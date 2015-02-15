/** \file
 *******************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 12106 $
 *
 * $LastChangedDate: 2012-05-21 20:15:40 +0800 (Mon, 21 May 2012) $
 *
 * $Id: CMClient.cpp 12106 2012-05-21 12:15:40Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <mxml.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "util/Log.h"
#include "CMClient.h"
#include "CMClientLocal.h"
#include "CMClientRemote.h"

namespace clustermap {

const uint16_t SUB_CHILD_PORT = 10000;
const uint16_t SUB_ALL_PORT = 20000;
const uint16_t SUB_SELFCLUSTER_PORT = 30000;

CMClient::CMClient()
{
    //printf("CMClient new\n");
    m_state = 0;
    //m_subtype = -1;
    m_stop_flag = 0;
    m_thread_num = 0;
    m_report_period =50000;
    m_cpClientHead = NULL;
    m_cpWatchPonit = NULL;
    m_thread_udp_recv_isStart = false;
    pthread_mutex_init(&m_lock, NULL);
}

CMClient::~CMClient()
{
    //printf("CMClient del\n");
    m_stop_flag = 1;
    if(m_thread_udp_recv_isStart) {
        CDGramClient cClient;
        if(cClient.InitDGramClient(m_app_ip, m_sub_port) == 0) {
            char message_exit[64];
            message_exit[0] = msg_exit;
            *(int32_t*)(message_exit+1) = htonl(5);
            cClient.send(message_exit, 5);
        }
    }
#ifdef _DEBUG
    char* _pipe_path = "mmm";
    int fd = open(_pipe_path, O_WRONLY);
    if(fd) {
        char ch = 0;
        write(fd, &ch, 1);
        close(fd);
    }
#endif
    while(m_stop_flag-1 < m_thread_num) usleep(1);

    CMClientInterface* next = m_cpClientHead;
    while(next){
        next = m_cpClientHead->next;
        delete m_cpClientHead;
        m_cpClientHead = next;
    }
    m_cpClientHead = NULL;

    WatchPoint* p = m_cpWatchPonit;
    while(p)
    {
        p = m_cpWatchPonit->next;
        delete m_cpWatchPonit;
        m_cpWatchPonit = p;
    }
    m_cpWatchPonit = NULL;
    pthread_mutex_destroy(&m_lock);
}

int32_t CMClient::Initialize(const char* app_spec, std::vector<std::string>& map_server, const char* local_cfg)
{
    /*
       char log_name[128];
       sprintf(log_name, "clusterClient_%d.log", getpid());
       FILE* fp = fopen(log_name, "w");
       if(fp == NULL)
       return -1;
       fprintf(fp, "alog.rootLogger=INFO, A\n");
       fprintf(fp, "alog.appender.A=FileAppender\n");
       fprintf(fp, "alog.appender.A.fileName=%s\n", log_name);
       fprintf(fp, "alog.appender.A.flush=true\n");
       fclose(fp);
       alog::Configurator::configureLogger(log_name);
     */
    if(local_cfg && strlen(local_cfg) > PATH_MAX)
        return -1;
    if(local_cfg == NULL || local_cfg[0] == 0)
        m_local_cfg[0] = 0;
    else
        strcpy(m_local_cfg, local_cfg);
    if(parseAppSpec(app_spec) != 0)
        return -1;

    m_server_num = 0;
    for(int32_t i = 0; i < (int32_t)map_server.size(); i++) {
        if(parseServer(map_server[i].c_str()) != 0)
            continue;
        m_server_num++;
        if(m_server_num >= 2) break;
    }

    CMClientInterface** next = &m_cpClientHead;	
    for(int32_t i = 0; i < m_server_num; i++) {
        CMClientRemote* pClient = new CMClientRemote();
        if(pClient == NULL)
            return -1;
        if(pClient->init(m_server_ip[i], m_server_udp_port[i], m_server_tcp_port[i]) != 0 ) {
            delete pClient;
            return -1;
        }
        *next = pClient;
        next = &pClient->next;
    }

    if(m_local_cfg[0]) {
        CMClientLocal* pClient = new CMClientLocal();
        if(pClient == NULL)
            return -1;
        if(pClient->init(m_local_cfg) != 0){
            delete pClient;
            return -1;
        }
        *next = pClient;
        next = &pClient->next;
    }
    next = NULL;

    InitInput in;
    InitOutput out;
    in.type = msg_init;
    strcpy(in.ip, m_app_ip);
    in.port = m_app_port;
    in.protocol = m_app_pro;
    if(m_cClient.Initialize(m_cpClientHead, &in) != 0) {
        if(m_app_ip[0] == 0)
            return -1;
        //m_subtype = sub_all;	// only sub all
    } else {
        strcpy(m_app_ip, in.ip);
    }
#ifdef _DEBUG
    pthread_t id;
    if(pthread_create(&id, NULL, thread_monitor, this))
        return -1;
    m_thread_num++;
#endif
    return 0;
}

int32_t CMClient::Register()
{
    RegInput in;
    in.type = msg_register;
    strcpy(in.ip, m_app_ip);
    in.port = m_app_port;
    in.protocol = m_app_pro;

    if(m_cClient.Register(&in) != 0)
        return -1;

    pthread_t id;
    if(pthread_create(&id, NULL, thread_report, this))
        return -1;
    m_thread_num++;

    return 0;
}

int32_t CMClient::Subscriber(int32_t subtype)
{
    //if (m_subtype > 0 && subtype != m_subtype)
    //	return -1;
    SubInput in;
    if (subtype == sub_child) {
        in.type = msg_sub_child;
        m_sub_port = SUB_CHILD_PORT;
    } else if (subtype == sub_all) {
        in.type = msg_sub_all;
        m_sub_port = SUB_ALL_PORT;
    } else if (subtype == sub_selfcluster) {
        in.type = msg_sub_selfcluster;
        m_sub_port = SUB_SELFCLUSTER_PORT;
    } else {
        return -1;
    }
    int ret_value = -1;
    for(int32_t i = 0; i < 1000; i++) {
        if(m_udpserver.InitDGramServer(m_sub_port) == 0) {
            ret_value = 0;
            break;
        }
        m_sub_port++;
    }
    if(ret_value == -1)
        return ret_value;

    strcpy(in.ip, m_app_ip);
    in.port = m_app_port;
    in.protocol = m_app_pro;
    in.sub_port = m_sub_port;
    if(m_cClient.Subscriber(&in) != 0)
        return -1;

    if(m_local_cfg[0]) {
        MsgOutput out;
        in.type = cmd_set_state_all_normal;
        if(m_cClient.process(&in, &out) != 0)
            return -1;
    }
    pthread_t id;
    if(pthread_create(&id, NULL, thread_udp_recv, this))
        return -1;
    m_thread_num++;
    m_thread_udp_recv_isStart = true;

    return 0;
}

int32_t CMClient::setReportPeriod(int32_t report_period)
{
    m_report_period = report_period * 1000;
    return 0;
}

int32_t CMClient::addWatchPoint(WatchPoint* cpWatchPonit)
{
    if(!cpWatchPonit) return -1;
    cpWatchPonit->next = m_cpWatchPonit;
    m_cpWatchPonit = cpWatchPonit;
    return 0;
}

int32_t CMClient::allocSubRow(int32_t *&ids, int32_t &size, enodetype nodetype, int level, LogicOp logic_op, uint32_t locating_signal)
{
    allocSubInput in;
    allocSubOutput out;

    in.type = cmd_alloc_sub;
    in.node_type = nodetype;
    in.level = level;
    in.logic_op = logic_op;
    in.locating_signal = locating_signal;

    if(m_cClient.process(&in, &out) != 0) {
        size = 0;
        ids = NULL;
        return -1;
    }	else {
        size = out.size;
        ids = out.ids;
        return 0;
    }

    return -1;
}

int32_t CMClient::allocSubCluster(int32_t *&ids, int32_t &size, enodetype nodetype, int level, LogicOp logic_op, uint32_t locating_signal)
{
    allocSubInput in;
    allocSubOutput out;

    in.type = cmd_alloc_subcluster;
    in.node_type = nodetype;
    in.level = level;
    in.logic_op = logic_op;
    in.locating_signal = locating_signal;

    if(m_cClient.process(&in, &out) != 0) {
        size = 0;
        ids = NULL;
        return -1;
    }	else {
        size = out.size;
        ids = out.ids;
        return 0;
    }

    return -1;
}

int32_t CMClient::allocSelfCluster(int32_t *&ids, int32_t &size, enodetype nodetype, int level, LogicOp logic_op, uint32_t locating_signal)
{
    allocSubInput in;
    allocSubOutput out;

    in.type = cmd_alloc_selfcluster;
    in.node_type = nodetype;
    in.level = level;
    in.logic_op = logic_op;
    in.locating_signal = locating_signal;

    if(m_cClient.process(&in, &out) != 0) {
        size = 0;
        ids = NULL;
        return -1;
    }	else {
        size = out.size;
        ids = out.ids;
        return 0;
    }

    return -1;
}

int32_t CMClient::freeSubRow(int32_t* ids)
{
    if(ids == NULL)
        return 0;
    free(ids);
    ids = NULL;
    return 0;
}

CMISNode* CMClient::getNodeInfo(int32_t nodeid)
{
    getNodeInput in;
    getNodeOutput out;

    in.local_id = nodeid;
    in.type = cmd_get_info;

    if(m_cClient.process(&in, &out) != 0)
        return NULL;
    CMISNode* cpNode = out.cpNode;
    out.cpNode = NULL;
    return cpNode;
}

CMISNode* CMClient::allocNode(CMISNode *node, enodetype type_id)
{
    allocNodeInput in;
    allocNodeOutput out;	

    in.cpNode = node;
    in.node_type = type_id;
    in.type = cmd_alloc_node_by_node;
    if(m_cClient.process(&in, &out) != 0)
        return NULL;
    return out.cpNode;
}

CMISNode* CMClient::allocNode(const char * subclustername, enodetype type_id)
{
    allocNodeInput in;
    allocNodeOutput out; 

    in.name = (char*)subclustername; 
    in.node_type = type_id;
    in.type = cmd_alloc_node_by_name;
    if(m_cClient.process(&in, &out) != 0)
        return NULL;
    return out.cpNode;
}

uint32_t CMClient::getClusterCountByStatus(estate_cluster status)
{
    getClusterCountByStatusInput in;
    getClusterCountByStatusOutput out;

    in.state_cluster = status;
    in.type = cmd_get_ClusterCount_ByStatus;
    if(m_cClient.process(&in, &out) != 0)
        return 0;
    return out.ClusterCount;
}

std::string CMClient::getGlobalEnvVar(const char *EnvVar_Key)
{
    getGlobalEnvVarInput in;
    getGlobalEnvVarOutput out;

    if (!EnvVar_Key || EnvVar_Key[0] == '\0') {
        return "nil";
    }
    in.EnvVar_Key = EnvVar_Key;
    in.type = cmd_get_GlobalEnvVar;
    if(m_cClient.process(&in, &out) != 0)
        return "nil";
    return out.EnvVar_Value;
}        

int32_t CMClient::setSubClusterStation(char* stateBitmap, int32_t stateBitmapLen, int32_t node_count)
{
    setStatInput in;
    setStatOutput out; 

    in.stateBitmap = stateBitmap;
    in.node_num = node_count;   // node_count = max_node_num + max_col_num
    in.type = cmd_set_state;

    if(m_cClient.process(&in, &out) != 0)
        return -1;
    return 0;
}

int32_t CMClient::parseAppSpec(const char* app_spec)
{
    const char* p = strchr(app_spec, ':');
    if(p == NULL)
        return -1;

    char buf[32];
    CProtocol cProtocol;
    if(p-app_spec >= 32)
        return -1;
    memcpy(buf, app_spec, p-app_spec);
    buf[p-app_spec] = 0;
    if(cProtocol.strToProtocol(buf, m_app_pro) != 0)
        return -1;

    const char* p1 = p + 1;
    p = strchr(p1, ':');
    if(p == NULL || p - p1 >= 24)
        return -1;
    memcpy(m_app_ip, p1, p - p1);
    m_app_ip[p-p1] = 0;

    p1 = p + 1;
    m_app_port = (uint16_t)atol(p1);
    if(m_app_port <= 0)
        return -1;
    return 0;
}

int32_t CMClient::parseServer(const char* map_server)
{
    char* p = (char*)map_server;
    char* p1 = strchr(p, ':');
    if(p1 == NULL || p1 - p >= 24)
        return -1;
    char* server_ip = m_server_ip[m_server_num];
    memcpy(server_ip, p, p1-p);
    server_ip[p1-p] = 0;

    p = p1 + 1;
    p1 = strchr(p, ':');
    if(p1 == NULL)
        return -1;
    m_server_tcp_port[m_server_num] = (uint16_t)atol(p);

    p = p1 + 1;
    m_server_udp_port[m_server_num] = (uint16_t)atol(p);

    return 0;
}

int32_t CMClient::getTree(void*& tree, time_t& update_time)
{
    getTreeInput in;
    getTreeOutput out;
    in.type = cmd_get_tree;
    in.tree_time = m_tree_time;
    in.tree_no = m_tree_no;
    in.tree = (CMTree*)tree;	
    if(m_cClient.process(&in, &out) != 0) {
        tree = NULL;
        return -1;
    }
    tree = (void*)out.tree;
    update_time = m_update_time;
    return 0;
}

int32_t CMClient::display()
{
    displayInput in;
    displayOutput out;
    in.type = cmd_display_all;
    if(m_cClient.process(&in, &out) != 0) {
        printf("display all failed\n");
        return -1;
    }
    CProtocol cPro;
    CNodetype cType;
    for(int32_t i = 0; i < out.node_num; i++) {
        printf("%s %s:%s:%u\n", cType.nodetypeToStr(out.cpNode[i].m_type), 
                cPro.protocolToStr(out.cpNode[i].m_protocol), out.cpNode[i].m_ip, out.cpNode[i].m_port);
    }
    return 0;
}

int32_t CMClient::getCMTCPServer(std::vector<std::string> &server_spec)
{
    std::string s, ip;
    char port[2][10];
    for(int32_t i = 0; i < m_server_num; i++) {
        sprintf(port[i], ":%u", m_server_tcp_port[i]);
        ip = m_server_ip[i]; 
        s = "tcp:";
        s += ip;
        s += port[i];
        server_spec.push_back(s);
    }
    return 0;
}    

int32_t CMClient::getSubClusterCount()
{
    getNodesInput in;
    getNodesOutput out;

    in.type = cmd_get_clustercount;

    if(m_cClient.process(&in, &out) != 0) {
        return 0;
    }	else {
        return out.size;
    }
}

int32_t CMClient::getAllNodesInOneSubCluster(int32_t subcluster_num, int32_t*& ids, int32_t &size, enodetype nodetype)
{
    getNodesInput in;
    getNodesOutput out;

    in.type = cmd_get_allnodes_col;
    in.node_type = nodetype;
    in.subcluster_num = subcluster_num;

    if(m_cClient.process(&in, &out) != 0) {
        size = 0;
        ids = NULL;
        return -1;
    }	else {
        size = out.size;
        ids = out.ids;
        return 0;
    }

    return -1;
}

int32_t CMClient::getNormalNodesInOneSubCluster(int32_t subcluster_num, int32_t*& ids, int32_t &size, enodetype nodetype)
{
    getNodesInput in;
    getNodesOutput out;

    in.type = cmd_get_normalnodes_col;
    in.node_type = nodetype;
    in.subcluster_num = subcluster_num;

    if(m_cClient.process(&in, &out) != 0) {
        size = 0;
        ids = NULL;
        return -1;
    }	else {
        size = out.size;
        ids = out.ids;
        return 0;
    }

    return -1;
}

int32_t CMClient::freeSubNodes(int32_t* ids)
{
    if(ids == NULL)
        return 0;
    free(ids);
    ids = NULL;
    return 0;
}

void* CMClient::thread_report(void* vpPara)
{
    CMClient* cpClient = (CMClient*)vpPara;
    int qps = 0;
    int latency = 0;

    while(cpClient->m_stop_flag == 0)
    {
        WatchPoint* cpWatch = cpClient->m_cpWatchPonit;
        while(cpWatch) {
            if(!cpWatch->isOK()) {
                cpClient->m_state = 1;
                TLOG("WatchPoint failed:%s", cpWatch->printMessage().c_str());
                break;
            }
            if(cpWatch->type == SERVER_WATCH) {
                ServerWatchPoint* w = (ServerWatchPoint*)cpWatch;
                w->getCurrentResource();
                qps = w->_qps;
                latency = w->_latency;
            }
            cpWatch = cpWatch->next;
        }
        if(cpWatch == NULL)	cpClient->m_state = 0;

        CMISNode* cpNode = cpClient->getNodeInfo(-1);
        if(cpNode == NULL) {
            usleep(cpClient->m_report_period);
            continue;
        }

        StatInput in;
        StatOutput out;

        in.type = msg_state_heartbeat;
        strcpy(in.ip, cpNode->m_ip);
        in.protocol = cpNode->m_protocol;
        in.port = cpNode->m_port;
        in.state = cpClient->m_state;
        in.tree_time = cpClient->m_tree_time;
        in.tree_no = cpClient->m_tree_no;
        in.load_one = cpClient->m_cSysloadWatchPoint.getCurrentResource();

        CPUWatchResult res;
        res.value = cpClient->m_cCPUWatchPoint.getCurrentResource();
        in.cpu_busy = res.sVal.cpu_busy;
        in.io_wait  = res.sVal.io_wait;
        in.latency  = latency;
        in.qps      = qps;
        if(in.nodeToBuf() <= 0)
            continue;

        CMClientInterface* cpInterface = cpClient->m_cpClientHead;
        while(cpInterface){
            cpInterface->reportState(&in, &out);
            cpInterface = cpInterface->next;
        }
        usleep(cpClient->m_report_period);
    }

    pthread_mutex_lock(&cpClient->m_lock);
    cpClient->m_stop_flag++;
    pthread_mutex_unlock(&cpClient->m_lock);

    return NULL;
}

void* CMClient::thread_udp_recv(void* vpPara)
{
    CMClient* cpClass = (CMClient*)vpPara;

    int32_t ret_len, last_sub_time = 0, sub_time = 0, count = 0;
    char buffer[MAX_UDP_PACKET_SIZE];

    while(cpClass->m_stop_flag == 0)
    {
        ret_len = cpClass->m_udpserver.receive(buffer, MAX_UDP_PACKET_SIZE);
        if(cpClass->m_stop_flag)
            break;
        eMessageType type = (eMessageType)(*buffer);
        sub_time = ntohl(*(int32_t*)(buffer+1));
        cpClass->m_update_time = time(NULL);
        if(type == msg_unvalid)
            continue;		
        if(type == msg_reinit) {
            count++;
            TLOG("recv reinit message -- %d, lasttime=%u, nowtime=%u", getpid(), last_sub_time, sub_time);
            if(sub_time != last_sub_time || count > 10) { 
                if(cpClass->m_cClient.reload()) {
                    last_sub_time = sub_time;
                    count = 0;
                    cpClass->m_tree_time = sub_time;
                    cpClass->m_tree_no++;
                    TLOG("client reload success");
                }else {
                    TLOG("client reload failed");
                }
            }
        } else {
            count = 0;
            cpClass->setSubClusterStation(buffer+5, ret_len-5, ret_len-5);
        }
    }

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

void* CMClient::thread_monitor(void* vpPara)
{
    CMClient* cpClass = (CMClient*)vpPara;

    char* _pipe_path = "mmm";
    umask(0);
    if((mkfifo(_pipe_path, (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) < 0) && 
            (errno != EEXIST)) {
        return NULL;
    }

    char c;
    int fd, n;
    fd = open(_pipe_path, O_RDONLY);
    if(fd < 0) return NULL;

    while(cpClass->m_stop_flag == 0)
    {
        if((n=read(fd, &c, 1)) < 0 && errno != EAGAIN) break;
        if(cpClass->m_stop_flag) break;
        if(n != 1) {
            sleep(1);
            continue;
        }
        if(c == 'a')
            cpClass->display();
    }
    close(fd);

    pthread_mutex_lock(&cpClass->m_lock);
    cpClass->m_stop_flag++;
    pthread_mutex_unlock(&cpClass->m_lock);

    return NULL;
}

}
