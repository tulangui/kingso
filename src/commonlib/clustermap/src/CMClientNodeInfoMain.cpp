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
 * $Id: CMClientNodeInfoMain.cpp 11379 2012-04-18 09:14:23Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "anet/anet.h"
#include "CMTCPClient.h"
#include "CMClient.h"
#include "kingso_cm.pb.h"

using namespace anet;
using namespace clustermap;

class Args_Nodeinfo {
    public:
        Args_Nodeinfo();
        ~Args_Nodeinfo() { }
        bool parseOption(int argc, char * argv[]);
        const char * getMasterCMServer() const { return _master_cmserver; }
        const char * getSlaveCMServer() const { return _slave_cmserver; }
        const char * getDeleteConfPath() const { return _Delete_path; }
        const char * getRecoverConfPath() const { return _Recover_path; }
        char * getClustermapConfPath() { return _Clustermap_path; }
        bool isPrintAllNodes() const { return _printAllNodes; }
        const char * getCluster_Name() const { return _Cluster_Name; }
        const char * getCluster_State() const { return _Cluster_State; }
        const char * getEnvVar_Key() const { return _EnvVar_Key; }
        const char * getEnvVar_Value() const { return _EnvVar_Value; }
        int32_t getEnvVarCommand() const { return _nEnvVarCommand; }
        bool isAlphaNum_EnvVar(char *EnvVar);
        bool isNum(char ch) { return ( (ch) >= '0' && (ch) <= '9' ); }
        bool isAlpha(char ch) { return (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z')); }
        bool isAlphaNum(const char ch) { return (isAlpha(ch) || isNum(ch)); }    
        char command() { return _command; }

    private:	
        char _master_cmserver[PATH_MAX + 1];
        char _slave_cmserver[PATH_MAX + 1];
        char _Delete_path[PATH_MAX + 1];
        char _Recover_path[PATH_MAX + 1];
        char _Clustermap_path[PATH_MAX + 1];			
        char _Cluster_Name[PATH_MAX + 1];
        char _Cluster_State[PATH_MAX + 1];
        char _EnvVar_Key[PATH_MAX + 1];
        char _EnvVar_Value[PATH_MAX + 1];
        char _command;              // 命令
        bool _printAllNodes;		
        int32_t _nEnvVarCommand;    // Default=0    Set=1    Get=2   Remove=3
};

Args_Nodeinfo::Args_Nodeinfo() 
{
    _command = 0;
    _master_cmserver[0] = '\0';
    _slave_cmserver[0] = '\0';
    _Delete_path[0] = '\0';
    _Recover_path[0] = '\0';
    _Clustermap_path[0] = '\0';
    _Cluster_Name[0] = '\0';
    _Cluster_State[0] = '\0';
    _EnvVar_Key[0] = '\0';
    _EnvVar_Value[0] = '\0';
    _printAllNodes = false;
    _nEnvVarCommand = 0;
}

bool Args_Nodeinfo::parseOption(int argc, char * argv[]) {
    int c;

    if (argc >= 3) {
        strcpy(_master_cmserver, argv[1]);
        strcpy(_slave_cmserver, argv[2]);
        if (argc == 3)
            return true;
    } else {
        TLOG("Wrong option number! argc: %d\n", argc);
        printf("format %s master_cmserver(tcp:ip:tcp_port) slave_cmserver_spec(tcp:ip:tcp_port) [{-d todel.dat} / {-r torec.dat} / {-s newcfg.xml} / {-w all} / {-c Cluster_Name=Cluster_State} / {-e EnvVar_Key=EnvVar_Value} / {-g EnvVar_Key} / {-m EnvVar_Key} /]\n", argv[0]);
        return false;
    }

    bool retFlag = false;
    while((c = getopt(argc, argv, "d:r:s:c:e:g:m:w:u:")) != -1) {
        _command = c;
        switch(c) {
            case 'd':
                strcpy(_Delete_path, optarg); break;
            case 'r':
                strcpy(_Recover_path, optarg); break;
            case 's':
                strcpy(_Clustermap_path, optarg); break;
            case 'u':
                retFlag = true;
                strcpy(_Cluster_Name, optarg); break;
            case 'c':
                {
                    strcpy(_Cluster_Name, optarg);
                    char *ch = strchr(_Cluster_Name, '=');
                    if (!ch) {
                        printf("format: -c Cluster_Name=Cluster_State  or  -c ALLSEARCH=Cluster_State\n");
                        printf("Cluster_State include: WHITE, GREEN, YELLOW, RED.\n");
                        printf("can not set ALLSEARCH clusters' state to RED!\n");
                        return false;
                    }
                    *ch++ = 0;
                    strcpy(_Cluster_State, ch);
                    if (!_Cluster_Name || _Cluster_Name[0] == '\0'
                            || !_Cluster_State || _Cluster_State[0] == '\0'
                            || (strcasecmp(_Cluster_State, "WHITE")
                                && strcasecmp(_Cluster_State, "GREEN")
                                && strcasecmp(_Cluster_State, "YELLOW")
                                && strcasecmp(_Cluster_State, "RED"))) {
                        printf("format: -c Cluster_Name=Cluster_State  or  -c ALLSEARCH=Cluster_State\n");
                        printf("Cluster_State include: WHITE, GREEN, YELLOW, RED.\n");
                        printf("can not set ALLSEARCH clusters' state to RED!\n");
                        return false;
                    }
                    break;
                }
            case 'e':   // set EnvVar_Key=EnvVar_Value
                {
                    strcpy(_EnvVar_Key, optarg);
                    char *ch_e = strchr(_EnvVar_Key, '=');
                    if (!ch_e) {
                        printf("Set EnvVar Format: -e EnvVar_Key=EnvVar_Value\n");
                        printf("EnvVar_Key EnvVar_Value format: [a-zA-Z][a-zA-Z0-9_]*, < 32 characters\n");
                        return false;
                    }
                    *ch_e++ = 0;
                    strcpy(_EnvVar_Value, ch_e);
                    if (!_EnvVar_Key || !_EnvVar_Value || _EnvVar_Key[0] == '\0' || _EnvVar_Value[0] == '\0' 
                            || !isAlphaNum_EnvVar(_EnvVar_Key) || strlen(_EnvVar_Key) >= 32
                            || !isAlphaNum_EnvVar(_EnvVar_Value) || strlen(_EnvVar_Value) >= 32) {
                        printf("Set EnvVar Format: -e EnvVar_Key=EnvVar_Value\n");
                        printf("EnvVar_Key EnvVar_Value format: [a-zA-Z][a-zA-Z0-9_]*, < 32 characters\n");
                        return false;
                    }
                    _nEnvVarCommand = 1;    
                    break;
                }
            case 'g':   // get one EnvVar_Value by EnvVar_Key
                strcpy(_EnvVar_Key, optarg);
                if (!_EnvVar_Key || _EnvVar_Key[0] == '\0' || !isAlphaNum_EnvVar(_EnvVar_Key) || strlen(_EnvVar_Key) >= 32) {
                    printf("Get EnvVar Format: -g EnvVar_Key\n");
                    printf("EnvVar_Key EnvVar_Value format: [a-zA-Z][a-zA-Z0-9_]*, < 32 characters\n");
                    return false;
                }
                _nEnvVarCommand = 2;
                break;
            case 'm':   // remove one EnvVar by EnvVar_Key
                strcpy(_EnvVar_Key, optarg);
                if (!_EnvVar_Key || _EnvVar_Key[0] == '\0' || !isAlphaNum_EnvVar(_EnvVar_Key) || strlen(_EnvVar_Key) >= 32) {
                    printf("Remove EnvVar Format: -m EnvVar_Key\n");
                    printf("EnvVar_Key EnvVar_Value format: [a-zA-Z][a-zA-Z0-9_]*, < 32 characters\n");
                    return false;
                }
                _nEnvVarCommand = 3;
                break;
            case 'w':
                _printAllNodes = true; break;
            case '?':
                if(isprint(optopt)) {
                    TLOG("Unknown option `-%c'.\n", optopt);
                } else {
                    TLOG("Unknown option character `\\x%x'.\n", optopt);
                }
                break;
            default:
                abort();
        }
        if(retFlag) break;
    }
    return true;
}											

bool Args_Nodeinfo::isAlphaNum_EnvVar(char *EnvVar)
{
    if (!EnvVar || EnvVar[0] == '\0')
        return false;
    int i = 0;
    while (EnvVar[i]) {
        if (!isAlphaNum(EnvVar[i]) && EnvVar[i] != '_')
            return false;
        i++;
    }
    return true;
}



int32_t parseAppServer(ValidInput& in, const char* app_server)
{    
    char* p = (char*)app_server;
    char* p1 = strchr(p, ':');
    if(p1 == NULL)
        return -1;
    char proto[10];
    memcpy(proto, p, p1-p);
    proto[p1-p] = 0;

    CProtocol cProtocol;
    if(cProtocol.strToProtocol(proto, in.protocol) != 0)
        return -1;

    p = p1 + 1;
    p1 = strchr(p, ':');
    if(p1 == NULL)
        return -1;
    *p1++ = 0;
    strcpy(in.ip, p);

    p = p1;
    in.port = (uint16_t)atol(p);

    return 0;
}

bool showCluster(const char* listName, const char* map_server)
{
    CMTCPClient cClient;
    cClient.setServerAddr(map_server);
    cClient.start();

    char url[1024];
    int len = sprintf(url, "/cm?cluster=%s&type=1", listName);
    
    cClient.send(url, len);
    int32_t recv_len = 0;
    char* recv_buf = NULL;
    cClient.recv(recv_buf, recv_len);
    if(recv_len <= 0) {
        printf("recv from %s error\n", map_server);
        return false;
    }
    recv_buf[recv_len] = 0;

    kingso_cm::CMResponse res;
    std::string req(recv_buf);
    if(res.ParseFromArray(recv_buf, recv_len) == false) {
        printf("parse %s response error\n", map_server);
        return false;
    }
    
    kingso_cm::ResponseType type = res.response_type();
    if(type != kingso_cm::CM_SUCCEED) {
        printf("response failed, url=%s", url);
        return true;
    }

    CProtocol prot;
    char strSt[32], strPr[32];

    for(int32_t i = 0; i < res.cluster_num(); i++) {
        const ::kingso_cm::NodeInfo& node = res.nodes(i);
        strcpy(strPr, prot.protocolToStr(clustermap::eprotocol(node.protocol())));
        switch (node.state()) {
            case kingso_cm::Normal:
                sprintf(strSt, "normal");
                break;
            case kingso_cm::timeout:
                sprintf(strSt, "timeout");
                break;
            case kingso_cm::unvalid:
                sprintf(strSt, "unvalid");
                break;
            case kingso_cm::uninit:
                sprintf(strSt, "uninit");
                break;
            default:
                sprintf(strSt, "abnormal");
                break;
        }
        printf("ip=%s, port=%d, protocol=%s, state=%s(%d), col_id=%d\n", node.node_ip().c_str(), 
                node.node_port(), strPr, strSt, node.state(), node.col_id());
    }
    return true;
}

int32_t setNodeValid(ValidInput& in, const char* filename, const char* map_server)
{
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {
        printf("File: %s don't exist!\n", filename);
        return -1;
    }
    char buf[256];

    CMTCPClient cClient;
    cClient.setServerAddr(map_server);
    cClient.start();

    while(fgets(buf, 200, fp)){
        int len = strlen(buf) - 1;
        while(len > 0 && (buf[len] == '\r' || buf[len] == '\n' || buf[len] == ' ' || buf[len] == '\t')) len--;
        if(len <= 0) continue;
        buf[++len] = 0;

        if(parseAppServer(in, buf)) {
            printf("parse %s error\n", buf);
            continue;
        }
        if(in.nodeToBuf() <= 0) {
            printf("node to buf error\n");
            continue;
        }

        cClient.send(in.buf, in.len);
        int32_t recv_len = 0;
        char* recv_buf = NULL;
        cClient.recv(recv_buf, recv_len);
        char *sValid;
        if(in.valid == 0)
            sValid = "unvalid";
        else
            sValid = "valid";
        ValidOutput out;
        if(recv_len <= 0 || out.isSuccess(recv_buf, recv_len) == false)
            printf("set node %s failed:ip=%s, port=%u, server_spec=%s\n", sValid, in.ip, in.port, map_server);
        else
            printf("set node %s success:ip=%s, port=%u, server_spec=%s\n", sValid, in.ip, in.port, map_server);
    }
    fclose(fp);

    return 0;	
}

int32_t setNodeValid_auto(ValidInput& in, const char* filename, const char* map_server)
{
    FILE* fp = fopen(filename, "r");
    if(fp == NULL) {
        TLOG("File: %s don't exist!\n", filename);
        return -1;
    }
    char buf[256];
    char buf_port[16];

    CMTCPClient cClient;
    cClient.setServerAddr(map_server);
    if (cClient.start() != 0) {
        fclose(fp);
        TLOG("TCPClient start() to server (%s) fail!\n", map_server);
        return -1;
    }

    int count_succeed = 0;
    int count_fail = 0;
    std::string st0;
    std::string str_fail;
    CProtocol prot;

    while(fgets(buf, 200, fp)){
        int len = strlen(buf) - 1;
        while(len > 0 && (buf[len] == '\r' || buf[len] == '\n' || buf[len] == ' ' || buf[len] == '\t')) len--;
        if(len <= 0) continue;
        buf[++len] = 0;

        if(parseAppServer(in, buf)) {
            TLOG("parse %s error\n", buf);
            continue;
        }
        if(in.nodeToBuf() <= 0) {
            TLOG("node to buf error\n");
            continue;
        }

        cClient.send(in.buf, in.len);
        int32_t recv_len = 0;
        char* recv_buf = NULL;
        if (cClient.recv(recv_buf, recv_len) != 0 || recv_len <= 0) {
            fclose(fp);
            TLOG("recv_len <= 0 to server (%s)\n", map_server);
            return -1;
        }
        char *sValid;
        if(in.valid == 0)
            sValid = "unvalid";
        else
            sValid = "valid";
        ValidOutput out;
        if(out.isSuccess(recv_buf, recv_len) == false) {
            count_fail++;
            if (count_fail > 1) {
                str_fail += ",";
            }
            str_fail += std::string(prot.protocolToStr(in.protocol)) + ":";
            str_fail += in.ip;
            sprintf(buf_port, ":%u", in.port);
            str_fail += buf_port;
            TLOG("set node %s failed:ip=%s, port=%u, server_spec=%s\n", sValid, in.ip, in.port, map_server);			
        } else {
            count_succeed++;
            if (count_succeed > 1) {
                st0 += ","; 
            }       
            st0 += std::string(prot.protocolToStr(in.protocol)) + ":";
            st0 += in.ip;
            sprintf(buf_port, ":%u", in.port);
            st0 += buf_port;
            TLOG("set node %s success:ip=%s, port=%u, server_spec=%s\n", sValid, in.ip, in.port, map_server);
        }
    }
    fclose(fp);
    printf("Succeeded (%d): %s\n", count_succeed, st0.c_str());
    printf("Failed (%d): %s\n", count_fail, str_fail.c_str());
    return 0;	
}

int32_t setClusterState_auto(ClusterStateInput& in, const char* map_server)
{
    CMTCPClient cClient;
    cClient.setServerAddr(map_server);
    if (cClient.start() != 0) {
        TLOG("TCPClient start() to server (%s) fail!\n", map_server);
        return -1;
    }

    if(in.nodeToBuf() <= 0) {
        TLOG("node to buf error\n");
        return -1;
    }

    ClusterStateOutput out;
    cClient.send(in.buf, in.len);
    int32_t recv_len = 0;
    char* recv_buf = NULL;
    if (cClient.recv(recv_buf, recv_len) != 0 || recv_len <= 0) {
        TLOG("recv_len <= 0 to server (%s)\n", map_server);
        return -1;
    }
    if(out.isSuccess(recv_buf, recv_len) == false) {
        TLOG("set Cluster State %s=%s failed\n", in.clustername, in.sClusterState);
        printf("Failed (%s=%s)\n", in.clustername, in.sClusterState);
    } else {
        TLOG("set Cluster State %s=%s success\n", in.clustername, in.sClusterState);
        printf("Succeeded (%s=%s)\n", in.clustername, in.sClusterState);
    }
    return 0;
}

int32_t EnvVarCommand_auto(EnvVarInput& in, const char* map_server)
{
    CMTCPClient cClient;
    cClient.setServerAddr(map_server);
    if (cClient.start() != 0) {
        TLOG("TCPClient start() to server (%s) fail!\n", map_server);
        return -1;
    }

    if(in.nodeToBuf() <= 0) {
        TLOG("node to buf error\n");
        return -1;
    }

    EnvVarOutput out;
    cClient.send(in.buf, in.len);
    int32_t recv_len = 0;
    char* recv_buf = NULL;
    if (cClient.recv(recv_buf, recv_len) != 0 || recv_len <= 0) {
        TLOG("recv_len <= 0 to server (%s)\n", map_server);
        return -1;
    }
    out.bufToNode(recv_buf, recv_len);
    if(out.isSuccess(recv_buf, recv_len) == false) {
        if (in.EnvVarCommand == 1) { // Set EnvVar
            TLOG("Set EnvVar %s=%s failed\n", in.EnvVar_Key, in.EnvVar_Value);
            printf("Failed (%s=%s)\n", in.EnvVar_Key, in.EnvVar_Value);
        } else if (in.EnvVarCommand == 2) { // Get EnvVar
            TLOG("Get EnvVar %s failed\n", in.EnvVar_Key);
            printf("Failed (%s)\n", in.EnvVar_Key);
        } else if (in.EnvVarCommand == 3) { // Remove EnvVar
            TLOG("Remove EnvVar %s failed\n", in.EnvVar_Key);
            printf("Failed (%s)\n", in.EnvVar_Key);
        }
    } else {
        if (in.EnvVarCommand == 1) { // Set EnvVar
            if (strcmp(out.EnvVar_Value, "EnvVarSizeGreaterthan10")) {
                TLOG("Set EnvVar %s=%s success\n", in.EnvVar_Key, in.EnvVar_Value);
                printf("Succeeded (%s=%s)\n", out.EnvVar_Key, out.EnvVar_Value);
            } else {
                TLOG("Set EnvVar %s=%s failed\n", in.EnvVar_Key, in.EnvVar_Value);
                printf("Failed (%s=%s)\n", out.EnvVar_Key, out.EnvVar_Value);
            }
        } else if (in.EnvVarCommand == 2) { // Get EnvVar
            if (strcmp(out.EnvVar_Value, "nil")) {
                TLOG("Get EnvVar %s success\n", in.EnvVar_Key);
                printf("Succeeded (%s=%s)\n", out.EnvVar_Key, out.EnvVar_Value);
            } else {
                TLOG("Get EnvVar %s failed\n", in.EnvVar_Key);
                printf("Failed (%s=%s)\n", out.EnvVar_Key, out.EnvVar_Value);
            }
        } else if (in.EnvVarCommand == 3) { // Remove EnvVar
            TLOG("Remove EnvVar %s success\n", in.EnvVar_Key);
            printf("Succeeded (%s)\n", in.EnvVar_Key);
        }
    }
    return 0;
}

int32_t getNodeInfo(const char* server_spec, getNodeInput* in, getNodeOutput* out)
{
    CMTCPClient cClient;
    cClient.setServerAddr(server_spec);
    cClient.start();
    CNodetype cNodeType;
    CProtocol cProtocol;

    char* replyBody = NULL;
    int32_t replyLen = 0;
    if(cClient.send(in->buf, in->len) != 0) {
        printf("send to server %s failed\n", server_spec);
    }
    if(cClient.recv(replyBody, replyLen) != 0) {
        printf("Nodes Infomation from Server(%s)\n", server_spec);
        printf("Nodeinfo failed: maybe don't exist!\n");
    } else {
        printf("Nodes Infomation from Server(%s)\n", server_spec);
        char* p = replyBody;
        eMessageType type = (eMessageType)(*p);
        if (type == cmd_get_info_watchpoint) {			
            p += 1;
            int32_t node_num = ntohl(*(int32_t*)p);
            p += 4;
            for (int32_t j = 0; j < node_num; j++) {
                printf("\t");
                printf("%s", cProtocol.protocolToStr((eprotocol)*p));
                p += 1;
                std::string ip = p;
                printf(":%s", ip.c_str());
                p += 24;
                printf(":%u", ntohs(*(uint16_t*)p));
                p += 2;
                printf("\t");
                estate state = (estate)(*p);
                p += 1;
                if (state == normal) {
                    printf("state:normal");
                } else if (state == abnormal) {
                    printf("state:abnormal");
                } else if (state == timeout) {
                    printf("state:timeout");
                } else if (state == unvalid) {
                    printf("state:unvalid");
                } else if (state == uninit) {
                    printf("state:uninit");
                } else {
                    printf("state:unknown");
                }
                printf("\tCPU_Busy:  %u%%", ntohl(*(uint32_t*)p));
                p += 4;
                printf("\tLoad_One:  %u\n", ntohl(*(uint32_t*)p));
                p += 4;
                printf("\tIo_wait:   %u\n", ntohl(*(uint32_t*)p));
                p += 4;
                printf("\tQps:       %u\n", ntohl(*(uint32_t*)p));
                p += 4;
                printf("\tLatency:   %u\n", ntohl(*(uint32_t*)p));
                p += 4;
            }
            return 0;
        }
        if(out->bufToNode(replyBody, replyLen) <= 0) {
            printf("communicate(%s) error\n", server_spec);
            return -1;
        }
        if (out->type != cmd_get_info_allbad) {
            if (out->cpCluster->m_type == blender_cluster) {
                printf(" clustertype:blender_cluster");   
            } else if (out->cpCluster->m_type == merger_cluster) {
                printf(" clustertype:merger_cluster");    
            } else if (out->cpCluster->m_type == search_cluster) {
                printf(" clustertype:search_cluster");    
            } else {
                printf(" clustertype:unknown");
            }
            printf(" clustername:%s", out->cpCluster->m_name);
            printf(" rowcount:%d", out->cpCluster->node_num); 
            printf(" level:%d", out->cpCluster->level); 
            printf("\n");
        }
        CMISNode* cpNode = out->cpNode;
        time_t now = time(NULL);
        if (out->type == cmd_get_info_allbad) {   
            std::cout << "There are " << out->node_num << " Unavailable Nodes." << std::endl;   
        }
        for (int32_t j = 0; j < out->node_num; j++, cpNode++) {
            printf("\tNode--nodetype:%s %s:%s:%u", cNodeType.nodetypeToStr(cpNode->m_type), 
                    cProtocol.protocolToStr(cpNode->m_protocol), cpNode->m_ip, cpNode->m_port);
            if (cpNode->sub_child)
                printf(" sub:%s:%u", cpNode->sub_child->sub_ip, cpNode->sub_child->sub_port);
            else
                printf(" sub:----");
            printf("\n");
            printf("\t");
            estate state = (estate)cpNode->m_state.state;
            if (state == normal) {
                printf("state:normal");
            } else if (state == abnormal) {
                printf("state:abnormal");
            } else if (state == timeout) {
                printf("state:timeout");
            } else if (state == unvalid) {
                printf("state:unvalid");
            } else if (state == uninit) {
                printf("state:uninit");
            } else {
                printf("state:unknown");
            }
            printf("\tDate:%s", ctime(&(cpNode->m_state.nDate.tv_sec)));  
            if (cpNode->m_startTime != 0) {
                printf("\tStart:%s", ctime(&(cpNode->m_startTime)));
                if (cpNode->m_dead_begin != 0) {
                    printf("\tLast Dead Begin:%s", ctime(&(cpNode->m_dead_begin)));
                } else {
                    printf("\tLast Dead Begin: /\n");
                }
                if (cpNode->m_dead_end != 0 && cpNode->m_state.state == normal) {
                    printf("\tLast Dead End:%s", ctime(&(cpNode->m_dead_end)));
                } else {
                    printf("\tLast Dead End: /\n");
                }
                time_t dead_time = cpNode->m_dead_time;
                if (cpNode->m_state.state != normal && now > cpNode->m_dead_begin) {    
                    dead_time += now - cpNode->m_dead_begin;
                }   
                if (dead_time > 0)
                    printf("\tTotal Dead Time: %d:%d:%d", (int32_t)dead_time/3600, (int32_t)dead_time%3600/60, (int32_t)dead_time%3600%60);
                else 
                    printf("\tTotal Dead Time: /");
                time_t alive_time = now - cpNode->m_startTime - dead_time;
                if (alive_time > 0)
                    printf("\tTotal Alive Time: %d:%d:%d", (int32_t)alive_time/3600, (int32_t)alive_time%3600/60, (int32_t)alive_time%3600%60);
                else
                    printf("\tTotal Alive Time: /");
            } else {
                printf("\tStart: Never Started!");
            }
            printf("\n\tCPU_Busy:%u%% Load_One:%u Io_wait:%u Qps:%u Latency:%d\n",
                    cpNode->m_cpu_busy, cpNode->m_load_one, cpNode->m_io_wait, cpNode->m_qps, cpNode->m_latency);
            printf("\n\n");
        }
    }
    return 0;
}

int32_t getNodeInfo_auto(const char* server_spec, getNodeInput* in, getNodeOutput* out)
{
    CMTCPClient cClient;
    cClient.setServerAddr(server_spec);
    cClient.start();
    CNodetype cNodeType;
    CProtocol prot;

    std::string strBlender;
    std::string strMerger;
    std::string strSearcher;
    std::string strCommon;
    char buf[1024];
    char buf_port[16];

    char* replyBody = NULL;
    int32_t replyLen = 0;
    if(cClient.send(in->buf, in->len) != 0) {
        TLOG("send to server %s failed\n", server_spec);
        return -1;
    }
    if(cClient.recv(replyBody, replyLen) != 0) {
        TLOG("Nodes Infomation from Server(%s)\n", server_spec);
        TLOG("Nodeinfo failed: maybe don't exist! replyLen=%d\n", replyLen);
        return -1;
    } else {
        TLOG("Nodes Infomation from Server(%s)\n", server_spec);
        char* p = replyBody;
        eMessageType type = (eMessageType)(*p);
        if (type == cmd_get_info_allauto) {			
            p += 1;
            int32_t max_cluster_num = ntohl(*(int32_t*)p);
            p += 4;
            //int32_t max_node_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t blender_good_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t blender_bad_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t merger_good_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t merger_bad_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t searcher_good_num = ntohl(*(int32_t*)p);
            p += 4;
            int32_t searcher_bad_num = ntohl(*(int32_t*)p);
            p += 4;
            sprintf(buf, "blender: Total (%d), Succeeded (%d), Failed (%d)\n----------\n", blender_good_num + blender_bad_num, blender_good_num, blender_bad_num);
            strBlender = buf;
            sprintf(buf, "merge: Total (%d), Succeeded (%d), Failed (%d)\n----------\n", merger_good_num + merger_bad_num, merger_good_num, merger_bad_num);
            strMerger = buf;
            sprintf(buf, "search: Total (%d), Succeeded (%d), Failed (%d)\n----------\n", searcher_good_num + searcher_bad_num, searcher_good_num, searcher_bad_num);
            strSearcher = buf;			
            for (int32_t i = 0; i < max_cluster_num; i++) {
                CMCluster* cpCluster = new CMCluster;
                strCommon.clear();
                p += cpCluster->bufToNode(p);
                strCommon = cpCluster->m_name;
                strCommon += " ";
                if (cpCluster->m_state_cluster == GREEN) {
                    strCommon += "GREEN";
                } else if (cpCluster->m_state_cluster == YELLOW) {
                    strCommon += "YELLOW";
                } else if (cpCluster->m_state_cluster == RED) {
                    strCommon += "RED";
                } else {
                    strCommon += "ERROR";
                }
                strCommon += "\n";
                int32_t node_num = cpCluster->node_num;
                for(int32_t j = 0; j < node_num; j++) {
                    CMISNode* cpNode = new CMISNode;	
                    p += cpNode->bufToNode(p);
                    strCommon += "  " + std::string(prot.protocolToStr(cpNode->m_protocol)) + ":";
                    strCommon += cpNode->m_ip;			
                    sprintf(buf_port, ":%u", cpNode->m_port);			
                    strCommon += buf_port;
                    if (cpNode->m_state.state == normal) {
                        strCommon += ": ok";
                    } else if (cpNode->m_state.state == unvalid) {
                        strCommon += ": offline";
                    } else {
                        strCommon += ": failed";
                    }
                    strCommon += "\n";					
                    delete cpNode;
                }
                if (cpCluster->m_type == blender_cluster) {
                    strBlender += strCommon;
                } else if (cpCluster->m_type == merger_cluster) {
                    strMerger += strCommon;
                } else if (cpCluster->m_type == search_cluster) {
                    strSearcher += strCommon;
                }
                delete cpCluster;
            }
            printf("%s\n", strBlender.c_str());
            printf("%s\n", strMerger.c_str());
            printf("%s", strSearcher.c_str());
            // EnvVar Part
            if (replyLen - (p - replyBody) <= 0) {
                return 0;
            }                
            char *pchEnvVar_1 = NULL;
            char *pchEnvVar_2 = NULL;
            char *pchEnvVar_3 = NULL;
            int32_t count_EnvVar = 0;
            // CMTree::getStateUpdate()     "^%s=%s$", iter->first.c_str(), iter->second.c_str()
            char *buf_EnvVar = new char[replyLen - (p - replyBody) + 1];
            if (!buf_EnvVar) {
                TLOG("buf_EnvVar malloc failed");
                return -1;
            }
            memcpy(buf_EnvVar, p, replyLen - (p - replyBody));
            buf_EnvVar[replyLen - (p - replyBody)] = '\0';
            p = buf_EnvVar;
            while ((pchEnvVar_1 = strchr(p + count_EnvVar, '^'))) { 
                if ((pchEnvVar_2 = strchr(p + count_EnvVar, '=')) == NULL) {
                    count_EnvVar++;
                    continue;
                }        
                if ((pchEnvVar_3 = strchr(p + count_EnvVar, '$')) == NULL) {
                    count_EnvVar++;
                    continue;
                }
                *pchEnvVar_2 = 0;
                *pchEnvVar_3 = 0;
                printf("EnvVar %s = %s\n", pchEnvVar_1 + 1, pchEnvVar_2 + 1);
                count_EnvVar += pchEnvVar_3 - pchEnvVar_1 + 1;
            }
            delete[] buf_EnvVar;

            return 0;
        } else {
            TLOG("msg_type err!\n");
            return -1;
        }
    }
    return -1;
}

int32_t getClusterMap(const char* spec, char* filename)
{
    getmapInput in;
    getmapOutput out;

    in.type = cmd_get_clustermap;
    in.nodeToBuf();

    CMTCPClient cClient;
    cClient.setServerAddr(spec);
    cClient.start();

    if(cClient.send(in.buf, in.len) != 0)
        return -1;
    int32_t recv_len = 0;
    char* recv_buf = NULL;
    if(cClient.recv(recv_buf, recv_len) != 0)
        return -1;
    if(!out.isSuccess(recv_buf, recv_len))
        return -1;
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("open file %s fail!\n", filename);
        return -1;
    }
    fwrite(recv_buf+5, recv_len-5, 1, fp);
    fclose(fp);
    return 0;
}
int32_t setClusterMap(const char* spec, char* filename, time_t op_time)
{
    int ret = access(filename, R_OK);
    if(ret) {
        printf("File: %s don't exist!\n", filename);
        return -1;
    }
    setmapInput in;
    setmapOutput out;

    in.type = cmd_set_clustermap;
    in.mapbuf = filename;
    in.op_time = op_time;
    if(in.nodeToBuf() <= 0)
        return -1;

    CMTCPClient cClient;
    cClient.setServerAddr(spec);
    cClient.start();

    if(cClient.send(in.buf, in.len) != 0)
        return -1;

    int32_t recv_len = 0;
    char* recv_buf = NULL;
    if(cClient.recv(recv_buf, recv_len) != 0)
        return -1;

    if(out.isSuccess(recv_buf, recv_len)) 
        return 0;

    return -1;
}

int32_t setClusterMap_auto(const char* spec, char* filename, time_t op_time)
{
    int ret = access(filename, R_OK);
    if(ret) {
        TLOG("File: %s don't exist!\n", filename);
        return -1;
    }
    setmapInput in;
    setmapOutput out;

    in.type = cmd_set_clustermap;
    in.mapbuf = filename;
    in.op_time = op_time;
    if(in.nodeToBuf() <= 0)
        return -1;

    CMTCPClient cClient;
    cClient.setServerAddr(spec);
    cClient.start();

    if(cClient.send(in.buf, in.len) != 0)
        return -1;

    int32_t recv_len = 0;
    char* recv_buf = NULL;
    if(cClient.recv(recv_buf, recv_len) != 0)
        return -1;

    if(out.isSuccess(recv_buf, recv_len)) 
        return 0;

    return -1;
}


int main(int argc, char** argv)
{
    FILE* fp = fopen("nodeinfo_test.log", "w");
    if(fp == NULL) {
        fprintf(stderr, "can't open file nodeinfo_test.log; maybe Current Path Permission Denied!\n");
        return -1;
    }
    fprintf(fp, "alog.rootLogger=INFO, A\n");
    fprintf(fp, "alog.appender.A=FileAppender\n");
    fprintf(fp, "alog.appender.A.fileName=%s\n", "nodeinfo_test.log");
    fprintf(fp, "alog.appender.A.flush=true\n");
    fclose(fp);
    alog::Configurator::configureLogger("nodeinfo_test.log");
    TLOG("nodeinfo start");

    Args_Nodeinfo args;
    if (!args.parseOption(argc, argv))
        return 1;
    TLOG("argv0:%s 1:%s 2:%s 3:%s 4:%s\n", argv[0], argv[1], argv[2], argv[3], argv[4]);
    TLOG("getMasterCMServer: %s\n", args.getMasterCMServer());
    TLOG("getSlaveCMServer: %s\n", args.getSlaveCMServer());
    TLOG("getDeleteConfPath: %s\n", args.getDeleteConfPath());
    TLOG("getRecoverConfPath: %s\n", args.getRecoverConfPath());
    TLOG("getClustermapConfPath: %s\n", args.getClustermapConfPath());
    TLOG("isPrintAllNodes: %d\n", args.isPrintAllNodes());

    switch (args.command()) {
        case 'u':
            {
                const char* name = args.getCluster_Name();
                if(name[0]) {
                    showCluster(name, args.getMasterCMServer());
                } else {
                    printf("command error\n");
                }
                return 0;
            }
        default:
            break;
    }

    if (*(args.getDeleteConfPath())) {
        ValidInput in;
        in.type = msg_valid;
        in.valid = 0;
        gettimeofday(&in.op_time, NULL);
        TLOG("Sending \"Delete Nodes\" Command to Master Server(%s)\n", args.getMasterCMServer());
        if (setNodeValid_auto(in, args.getDeleteConfPath(), args.getMasterCMServer()) != 0){
            TLOG("Sending \"Delete Nodes\" Command to Slave Server(%s)\n", args.getSlaveCMServer());
            if (setNodeValid_auto(in, args.getDeleteConfPath(), args.getSlaveCMServer()) != 0) {
                return 1;
            }
        }
        return 0;
    } else if (*(args.getRecoverConfPath())) {
        ValidInput in;
        in.type = msg_valid;
        in.valid = 1;
        gettimeofday(&in.op_time, NULL);
        TLOG("Sending \"Recover Nodes\" Command to Master Server(%s)\n", args.getMasterCMServer());
        if(setNodeValid_auto(in, args.getRecoverConfPath(), args.getMasterCMServer()) != 0){
            TLOG("Sending \"Recover Nodes\" Command to Slave Server(%s)\n", args.getSlaveCMServer());
            if (setNodeValid_auto(in, args.getRecoverConfPath(), args.getSlaveCMServer()) != 0) {
                return 1;
            }
        }		
        return 0;
    } else if (*(args.getClustermapConfPath())) {
        time_t op_time = time(NULL);
        TLOG("Set Clustermap Configure File to Master Server(%s)\n", args.getMasterCMServer());
        if (setClusterMap_auto(args.getMasterCMServer(), args.getClustermapConfPath(), op_time) != 0) {
            TLOG("Set Clustermap Configure File to Slave Server(%s)\n", args.getSlaveCMServer());
            if (setClusterMap_auto(args.getSlaveCMServer(), args.getClustermapConfPath(), op_time) != 0) {
                printf("Failed\n");
                return 1;
            }
        }
        printf("Succeeded\n");
        return 0;
    } else if (args.isPrintAllNodes()) {
        getNodeInput in;
        getNodeOutput out;
        in.type = cmd_get_info_allauto;
        if(in.nodeToBuf() <= 0) {
            TLOG("nodeToBuf error\n");
            return 1;
        }
        TLOG("Print All Hosts' State from Master Server(%s)\n", args.getMasterCMServer());
        if(getNodeInfo_auto(args.getMasterCMServer(), &in, &out) != 0) {
            TLOG("Print All Hosts' State from Slave Server(%s)\n", args.getSlaveCMServer());
            if(getNodeInfo_auto(args.getSlaveCMServer(), &in, &out) != 0) {
                return 1;
            }
        }
        return 0;
    } else if (*(args.getCluster_Name()) && *(args.getCluster_State())) {
        ClusterStateInput in;
        in.type = msg_ClusterState;
        strncpy(in.clustername, args.getCluster_Name(), 64);
        strncpy(in.sClusterState, args.getCluster_State(), 32);
        if (!strcasecmp(args.getCluster_State(), "WHITE"))
            in.ClusterState = (char)WHITE;
        else if (!strcasecmp(args.getCluster_State(), "GREEN"))
            in.ClusterState = (char)GREEN;
        else if (!strcasecmp(args.getCluster_State(), "YELLOW"))
            in.ClusterState = (char)YELLOW;
        else if (!strcasecmp(args.getCluster_State(), "RED"))
            in.ClusterState = (char)RED;
        if (!strcasecmp(args.getCluster_Name(), "ALLSEARCH")) {
            if (in.ClusterState == (char)RED) {
                printf("Can not set ALLSEARCH clusters' state to RED!\n");
                return 1;
            }
        }
        gettimeofday(&in.op_time, NULL);
        TLOG("Sending \"Set Cluster State\" Command to Master Server(%s)\n", args.getMasterCMServer());
        if (setClusterState_auto(in, args.getMasterCMServer()) != 0){
            TLOG("Sending \"Set Cluster State\" Command to Slave Server(%s)\n", args.getSlaveCMServer());
            if (setClusterState_auto(in, args.getSlaveCMServer()) != 0) {
                printf("Failed\n");
                return 1;
            }
        }
        //printf("Succeeded\n");
        return 0;
    } else if (args.getEnvVarCommand() != 0) {
        EnvVarInput in;
        in.type = msg_EnvVarCommand;
        in.EnvVarCommand = args.getEnvVarCommand();
        if (*(args.getEnvVar_Key())) {
            strncpy(in.EnvVar_Key, args.getEnvVar_Key(), 32);
        } else {
            in.EnvVar_Key[0] = '\0';
        }
        if (*(args.getEnvVar_Value())) {
            strncpy(in.EnvVar_Value, args.getEnvVar_Value(), 32);
        } else {
            in.EnvVar_Value[0] = '\0'; 
        }
        gettimeofday(&in.op_time, NULL);
        TLOG("Sending \"EnvVarCommand\" Command to Master Server(%s)\n", args.getMasterCMServer());
        if (EnvVarCommand_auto(in, args.getMasterCMServer()) != 0){
            TLOG("Sending \"EnvVarCommand\" Command to Slave Server(%s)\n", args.getSlaveCMServer());
            if (EnvVarCommand_auto(in, args.getSlaveCMServer()) != 0){
                printf("Failed\n");
                return 1;
            }
        }
        //printf("Succeeded\n");
        return 0;
    }

    char input1[1024], input;
    char clustername[64];
    char filename[128];
    while (1) {
        printf("NodeInfo Monitor-----Please choose:\n");
        printf("         n: query by Node;\n");
        printf("         c: query by Clustername;\n");
        printf("         d: Delete nodes by conf file;\n"); 
        printf("         r: Recover nodes by conf file;\n");
        printf("         g: Get Clustermap Configure File;\n"); 
        printf("         s: Set Clustermap Configure File;\n"); 
        printf("         p: Print Clustermap Configure File;\n");
        printf("         b: Print All Unvailable Nodes;\n");
        printf("         w: Print All Hosts' State and WatchPoint;\n");
        printf("         u: Print url req result\n");
        printf("         q: Quit.\n");
        scanf("%s", input1);
        int i = 0;
        while(input1[i] == '\t' || input1[i] == '\n' || input1[i] == '\r') i++;
        input = input1[i];
        if(input == '\0') continue;

        if (input == 'n' || input == 'N') { 
            getNodeInput in;
            getNodeOutput out;
            in.type = cmd_get_info_by_spec;
            printf("Query by Node\tInput format--  protocol:ip:port\n");

            char app_server[64];
            scanf("%s", app_server);

            char* p = (char*)app_server;
            char* p1 = strchr(p, ':');
            if(p1 == NULL) {
                printf("format error, need( protocol:ip:port ), input = %s\n", app_server);
                continue;
            }
            *p1++ = 0;
            CProtocol cProtocol;
            if(cProtocol.strToProtocol(p, in.protocol) != 0) {
                printf("protocol error, support tcp,udp,http, input = %s\n", p);
                continue;
            }

            p = p1;
            p1 = strchr(p, ':');
            if(p1 == NULL) {
                printf("format error, need( protocol:ip:port ), input = %s\n", app_server);
                continue;
            }
            *p1++ = 0;
            strcpy(in.ip, p);

            p = p1;
            in.port = (uint16_t)atol(p);
            if(in.port == 0) {
                printf("port error, input = %u\n", in.port);
                continue;
            }
            if(in.nodeToBuf() <= 0) {
                printf("nodeToBuf error\n");
                continue;
            }
            if(getNodeInfo(args.getMasterCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getMasterCMServer());
            if(getNodeInfo(args.getSlaveCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getSlaveCMServer());
        } else if (input == 'c' || input == 'C') {
            getNodeInput in;
            getNodeOutput out;
            in.type = cmd_get_info_by_name;
            printf("Query by Clustername\tPlease input clustername:\n");
            scanf("%s", clustername);
            strncpy(in.name, clustername, 64);
            in.name[63] = 0;
            if(in.nodeToBuf() <= 0) {
                printf("nodeToBuf error\n");
                continue;
            }
            if(getNodeInfo(args.getMasterCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getMasterCMServer());
            if(getNodeInfo(args.getSlaveCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getSlaveCMServer());
        } else if (input == 'd' || input == 'D') {
            ValidInput in;
            in.type = msg_valid;
            in.valid = 0;
            gettimeofday(&in.op_time, NULL);
            printf("Delete nodes by conf file\tPlease input conf file's name:\n");
            scanf("%s", filename);
            printf("Sending \"Delete Nodes\" Command to Server(%s):\n", args.getMasterCMServer());
            setNodeValid(in, filename, args.getMasterCMServer());
            printf("Sending \"Delete Nodes\" Command to Server(%s):\n", args.getSlaveCMServer());
            setNodeValid(in, filename, args.getSlaveCMServer());
        } else if (input == 'r' || input == 'R') {
            ValidInput in;
            in.type = msg_valid;
            in.valid = 1;
            gettimeofday(&in.op_time, NULL);
            printf("Recover nodes by conf file\tPlease input conf file's name:\n");
            scanf("%s", filename);			
            printf("Sending \"Recover Nodes\" Command to Server(%s):\n", args.getMasterCMServer());
            setNodeValid(in, filename, args.getMasterCMServer());
            printf("Sending \"Recover Nodes\" Command to Server(%s):\n", args.getSlaveCMServer());
            setNodeValid(in, filename, args.getSlaveCMServer());
        } else if (input == 'g' || input == 'G') {
            printf("Get Clustermap Configure File\tPlease input local conf file's name:\n");
            scanf("%s", filename);
            if (getClusterMap(args.getMasterCMServer(), filename) != 0) {
                if (getClusterMap(args.getSlaveCMServer(), filename) != 0) {
                    printf("Get Clustermap Configure File Failed!\n");				
                    continue;
                }
            }
            printf("Get Clustermap Configure File Succeed!\n");
        } else if (input == 's' || input == 'S') {
            printf("Set Clustermap Configure File\tPlease input local conf file's name:\n");
            scanf("%s", filename);
            time_t op_time = time(NULL);
            if (setClusterMap(args.getMasterCMServer(), filename, op_time) != 0) {
                printf("Set Clustermap Configure File to Server(%s) Failed!\n", args.getMasterCMServer());
            } else {
                printf("Set Clustermap Configure File to Server(%s) Succeed!\n", args.getMasterCMServer());
            }
            if (setClusterMap(args.getSlaveCMServer(), filename, op_time) != 0) {
                printf("Set Clustermap Configure File to Server(%s) Failed!\n", args.getSlaveCMServer());
            } else {
                printf("Set Clustermap Configure File to Server(%s) Succeed!\n", args.getSlaveCMServer());
            }
        } else if (input == 'p' || input == 'P') {
            printf("Print Clustermap Configure File\tPlease input local conf file's name:\n");
            scanf("%s", filename);
            fprintf(stdout, "\n\n"); 
            struct stat st;
            if(stat(filename, &st) != 0) {
                printf("File: %s don't exist!\n", filename);
                continue;
            }
            char * pstr = new char[st.st_size + 1];
            FILE * fp = fopen(filename, "r");
            if(!fp) {
                delete[] pstr;
                printf("File: %s don't exist!\n", filename);
                continue;
            }
            ssize_t rd = fread(pstr, 1, st.st_size, fp);
            if(rd > 0) {
                fwrite(pstr, 1, rd, stdout);
            }
            delete[] pstr;
            fprintf(stdout, "\n\n");
            fclose(fp);

            CMTree cCMTree;
            if(cCMTree.init(filename, 0, time(NULL), 1) != 0) {
                printf("error config file\n");
            } else {
                printf("correct config file\n");

            }
        } else if (input == 'b' || input == 'B') {
            getNodeInput in;
            getNodeOutput out;
            in.type = cmd_get_info_allbad;
            printf("Print All Unvailable Nodes:\n");
            if(in.nodeToBuf() <= 0) {
                printf("nodeToBuf error\n");
                continue;
            }	
            if(getNodeInfo(args.getMasterCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getMasterCMServer());
            if(getNodeInfo(args.getSlaveCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getSlaveCMServer());
        } else if (input == 'w' || input == 'W') {
            getNodeInput in;
            getNodeOutput out;
            in.type = cmd_get_info_watchpoint;
            printf("Print All Hosts' State and WatchPoint:\n");
            if(in.nodeToBuf() <= 0) {
                printf("nodeToBuf error\n");
                continue;
            }
            if(getNodeInfo(args.getMasterCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getMasterCMServer());
            if(getNodeInfo(args.getSlaveCMServer(), &in, &out) != 0)
                printf("get from (%s) error\n", args.getSlaveCMServer());
        } else if (input == 'u' || input == 'U') {
            char listName[1024];
            scanf("%s", listName);
            showCluster(listName, args.getMasterCMServer());
        } else if (input == 'q' || input == 'Q') {
            break;
        } else {
            printf("Wrong args, please choose again!\n");
            continue;
        }
    }

    alog::Logger::shutdown();
    return 0;
}
