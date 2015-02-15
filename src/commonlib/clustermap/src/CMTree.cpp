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
 * $Id: CMTree.cpp 12106 2012-05-21 12:15:40Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include "util/MD5.h"
#include "util/Log.h"
#include "CMTree.h"
#include "CMClientInterface.h"

namespace clustermap {

int32_t ComInput::bufToNode(char* in_buf, int32_t in_len) {
    type = (eMessageType)(*in_buf);
    return 1;
}
    int32_t ComInput::nodeToBuf() {
        if(buf == NULL) 
            buf = new char[2];
        if(buf == NULL)
            return -1;
        *buf = (char)type;
        len = 1;
        return len;
    }

int32_t InitInput::bufToNode(char* in_buf, int32_t in_len){
    char* p = in_buf;

    type = (eMessageType)*p;
    p += 1;
    strncpy(ip, p, 24);
    ip[23] = 0;
    p += 24;
    port = ntohs(*(uint16_t*)p);
    p += 2;
    protocol = (eprotocol)(*p);
    p += 1;
    return p - in_buf;
}
int32_t InitInput::nodeToBuf(){
    len = sizeof(InitInput) + 24;
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf;

    *p = (char)type;
    p += 1;
    strncpy(p, ip, 24);
    p[23] = 0;
    p += 24;
    *(uint16_t*)p = htons(port);
    p += 2;
    *p = protocol;
    p += 1;
    len = p - buf;
    return len; 
}

int32_t RegInput::bufToNode(char* in_buf, int32_t in_len){
    char* p = in_buf;

    type = (eMessageType)*p;
    p += 1;
    strncpy(ip, p, 24);
    ip[23] = 0;
    p += 24;
    port = ntohs(*(uint16_t*)p);
    p += 2;
    protocol = (eprotocol)(*p);
    p += 1;
    return p-in_buf;
}
int32_t RegInput::nodeToBuf(){
    len = sizeof(RegInput) + 24;
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    strncpy(p, ip, 24);
    p[23] = 0;
    p += 24;
    *(uint16_t*)p = htons(port);
    p += 2;
    *p = protocol;
    p += 1;
    len = p-buf;
    return len;
}

int32_t SubInput::bufToNode(char* in_buf, int32_t in_len){
    char* p = in_buf;

    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;
    strncpy(ip, p, 24);
    ip[23] = 0;
    p += 24;
    port = ntohs(*(uint16_t*)p);
    p += 2;
    protocol = (eprotocol)(*p);
    p += 1;
    sub_port = ntohs(*(uint16_t*)p);
    p += 2;
    op_time = ntohl(*(uint32_t*)p);
    p += 4;
    return p - in_buf;
}
int32_t SubInput::nodeToBuf(){
    len = sizeof(SubInput) + 24;
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    strncpy(p, ip, 24);
    p[23] = 0;
    p += 24;
    *(uint16_t*)p = htons(port);
    p += 2;
    *p = protocol;
    p += 1;
    *(uint16_t*)p = htons(sub_port);
    p += 2;
    *(uint32_t*)p = htonl(op_time);
    p += 4;
    len = p - buf;
    return len;
}

int32_t StatInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;
    char* end = in_buf + in_len;
    
    if(p+1 > end) return -1;
    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;

    if(p+4 > end) return -1;
    tree_time = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+4 > end) return -1;
    tree_no = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+24 > end) return -1;
    strncpy(ip, p, 24);
    ip[23] = 0;
    p += 24;

    if(p+2 > end) return -1;
    port = ntohs(*(uint16_t*)p);
    p += 2;

    if(p+1 > end) return -1;
    protocol = (eprotocol)(*p);
    p += 1;

    if(p+1 > end) return -1;
    state = *p;
    p += 1;

    if(p+4 > end) return -1;
    cpu_busy = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+4 > end) return -1;
    load_one = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+4 > end) return -1;
    io_wait = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+4 > end) return -1;
    latency = ntohl(*(uint32_t*)p);
    p += 4;

    if(p+4 > end) return -1;
    qps = ntohl(*(uint32_t*)p);
    p += 4;
    return p - in_buf;
}
int32_t StatInput::nodeToBuf() {
    len = sizeof(StatInput)+24;
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    *(uint32_t*)p = htonl(tree_time);
    p += 4;
    *(uint32_t*)p = htonl(tree_no);
    p += 4;
    strncpy(p, ip, 24);
    p[23] = 0;
    p += 24;
    *(uint16_t*)p = htons(port);
    p += 2;
    *p = protocol;
    p += 1;
    *p = state;
    p += 1;
    *(uint32_t*)p = htonl(cpu_busy);
    p += 4;
    *(uint32_t*)p = htonl(load_one);
    p += 4;
    *(uint32_t*)p = htonl(io_wait);
    p += 4;
    *(uint32_t*)p = htonl(latency);
    p += 4;
    *(uint32_t*)p = htonl(qps);
    p += 4;

    len = p - buf;
    return len;
}

int32_t ValidInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;

    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;
    strncpy(ip, p, 24);
    ip[23] = 0;
    p += 24;
    port = ntohs(*(uint16_t*)p);
    p += 2;
    protocol = (eprotocol)(*p);
    p += 1;
    valid = *p;
    p += 1;
    op_time.tv_sec = ntohl(*(int32_t*)p);
    p += 4;
    op_time.tv_usec = ntohl(*(int32_t*)p);
    p += 4;

    return p - in_buf;
}
int32_t ValidInput::nodeToBuf() {
    len = sizeof(ValidInput) + 24;
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    strncpy(p, ip, 24);
    p[23] = 0;
    p += 24;
    *(uint16_t*)p = htons(port);
    p += 2;
    *p = protocol;
    p += 1;
    *p = valid;
    p += 1;
    *(int32_t*)p = htonl(op_time.tv_sec);
    p += 4;
    *(int32_t*)p = htonl(op_time.tv_usec);
    p += 4;	
    len = p - buf;
    return len;
}

int32_t ClusterStateInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;

    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;
    strncpy(clustername, p, 64);
    clustername[63] = 0;
    p += 64;
    ClusterState = *p;
    p += 1;
    if (ClusterState == WHITE){
        strcpy(sClusterState, "WHITE");
    } else if (ClusterState == GREEN) {
        strcpy(sClusterState, "GREEN");
    } else if (ClusterState == YELLOW) {
        strcpy(sClusterState, "YELLOW");
    } else if (ClusterState == RED) {
        strcpy(sClusterState, "RED");
    } else {
        TERR("in ClusterStateInput::bufToNode(): ClusterState (%c) ERR!", ClusterState);
    }

    op_time.tv_sec = ntohl(*(int32_t*)p);
    p += 4;
    op_time.tv_usec = ntohl(*(int32_t*)p);
    p += 4;

    return p - in_buf;
}
int32_t ClusterStateInput::nodeToBuf() {
    len = sizeof(ClusterStateInput);
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    strncpy(p, clustername, 64);
    p[63] = 0;
    p += 64;
    *p = ClusterState;
    p += 1;
    *(int32_t*)p = htonl(op_time.tv_sec);
    p += 4;
    *(int32_t*)p = htonl(op_time.tv_usec);
    p += 4;	
    len = p - buf;
    return len;
}

int32_t EnvVarInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;

    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;
    EnvVarCommand = ntohl(*(int32_t*)p);
    p +=4;
    strncpy(EnvVar_Key, p, 32);
    EnvVar_Key[31] = 0;
    p += 32;
    strncpy(EnvVar_Value, p, 32);
    EnvVar_Value[31] = 0;
    p += 32;
    op_time.tv_sec = ntohl(*(int32_t*)p);
    p += 4;
    op_time.tv_usec = ntohl(*(int32_t*)p);
    p += 4;

    return p - in_buf;
}
int32_t EnvVarInput::nodeToBuf() {
    len = sizeof(EnvVarInput);
    if(buf == NULL) 
        buf = new char[len+1];
    if(buf == NULL)
        return -1;
    char *p = buf; 

    *p = (char)type;
    p += 1;
    *(int32_t*)p = htonl(EnvVarCommand);
    p += 4;
    strncpy(p, EnvVar_Key, 32);
    p[31] = 0;
    p += 32;
    strncpy(p, EnvVar_Value, 32);
    p[31] = 0;
    p += 32;
    *(int32_t*)p = htonl(op_time.tv_sec);
    p += 4;
    *(int32_t*)p = htonl(op_time.tv_usec);
    p += 4;	
    len = p - buf;
    return len;
}

int32_t EnvVarOutput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;

    type = (eMessageType)*p;
    if(type == msg_unvalid)
        return -1;
    p += 1;
    strncpy(EnvVar_Key, p, 32);
    EnvVar_Key[31] = 0;
    p += 32;
    strncpy(EnvVar_Value, p, 32);
    EnvVar_Value[31] = 0;
    p += 32;

    return p - in_buf;
}
int32_t EnvVarOutput::nodeToBuf() {
    outlen = sizeof(EnvVarInput);
    if(outbuf == NULL) 
        outbuf = new char[outlen+1];
    if(outbuf == NULL)
        return -1;
    char *p = outbuf; 

    *p = (char)type;
    p += 1;
    strncpy(p, EnvVar_Key, 32);
    p[31] = 0;
    p += 32;
    strncpy(p, EnvVar_Value, 32);
    p[31] = 0;
    p += 32;
    outlen = p - outbuf;
    return outlen;
}

int32_t setmapInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;
    type = (eMessageType)*p;
    p += 1;
    op_time = ntohl(*(uint32_t*)p);
    p += 4;

    len = in_len;
    if(buf == NULL)
        delete[] buf;
    buf = new char[in_len];
    if(buf == NULL)
        return -1;
    mapbuf = buf;
    maplen = in_len - (p - in_buf);
    memcpy(mapbuf, p, maplen);

    return in_len;
}
int32_t setmapInput::nodeToBuf() {
    struct stat st;
    if(stat(mapbuf, &st))
        return -1;
    maplen = st.st_size;
    len = maplen + 5;
    buf = new char[len];
    if(buf == NULL)
        return -1;
    FILE* fp = fopen(mapbuf, "rb");
    if(fp == NULL)
        return -1;
    char* p = buf;
    *p = (char)type;
    p += 1;
    *(uint32_t*)p = htonl(op_time);
    p += 4;
    fread(p, maplen, 1, fp);
    fclose(fp);
    return len;
}

int32_t getmapInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf; 
    type = (eMessageType)*p;
    p += 1;
    op_time = ntohl(*(uint32_t*)p);
    p += 4;
    return in_len; 
}
int32_t getmapInput::nodeToBuf() {
    len = 5;
    buf = new char[len];
    if(buf == NULL)
        return -1;
    char* p = buf;
    *p = (char)type;
    p++;
    *(uint32_t*)p = htonl(op_time);
    p += 4;
    return len;
}

int32_t getNodeInput::nodeToBuf() {
    len = sizeof(getNodeInput) + 64 + 1;
    buf = new char[len];
    if(buf == NULL)
        return -1;
    char* p1 = buf;
    *p1 = (char)type;
    p1 += 1;
    if(type == cmd_get_info_by_spec) {
        strncpy(p1, ip, 23);
        p1[23] = 0;
        p1 += 24;
        *(uint16_t*)p1 = htons(port);
        p1 += 2;
        *p1 = protocol;
        p1 += 1;
    } else if(type == cmd_get_info_by_name) {
        if(name == NULL || strlen(name) >= 64)
            return -1;
        memcpy(p1, name, 64);
        p1 += 64;
    } else if (type == cmd_get_info_allbad) {

    } else if (type == cmd_get_info_watchpoint) {

    } else if (type == cmd_get_info_allauto) {

    } else {
        return -1;
    }
    return p1 - buf;
}

int32_t getNodeInput::bufToNode(char* in_buf, int32_t in_len) {
    char* p = in_buf;
    type = (eMessageType)(*p);
    p += 1;
    if(type == cmd_get_info_by_spec) {
        strncpy(ip, p, 24);
        ip[23] = 0;
        p += 24;
        port = ntohs(*(uint16_t*)p);
        p += 2;
        protocol = (eprotocol)(*p);
        p += 1;
    } else if(type == cmd_get_info_by_name) {
        memcpy(name, p, 64);
        name[63] = 0;
        p += 64;
    } else if (type == cmd_get_info_allbad) {

    } else if (type == cmd_get_info_watchpoint) {

    } else if (type == cmd_get_info_allauto) {

    } else {
        return -1;
    }
    return p - in_buf;
}

int32_t getNodeOutput::bufToNode(char* in_buf, int32_t in_len) {
    char* p1 = in_buf;
    type = (eMessageType)(*p1);
    if(type == msg_unvalid)
        return -1;
    p1 += 1;
    node_num = ntohl(*(int32_t*)p1);
    p1 += 4;

    if(cpCluster == NULL)
        cpCluster = new CMCluster;
    if(cpNode)
        delete[] cpNode;
    cpNode = new CMISNode[node_num];
    if(cpNode == NULL || cpCluster == NULL)
        return -1;
    p1 += cpCluster->bufToNode(p1);
    for(int32_t i = 0; i < node_num; i++) {
        p1 += cpNode[i].bufToNode(p1);
        cpNode[i].m_clustername = cpCluster->m_name;
    }
    return p1 - in_buf;
}

CMTree::CMTree()
{
    //printf("CMTree new\n");
    m_cTagCluster[0].list_name = "blender_list";
    m_cTagCluster[0].cluter_name = "blender_cluster";
    m_cTagCluster[1].list_name = "merger_list";
    m_cTagCluster[1].cluter_name = "merger_cluster";
    m_cTagCluster[2].list_name = "search_list";
    m_cTagCluster[2].cluter_name = "search_cluster";

    m_cTagRelation[0].list_name = "merger_cluster_list";
    m_cTagRelation[0].cluter_name = "merger_cluster";
    m_cTagRelation[1].list_name = "blender_cluster_list";
    m_cTagRelation[1].cluter_name = "blender_cluster";

    m_cTagMISNode[0] = "blender";
    m_cTagMISNode[1] = "merger";
    m_cTagMISNode[2] = "search";
    m_cTagMISNode[3] = "doc";
    m_cTagMISNode[4] = "mix";

    m_max_node_num = 0;
    m_tree_time = 0;
    m_tree_no = 0;
    m_watchOnOff = 0;

    m_szpHead = NULL;
    m_cpSubAll = NULL;
    m_left_buf_size = -1;
    pthread_mutex_init(&m_file_lock, NULL);
    pthread_mutex_init(&m_mem_lock, NULL);
    pthread_mutex_init(&m_state_lock, NULL);
}

CMTree::~CMTree()
{
    //printf("CMTree del\n");
    char* p = m_szpHead, *bak;
    while(p) {
        bak = p;
        p = *(char**)p;
        delete[] bak;
    }
    pthread_mutex_destroy(&m_file_lock);
    pthread_mutex_destroy(&m_mem_lock);
    pthread_mutex_destroy(&m_state_lock);
}

int32_t CMTree::init(const char *conffile, char type, uint32_t tree_time, uint32_t No) 
{
    m_tree_no = No;
    m_tree_type = type;
    m_tree_time = tree_time;
    strcpy(m_cfg_file_name, conffile);

    if(type == 1) {
        m_sys_file = "m_clustermap.sys";
        m_sys_transin_file = "s_clustermap.in.sys";
        m_sys_bak_file = "m_clustermap.bak.sys";
    }	else {
        m_sys_file = "s_clustermap.sys";
        m_sys_transin_file = "m_clustermap.in.sys";
        m_sys_bak_file = "s_clustermap.bak.sys";
    }

    FILE* fp = fopen(conffile, "r");
    if(fp == NULL)
    {
        TLOG("配置文件 %s 打开出错, 文件可能不存在.\n", conffile);
        return -1;
    }
    mxml_node_t* pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    if(pXMLTree == NULL)
    {
        mxmlDelete(pXMLTree);
        TLOG( "配置文件 %s 格式有错, 请根据错误提示修正您的配置文件.\n", conffile);
        return -1;
    }
    fclose(fp);

    m_cRoot.cpCluster = allocMClusterNode();
    if(m_cRoot.cpCluster == NULL) {
        mxmlDelete(pXMLTree);
        return -1;
    }
    strcpy(m_cRoot.cpCluster->m_name, "clustermap");

    mxml_node_t* pRoot = mxmlFindElement(pXMLTree, pXMLTree, m_cRoot.cpCluster->m_name, NULL, NULL, MXML_DESCEND);
    if(pRoot == NULL)
    {
        TLOG("not find head flag clustermap\n");
        mxmlDelete(pXMLTree);
        return -1;
    }

    std::map<std::string, CMRelation*> root_map;
    for(int i = 0; i < TAG_CLUSTER_NUM; i++)
    {
        int32_t ret = getOneList(pRoot, m_cTagCluster[i].list_name, m_cTagCluster[i].cluter_name, root_map, 0);
        if(ret == -2) {
            TDEBUG("not find list_name %s, cluster_name %s\n", m_cTagCluster[i].list_name, m_cTagCluster[i].cluter_name);
        } else if(ret != 0){
            mxmlDelete(pXMLTree);
            return -1;
        }
    }
    for(int i = 0; i < TAG_RELATION_NUM; i++)
    {
        int32_t ret = getOneList(pRoot, m_cTagRelation[i].list_name, m_cTagRelation[i].cluter_name, root_map, 1);
        if(ret == -2) {
            TDEBUG("not find %s %s \n", m_cTagRelation[i].list_name, m_cTagRelation[i].cluter_name);
        } else if(ret != 0) {
            mxmlDelete(pXMLTree);
            return -1;
        }
    }
    mxmlDelete(pXMLTree);

    std::map<std::string, CMRelation*>::iterator iter;
    CMRelation** simbling = &m_cRoot.child;
    for(iter = root_map.begin(); iter != root_map.end(); iter++) {
        *simbling = iter->second;
        (*simbling)->parent = &m_cRoot;
        simbling = &(*simbling)->sibling;
    }

    m_max_node_num = m_misnodeMap.size();
    m_max_cluster_num = m_clustermap.size();
    m_max_relation_num = m_relationMap.size();
    m_cppMISNode = (CMISNode**) allocBuf(sizeof(CMISNode*) * m_max_node_num);
    m_cppCluster = (CMCluster**)allocBuf(sizeof(CMCluster*) * m_max_cluster_num);
    m_cppRelation = (CMRelation**)allocBuf(sizeof(CMRelation*) * m_max_relation_num);
    if(!m_cppMISNode || !m_cppCluster || !m_cppRelation)
        return -1;
    m_cLastAllState = allocBuf(m_max_node_num);
    if(m_cLastAllState == NULL)
        return -1;
    m_cLastAllState_Cluster = allocBuf(m_max_cluster_num);
    if (m_cLastAllState_Cluster == NULL)
        return -1;

    int32_t i, j;	
    std::map<std::string, CMISNode*>::iterator iter1;
    for(i = 0, iter1 = m_misnodeMap.begin(); i < m_max_node_num; i++, iter1++){
        m_cppMISNode[i] = iter1->second;
        m_cppMISNode[i]->m_nodeID = i;
        m_cLastAllState[i] = m_cppMISNode[i]->m_state.state;
    }	
    std::map<std::string, CMCluster*>::iterator iter2;
    for(i = 0, iter2 = m_clustermap.begin(); i < m_max_cluster_num; i++, iter2++) {
        m_cppCluster[i] = iter2->second;
        m_cppCluster[i]->m_clusterID = i;
        m_cLastAllState_Cluster[i] = m_cppCluster[i]->m_state_cluster;
        for(j = 0; j < m_cppCluster[i]->node_num; j++)
            m_cppCluster[i]->m_cmisnode[j].m_clusterID = i;
    }
    std::map<std::string, CMRelation*>::iterator iter3;
    for(i = 0, iter3 = m_relationMap.begin(); i < m_max_relation_num; i++, iter3++){
        m_cppRelation[i] = iter3->second;
        if(statRelationNum(m_cppRelation[i], i) != 0)
            return -1;
    }
    travelTree(stdout);

    return 0;
}

int32_t CMTree::process(const MsgInput* input, MsgOutput* output)
{
    switch(input->type) {
        case msg_init:				// init
            return clientInit((MsgInput*)input, output);
        case msg_register:			// register
            return clientReg((MsgInput*)input, output);
        case msg_sub_child:			// subscriber child
            return clientSubChild((MsgInput*)input, output);
        case msg_sub_all:			// subscriber all
            return clientSubAll((MsgInput*)input, output);
        case msg_sub_selfcluster:   // subscriber self_cluster
            return clientSubSelfCluster((MsgInput*)input, output);
        case msg_state_heartbeat:	// state heart beat
            return setNodeState((MsgInput*)input, output);
        case msg_valid:				// logic delete or recorver
            return setNodeValid((MsgInput*)input, output);
        case msg_check_timeout:
            return checkTimeOut((MsgInput*)input, output);
        case msg_update_all:
        case msg_check_update:
            return getStateUpdate((MsgInput*)input, output); 
        case cmd_get_clustermap:
            return getClustermap((MsgInput*)input, output);
        case cmd_set_clustermap:
            return setClustermap((MsgInput*)input, output);
        case cmd_get_info_by_spec:
        case cmd_get_info_by_name:
        case cmd_get_info_allbad:
        case cmd_get_info_watchpoint:
            return getNodeInfo((MsgInput*)input, output);
        case cmd_get_info_allauto:
            return getNodeInfo_auto((MsgInput*)input, output);
        case msg_ClusterState:
            return setClusterState((MsgInput*)input, output);
        case msg_EnvVarCommand:
            return setEnvVarCommand((MsgInput*)input, output);
        case cmd_by_url:
            return get_info_by_url((MsgInput*)input, output); 
        default:
            return -1;
    }
    return -1;
}

int32_t CMTree::get_info_by_url(MsgInput* input, MsgOutput* output)
{
static const char* field = "cluster=";
static const int fieldLen = strlen(field);

    ComInput* in = (ComInput*)input;
    ComOutput* out = (ComOutput*)output;

    char* url = in->buf;
    char* cluster = strstr(url, field);
    if(NULL == cluster) {
        TERR("need cluster, req=%s", url);
        return -1;
    }
    cluster += fieldLen;
    char* p = cluster;
    while(*p && *p != '&' && !isspace(*p)) p++;
    
    char name[PATH_MAX];
    int len = p - cluster;
    memcpy(name, cluster, len);
    name[len] = 0;

    CMCluster* cm = NULL;
    int num = 0;
    
    if(getCluster(name, cm) < 0) {
        TERR("getCluster %s error", name);
        return -1;
    }
    out->outlen = cm->node_num * sizeof(ClusterInfo); 
    out->outbuf = new char[out->outlen];
    ClusterInfo* info = (ClusterInfo*)out->outbuf;

    for(int32_t i = 0; i < cm->node_num; i++, num++) {
        strcpy(info[i].ip, cm->m_cmisnode[i].m_ip);
        info[i].port = cm->m_cmisnode[i].m_port;
        info[i].protocol = cm->m_cmisnode[i].m_protocol;
        info[i].valid = cm->m_cmisnode[i].m_state.valid;
        info[i].state = cm->m_cmisnode[i].m_state.state;
        info[i].col_id = i;
    }

    return 0;
}

int32_t CMTree::clientInit(MsgInput* in, MsgOutput* out)
{
    InitInput* input = (InitInput*)in;
    InitOutput* output = (InitOutput*)out;

    CMISNode* cpNode;
    if(getNode(input->ip, input->port, input->protocol, cpNode) != 0)
        return -1;
    CMCluster* cpCluster = m_cppCluster[cpNode->m_clusterID];

    output->outlen = cpNode->getSize() + cpCluster->getSize();
    output->outbuf = new char[output->outlen+1];
    if(output->outbuf == NULL)
        return -1;

    char* p = output->outbuf;
    *p++ = (char)msg_success;
    p += cpCluster->nodeToBuf(p);
    p += cpNode->nodeToBuf(p);
    output->outlen = p - output->outbuf;

    return 0;
}

int32_t CMTree::clientReg(MsgInput* in, MsgOutput* out)
{
    RegInput* input = (RegInput*)in;
    RegOutput* output = (RegOutput*)out;

    CMISNode* cpNode;
    if(getNode(input->ip, input->port, input->protocol, cpNode) != 0)
        return -1;

    output->outbuf = new char[8+1];
    if(output->outbuf == NULL)
        return -1;
    char* p = output->outbuf;
    *p++ = (char)msg_success;
    *(uint32_t*)p = htonl(m_tree_time);
    p += 4;
    *(uint32_t*)p = htonl(m_tree_no);
    p += 4;
    output->outlen = p - output->outbuf;
    cpNode->m_state.state = (char)normal;

    return 0;
}

int32_t CMTree::clientSubChild(MsgInput* in, MsgOutput* out)
{
    SubInput* input = (SubInput*)in;
    SubOutput* output = (SubOutput*)out;

    CMISNode* cpNode;
    if(getNode(input->ip, input->port, input->protocol, cpNode) != 0 ||
            cpNode->m_relationid < 0)
        return -1;
    CMCluster* cpCluster = m_cppCluster[cpNode->m_clusterID];

    int32_t child_num = m_cppRelation[cpNode->m_relationid]->child_num;
    CMRelation* cpChild = m_cppRelation[cpNode->m_relationid]->child;

    int32_t total_node_num = 0;
    CMRelation* sibling = cpChild;
    for(int32_t i = 0; i < child_num; i++, sibling = sibling->sibling)
        total_node_num += sibling->cpCluster->node_num;
    output->outlen = total_node_num*cpNode->getSize() + child_num*cpCluster->getSize() + 20 + 1;
    output->outbuf = new char[output->outlen];
    if(output->outbuf == NULL)
        return -1;
    if(cpNode->sub_child == NULL) {
        cpNode->sub_child = (CSubInfo*)allocBuf(sizeof(CSubInfo));
        if(cpNode->sub_child == NULL)
            return -1;
    }

    char* p = output->outbuf;
    *p++ = (char)msg_success;

    *(int32_t*)p = htonl(child_num);
    p += 4;
    *(int32_t*)p = htonl(total_node_num);
    p += 4;
    *(int32_t*)p = 0;
    p += 4;
    *(int32_t*)p = htonl(m_tree_time);
    p += 4;
    *(int32_t*)p = htonl(m_tree_no);
    p += 4;

    sibling = cpChild;
    pthread_mutex_lock(&m_state_lock);
    for(int32_t i = 0; i < child_num; i++, sibling = sibling->sibling) {
        p += sibling->cpCluster->nodeToBuf(p);
        for(int32_t j = 0; j < sibling->cpCluster->node_num; j++)
            p += sibling->cpCluster->m_cmisnode[j].nodeToBuf(p);
    }
    pthread_mutex_unlock(&m_state_lock);
    output->outlen = p - output->outbuf;

    strcpy(cpNode->sub_child->sub_ip, input->ip);
    cpNode->sub_child->sub_port = input->sub_port;
    cpNode->sub_child->sub_time = input->op_time;
    cpNode->sub_child->state = m_cppRelation[cpNode->m_relationid]->child_state;
    cpNode->sub_child->state_num = total_node_num;
    cpNode->sub_child->subState = 1;

    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        pthread_mutex_unlock(&m_file_lock);
        return -1;
    }
    fprintf(fp, "%d#1:%s:%s:%u,%s:%u\n", (int32_t)cpNode->sub_child->sub_time,
            m_cprotocol.protocolToStr(input->protocol), input->ip, input->port, input->ip, input->sub_port);
    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    return 0;
}

int32_t CMTree::clientSubAll(MsgInput* in, MsgOutput* out)
{
    SubInput* input = (SubInput*)in;
    SubOutput* output = (SubOutput*)out;

    char key[64];
    sprintf(key, "%s:%u", input->ip, input->sub_port);

    CSubInfo* subInfo;
    pthread_mutex_lock(&m_state_lock);
    std::map<std::string, CSubInfo*>::iterator iter = m_subAllMap.find(key);
    if(iter == m_subAllMap.end()) {
        subInfo = (CSubInfo*)allocBuf(sizeof(CSubInfo));
        if(subInfo == NULL) {
            pthread_mutex_unlock(&m_state_lock);
            return -1;
        }
        subInfo->subState = 0;
        m_subAllMap[key] = subInfo;
        subInfo->next = m_cpSubAll;
        m_cpSubAll = subInfo;
    }	else {
        subInfo = iter->second;
    }

    strcpy(subInfo->sub_ip, input->ip);
    subInfo->sub_port = input->sub_port;
    subInfo->sub_time = input->op_time;
    subInfo->subState = 1;
    pthread_mutex_unlock(&m_state_lock);

    output->outlen = m_max_node_num*m_cppMISNode[0]->getSize() + m_max_cluster_num*m_cppCluster[0]->getSize() +	m_max_relation_num*m_cppRelation[0]->getSize() + 20 + 1;
    output->outbuf = new char[output->outlen];
    if(output->outbuf == NULL)
        return -1;

    char* p = output->outbuf;
    *p++ = (char)msg_success;

    *(int32_t*)p = htonl(m_max_cluster_num);
    p += 4;
    *(int32_t*)p = htonl(m_max_node_num);
    p += 4;
    *(int32_t*)p = htonl(m_max_relation_num);
    p += 4;
    *(int32_t*)p = htonl(m_tree_time);
    p += 4;
    *(int32_t*)p = htonl(m_tree_no);
    p += 4;

    pthread_mutex_lock(&m_state_lock);
    for(int32_t i = 0; i < m_max_cluster_num; i++) {
        p += m_cppCluster[i]->nodeToBuf(p);
        for(int32_t j = 0; j < m_cppCluster[i]->node_num; j++)
            p += m_cppCluster[i]->m_cmisnode[j].nodeToBuf(p);
    }
    for(int32_t i = 0; i < m_max_relation_num; i++)
        p += m_cppRelation[i]->nodeToBuf(p);
    pthread_mutex_unlock(&m_state_lock);
    output->outlen = p - output->outbuf;

    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        pthread_mutex_unlock(&m_file_lock);
        return -1;
    }
    fprintf(fp, "%d#2:%s:%u\n", (int32_t)subInfo->sub_time, input->ip, input->sub_port);
    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    return 0;
}

int32_t CMTree::clientSubSelfCluster(MsgInput* in, MsgOutput* out)
{
    SubInput* input = (SubInput*)in;
    SubOutput* output = (SubOutput*)out;

    CMISNode* cpNode;
    if (getNode(input->ip, input->port, input->protocol, cpNode) != 0)
        return -1;
    CMCluster* cpCluster = m_cppCluster[cpNode->m_clusterID];

    output->outlen = cpCluster->node_num * cpNode->getSize() + 1 * cpCluster->getSize() + 20 + 1;
    output->outbuf = new char[output->outlen];
    if (output->outbuf == NULL)
        return -1;
    if (cpNode->sub_selfcluster == NULL) {
        cpNode->sub_selfcluster = (CSubInfo*)allocBuf(sizeof(CSubInfo));
        if (cpNode->sub_selfcluster == NULL)
            return -1;
    }

    char* p = output->outbuf;
    *p++ = (char)msg_success;

    *(int32_t*)p = htonl(1);
    p += 4;
    *(int32_t*)p = htonl(cpCluster->node_num);
    p += 4;
    *(int32_t*)p = 0;
    p += 4;
    *(int32_t*)p = htonl(m_tree_time);
    p += 4;
    *(int32_t*)p = htonl(m_tree_no);
    p += 4;

    pthread_mutex_lock(&m_state_lock);
    p += cpCluster->nodeToBuf(p);
    for (int32_t j = 0; j < cpCluster->node_num; j++)
        p += cpCluster->m_cmisnode[j].nodeToBuf(p);
    pthread_mutex_unlock(&m_state_lock);
    output->outlen = p - output->outbuf;

    strcpy(cpNode->sub_selfcluster->sub_ip, input->ip);
    cpNode->sub_selfcluster->sub_port = input->sub_port;
    cpNode->sub_selfcluster->sub_time = input->op_time;
    cpNode->sub_selfcluster->state = cpCluster->selfcluster_state;
    cpNode->sub_selfcluster->state_num = cpCluster->node_num;
    cpNode->sub_selfcluster->subState = 1;

    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if (fp == NULL) {
        pthread_mutex_unlock(&m_file_lock);
        return -1;
    }
    fprintf(fp, "%d#8:%s:%s:%u,%s:%u\n", (int32_t)input->op_time,
            m_cprotocol.protocolToStr(input->protocol), input->ip, input->port, input->ip, input->sub_port);
    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    return 0;
}

int32_t CMTree::recordNodeInfo(CMISNode* cpNode)
{
    pthread_mutex_lock(&m_file_lock);	
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        pthread_mutex_unlock(&m_file_lock);
        return -1;
    }
    int state;
    if (cpNode->m_state.valid) {
        state = cpNode->m_state.state;
    } else {
        state = unvalid;
    }
    fprintf(fp, "%d#4:%s:%s:%u,%d:%d:%d:%d:%d\n", (int32_t)cpNode->m_state.nDate.tv_sec, m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port, (int32_t)cpNode->m_startTime, (int32_t)cpNode->m_dead_begin, (int32_t)cpNode->m_dead_end, (int32_t)cpNode->m_dead_time, state);
    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    return 0;
}

estate_cluster CMTree::getClusterState(CMCluster *cpCluster)
{
    if (cpCluster->node_num == 0) {
        return GREEN;
    }
    int32_t yellow_count = 0, red_count = 0;
    CMISNode* cpNode = cpCluster->m_cmisnode;
    for (int32_t k = 0; k < cpCluster->node_num; k++, cpNode++) {
        if (cpNode->m_state.valid == 0 
                || cpNode->m_state.state != normal) {
            yellow_count++;
            red_count++;
            continue;
        }
        if (cpNode->m_cpu_busy > cpCluster->cluster_yellow_cpu 
                || cpNode->m_load_one > cpCluster->cluster_yellow_load) {
            yellow_count++;
        }
        if (cpNode->m_cpu_busy > cpCluster->cluster_red_cpu
                || cpNode->m_load_one > cpCluster->cluster_red_load) {
            red_count++;
        }
    }
    if (red_count > (int32_t)cpCluster->cluster_red_count || red_count * 100 / cpCluster->node_num > (int32_t)cpCluster->cluster_red_percent) {
        TLOG("in CMTree::getClusterState() Cluster State is RED! ClusterName: %s", cpCluster->m_name);
        return RED;
    }
    if (yellow_count > (int32_t)cpCluster->cluster_yellow_count || yellow_count * 100 / cpCluster->node_num > (int32_t)cpCluster->cluster_yellow_percent) {
        TLOG("in CMTree::getClusterState() Cluster State is YELLOW! ClusterName: %s", cpCluster->m_name);
        return YELLOW;
    }
    return GREEN;
}

int32_t CMTree::getStateUpdate(MsgInput* in, MsgOutput* out)
{
    StatOutput* output = (StatOutput*)out;
    StatOutput* head = NULL, *next;

    char state[m_max_node_num];
    char state_cluster[m_max_cluster_num];

    char sAllEnvVar[1024];
    int32_t nAllEnvVar = 0;
    for (std::map<std::string, std::string>::iterator iter = m_EnvVarMap.begin(); iter != m_EnvVarMap.end(); iter++) {
        int32_t count = sprintf(&sAllEnvVar[nAllEnvVar], "^%s=%s$", iter->first.c_str(), iter->second.c_str());
        nAllEnvVar += count;
    }

    // msg_sub_child
    for(int32_t i = 0; i < m_max_relation_num; i++)
    {
        int32_t statSize = 0;
        int32_t child_num = m_cppRelation[i]->child_num;
        if(child_num <= 0) continue;
        CMRelation* cpRelation = m_cppRelation[i]->child;
        for(int32_t j = 0; j < child_num; j++, cpRelation = cpRelation->sibling)
        {
            CMISNode* cpNode = cpRelation->cpCluster->m_cmisnode;
            for(int32_t k = 0; k < cpRelation->cpCluster->node_num; k++, statSize++, cpNode++)
            {       
                if(cpNode->m_state.valid)
                    state[statSize] = cpNode->m_state.state;
                else    
                    state[statSize] = (char)unvalid;
            }
            cpRelation->cpCluster->m_state_cluster = getClusterState(cpRelation->cpCluster);
            if (cpRelation->cpCluster->m_state_cluster_manual == WHITE)
                state_cluster[j] = cpRelation->cpCluster->m_state_cluster;
            else
                state_cluster[j] = cpRelation->cpCluster->m_state_cluster_manual;
        }
        bool isUpdate = false;
        if(memcmp(state, m_cppRelation[i]->child_state, statSize)) {
            isUpdate = true;
            memcpy(m_cppRelation[i]->child_state, state, statSize);
        }
        if (memcmp(state_cluster, m_cppRelation[i]->child_state_cluster, child_num)) {
            isUpdate = true;
            memcpy(m_cppRelation[i]->child_state_cluster, state_cluster, child_num);
        }            
        if(isUpdate == false && in->type != msg_update_all)
            continue;

        int32_t node_num = m_cppRelation[i]->cpCluster->node_num;
        CMISNode* cmisnode = m_cppRelation[i]->cpCluster->m_cmisnode;
        for(int32_t j = 0; j < node_num; j++, cmisnode++) {
            if(cmisnode->sub_child == NULL || cmisnode->m_state.valid == 0 || cmisnode->m_state.state != (char)normal)
                continue;
            int32_t tmp_len = sizeof(CSubInfo)+sizeof(StatOutput)+statSize+5 + child_num + nAllEnvVar;
            char* tmp_buf = new char[tmp_len]; 
            if(tmp_buf == NULL)
                continue;
            char* p = tmp_buf;
            next = new(p) StatOutput;
            p += sizeof(StatOutput);
            next->sub = new(p) CSubInfo;
            p += sizeof(CSubInfo);
            next->send_buf = p;
            p += statSize + 5;

            next->outbuf = tmp_buf;
            next->outlen = tmp_len;

            *next->sub = *cmisnode->sub_child;
            if(cmisnode->sub_child->subState) {
                *next->send_buf = (char)in->type;
                *(int32_t*)(next->send_buf+1) = htonl(next->sub->sub_time);
            } else {
                *next->send_buf = (char)msg_reinit;
                *(int32_t*)(next->send_buf+1) = htonl(m_tree_time);
            }
            memcpy(next->send_buf+5, state, statSize);
            next->send_len = statSize + 5;
            memcpy(next->send_buf + next->send_len, state_cluster, child_num);
            next->send_len += child_num;
            memcpy(next->send_buf + next->send_len, sAllEnvVar, nAllEnvVar);
            next->send_len += nAllEnvVar;
            next->next = head;
            head = next;
        }
    }

    // msg_sub_selfcluster
    for(int32_t i = 0; i < m_max_cluster_num; i++)
    {
        int32_t statSize = 0;
        char state_selfcluster = 0;
        CMCluster* cpCluster = m_cppCluster[i];
        if (cpCluster->node_num <= 0) {
            continue;
        }
        CMISNode* cpNode = cpCluster->m_cmisnode;
        for(int32_t k = 0; k < cpCluster->node_num; k++, statSize++, cpNode++)
        {       
            if (cpNode->m_state.valid)
                state[statSize] = cpNode->m_state.state;
            else    
                state[statSize] = (char)unvalid;
        }
        state_selfcluster = getClusterState(cpCluster);
        if (cpCluster->m_state_cluster_manual == WHITE)
            state_selfcluster = cpCluster->m_state_cluster;
        else
            state_selfcluster = cpCluster->m_state_cluster_manual;
        bool isUpdate = false;
        if (memcmp(state, cpCluster->selfcluster_state, statSize)) {
            isUpdate = true;
            memcpy(cpCluster->selfcluster_state, state, statSize);
        }
        if (isUpdate == false && in->type != msg_update_all)
            continue;

        int32_t node_num = cpCluster->node_num;
        CMISNode* cmisnode = cpCluster->m_cmisnode;
        for (int32_t j = 0; j < node_num; j++, cmisnode++) {
            if (cmisnode->sub_selfcluster == NULL || cmisnode->m_state.valid == 0 || cmisnode->m_state.state != (char)normal)
                continue;
            //printf("%u subself: statSize = %d  state = ", cmisnode->m_port, statSize);
            //for (int kk =0; kk < statSize; kk++) {
            //    printf(" %d", state[kk]);
            //}
            //printf("\n");
            int32_t tmp_len = sizeof(CSubInfo) + sizeof(StatOutput) + statSize + 5 + 1 + nAllEnvVar;
            char* tmp_buf = new char[tmp_len]; 
            if (tmp_buf == NULL)
                continue;
            char* p = tmp_buf;
            next = new(p) StatOutput;
            p += sizeof(StatOutput);
            next->sub = new(p) CSubInfo;
            p += sizeof(CSubInfo);
            next->send_buf = p;
            p += statSize + 5;

            next->outbuf = tmp_buf;
            next->outlen = tmp_len;

            *next->sub = *cmisnode->sub_selfcluster;
            if (cmisnode->sub_selfcluster->subState) {
                *next->send_buf = (char)in->type;
                *(int32_t*)(next->send_buf+1) = htonl(next->sub->sub_time);
            } else {
                *next->send_buf = (char)msg_reinit;
                *(int32_t*)(next->send_buf+1) = htonl(m_tree_time);
            }
            memcpy(next->send_buf+5, state, statSize);
            next->send_len = statSize + 5;
            memcpy(next->send_buf + next->send_len, &state_selfcluster, 1);
            next->send_len += 1;
            memcpy(next->send_buf + next->send_len, sAllEnvVar, nAllEnvVar);
            next->send_len += nAllEnvVar;
            next->next = head;
            head = next;
        }
    }

    // msg_sub_all
    int32_t statSize = 0;
    for(int32_t i = 0; i < m_max_cluster_num; i++) {
        for(int32_t j = 0; j < m_cppCluster[i]->node_num; j++, statSize++) {
            if(m_cppCluster[i]->m_cmisnode[j].m_state.valid)
                state[statSize] = m_cppCluster[i]->m_cmisnode[j].m_state.state;
            else
                state[statSize] = (char)unvalid;
        }
        if (m_cppCluster[i]->m_state_cluster_manual == WHITE)
            state_cluster[i] = m_cppCluster[i]->m_state_cluster;
        else
            state_cluster[i] = m_cppCluster[i]->m_state_cluster_manual;
    }

    bool isUpdate = false;
    if(memcmp(m_cLastAllState, state, statSize)) {
        isUpdate = true;
        memcpy(m_cLastAllState, state, statSize);
    }
    if (memcmp(m_cLastAllState_Cluster, state_cluster, m_max_cluster_num)) {
        isUpdate = true;
        memcpy(m_cLastAllState_Cluster, state_cluster, m_max_cluster_num);
    }
    if(isUpdate == true || in->type == msg_update_all) {
        pthread_mutex_lock(&m_state_lock);
        CSubInfo* cpSub = m_cpSubAll;
        while(cpSub) {

            int32_t tmp_len = sizeof(CSubInfo)+sizeof(StatOutput)+statSize+5 + m_max_cluster_num + nAllEnvVar;
            char* tmp_buf = new char[tmp_len];
            if(tmp_buf == NULL)
                continue;
            char* p = tmp_buf;
            next = new(p) StatOutput;
            p += sizeof(StatOutput);
            next->sub = new(p) CSubInfo;
            p += sizeof(CSubInfo);
            next->send_buf = p;
            p += statSize + 5;

            next->outbuf = tmp_buf;
            next->outlen = tmp_len;

            *next->sub = *cpSub;
            if(cpSub->subState) {
                *next->send_buf = (char)in->type;
                *(int32_t*)(next->send_buf+1) = htonl(next->sub->sub_time);
            } else {
                *next->send_buf = (char)msg_reinit;
                *(int32_t*)(next->send_buf+1) = htonl(m_tree_time);
            }
            memcpy(next->send_buf+5, state, statSize);
            next->send_len = statSize + 5;
            memcpy(next->send_buf + next->send_len, state_cluster, m_max_cluster_num);
            next->send_len += m_max_cluster_num;
            memcpy(next->send_buf + next->send_len, sAllEnvVar, nAllEnvVar);
            next->send_len += nAllEnvVar;
            next->next = head;
            head = next;

            cpSub = cpSub->next;
        }
        pthread_mutex_unlock(&m_state_lock);
    }

    //
    output->next = head;

    return 0;
}

int32_t CMTree::getNodeInfo(MsgInput* input, MsgOutput* output)
{
    getNodeInput* in = (getNodeInput*)input;
    getNodeOutput* out = (getNodeOutput*)output;
    int32_t node_num = 0;
    CMISNode* cpNode = NULL;
    CMCluster* cpCluster = NULL;

    if(in->type == cmd_get_info_by_name) {
        if(in->name == NULL) return -1;
        std::map<std::string, CMCluster*>::iterator iter = m_clustermap.find(in->name);
        if(iter == m_clustermap.end())
            return -1;
        cpCluster = iter->second;
        node_num = cpCluster->node_num;
        cpNode = cpCluster->m_cmisnode;
    } else if(in->type == cmd_get_info_by_spec) {
        char key[64];
        if(getProtocolIpPortKey(in->protocol, in->ip, in->port, key) != 0)
            return -1;
        std::map<std::string, CMISNode*>::iterator iter = m_misnodeMap.find(key);
        if(iter == m_misnodeMap.end())
            return -1;
        node_num = 1;
        cpNode = iter->second;
        cpCluster = m_cppCluster[cpNode->m_clusterID];
    } else if (in->type == cmd_get_info_allbad) {
        int BadNode_Max = 100;
        CMISNode **cppNode = NULL;
        cppNode =  new CMISNode *[BadNode_Max];
        if (cppNode == NULL) {
            return -1;
        }
        int node_num_total = 0;
        std::map<std::string, CMCluster*>::iterator iter;
        for (iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
            cpCluster = iter->second;
            node_num = cpCluster->node_num;
            cpNode = cpCluster->m_cmisnode;       
            if(node_num <= 0 || cpCluster == NULL) {
                continue;
            }
            for(int32_t i = 0; i < node_num; i++) {
                if (cpNode[i].m_state.state != (char)normal || cpNode[i].m_state.valid == 0) {
                    cppNode[node_num_total] = &cpNode[i];
                    node_num_total++;
                    if (node_num_total >= BadNode_Max)
                        break;
                }
            }
        }
        node_num = node_num_total;
        //    	
        out->outlen = cpNode->getSize() * node_num + cpCluster->getSize() + 4 + 1;
        out->outbuf = new char[out->outlen];
        if(out->outbuf == NULL)
            return -1;
        char* p1 = out->outbuf;
        *p1 = (char)in->type;
        p1 += 1;
        *(int32_t*)p1 = htonl(node_num);
        p1 += 4;
        p1 += cpCluster->nodeToBuf(p1);
        for(int32_t i = 0; i < node_num; i++)
            p1 += cppNode[i]->nodeToBuf(p1);
        out->outlen = p1 - out->outbuf;
        return 0;
    } else if (in->type == cmd_get_info_watchpoint) {
        node_num = m_max_node_num;
        out->outlen = 50 * node_num;
        out->outbuf = new char[out->outlen];
        if(out->outbuf == NULL)
            return -1;
        char* p = out->outbuf;
        *p = (char)in->type;
        p += 1;
        *(int32_t*)p = htonl(node_num);
        p += 4;
        std::map<std::string, CMCluster*>::iterator iter;
        for (iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
            cpCluster = iter->second;
            int32_t num = cpCluster->node_num;
            cpNode = cpCluster->m_cmisnode;
            if(num <= 0 || cpCluster == NULL) {
                continue;
            }
            for(int32_t i = 0; i < num; i++) {
                *p = (char)cpNode[i].m_protocol;
                p += 1;
                strcpy(p, cpNode[i].m_ip);
                p += 24;
                *(uint16_t*)p = htons(cpNode[i].m_port);
                p += 2;
                if(cpNode[i].m_state.valid)
                    *p = cpNode[i].m_state.state;
                else
                    *p = (char)unvalid;
                p += 1;
                *(uint32_t*)p = htonl(cpNode[i].m_cpu_busy);
                p += 4;
                *(uint32_t*)p = htonl(cpNode[i].m_load_one);
                p += 4;		
            }
        }
        out->outlen = p - out->outbuf;
        return 0;
    } else {
        node_num = 0;
        cpNode = NULL;
    }
    if(node_num <= 0 || cpCluster == NULL)
        return -1;

    out->outlen = cpNode->getSize() * node_num + cpCluster->getSize() + 4 + 1;
    out->outbuf = new char[out->outlen];
    if(out->outbuf == NULL)
        return -1;
    char* p1 = out->outbuf;
    *p1 = (char)in->type;
    p1 += 1;
    *(int32_t*)p1 = htonl(node_num);
    p1 += 4;
    p1 += cpCluster->nodeToBuf(p1);
    for(int32_t i = 0; i < node_num; i++)
        p1 += cpNode[i].nodeToBuf(p1);
    out->outlen = p1 - out->outbuf;
    return 0;
}

int32_t CMTree::getNodeInfo_auto(MsgInput* input, MsgOutput* output)
{
    getNodeInput* in = (getNodeInput*)input;
    getNodeOutput* out = (getNodeOutput*)output;
    int32_t max_cluster_num = m_max_cluster_num;
    int32_t max_node_num = m_max_node_num;
    int32_t blender_good_num = 0;
    int32_t blender_bad_num = 0;
    int32_t merger_good_num = 0;
    int32_t merger_bad_num = 0;
    int32_t searcher_good_num = 0;
    int32_t searcher_bad_num = 0;
    CMISNode* cpNode = NULL;
    CMCluster* cpCluster = NULL;
    int32_t node_num = 0;
    int32_t bad_num = 0;

    char sAllEnvVar[1024];
    int32_t nAllEnvVar = 0;
    for (std::map<std::string, std::string>::iterator iter = m_EnvVarMap.begin(); iter != m_EnvVarMap.end(); iter++) {
        int32_t count = sprintf(&sAllEnvVar[nAllEnvVar], "^%s=%s$", iter->first.c_str(), iter->second.c_str());
        nAllEnvVar += count;
    }

    out->outlen = cpNode->getSize() * max_node_num + cpCluster->getSize() * max_cluster_num + 4 * 8 + 1 + nAllEnvVar;
    out->outbuf = new char[out->outlen];
    if(out->outbuf == NULL)
        return -1;
    char *p = out->outbuf;
    *p = (char)in->type;
    p += 1;
    *(int32_t*)p = htonl(max_cluster_num);
    p += 4;
    *(int32_t*)p = htonl(max_node_num);
    p += 4;
    p += 4 * 6; // for blender_good_num ... searcher_bad_num

    std::map<std::string, CMCluster*>::iterator iter;
    for (iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
        cpCluster = iter->second;
        node_num = cpCluster->node_num;
        cpNode = cpCluster->m_cmisnode;       
        if(node_num <= 0 || cpCluster == NULL) {
            continue;
        }
        bad_num = 0;
        for(int32_t i = 0; i < node_num; i++) {
            if (cpNode[i].m_state.state != (char)normal || cpNode[i].m_state.valid == 0) {
                bad_num++;
            }
        }
        if (cpCluster->m_type == blender_cluster) {
            blender_bad_num += bad_num;
            blender_good_num += node_num - bad_num;
        } else if (cpCluster->m_type == merger_cluster) {
            merger_bad_num += bad_num;  
            merger_good_num += node_num - bad_num;
        } else if (cpCluster->m_type == search_cluster) {
            searcher_bad_num += bad_num;
            searcher_good_num += node_num - bad_num;
        }
        p += cpCluster->nodeToBuf(p);	
        for(int32_t i = 0; i < node_num; i++) {	
            p += cpNode[i].nodeToBuf(p);
        }
    }

    if (nAllEnvVar > 0) {
        memcpy(p, sAllEnvVar, nAllEnvVar);
        p += nAllEnvVar;
    }

    out->outlen = p - out->outbuf;

    p = out->outbuf + 9;
    *(int32_t*)p = htonl(blender_good_num);
    p +=4;
    *(int32_t*)p = htonl(blender_bad_num);
    p +=4;
    *(int32_t*)p = htonl(merger_good_num);
    p +=4;
    *(int32_t*)p = htonl(merger_bad_num);
    p +=4;
    *(int32_t*)p = htonl(searcher_good_num);
    p +=4;
    *(int32_t*)p = htonl(searcher_bad_num);
    p +=4;

    return 0;
}

int32_t CMTree::travelTree(FILE* fp)
{
    if(fp == NULL)
        fp = stdout;
    if(travelTree(&m_cRoot, 0, fp) != 0)
        return -1;
    CMISNode* cpNode;
    for(int32_t i = 0; i < m_max_cluster_num; i++){
        TLOG("<%s>", m_cppCluster[i]->m_name);
        cpNode = m_cppCluster[i]->m_cmisnode;
        OfflineLimit& limit = cpNode->m_limit;
        for(int32_t j = 0; j < m_cppCluster[i]->node_num; j++, cpNode++){ 
            TLOG("<nodeid=%d relationid=%d, ip=%s, port=%u limit(cpu:%d,load:%d,io:%d,qps:%d,latency:%d,max=%d,hold=%d>",
                    cpNode->m_nodeID, cpNode->m_relationid, cpNode->m_ip, cpNode->m_port, limit.cpu_limit, limit.load_one_limit,
                    limit.io_wait_limit, limit.qps_limit, limit.latency_limit, limit.offline_limit, limit.hold_time);
        }
    }
    return 0;
}

int32_t CMTree::getOneList(mxml_node_t* root, const char* list_name, const char* cluster_name, std::map<std::string, CMRelation*>& root_map, const char type)
{
    std::vector<std::string>& nameVec = _listTable[list_name];

    root = mxmlFindElement(root, root, list_name, NULL, NULL, MXML_DESCEND);
    if(root == NULL)
        return -2;
    mxml_node_t* first = mxmlFindElement(root, root, cluster_name, NULL, NULL, MXML_DESCEND_FIRST);
    mxml_node_t* cpNode = first;
    while(cpNode)
    {// <blender_cluster name=mix>
        const char* name = mxmlElementGetAttr(cpNode, "name");
        const char* docsep = mxmlElementGetAttr(cpNode, "docsep");
        const char* level = mxmlElementGetAttr(cpNode, "level");
        const char* cpu_limit = mxmlElementGetAttr(cpNode, "cpu_limit");
        const char* load_one_limit = mxmlElementGetAttr(cpNode, "load_one_limit");
        const char* io_wait_limit = mxmlElementGetAttr(cpNode, "io_wait_limit");
        const char* qps_limit = mxmlElementGetAttr(cpNode, "qps_limit");
        const char* latency_limit = mxmlElementGetAttr(cpNode, "latency_limit");
        const char* offline_limit = mxmlElementGetAttr(cpNode, "offline_limit");
        const char* hold_time = mxmlElementGetAttr(cpNode, "hold_time");
        const char* yellow_cpu = mxmlElementGetAttr(cpNode, "yellow_cpu");
        const char* yellow_load = mxmlElementGetAttr(cpNode, "yellow_load");
        const char* yellow_count = mxmlElementGetAttr(cpNode, "yellow_count");
        const char* yellow_percent = mxmlElementGetAttr(cpNode, "yellow_percent");
        const char* red_cpu = mxmlElementGetAttr(cpNode, "red_cpu");
        const char* red_load = mxmlElementGetAttr(cpNode, "red_load");
        const char* red_count = mxmlElementGetAttr(cpNode, "red_count");
        const char* red_percent = mxmlElementGetAttr(cpNode, "red_percent");

        if(name == NULL)
            return -1;
        if(cpu_limit || load_one_limit || io_wait_limit || qps_limit || latency_limit) {
            if(offline_limit == NULL) {
                TERR("have limit condition, but not have offline_limit, limit invalid");
                return -1;
            }
            if(hold_time == NULL) {
                TERR("have limit condition, but not have hold_time, limit invalid");
                return -1;
            }
            m_watchOnOff = 1;   // 配置检测上限后，默认打开
        }
            
        nameVec.push_back(name);
        CMCluster* cpCluster = m_clustermap[name];

        if(type == 0)
        {// <blender ip=192.168.1.1 port=8888 protocol=tcp/>
            if(cpCluster)
            {
                TLOG("redefine name %s\n", name);
                return -1;
            }
            cpCluster = allocMClusterNode();
            if(cpCluster == NULL)
                return -1;
            strcpy(cpCluster->m_name, name);
            if(strToClusterType(cluster_name, cpCluster->m_type) != 0)
                return -1;
            if(docsep){
                if(strcasecmp(docsep, "false") == 0)
                    cpCluster->docsep = false;
                else if(strcasecmp(docsep, "true") == 0)
                    cpCluster->docsep = true;
                else return -1;
            }
            else if(cpCluster->m_type != search_cluster)
                cpCluster->docsep = false;			
            if(level) {
                const char *p = level;
                bool isNum = true;
                while (*p) {
                    if (*p < '0' || *p > '9') {
                        isNum = false;
                        break;
                    }
                    p++;
                }
                if (isNum) {
                    cpCluster->level = atol(level);
                    if (cpCluster->level <= 0)
                        isNum = false;
                } 
                if (!isNum) {
                    TLOG("level must be integer and greater than 0. level = %s", level);
                    return -1;
                }
            }
            // <search_cluster name=c1 docsep=false level=1 cpu_limit=80 load_one_limit=8 yellow_cpu=80 yellow_load=4 yellow_count=3 yellow_percent=60 red_cpu=80 red_load=4 red_count=3 red_percent=60 >
            if (parseClusterState(cpCluster->limit.cpu_limit, cpu_limit, "cpu_limit") != 0) 
                return -1;
            if (parseClusterState(cpCluster->limit.load_one_limit, load_one_limit, "load_one_limit") != 0) 
                return -1;
            if (parseClusterState(cpCluster->limit.io_wait_limit, io_wait_limit, "io_wait_limit") != 0) 
                return -1;
            if (parseClusterState((uint32_t&)cpCluster->limit.qps_limit, qps_limit, "qps_limit", true) != 0) 
                return -1;
            if (parseClusterState((uint32_t&)cpCluster->limit.latency_limit, latency_limit, "latency_limit", true) != 0) 
                return -1;
            if (parseClusterState(cpCluster->limit.offline_limit, offline_limit, "offline_limit") != 0) 
                return -1;
            if (parseClusterState(cpCluster->limit.hold_time, hold_time, "hold_time") != 0) 
                return -1;
            
            if (parseClusterState(cpCluster->cluster_yellow_cpu, yellow_cpu, "cluster_yellow_cpu") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_yellow_load, yellow_load, "cluster_yellow_load") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_yellow_count, yellow_count, "cluster_yellow_count") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_yellow_percent, yellow_percent, "cluster_yellow_percent") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_red_cpu, red_cpu, "cluster_red_cpu") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_red_load, red_load, "cluster_red_load") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_red_count, red_count, "cluster_red_count") != 0) 
                return -1;
            if (parseClusterState(cpCluster->cluster_red_percent, red_percent, "cluster_red_percent") != 0) 
                return -1;

            m_clustermap[name] = cpCluster;
            if(getOneCluster(cpNode, cpCluster) != 0)
                return -1;
        }
        else
        {// <search_cluster name=c1/>
            CMRelation* cpRelation = root_map[name];
            if(cpRelation == NULL)
            {
                cpRelation = allocRelationNode();
                if(cpRelation == NULL)
                    return -1;
                root_map[name] = cpRelation;
            }
            if(cpCluster == NULL)
            {
                TLOG("not find cluster %s\n", name);
                return -1;
            }
            cpRelation->cpCluster = cpCluster;
            m_relationMap[name] = cpRelation;
            if(getOneRelation(cpNode, cpRelation, root_map) != 0)
                return -1;
        }
        cpNode = mxmlFindElement(cpNode, root, cluster_name, NULL, NULL, MXML_NO_DESCEND);
    }
    //std::vector<std::string>& vec = _listTable[list_name];

    return 0;
}

int32_t CMTree::getOneCluster(mxml_node_t* root, CMCluster* cpCluster)
{
    mxml_node_t* cpNode = mxmlFindElement(root, root, NULL, NULL, NULL, MXML_DESCEND_FIRST);
    if(cpNode == NULL)
        return -1;
    /*
       <blender_cluster name=other>
       <blender ip=192.168.1.1 port=7777 protocol=udp/>
       <blender ip=192.168.1.2 port=6666 protocol=tcp/>
       </blender_cluster>
     */
    CMISNode cNode;
    std::vector<CMISNode> cNodeArray;

    while(cpNode)
    {		
        const char* ip = mxmlElementGetAttr(cpNode, "ip");
        const char* port = mxmlElementGetAttr(cpNode, "port");
        const char* protocol = mxmlElementGetAttr(cpNode, "protocol");
        if(ip == NULL || port == NULL || protocol == NULL)
            return -1;		
        strcpy(cNode.m_ip, ip);
        cNode.m_port = atoi(port);
        if(strToProtocol(protocol, cNode.m_protocol) != 0)
            return -1;
        cNode.m_nodeID = m_max_node_num;
        cNode.m_clustername = cpCluster->m_name;
        const char* type = mxmlElementGetAttr(cpNode, "type");
        if(type == NULL)
            type = cpNode->value.element.name;
        if(strToNodeType(type, cNode.m_type) != 0)
            return -1;
        // 继承cluster限制信息
        cNode.m_limit = cpCluster->limit;

        const char* cpu_limit = mxmlElementGetAttr(cpNode, "cpu_limit");
        if(cpu_limit){
            if (parseClusterState(cNode.m_limit.cpu_limit, cpu_limit, "cpu_limit") != 0) 
                return -1;
        }
        const char* load_one_limit = mxmlElementGetAttr(cpNode, "load_one_limit");
        if(load_one_limit) {
            if (parseClusterState(cNode.m_limit.load_one_limit, load_one_limit, "load_one_limit") != 0) 
                return -1;
        }
        const char* io_wait_limit = mxmlElementGetAttr(cpNode, "io_wait_limit");
        if(io_wait_limit) {
            if (parseClusterState(cNode.m_limit.io_wait_limit, io_wait_limit, "io_wait_limit") != 0) 
                return -1;
        }
        const char* hold_time = mxmlElementGetAttr(cpNode, "hold_time");
        if(io_wait_limit) {
            if (parseClusterState(cNode.m_limit.hold_time, hold_time, "hold_time") != 0) 
                return -1;
        }

        cNodeArray.push_back(cNode);
        cpNode = mxmlFindElement(cpNode, root, NULL, NULL, NULL, MXML_DESCEND);
    }

    cpCluster->node_num = cNodeArray.size();
    CMISNode* cpMISNode = allocMISNode(cpCluster->node_num);
    if(cpMISNode == NULL)
        return -1;

    char str[64], type[MAX_NODETYPE_COUNT];
    memset(type, 0, MAX_NODETYPE_COUNT);
    std::map<std::string,CMISNode*>::iterator iter;
    for(int32_t i = 0; i < cpCluster->node_num; i++)
    {
        cpMISNode[i].m_clustername = cpCluster->m_name;
        cpMISNode[i] = cNodeArray[i];
        if(getProtocolIpPortKey(cpMISNode[i].m_protocol, cpMISNode[i].m_ip, cpMISNode[i].m_port, str) != 0)
            return -1;
        iter = m_misnodeMap.find(str);
        if(iter != m_misnodeMap.end()){
            TLOG("node repeat: ip=%s port=%u\n", cpMISNode[i].m_ip, cpMISNode[i].m_port);
            //return -1;
        }
        m_misnodeMap[str] = &cpMISNode[i];
        type[cpMISNode[i].m_type] = 1;
    }
    cpCluster->m_cmisnode = cpMISNode;

    cpCluster->selfcluster_state = allocBuf(cpCluster->node_num);
    if (cpCluster->selfcluster_state == NULL)
        return -1;
    memset(cpCluster->selfcluster_state, (char)abnormal, cpCluster->node_num);

    //if(cpCluster->docsep == true && cpCluster->m_type == search_cluster) {
    //    if(type[search_mix] || (type[search] && type[search_doc]))
    //        return 0;
    //    TLOG("clustername=%s error, not have enough info\n", cpCluster->m_name);
    //    return -1;
    //}

    return 0;
}

int32_t CMTree::getOneRelation(mxml_node_t* root, CMRelation* cpRelation, std::map<std::string, CMRelation*>& root_map)
{
    mxml_node_t* cpNode = mxmlFindElement(root, root, NULL, NULL, NULL, MXML_DESCEND_FIRST);
    if(cpNode == NULL)
        return -1;

    CMRelation** cpSibling = &cpRelation->child;
    const char* name, *type;
    std::map<std::string, CMRelation*>::iterator iter;

    while(cpNode)
    {
        /*
           <merger_cluster name=p4p>
           <search_cluster name=c1/>
           <search_cluster name=c2/>
           <search_cluster name=c3/>
           </merger_cluster> */

        name = mxmlElementGetAttr(cpNode, "name");
        type = cpNode->value.element.name;
        if(name == NULL || type == NULL)
        {
            TLOG("not find attr\n");
            return -1;
        }

        // cluster relation
        iter = m_relationMap.find(name);
        if(iter == m_relationMap.end())
            *cpSibling = NULL;
        else
            *cpSibling = iter->second;
        if(*cpSibling == NULL)
        {
            *cpSibling = allocRelationNode();
            if(*cpSibling == NULL)
                return -1;
            // cluster info
            (*cpSibling)->cpCluster = m_clustermap[name];
            if((*cpSibling)->cpCluster == NULL)
                return -1;
            m_relationMap[name] = *cpSibling;
        }
        (*cpSibling)->parent = cpRelation;
        cpSibling = &(*cpSibling)->sibling;

        iter = root_map.find(name);
        if(iter != root_map.end())
            root_map.erase(iter);

        cpNode = mxmlFindElement(cpNode, root, NULL, NULL, NULL, MXML_DESCEND);		
    }
    return 0;
}

int32_t CMTree::travelTree(CMRelation* root, int indent, FILE* fp)
{
    //for(int i = 0; i < indent; i++)
    //	TLOG("\t");
    TLOG("<%s>", root->cpCluster->m_name);

    if(root->child)
        travelTree(root->child, indent+1, fp);
    //for(int i = 0; i < indent; i++)
    //	TLOG("\t");
    TLOG("</%s>", root->cpCluster->m_name);

    if(root->sibling)
        travelTree(root->sibling, indent, fp);

    return 0;
}

int32_t CMTree::parseClusterState(uint32_t &limit_value, const char *str_limit_value, const char *limit_name, bool negative)
{
    if (str_limit_value) {
        if (negative == false && atol(str_limit_value) <= 0) {
            TERR("%s must be integer and greater than 0. Now %s = %s", limit_name, limit_name, str_limit_value);
            return -1;
        }
        limit_value = atol(str_limit_value);
    }
    return 0;
}

int32_t CMTree::getRelationArray(CMRelation**& cppRelation, int32_t& relation_num, int32_t& all_node_num)
{
    cppRelation = m_cppRelation;
    relation_num = m_max_relation_num;
    all_node_num = m_max_node_num;

    return 0;
}

int32_t CMTree::getRelation(int32_t relationid, CMRelation*& cpRelation)
{
    if(relationid < 0 || relationid >= m_max_relation_num)
        return -1;
    cpRelation = m_cppRelation[relationid];
    return 0; 
}

int32_t CMTree::strToProtocol(const char* str, eprotocol& protocol)
{
    if(strncasecmp(str, "tcp", 3) == 0)
        protocol = tcp;
    else if(strncasecmp(str, "udp", 3) == 0)
        protocol = udp;
    else if (strncasecmp(str, "http", 4) == 0)
        protocol = http;
    else
        return -1;
    return 0;
}

int32_t CMTree::strToNodeType(const char* str, enodetype& nodetype)
{
    if(strcasecmp(str, "blender") == 0)
        nodetype = blender;
    else if(strcasecmp(str, "merger") == 0)
        nodetype = merger;
    else if(strcasecmp(str, "search") == 0)
        nodetype = search;
    else if(strcasecmp(str, "doc") == 0)
        nodetype = search_doc;
    else if(strcasecmp(str, "mix") == 0)
        nodetype = search_mix;
    else if(strcasecmp(str, "dispatcher") == 0)
        nodetype = dispatcher;
    else if(strcasecmp(str, "builder") == 0)
        nodetype =builder;
    else if(strcasecmp(str, "mergerOnSearch") == 0)
        nodetype =mergerOnSearch;
    else if(strcasecmp(str, "mergerOnDetail") == 0)
        nodetype =mergerOnDetail;
    else
        return -1;
    return 0;
}

int32_t CMTree::statRelationNum(CMRelation* cpRelation, int32_t relation_id)
{
    CMRelation* cpChild = cpRelation->child;

    cpRelation->child_num = 0;
    int32_t child_node_num = 0;

    while(cpChild){
        cpRelation->child_num++;
        child_node_num += cpChild->cpCluster->node_num;
        cpChild = cpChild->sibling;
    }

    cpRelation->child_state = allocBuf(child_node_num);
    if(cpRelation->child_state == NULL)
        return -1;
    for(int32_t i = 0; i < child_node_num; i++)
        cpRelation->child_state[i] = (char)normal;

    cpRelation->child_state_cluster = allocBuf(cpRelation->child_num);
    if (cpRelation->child_state_cluster == NULL)
        return -1;
    for (int32_t i = 0; i < cpRelation->child_num; i++)
        cpRelation->child_state_cluster[i] = (char)GREEN;

    CMCluster* cpCluster = cpRelation->cpCluster;
    for(int32_t i = 0; i < cpCluster->node_num; i++)
        cpCluster->m_cmisnode[i].m_relationid = relation_id;

    return 0;
}

int32_t CMTree::strToClusterType(const char* str, eclustertype& type)
{
    if(strncasecmp(str, "blender_cluster", 15) == 0)
        type = blender_cluster;
    else if(strncasecmp(str, "merger_cluster", 14) == 0)
        type = merger_cluster;
    else if(strncasecmp(str, "search_cluster", 14) == 0)
        type = search_cluster;
    else
        return -1;
    return 0;
}

int32_t CMTree::getProtocolIpPortKey(eprotocol protocol, const char* ip, uint16_t port, char* key)
{
    sprintf(key, "%d:%s:%u", protocol, ip, port);
    return 0;
}

char* CMTree::allocBuf(int size)
{
    pthread_mutex_lock(&m_mem_lock);
    if(m_left_buf_size <= 0)
    {
        char* p = new char[BLOCK_BUF_SIZE];
        if(p == NULL) {
            pthread_mutex_unlock(&m_mem_lock);
            return NULL;
        }
        *(char**)p = m_szpHead;
        m_szpHead = p;
        m_szpBuf = p + sizeof(char*);
        m_left_buf_size = BLOCK_BUF_SIZE - sizeof(char*);
    }

    char* ret = m_szpBuf;
    m_szpBuf += size;
    m_left_buf_size -= size;
    if(m_left_buf_size < 0) {
        pthread_mutex_unlock(&m_mem_lock);
        return NULL;
    }
    pthread_mutex_unlock(&m_mem_lock);

    return ret;
}

CMISNode* CMTree::allocMISNode(int num)
{
    char* buf = allocBuf(sizeof(CMISNode) * num);
    if(buf == NULL)
        return NULL;
    return new(buf) CMISNode[num];
}

CMCluster* CMTree::allocMClusterNode(int num)
{
    char* buf = allocBuf(sizeof(CMCluster) * num);
    if(buf == NULL)
        return NULL;
    CMCluster* cpCluster = new(buf) CMCluster[num];
    for(int32_t i = 0; i < num; i++) {
        cpCluster[i].docsep = true;
        cpCluster[i].level = 1;
    }
    return cpCluster;
}

CMRelation* CMTree::allocRelationNode(int num)
{
    char* buf = allocBuf(sizeof(CMRelation) * num);
    if(buf == NULL)
        return NULL;
    CMRelation* cpNode = new(buf) CMRelation[num];

    return cpNode;
}

int32_t CMTree::getNode(int32_t clusterid, int32_t nodeid, CMISNode *&cmisnode)
{
    if(nodeid < 0 || nodeid >= m_max_node_num)
        return -1;
    cmisnode = m_cppMISNode[nodeid];
    return 0;
}

int32_t CMTree::getNode(char* ip, int32_t port, eprotocol proto, CMISNode*& cmisnode)
{
    char key[64];
    if(getProtocolIpPortKey(proto, ip, port, key) != 0)
        return -1;

    std::map<std::string, CMISNode*>::iterator iter = m_misnodeMap.find(key);
    if(iter == m_misnodeMap.end())
        return -1;
    cmisnode = iter->second;
    if(cmisnode == NULL)
        return -1;
    return 0;
}

int32_t CMTree::getCluster(int32_t clusterid, CMCluster *&cmcluster)
{
    if(clusterid < 0 || clusterid >= m_max_cluster_num)
        return -1;
    cmcluster = m_cppCluster[clusterid];
    return 0;
}

int32_t CMTree::getCluster(const char* clustername, CMCluster *&cmcluster)
{
    if(clustername == NULL)
        return -1;
    std::map<std::string, CMCluster*>::iterator iter = m_clustermap.find(clustername);
    if(iter == m_clustermap.end())
        return -1;
    cmcluster = iter->second;
    if(cmcluster == NULL)
        return -1;
    return 0;
}

int32_t CMTree::getSubCluster(int32_t relationid, CMCluster **&cmcluster, int32_t &size)
{
    if(relationid < 0 || relationid >= m_max_relation_num) {
        size = 0;
        cmcluster = NULL;
        return 0;
    }

    CMRelation* cpRelation = m_cppRelation[relationid];

    size = cpRelation->child_num;
    cmcluster = new CMCluster*[size];
    if(cmcluster == NULL)
        return -1;
    cpRelation = cpRelation->child;
    for(int32_t i = 0; i < size; i++)
    {
        cmcluster[i] = cpRelation->cpCluster;
        cpRelation = cpRelation->sibling;
    }

    return 0;
}

int32_t CMTree::freeSubCluster(CMCluster** cmcluster)
{
    if(cmcluster == NULL)
        return 0;
    delete[] cmcluster;
    return 0;
}

int32_t CMTree::setNodeState(MsgInput* in, MsgOutput* out)
{
    StatInput* input = (StatInput*)in;

    char key[64];
    if(getProtocolIpPortKey(input->protocol, input->ip, input->port, key) != 0)
        return -1;
    std::map<std::string, CMISNode*>::iterator iter = m_misnodeMap.find(key);
    if(iter == m_misnodeMap.end())
        return -1;
    
    int32_t node_id = iter->second->m_nodeID;
    int32_t qps = 0, latency = 0, offNum = 0, limit = 0;
    bool offline = false;
    CMISNode* pNode = m_cppMISNode[node_id];
    int ret = getAvgValue(pNode, qps, latency, offNum, limit);

    pthread_mutex_lock(&m_state_lock);
    if(m_watchOnOff == 1 && ret >= 0 && offNum + 1 <= limit) {
        bool check = false;
        if(input->cpu_busy >= pNode->m_limit.cpu_limit) {
            check = offline = true;
            TWARN("offline ip:%s port:%u cpu limit, cpu:%u, limit:%u",
                    input->ip, input->port, input->cpu_busy, pNode->m_limit.cpu_limit);
        }
        if(input->load_one >= pNode->m_limit.load_one_limit) {
            check = offline = true;
            TWARN("offline ip:%s port:%u load limit, load:%u, limit:%u",
                    input->ip, input->port, input->load_one, pNode->m_limit.load_one_limit);
        }
        if(input->io_wait >= pNode->m_limit.io_wait_limit) {
            check = offline = true;
            TWARN("offline ip:%s port:%u io_wait limit, iowait:%u, limit:%u, off:%d/%d", 
                    input->ip, input->port, input->io_wait, pNode->m_limit.io_wait_limit, offNum, limit);
        }
        if((pNode->m_limit.latency_limit > 0 && input->latency > latency * (1.0 + pNode->m_limit.latency_limit / 100.0)) ||
           ((pNode->m_limit.latency_limit < 0 && input->latency < latency * (1.0 + pNode->m_limit.latency_limit / 100.0)))) {
            check = offline = true;
            TWARN("offline ip:%s port:%u latency limit, latency:%u, avg=%d, limit:%d",
                    input->ip, input->port, input->latency, latency, pNode->m_limit.latency_limit);
        }
        if((pNode->m_limit.qps_limit > 0 && input->qps > qps * (1.0 + pNode->m_limit.qps_limit / 100.0)) ||
           ((pNode->m_limit.qps_limit < 0 && input->qps < qps * (1.0 + pNode->m_limit.qps_limit / 100.0)))) {
            check = offline = true;
            TWARN("offline ip:%s port:%u qps limit, qps:%u, avg=%d, limit:%d",
                    input->ip, input->port, input->qps, qps, pNode->m_limit.qps_limit);
        }
        if(check == false) pNode->m_err_start = 0;
    }
    if(offline == true && pNode->m_err_start && input->op_time.tv_sec - pNode->m_err_start > pNode->m_limit.hold_time) {
        m_cppMISNode[node_id]->m_state.valid = 0;
        pthread_mutex_lock(&m_file_lock);
        FILE* fp = fopen(m_sys_file, "a");
        if(fp == NULL) {
            pthread_mutex_unlock(&m_file_lock);
            pthread_mutex_unlock(&m_state_lock);
            return -1;
        }
        fprintf(fp, "%d#3:%s:%s:%u,%d\n", (int32_t)input->op_time.tv_sec,
                m_cprotocol.protocolToStr(pNode->m_protocol), pNode->m_ip,
                pNode->m_port, pNode->m_state.valid);
        fclose(fp);
        pthread_mutex_unlock(&m_file_lock);
        TERR("offline %s:%d", input->ip, input->port);
    } else if(offline == true && pNode->m_err_start == 0) {
        pNode->m_err_start = input->op_time.tv_sec;
    }
    m_cppMISNode[node_id]->m_state.state = input->state;
    m_cppMISNode[node_id]->m_state.nDate.tv_sec = input->op_time.tv_sec;
    m_cppMISNode[node_id]->m_state.nDate.tv_usec = input->op_time.tv_usec;
    m_cppMISNode[node_id]->m_cpu_busy = input->cpu_busy;
    m_cppMISNode[node_id]->m_load_one = input->load_one;
    m_cppMISNode[node_id]->m_io_wait = input->io_wait;
    m_cppMISNode[node_id]->m_latency = input->latency;
    m_cppMISNode[node_id]->m_qps = input->qps;
    pthread_mutex_unlock(&m_state_lock);

    return 0;
}

int32_t CMTree::getAvgValue(CMISNode* cur, int32_t& qps, int32_t& latency, int32_t& offNum, int32_t& limit)
{
    offNum = qps = latency = limit = 0;
    CMCluster* cpCluster = m_cppCluster[cur->m_clusterID];
    
    int num = 0;
    int allNum = 0;
    
    int32_t cur_qps = 0;
    int32_t cur_latency = 0;
    bool    cur_ok  = false;

    CMISNode* pNode = cpCluster->m_cmisnode;
    for(int i = 0; i < cpCluster->node_num; i++) {
        if(pNode[i].m_type != cur->m_type) { // 不同角色
            continue;
        }
        allNum++;
        if(pNode[i].m_state.valid == 0 || pNode[i].m_state.state != 0) {
            offNum++;
            continue;
        }

        if(pNode[i].m_nodeID == cur->m_nodeID) {
            cur_qps     = pNode[i].m_qps;
            cur_latency = pNode[i].m_latency;
            cur_ok      = true;
        }

        num++;
        qps += pNode[i].m_qps;
        latency += pNode[i].m_latency;
    }
    limit = allNum * cur->m_limit.offline_limit / 100;
    if(num <= 0 || offNum >= limit) {
        if(num > 0) {
            TWARN("offline over limit %d/%d", offNum, allNum * cur->m_limit.offline_limit / 100);
        } else {
            TWARN("not find valid node");
        }
        return -1;
    }

    if (cur_ok) {
        qps     -= cur_qps;
        latency -= cur_latency;
        --num;
    }

    if (num == 0) {
        qps = cur_qps;
        latency = cur_latency;
    }
    else {
        qps /= num;
        latency /= num;
    }
    return 0;
}

int32_t CMTree::setNodeValid(MsgInput* input, MsgOutput* output)
{
    ValidInput* in = (ValidInput*)input;
    ValidOutput* out = (ValidOutput*)output;
    out->type = in->type;

    char key[64];
    if(getProtocolIpPortKey(in->protocol, in->ip, in->port, key) != 0)
        return -1;
    std::map<std::string, CMISNode*>::iterator iter = m_misnodeMap.find(key);
    if(iter == m_misnodeMap.end())
        return -1;
    CMISNode* cpNode = iter->second;
    if(cpNode == NULL)
        return -1;

    pthread_mutex_lock(&m_state_lock);
    cpNode->m_state.valid = in->valid;
    cpNode->m_state.valid_time = in->op_time.tv_sec;

    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        pthread_mutex_unlock(&m_file_lock);
        pthread_mutex_unlock(&m_state_lock);
        return -1;
    }
    fprintf(fp, "%d#3:%s:%s:%u,%d\n", (int32_t)in->op_time.tv_sec,
            m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port, in->valid);
    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    pthread_mutex_unlock(&m_state_lock);

    out->outlen = 1;
    out->outbuf = new char[out->outlen];
    if(out->outbuf == NULL)
        return -1;
    char* p = out->outbuf;
    *p = (char)in->type;
    p += 1;

    return 0;
}

int32_t CMTree::setClusterState(MsgInput* input, MsgOutput* output)
{
    ClusterStateInput* in = (ClusterStateInput*)input;
    ClusterStateOutput* out = (ClusterStateOutput*)output;
    out->type = in->type;

    pthread_mutex_lock(&m_state_lock);
    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        TLOG("Set Cluster State Failed: m_sys_file %s cannot open", m_sys_file);
        pthread_mutex_unlock(&m_file_lock);
        pthread_mutex_unlock(&m_state_lock);
        return -1;
    }

    if (!strcasecmp(in->clustername, "ALLSEARCH")) {
        std::map<std::string, CMCluster*>::iterator iter;
        for (iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
            CMCluster* cpCluster = iter->second;
            if (cpCluster == NULL || cpCluster->m_type != search_cluster)
                continue;
            cpCluster->m_state_cluster_manual = (estate_cluster)in->ClusterState;
            fprintf(fp, "%d#6:%s,%d\n", (int32_t)in->op_time.tv_sec, cpCluster->m_name, in->ClusterState);
        }
    } else {
        std::map<std::string, CMCluster*>::iterator iter = m_clustermap.find(in->clustername);
        if(iter == m_clustermap.end()) {
            fclose(fp);
            pthread_mutex_unlock(&m_file_lock);
            pthread_mutex_unlock(&m_state_lock);
            return -1;
        }
        CMCluster* cpCluster = iter->second;
        if(cpCluster == NULL) {
            TLOG("Set Cluster State Failed: clustername:%s cannot be found", in->clustername);
            fclose(fp);
            pthread_mutex_unlock(&m_file_lock);
            pthread_mutex_unlock(&m_state_lock);
            return -1;
        }
        cpCluster->m_state_cluster_manual = (estate_cluster)in->ClusterState;
        fprintf(fp, "%d#6:%s,%d\n", (int32_t)in->op_time.tv_sec, in->clustername, in->ClusterState);
    }

    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    pthread_mutex_unlock(&m_state_lock);

    out->outlen = 1;
    out->outbuf = new char[out->outlen];
    if(out->outbuf == NULL)
        return -1;
    char* p = out->outbuf;
    *p = (char)in->type;
    p += 1;

    TLOG("Set Cluster State Success: clustername:%s, state_cluster_manual:%s", in->clustername, in->sClusterState);
    return 0;
}

int32_t CMTree::setEnvVarCommand(MsgInput* input, MsgOutput* output)
{
    EnvVarInput* in = (EnvVarInput*)input;
    EnvVarOutput* out = (EnvVarOutput*)output;
    out->type = in->type;
    strncpy(out->EnvVar_Key, in->EnvVar_Key, 32);
    out->EnvVar_Key[31] = 0;

    pthread_mutex_lock(&m_state_lock);
    if (in->EnvVarCommand == 1) {           // set EnvVar_Key=EnvVar_Value
        if (m_EnvVarMap.size() > 10) {
            strncpy(out->EnvVar_Value, "EnvVarSizeGreaterthan10", 32);
            out->EnvVar_Value[31] = 0;
            TLOG("Set EnvVar Failed. EnvVar size > 10");
            pthread_mutex_unlock(&m_state_lock);
            out->nodeToBuf();
            return 0;
        }
        m_EnvVarMap[std::string(in->EnvVar_Key)] = std::string(in->EnvVar_Value);
        TLOG("Set EnvVar Succeed. %s=%s", in->EnvVar_Key, in->EnvVar_Value);
        strncpy(out->EnvVar_Value, in->EnvVar_Value, 32);
        out->EnvVar_Value[31] = 0;
        
        if(strcmp(in->EnvVar_Key, "watch_on_off") == 0) {
            if(strcmp(in->EnvVar_Value, "on") == 0) {
                m_watchOnOff = 1;
            } else if(strcmp(in->EnvVar_Value, "off") == 0) {
                m_watchOnOff = 0;
            } else {
                m_watchOnOff = 0;
                TWARN("EnvVar value error. %s=%s", in->EnvVar_Key, in->EnvVar_Value);
            }
        }
    } else if (in->EnvVarCommand == 2) {    // get one EnvVar_Value by EnvVar_Key
        std::map<std::string, std::string>::iterator iter = m_EnvVarMap.find(std::string(in->EnvVar_Key));
        if (iter == m_EnvVarMap.end()) {
            strncpy(out->EnvVar_Value, "nil", 32);
            out->EnvVar_Value[31] = 0;
            TLOG("Get EnvVar Failed. EnvVar %s dose not exist", in->EnvVar_Key);
            pthread_mutex_unlock(&m_state_lock);
            out->nodeToBuf();
            return 0;
        }
        std::string EnvVar_Value = iter->second;
        strncpy(out->EnvVar_Value, EnvVar_Value.c_str(), 32);
        out->EnvVar_Value[31] = 0;
    } else if (in->EnvVarCommand == 3) {    // remove one EnvVar by EnvVar_Key
        m_EnvVarMap.erase(std::string(in->EnvVar_Key));
        TLOG("Remove EnvVar Succeed. EnvVar_Key: %s", in->EnvVar_Key);
    }

    pthread_mutex_lock(&m_file_lock);
    FILE* fp = fopen(m_sys_file, "a");
    if(fp == NULL) {
        TLOG("setEnvVarCommand Failed: m_sys_file %s cannot open", m_sys_file);
        fclose(fp);
        pthread_mutex_unlock(&m_file_lock);
        pthread_mutex_unlock(&m_state_lock);
        return -1;
    }
    if (in->EnvVarCommand == 1)
        fprintf(fp, "%d#7:%s,1,%s,\n", (int32_t)in->op_time.tv_sec, in->EnvVar_Key, in->EnvVar_Value);
    else if (in->EnvVarCommand == 3)
        fprintf(fp, "%d#7:%s,3,\n", (int32_t)in->op_time.tv_sec, in->EnvVar_Key);

    fclose(fp);
    pthread_mutex_unlock(&m_file_lock);
    pthread_mutex_unlock(&m_state_lock);

    out->nodeToBuf();

    return 0;
}

int32_t CMTree::getClustermap(MsgInput* input, MsgOutput* output)
{
    getmapInput* in = (getmapInput*)input;
    getmapOutput* out = (getmapOutput*)output;
    out->type = in->type;
    struct stat st;
    if(stat(m_cfg_file_name, &st))
        return -1;
    FILE* fp = fopen(m_cfg_file_name, "rb");
    if(fp == NULL)
        return -1;
    out->outlen = st.st_size + 5;
    out->outbuf = new char[out->outlen];
    if(out->outbuf == NULL) {
        fclose(fp);
        return -1;
    }
    char* p = out->outbuf;
    *p = (char)in->type;
    p += 1;
    *(uint32_t*)p = htonl(in->op_time);
    p += 4;
    fread(p, st.st_size, 1, fp);
    fclose(fp);
    return 0;
}

int32_t CMTree::setClustermap(MsgInput* input, MsgOutput* output) 
{
    setmapInput* in = (setmapInput*)input;
    setmapOutput* out = (setmapOutput*)output;
    out->type = in->type;
    char filename[PATH_MAX+1];
    char *name;


    char fullpath[1024],currfile[1024];  
    struct dirent   *s_dir;  
    struct stat   file_stat;
    strcpy(fullpath, m_cfg_file_name);
    char* path = strrchr(fullpath, '/');
    if(path) {
        *path = 0;
        name = path+1;
    } else {
        strcpy(fullpath, ".");
        name = m_cfg_file_name;
    }
    sprintf(filename, "%s/%d.%s", fullpath, (int32_t)in->op_time, name);
    FILE* fp = fopen(filename, "wb");
    if(fp == NULL) {
        TLOG("Set Clustermap conf file failed, timestamp file %s cannot open", filename);
        return -1;
    }
    fclose(fp);


    DIR *dir = opendir(fullpath);
    while ((s_dir = readdir(dir))!=NULL) {  
        if ((strcmp(s_dir->d_name,".") == 0)
                ||(strcmp(s_dir->d_name,"..") == 0))   
            continue;  
        sprintf(currfile, "%s/%s", fullpath, s_dir->d_name);  
        stat(currfile, &file_stat);  
        if (S_ISDIR(file_stat.st_mode))  
            continue;
        char* time_file = strstr(s_dir->d_name, m_cfg_file_name);
        if(time_file == NULL)
            continue;
        if(atol(s_dir->d_name) != in->op_time)
            remove(currfile);
    }  
    closedir(dir);   

    sprintf(filename, "%s.tmp", m_cfg_file_name);
    fp = fopen(filename, "wb");
    if(fp == NULL) {
        TLOG("Set Clustermap conf file failed, conf file %s cannot open", filename);
        return -1;
    }
    fwrite(in->mapbuf, in->maplen, 1, fp); 
    fclose(fp);
    remove(m_cfg_file_name);
    rename(filename, m_cfg_file_name);

    out->outlen = 1;   
    out->outbuf = new char;   
    if(out->outbuf == NULL)   
        return -1;   
    char* p = out->outbuf;   
    *p = (char)in->type;   
    p += 1; 

    TLOG("Set Clustermap conf file success");

    return 0;
}

int32_t CMTree::checkTimeOut(MsgInput* input, MsgOutput* output)
{
    checkInput* in = (checkInput*)input;

    CMISNode* cpNode;
    struct timeval minUpdateTime = in->minUpdateTime; 

    for(int32_t i = 0; i < m_max_node_num; i++)
    {
        cpNode = m_cppMISNode[i];

        pthread_mutex_lock(&m_state_lock);
        if(cpNode->m_state.nDate.tv_sec < minUpdateTime.tv_sec ||
                (cpNode->m_state.nDate.tv_sec == minUpdateTime.tv_sec && 
                 cpNode->m_state.nDate.tv_usec < minUpdateTime.tv_usec)) {
            cpNode->m_state.state = (char)timeout;
            TLOG("port=%u, cur=%ld, min=%ld", cpNode->m_port, cpNode->m_state.nDate.tv_sec, minUpdateTime.tv_sec); 
            cpNode->m_state.nDate.tv_sec = in->op_time.tv_sec;
            cpNode->m_state.nDate.tv_usec = in->op_time.tv_usec;
        }
        char state = cpNode->m_state.state;
        char laststate = cpNode->m_state.laststate;
        if(cpNode->m_state.valid == 0)
            state = (char)unvalid;
        bool flag = false;
        if(cpNode->m_startTime <= 0 && state == 0) {
            cpNode->m_startTime = in->op_time.tv_sec;
            pthread_mutex_lock(&m_file_lock);
            FILE* fp = fopen(m_sys_file, "a");
            if(fp == NULL) {
                pthread_mutex_unlock(&m_file_lock);
                continue;
            }
            fprintf(fp, "%d#5:%s:%s:%u,\n", (int32_t)cpNode->m_startTime, m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port);
            fclose(fp);
            pthread_mutex_unlock(&m_file_lock);
        }
        if(cpNode->m_startTime <= 0) {
            pthread_mutex_unlock(&m_state_lock);
            continue;
        }
        if(laststate == 0 && state != 0) { // good to bad
            cpNode->m_dead_begin = in->op_time.tv_sec;
            cpNode->m_dead_end = 0;
            TLOG("deadbegin=%ld,deadend=%ld,dead_time=%ld, laststate=%d, state=%d", cpNode->m_dead_begin, cpNode->m_dead_end, cpNode->m_dead_time, laststate, state);
            flag = true;
        } else if(laststate != uninit && laststate != 0 && state == 0) { // bad to good
            cpNode->m_dead_end = in->op_time.tv_sec;
            cpNode->m_dead_time += cpNode->m_dead_end - cpNode->m_dead_begin;
            TLOG("deadbegin=%ld,deadend=%ld,dead_time=%ld, laststate=%d, state=%d", cpNode->m_dead_begin, cpNode->m_dead_end, cpNode->m_dead_time, laststate, state);
            flag = true;
        } else if(laststate == uninit && state) {
            if(cpNode->m_dead_begin <= 0)
                cpNode->m_dead_begin = in->op_time.tv_sec;
            cpNode->m_dead_end = 0;
            flag = true;
            TLOG("deadbegin=%ld,deadend=%ld,dead_time=%ld, laststate=%d, state=%d", cpNode->m_dead_begin, cpNode->m_dead_end, cpNode->m_dead_time, laststate, state);
        }
        if(flag) recordNodeInfo(cpNode);
        cpNode->m_state.laststate = state;
        pthread_mutex_unlock(&m_state_lock);
    }
    return 0;
}

int32_t CMTree::load()
{
    std::map<std::string, std::string> sys;

    char* first, *second;
    if(m_tree_type == 0) { 
        first = m_sys_file;
        second = m_sys_transin_file;
    } else {
        first = m_sys_transin_file;
        second = m_sys_file;
    }
    if(access(second, F_OK) == 0) {
        if(readFiletoSysMap(sys, second) != 0)
            return -1;
    }
    if(access(first, F_OK) == 0) {
        if(readFiletoSysMap(sys, first) != 0)
            return -1;
    }

    char key[64], ip[24], sub_ip[24], protocol_str[32];
    uint16_t port, sub_port;
    eprotocol protocol;
    time_t now_time = time(NULL);

    std::map<std::string, std::string>::iterator iter;
    for (iter = sys.begin(); iter != sys.end(); iter++) {
        char* p1 = (char*)iter->second.c_str();
        int32_t timestamp = atol(p1);

        p1 = strchr(p1, '#');
        if(p1 == NULL)
            continue;
        p1++;
        char* p2 = strchr(p1, ':');
        if(p2 == NULL)
            return -1;
        int type = atol(p1);
        p1 = p2 + 1;

        if(type == 1) { // sub child
            p2 = strchr(p1, ':');
            if(p2 == NULL) 	continue;
            memcpy(protocol_str, p1, p2 - p1);
            protocol_str[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(ip, p1, p2-p1);
            ip[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            port = atol(p1);
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(sub_ip, p1, p2-p1);
            sub_ip[p2-p1] = 0;
            p1 = p2 + 1;
            sub_port = atol(p1);

            if(m_cprotocol.strToProtocol(protocol_str, protocol) != 0)
                continue;
            CMISNode* cpNode = NULL;	
            if(getNode(ip, port, protocol, cpNode) != 0)
                continue;
            if(cpNode->sub_child == NULL || timestamp > cpNode->sub_child->sub_time) {
                CSubInfo* cpSub;
                if(cpNode->sub_child == NULL) {
                    cpSub = (CSubInfo*)allocBuf(sizeof(CSubInfo));
                    if(cpSub == NULL)
                        return -1;
                    cpSub->subState = 0;
                } else {
                    cpSub = cpNode->sub_child;
                }
                strcpy(cpSub->sub_ip, ip);
                cpSub->sub_port = sub_port;
                cpSub->sub_time = timestamp;
                cpNode->sub_child = cpSub;
            }
        } else if(type == 2) { // sub all
            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(sub_ip, p1, p2-p1);
            sub_ip[p2-p1] = 0;
            p1 = p2 + 1;
            sub_port = atol(p1);
            sprintf(key, "%s:%u", sub_ip, sub_port);

            CSubInfo* subInfo;
            std::map<std::string, CSubInfo*>::iterator iter = m_subAllMap.find(key);
            if(iter == m_subAllMap.end()) {
                subInfo = (CSubInfo*)allocBuf(sizeof(CSubInfo));
                if(subInfo == NULL)
                    return -1;
                subInfo->subState = 0;
                strcpy(subInfo->sub_ip, sub_ip);
                subInfo->sub_port = sub_port;
                subInfo->sub_time = timestamp;
                m_subAllMap[key] = subInfo;
                subInfo->next = m_cpSubAll;
                m_cpSubAll = subInfo;
            }
        } else if(type == 3) { // valid unvalid
            p2 = strchr(p1, ':');
            if(p2 == NULL) 	continue;
            memcpy(protocol_str, p1, p2 - p1);
            protocol_str[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(ip, p1, p2-p1);
            ip[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            port = atol(p1);
            p1 = p2 + 1;

            if(m_cprotocol.strToProtocol(protocol_str, protocol) != 0)
                continue;
            CMISNode* cpNode = NULL;
            if(getNode(ip, port, protocol, cpNode) != 0)
                continue;
            if(timestamp > cpNode->m_state.valid_time) {
                cpNode->m_state.valid = (char)atol(p1);
                cpNode->m_state.valid_time = timestamp;
            }
        }	else if(type == 4) { // init, reg, state change
            p2 = strchr(p1, ':');
            if(p2 == NULL) 	continue;
            memcpy(protocol_str, p1, p2 - p1);
            protocol_str[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(ip, p1, p2-p1);
            ip[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            port = atol(p1);
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            int32_t start_time = atol(p1);
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            int32_t dead_begin = atol(p1);
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            int32_t dead_end = atol(p1);
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            int32_t dead_time = atol(p1);
            p1 = p2 + 1;

            char state = atol(p1);

            if(m_cprotocol.strToProtocol(protocol_str, protocol) != 0)
                continue;
            CMISNode* cpNode = NULL;
            if(getNode(ip, port, protocol, cpNode) != 0)
                continue;
            pthread_mutex_lock(&m_state_lock);
            if(cpNode->m_startTime > start_time || cpNode->m_startTime <= 0)
                cpNode->m_startTime = start_time;
            if(dead_end > now_time) {
                pthread_mutex_unlock(&m_state_lock);
                continue;
            }
            if(state == 0) {
                if(dead_begin > 0 && dead_end > 0 &&
                        dead_begin >= cpNode->m_dead_begin && dead_end >= cpNode->m_dead_end) {
                    cpNode->m_dead_begin = dead_begin;
                    cpNode->m_dead_end = dead_end;
                    cpNode->m_state.state = state;
                    cpNode->m_state.laststate = state;
                    gettimeofday(&cpNode->m_state.nDate, NULL);
                }
            } else {
                if(dead_end == 0 && dead_begin > 0) {
                    cpNode->m_dead_begin = dead_begin;
                    cpNode->m_dead_end = dead_end;
                    cpNode->m_state.state = state;
                    cpNode->m_state.laststate = state;
                    gettimeofday(&cpNode->m_state.nDate, NULL);
                }
            }
            if(cpNode->m_dead_time < dead_time)
                cpNode->m_dead_time = dead_time;
            pthread_mutex_unlock(&m_state_lock);
        } else if(type == 5) {
            p2 = strchr(p1, ':');
            if(p2 == NULL) 	continue;
            memcpy(protocol_str, p1, p2 - p1);
            protocol_str[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ':');
            if(p2 == NULL) continue;
            memcpy(ip, p1, p2-p1);
            ip[p2-p1] = 0;
            p1 = p2 + 1;

            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            port = atol(p1);
            p1 = p2 + 1;

            if(m_cprotocol.strToProtocol(protocol_str, protocol) != 0)
                continue;
            CMISNode* cpNode = NULL;
            if(getNode(ip, port, protocol, cpNode) != 0)
                continue;
            if(cpNode->m_startTime > timestamp || cpNode->m_startTime <= 0)
                cpNode->m_startTime = timestamp;
        } else if(type == 6) { // setClusterState   fprintf(fp, "%d#6:%s,%d\n", in->op_time.tv_sec, in->clustername, in->ClusterState);
            char clustername[64];
            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            memcpy(clustername, p1, p2-p1);
            clustername[p2-p1] = 0;
            p1 = p2 + 1;

            char ClusterState = (char)atol(p1);

            if (!strcasecmp(clustername, "ALLSEARCH")) {
                std::map<std::string, CMCluster*>::iterator iter;
                for (iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
                    CMCluster* cpCluster = iter->second;
                    if (cpCluster == NULL || cpCluster->m_type != search_cluster)
                        continue;
                    cpCluster->m_state_cluster_manual = (estate_cluster)ClusterState;
                    cpCluster->m_state_cluster_manual_time = timestamp;
                } 
            } else {
                std::map<std::string, CMCluster*>::iterator iter = m_clustermap.find(clustername);
                if(iter == m_clustermap.end())
                    continue;
                CMCluster* cpCluster = iter->second;
                if(cpCluster == NULL)
                    continue;
                cpCluster->m_state_cluster_manual = (estate_cluster)ClusterState;
                cpCluster->m_state_cluster_manual_time = timestamp;
            }
        } else if (type == 7) { 
            // set EnvVar_Key=EnvVar_Value  fprintf(fp, "%d#7:%s,1,%s,\n", in->op_time.tv_sec, in->EnvVar_Key, in->EnvVar_Value);
            // remove EnvVar_Key            fprintf(fp, "%d#7:%s,3,\n", in->op_time.tv_sec, in->EnvVar_Key);
            char EnvVar_Key[64];
            char EnvVar_Value[64];
            p2 = strchr(p1, ',');
            if(p2 == NULL) continue;
            memcpy(EnvVar_Key, p1, p2-p1);
            EnvVar_Key[p2-p1] = 0;
            p1 = p2 + 1;

            if (*p1 == '1') {
                p2 = strchr(p1, ',');
                if(p2 == NULL) continue;
                p1 = p2 + 1;

                p2 = strchr(p1, ',');
                if(p2 == NULL) continue;
                memcpy(EnvVar_Value, p1, p2-p1);
                EnvVar_Value[p2-p1] = 0;
                p1 = p2 + 1;

                if (m_EnvVarMap.size() > 10) {
                    TLOG("in load(). Set EnvVar Failed. EnvVar size > 10");
                    continue;
                }
                m_EnvVarMap[std::string(EnvVar_Key)] = std::string(EnvVar_Value);
                TLOG("in load(). Set EnvVar Succeed. %s=%s", EnvVar_Key, EnvVar_Value);
            } else if (*p1 == '3') {
                m_EnvVarMap.erase(std::string(EnvVar_Key));
                TLOG("in load(). Remove EnvVar Succeed. EnvVar_Key: %s", EnvVar_Key); 
            } else {
                continue;
            }
        } else {
            continue;
        }
    }
    FILE* fp = fopen(m_sys_bak_file, "w");
    if(fp == NULL)
        return -1;
    CMISNode* cpNode;
    for(int i = 0; i < m_max_node_num; i++) {
        cpNode = m_cppMISNode[i];
        if(cpNode->sub_child)
            fprintf(fp, "%d#1:%s:%s:%u,%s:%u\n", (int32_t)cpNode->sub_child->sub_time,
                    m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip,
                    cpNode->m_port, cpNode->m_ip, cpNode->sub_child->sub_port);
        if(cpNode->m_state.valid_time > 0)
            fprintf(fp, "%d#3:%s:%s:%u,%d\n", (int32_t)cpNode->m_state.valid_time,
                    m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, 
                    cpNode->m_port, cpNode->m_state.valid);
        int state;
        if (cpNode->m_state.valid) {
            state = cpNode->m_state.state;
        } else {
            state = unvalid;
        }
        fprintf(fp, "%d#4:%s:%s:%u,%d:%d:%d:%d:%d\n", (int32_t)cpNode->m_state.nDate.tv_sec, m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port, (int32_t)cpNode->m_startTime, (int32_t)cpNode->m_dead_begin, (int32_t)cpNode->m_dead_end, (int32_t)cpNode->m_dead_time, state);
        fprintf(fp, "%d#5:%s:%s:%u,\n", (int32_t)cpNode->m_startTime, m_cprotocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port);
    }
    CSubInfo* subInfo = m_cpSubAll;
    while(subInfo) {
        fprintf(fp, "%d#2:%s:%u\n", (int32_t)subInfo->sub_time, subInfo->sub_ip, subInfo->sub_port);
        subInfo = subInfo->next;
    }
    for (std::map<std::string, CMCluster*>::iterator iter = m_clustermap.begin(); iter != m_clustermap.end(); iter++) {
        CMCluster* cpCluster = iter->second;
        if (cpCluster == NULL)
            continue;
        fprintf(fp, "%d#6:%s,%d\n", (int32_t)cpCluster->m_state_cluster_manual_time, cpCluster->m_name, cpCluster->m_state_cluster_manual);
    }
    for (std::map<std::string, std::string>::iterator iter = m_EnvVarMap.begin(); iter != m_EnvVarMap.end(); iter++) {
        fprintf(fp, "%d#7:%s,1,%s,\n", (int32_t)now_time, iter->first.c_str(), iter->second.c_str());
    }

    fclose(fp);
    if(access(m_sys_file, 0) == 0){
        /*		int i = 1;
                char name[128];
                while(1) {
                sprintf(name, "%.5d.sys", i); 
                if(access(name, 0)) {
                rename(m_sys_file, name);
                break;
                }
                i++;
                }
         */
        remove(m_sys_file);
    }
    rename(m_sys_bak_file, m_sys_file);
    remove(m_sys_transin_file);

    std::map<std::string, std::string>::iterator it;
    if((it = m_EnvVarMap.find(std::string("watch_on_off"))) != m_EnvVarMap.end()) {
        if(strcmp(it->second.c_str(), "on") == 0) {
            m_watchOnOff = 1;
        } else if(strcmp(it->second.c_str(), "off") == 0) {
            m_watchOnOff = 0;
        } else {
            m_watchOnOff = 0;
            TWARN("EnvVar value error. %s=%s", "watch_on_off", it->second.c_str());
        }
    }

    return 0;
}

int32_t CMTree::readFiletoSysMap(std::map<std::string, std::string> &sys, const char* filename)
{
    char buf[1024];
    FILE* fp = fopen(filename, "r");
    if (fp == NULL)
        return -1;
    std::string str;

    while(fgets(buf, 1000, fp)){
        char* p1 = buf;
        char* p2 = strchr(p1, '#');
        if(p2 == NULL)
            continue;
        char type = atol(p2+1);
        int32_t timestamp = atol(p1);

        p1 = p2 + 1; 
        p2 = strchr(p1, ',');
        if(p2 == NULL) {
            str = p1;	
        }	else {
            *p2 = 0;
            str = p1;
            *p2 = ',';
        }
        std::map<std::string, std::string>::iterator iter = sys.find(str);
        if (iter == sys.end()) {
            sys[str] = buf;
        } else {
            if(type != 5 && timestamp >= atol(iter->second.c_str()))
                sys[str] = buf;
            else if(type == 5 && (atol(iter->second.c_str()) <= 0 ||
                        (timestamp > 0 && timestamp < atol(iter->second.c_str()))))
                sys[str] = buf;
        }
    }
    fclose(fp);
    return 0;
}

int32_t CMTree::parseSpec(const char* spec, char* ip, uint16_t& port, eprotocol& proto)
{
    char* p1 = (char*)spec;
    char* p2 = strchr(p1, ':');
    if(p2 == NULL)
        return -1;
    char pro[64];
    memcpy(pro, p1, p2-p1);
    pro[p2-p1] = 0;

    if(strToProtocol(pro, proto) != 0)
        return -1;

    p1 = p2 + 1;
    p2 = strchr(p1, ':');
    if(p2 == NULL)
        return -1;
    memcpy(ip, p1, p2-p1);
    ip[p2-p1] = 0;

    p1 = p2 + 1;
    port = (uint16_t)atol(p1);

    return 0;
}

int32_t CMTree::clearTree() 
{
    m_clustermap.clear();
    m_relationMap.clear();
    m_misnodeMap.clear();
    m_subAllMap.clear();

    m_max_node_num = 0;
    m_max_cluster_num = 0;
    m_max_relation_num = 0;

    m_cpSubAll = NULL;
    m_left_buf_size = -1;
    char* p = m_szpHead, *bak;
    while(p) {
        bak = p;
        p = *(char**)p;
        delete[] bak;
    }
    m_szpHead = NULL;
    return 0;
}

int32_t CMTree::init(const void* vpInfo, uint32_t tree_time, uint32_t tree_no)
{
    ClientNodeInfo* cpInfo = (ClientNodeInfo*)vpInfo;
    if(m_tree_time != tree_time || m_tree_no != tree_no) {
        clearTree();
        m_cRoot.cpCluster = allocMClusterNode();
        if(m_cRoot.cpCluster == NULL) {
            return -1;
        }
        m_tree_time = tree_time;
        m_tree_no = tree_no;
        strcpy(m_cRoot.cpCluster->m_name, "clustermap");

        char key[64];
        m_max_node_num = cpInfo->max_node_num;
        m_max_cluster_num = cpInfo->max_col_num;
        m_max_relation_num = cpInfo->max_relation_num;
        m_cppMISNode = (CMISNode**)allocBuf(sizeof(CMISNode*) * m_max_node_num);
        m_cppCluster = (CMCluster**)allocBuf(sizeof(CMCluster*) * m_max_cluster_num);
        m_cppRelation = (CMRelation**)allocBuf(sizeof(CMRelation*) * m_max_relation_num);
        if(m_cppMISNode == NULL || m_cppCluster == NULL || m_cppRelation == NULL)
            return -1;

        for(int32_t i = 0; i < m_max_cluster_num; i++) {
            CMCluster* cpCluster = allocMClusterNode();
            if(cpCluster == NULL)
                return -1;
            *cpCluster = cpInfo->cpCluster[i];
            m_cppCluster[i] = cpCluster;
            m_clustermap[cpCluster->m_name] = cpCluster;
            cpCluster->m_cmisnode = allocMISNode(cpCluster->node_num);
            if(cpCluster->m_cmisnode == NULL)
                return -1;
            for(int32_t j = 0; j < cpCluster->node_num; j++) {
                cpCluster->m_cmisnode[j] = cpInfo->cpCluster[i].m_cmisnode[j];
                cpCluster->m_cmisnode[j].m_clustername = cpCluster->m_name;
                cpCluster->m_cmisnode[j].sub_child = NULL;
                getProtocolIpPortKey(cpCluster->m_cmisnode[j].m_protocol, cpCluster->m_cmisnode[j].m_ip, cpCluster->m_cmisnode[j].m_port, key);
                m_misnodeMap[key] = &cpCluster->m_cmisnode[j];
                m_cppMISNode[cpCluster->m_cmisnode[j].m_nodeID] = &cpCluster->m_cmisnode[j]; 
            }
        }

        for(int32_t i = 0; i < m_max_relation_num; i++) {
            CMRelation* cpRelation = allocRelationNode();
            if(cpRelation == NULL)
                return -1;
            m_cppRelation[i] = cpRelation;
            if(cpInfo->cpRelation[i].cpCluster) {
                cpRelation->cpCluster = m_clustermap[cpInfo->cpRelation[i].cpCluster->m_name];
                if(cpRelation->cpCluster == NULL)
                    printf("error\n");
                m_relationMap[cpInfo->cpRelation[i].cpCluster->m_name] = cpRelation;
            }
        }
        CMRelation** child = &m_cRoot.child;
        for(int32_t i = 0; i < m_max_relation_num; i++) {
            if(cpInfo->cpRelation[i].parent && cpInfo->cpRelation[i].parent->cpCluster)
                m_cppRelation[i]->parent = m_relationMap[cpInfo->cpRelation[i].parent->cpCluster->m_name];
            if(m_cppRelation[i]->parent == NULL) {
                m_cppRelation[i]->parent = &m_cRoot;
                *child = m_cppRelation[i];
                child = &m_cppRelation[i]->sibling; 
            }
            m_cppRelation[i]->parent->child_num++;
            if(cpInfo->cpRelation[i].child && cpInfo->cpRelation[i].child->cpCluster)
                m_cppRelation[i]->child = m_relationMap[cpInfo->cpRelation[i].child->cpCluster->m_name];
            if(cpInfo->cpRelation[i].sibling && cpInfo->cpRelation[i].sibling->cpCluster)
                m_cppRelation[i]->sibling = m_relationMap[cpInfo->cpRelation[i].sibling->cpCluster->m_name];
        }
    } else { // same tree
        for(int32_t i = 0; i < m_max_node_num; i++) {
            int32_t id = cpInfo->cpMISNode[i].m_nodeID;
            m_cppMISNode[id]->m_state = cpInfo->cpMISNode[i].m_state;
            m_cppMISNode[id]->m_startTime = cpInfo->cpMISNode[i].m_startTime;
            m_cppMISNode[id]->m_dead_begin = cpInfo->cpMISNode[i].m_dead_begin;
            m_cppMISNode[id]->m_dead_end = cpInfo->cpMISNode[i].m_dead_end;
            m_cppMISNode[id]->m_dead_time = cpInfo->cpMISNode[i].m_dead_time;
        }
    }
    //travelTree(stdout);

    return 0;
}

CMTreeProxy::CMTreeProxy()
{
    //printf("CMTreeProxy new\n");
    _res = NULL;
    _update_count = 0;
}
CMTreeProxy::~CMTreeProxy()
{
    //printf("CMTreeProxy del\n");
    WR_LOCK(_lock);
    if(_res) delete _res;
    _res = NULL;
    WR_UNLOCK(_lock);
}
int32_t CMTreeProxy::init(const char *conffile, char type)
{
    if(conffile==NULL || strlen(conffile) > PATH_MAX)
        return -1;
    _type = type;
    strcpy(_path, conffile);

    if(reload()) 
        return 0;
    return -1;
}
int32_t CMTreeProxy::loadSyncInfo()
{
    if(_res == NULL)
        return -1;

    WR_LOCK(_lock);
    if(_res->load() != 0) {
        TLOG("treeProxy load error");
    }
    WR_UNLOCK(_lock);

    return 0;
}
bool CMTreeProxy::reload()
{
    CMTree * res = new (std::nothrow) CMTree();
    if(!res) return false;
    _update_count++;
    if(res->init(_path, _type, time(NULL), _update_count) != 0) {
        TLOG("treeProxy init tree error, path=%s, type=%d", _path, _type);
        delete res;
        return false;
    }
    time_t tree_time;
    char tree_md5[16];
    if(getTreeVersion(_path, tree_md5, tree_time) != 0) {
        TLOG("treeProxy get tree version error");
        delete res;
        return false;
    }

    WR_LOCK(_lock);
    if(res->load() != 0) {
        TLOG("treeProxy load error");
        delete res;
        return false;
    }
    if(_res) delete _res;
    _res = res;
    _tree_time = tree_time;
    memcpy(_tree_md5, tree_md5, 16);
    WR_UNLOCK(_lock);

    TDEBUG("treeProxy reload Tree Succeed");
    return true;
}
int32_t CMTreeProxy::process(const MsgInput* input, MsgOutput* out)
{
    RD_LOCK(_lock);
    if(!_res) return -1;
    return _res->process(input, out);
    RD_UNLOCK(_lock);

    return 0;
}
int32_t CMTreeProxy::getversion(char* md5, time_t& up_time)
{
    RD_LOCK(_lock);
    memcpy(md5, _tree_md5, 16);
    up_time = _tree_time;
    RD_UNLOCK(_lock);
    return 0;
}
int32_t CMTreeProxy::getTreeVersion(char* cfg_file, char* md5, time_t& up_time)
{
    char fullpath[1024],currfile[1024];  
    struct dirent   *s_dir;  
    struct stat   file_stat;
    strcpy(fullpath, cfg_file);
    char* path = strrchr(fullpath, '/');
    if(path)
        *path = 0;
    else
        strcpy(fullpath, "./");

    time_t max_time = 0;
    DIR *dir = opendir(fullpath);
    while ((s_dir = readdir(dir))!=NULL) {  
        if ((strcmp(s_dir->d_name,".") == 0)
                ||(strcmp(s_dir->d_name,"..") == 0))   
            continue;  
        sprintf(currfile, "%s/%s", fullpath, s_dir->d_name);  
        stat(currfile, &file_stat);  
        if (S_ISDIR(file_stat.st_mode))  
            continue;
        char* time_file = strstr(s_dir->d_name, cfg_file);
        if(time_file == NULL)
            continue;
        if(atol(s_dir->d_name) > max_time) {
            max_time = atol(s_dir->d_name);
        }
    }
    closedir(dir);
    up_time = max_time;
    struct stat st;
    if(stat(cfg_file, &st))
        return -1;
    char* buf = new char[st.st_size + 1];
    if(buf == NULL)
        return -1;
    FILE *fp = fopen(cfg_file, "rb");
    if(fp == NULL) {
        delete[] buf;
        return -1;
    }
    if(fread(buf, st.st_size, 1, fp) != 1) {
        fclose(fp);
        delete[] buf;
        return -1;
    }
    fclose(fp);
    buf[st.st_size] = 0;
    UTIL::MD5(buf, (unsigned char*)md5);
    delete[] buf;
    return 0;
}


}
