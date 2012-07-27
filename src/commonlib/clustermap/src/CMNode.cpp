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
 * $Id: CMNode.cpp 11379 2012-04-18 09:14:23Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <queue>
#include <arpa/inet.h>
#include <sys/time.h>
#include "CMNode.h"

namespace clustermap {

CProtocol::CProtocol() {
    m_protocol[tcp] = "tcp";
    m_protocol[udp] = "udp";
    m_protocol[http] = "http";
}
int32_t CProtocol::strToProtocol(const char* str, eprotocol& protocol)
{
    if(strcasecmp(str, m_protocol[tcp]) == 0)
        protocol = tcp;
    else if(strcasecmp(str, m_protocol[udp]) == 0)
        protocol = udp;
    else if (strcasecmp(str, m_protocol[http]) == 0)
        protocol = http; 
    else    
        return -1;
    return 0;
}
char* CProtocol::protocolToStr(eprotocol protocol) {
    return m_protocol[protocol];
}

CNodetype::CNodetype() {
    m_nodetype[blender] = "blender";
    m_nodetype[merger] = "merger";
    m_nodetype[search] = "search";
    m_nodetype[search_doc] = "doc";
    m_nodetype[search_mix] = "mix";
}
int32_t CNodetype::strToNodeType(const char* str, enodetype& protocol)
{
    if(strcasecmp(str, m_nodetype[blender]) == 0)
        protocol = blender;
    else if(strcasecmp(str, m_nodetype[merger]) == 0)
        protocol = merger;
    else if(strcasecmp(str, m_nodetype[search]) == 0)
        protocol = search;
    else if(strcasecmp(str, m_nodetype[search_doc]) == 0)
        protocol = search_doc;
    else if(strcasecmp(str, m_nodetype[search_mix]) == 0)
        protocol = search_mix;
    else
        return -1;
    return 0;
}
char* CNodetype::nodetypeToStr(enodetype nodetype) {
    return m_nodetype[nodetype];
}

CMISNode::CMISNode()
{
    m_nodeID = -1;
    m_relationid = -1;
    m_localid = -1;	

    m_ip[0] = 0;
    m_clustername = NULL;

    m_startTime = 0;
    m_dead_begin = 0;
    m_dead_end = 0;
    m_dead_time = 0;
    sub_child = NULL;
    sub_selfcluster = NULL;

    m_state.valid = 1;
    m_state.valid_time = 0;
    m_state.state = abnormal;
    m_state.laststate = uninit;
    gettimeofday(&m_state.nDate, NULL);
    m_cpu_busy = 0;
    m_load_one = 0;
    m_io_wait  = 0;
    m_qps      = 0;
    m_latency  = 0;
    m_err_start = 0;
}

CMISNode::~CMISNode() {
    if (sub_child)
        delete sub_child;
    sub_child = NULL;
    if (sub_selfcluster)
        delete sub_selfcluster;
    sub_selfcluster = NULL;
}

int32_t CMISNode::nodeToBuf(char* buf)
{
    char *p = buf;

    *p = (char)m_type;
    p += 1;

    *(int32_t*)p = htonl(m_nodeID);  
    p += 4;

    *(int32_t*)p = htonl(m_relationid);  
    p += 4;

    *(int32_t*)p = htonl(m_startTime);
    p += 4;
    *(int32_t*)p = htonl(m_dead_begin);
    p += 4;
    *(int32_t*)p = htonl(m_dead_end);
    p += 4;
    *(int32_t*)p = htonl(m_dead_time);
    p += 4;

    *p = (char)m_protocol;
    p += 1;

    strcpy(p, m_ip);
    p += 24;

    *(uint16_t*)p = htons(m_port);
    p += 2;

    if(sub_child) {
        *p = 1;
        p += 1;
        memcpy(p, sub_child->sub_ip, 24);
        p += 24;
        *(uint16_t*)p = htons(sub_child->sub_port);
        p += 2;
    }else {
        *p = 0;
        p += 1;
    }

    if (sub_selfcluster) {
        *p = 1;
        p += 1;
        memcpy(p, sub_selfcluster->sub_ip, 24);
        p += 24;
        *(uint16_t*)p = htons(sub_selfcluster->sub_port);
        p += 2;
    }else {
        *p = 0;
        p += 1;
    }

    if(m_state.valid)	
        *p = m_state.state;
    else
        *p = (char)unvalid;
    p += 1;
    *p = m_state.laststate;
    p += 1;
    *(int32_t*)p = htonl(m_state.nDate.tv_sec);  
    p += 4;
    *(uint32_t*)p = htonl(m_cpu_busy);
    p += 4;
    *(uint32_t*)p = htonl(m_load_one);
    p += 4;
    *(uint32_t*)p = htonl(m_io_wait);
    p += 4;
    *(uint32_t*)p = htonl(m_qps);
    p += 4;
    *(uint32_t*)p = htonl(m_latency);
    p += 4;
    return p - buf;
}

int32_t CMISNode::bufToNode(char* buf)
{
    char *p = buf;

    m_type = (enodetype)(*p);
    p += 1;

    m_nodeID = ntohl(*(int32_t*)p); 
    p += 4;

    m_relationid = ntohl(*(int32_t*)p);  
    p += 4;

    m_startTime = ntohl(*(int32_t*)p);
    p += 4;
    m_dead_begin = ntohl(*(int32_t*)p);
    p += 4;
    m_dead_end = ntohl(*(int32_t*)p);
    p += 4;
    m_dead_time = ntohl(*(int32_t*)p);
    p += 4;

    m_protocol = (eprotocol)(*p);
    p += 1;

    memcpy(m_ip, p, 24);
    p += 24;

    m_port = ntohs(*(uint16_t*)p);
    p += 2;

    if (*p) {
        sub_child = new CSubInfo;
        if(sub_child == NULL)
            return -1;
        p += 1;
        memcpy(sub_child->sub_ip, p, 24);
        p += 24;
        sub_child->sub_port = ntohs(*(uint16_t*)p);
        p += 2;
    } else {
        sub_child = NULL;
        p += 1;
    }

    if (*p) {
        sub_selfcluster = new CSubInfo;
        if (sub_selfcluster == NULL)
            return -1;
        p += 1;
        memcpy(sub_selfcluster->sub_ip, p, 24);
        p += 24;
        sub_selfcluster->sub_port = ntohs(*(uint16_t*)p);
        p += 2;
    } else {
        sub_selfcluster = NULL;
        p += 1;
    }

    m_state.state = *p;
    p += 1;
    m_state.laststate = *p;
    p += 1;
    m_state.nDate.tv_sec = ntohl(*(int32_t*)p);
    m_state.nDate.tv_usec = 0;
    p += 4;
    m_cpu_busy = ntohl(*(uint32_t*)p);
    p += 4;
    m_load_one = ntohl(*(uint32_t*)p);
    p += 4;
    m_io_wait = ntohl(*(uint32_t*)p);
    p += 4;
    m_qps = ntohl(*(uint32_t*)p);
    p += 4;
    m_latency = ntohl(*(uint32_t*)p);
    p += 4;
    
    return p - buf;
}

int32_t CMISNode::getSize()
{
    return sizeof(CMISNode) + sizeof(CSubInfo);
}

bool CMISNode::isNodeStateNormal()
{
    return m_state.state == normal;
}

CMCluster::CMCluster()
{
    m_clusterID = 0;
    m_name[0] = 0;
    m_type = search_cluster;
    level = 1;
    docsep = false;

    limit.cpu_limit = 0xfffffff;
    limit.load_one_limit = 0xfffffff;
    limit.io_wait_limit = 0xfffffff;     // 
    limit.offline_limit = 0;
    limit.qps_limit = 0;
    limit.latency_limit = 0; 
    limit.hold_time = 0xfffffff;

    node_num = 0;
    m_cmisnode = NULL;
    selfcluster_state = NULL;
    m_state_cluster = GREEN;
    m_state_cluster_manual = WHITE;
    m_state_cluster_manual_time = 0;

    cluster_yellow_cpu = 0xfffffff;
    cluster_yellow_load = 0xfffffff;
    cluster_yellow_count = 0xfffffff;
    cluster_yellow_percent = 0xfffffff;

    cluster_red_cpu = 0xfffffff;
    cluster_red_load = 0xfffffff;
    cluster_red_count = 0xfffffff;
    cluster_red_percent = 0xfffffff;
}

int32_t CMCluster::nodeToBuf(char* buf) {
    char* p = buf;

    *(int32_t*)p = htonl(m_clusterID);
    p += 4;
    memcpy(p, m_name, 64);
    p += 64;	
    *p = (char)m_type;
    p += 1;
    if(docsep)
        *p = 1;
    else 
        *p = 0;
    p += 1;
    *(int32_t*)p = htonl(level);
    p += 4;
    *(int32_t*)p = htonl(node_num);
    p += 4;
    if (m_state_cluster_manual == WHITE)
        *p = (char)m_state_cluster;
    else
        *p = (char)m_state_cluster_manual;
    p += 1;
    *(int32_t*)p = htonl(m_state_cluster_manual_time);
    p += 4;

    return p - buf;
}

int32_t CMCluster::bufToNode(char* buf) {
    char* p = buf;

    m_clusterID = ntohl(*(int32_t*)p);
    p += 4; 
    memcpy(m_name, p, 64);
    p += 64;  
    m_type = (eclustertype)*p;
    p += 1; 
    if(*p)
        docsep = true;
    else
        docsep = false;
    p += 1; 
    level = ntohl(*(int32_t*)p);
    p += 4;
    node_num = ntohl(*(int32_t*)p);
    p += 4;
    m_state_cluster = (estate_cluster)*p;
    p += 1;
    m_state_cluster_manual_time = ntohl(*(int32_t*)p);
    p += 4;

    m_state_cluster_manual = WHITE;

    return p - buf;
}
int32_t CMCluster::getSize() {
    return sizeof(CMCluster);
}
int32_t CMRelation::nodeToBuf(char* buf) {
    char* p = buf;
    if(cpCluster)
        memcpy(p, cpCluster->m_name, 64);
    else
        memset(p, 0, 64);
    p += 64;

    if(parent && parent->cpCluster)
        memcpy(p, parent->cpCluster->m_name, 64);
    else
        memset(p, 0, 64);
    p += 64;

    if(child && child->cpCluster)
        memcpy(p, child->cpCluster->m_name, 64);
    else
        memset(p, 0, 64);
    p += 64;
    if(sibling && sibling->cpCluster)
        memcpy(p, sibling->cpCluster->m_name, 64);
    else
        memset(p, 0, 64);
    p += 64;
    return p - buf;
}
int32_t CMRelation::getSize() {
    return 64 * 4;
}

}
