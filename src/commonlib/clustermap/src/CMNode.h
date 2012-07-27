/** \file
 *******************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 11792 $
 *
 * $LastChangedDate: 2012-05-03 14:20:42 +0800 (Thu, 03 May 2012) $
 *
 * $Id: CMNode.h 11792 2012-05-03 06:20:42Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_NODE_H_
#define _CM_NODE_H_

#include <ctime>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <mxml.h>
#include "util/ThreadLock.h"
#include "util/ScopedLock.h"

namespace clustermap {

const int32_t TAG_CLUSTER_NUM = 3;
const int32_t TAG_RELATION_NUM = 2;
const int32_t TAG_MISNODE_NUM = 5;
const int32_t BLOCK_BUF_SIZE = 1<<20;

enum eMessageType {
    msg_success,
    msg_unvalid,
    msg_init,
    msg_reinit,
    msg_register,
    msg_sub_child,
    msg_sub_all,
    msg_check_update,
    msg_update_all,
    msg_valid,
    msg_active,
    msg_state_heartbeat,
    msg_check_timeout,
    msg_exit,
    cmd_get_info,
    cmd_get_info_by_name,
    cmd_get_info_by_spec,
    cmd_get_info_allbad,
    cmd_get_info_watchpoint,
    cmd_get_info_allauto,
    cmd_alloc_sub,
    cmd_display_all,
    cmd_alloc_node_by_node,
    cmd_alloc_node_by_name,
    cmd_set_state,
    cmd_set_state_all_normal,
    cmd_get_clustermap,
    cmd_set_clustermap,
    cmd_get_tree,
    msg_ClusterState,
    msg_EnvVarCommand,
    cmd_get_ClusterCount_ByStatus,
    cmd_get_GlobalEnvVar,
    msg_sub_selfcluster,
    cmd_alloc_selfcluster,
    cmd_alloc_subcluster,
    cmd_get_clustercount,
    cmd_get_allnodes_col,
    cmd_get_normalnodes_col,
    cmd_by_url='/'
};

enum eprotocol 
{
    tcp,  // 0
    udp,  // 1
    http
};

class CProtocol {
    public:
        CProtocol();
        int32_t strToProtocol(const char* str, eprotocol& protocol);
        char* protocolToStr(eprotocol protocol);

    private:
        char* m_protocol[3];
};

const int32_t MAX_NODETYPE_COUNT = 11;

enum enodetype
{
    blender,    // 0
    merger,     // merger
    search,     // for phase1 only
    search_doc, // for phase2 only
    search_mix, // search or mix
    doc_mix,	// doc or mix
    self,		// self node
    dispatcher, // for update, level == merger
    builder,    // for update, level == search
    mergerOnSearch, // merger for phase1 only
    mergerOnDetail  // merger for phase2 only
};

class CNodetype {
    public:
        CNodetype();
        int32_t strToNodeType(const char* str, enodetype& type);
        char* nodetypeToStr(enodetype nodetype);

    private:
        char* m_nodetype[7];
};

enum eclustertype
{
    blender_cluster, // 0 
    merger_cluster,  // 1
    search_cluster   // 2	
};

enum estate
{
    normal,			// 0 
    abnormal,  	// 1
    timeout,   	// 2
    unvalid,		// 3
    uninit			// 4
};

enum estate_cluster
{
    WHITE,  // 0
    GREEN,  // 
    YELLOW, // 
    RED     // 
};

struct HostStatus
{
    char valid;
    char state;
    char laststate;
    time_t valid_time;
    struct timeval nDate;
};

struct ClusterInfo {
    char ip[24];
    int port;
    int state;
    int valid;
    int col_id;
    eprotocol protocol; 
};

struct CSubInfo
{
    char sub_ip[24];
    uint16_t sub_port;
    time_t sub_time;
    char subState;
    char* state;
    int32_t state_num;
    CSubInfo* next;
};

/**
 * 订阅的类型
 */
enum esub_type
{
    sub_child,      // 订阅子列
    sub_all,        // 订阅全部节点
    sub_selfcluster // 订阅自身列
};

/*
 * 异常状况下线阀值
 */
struct OfflineLimit {
    uint32_t cpu_limit;
    uint32_t load_one_limit;
    uint32_t io_wait_limit;     // 
    uint32_t offline_limit;     // 下线机器数比例限制
    int32_t  qps_limit;         // qps异常限制，整数表示高于平均比例下线，负值表示低于平均比例下线
    int32_t  latency_limit;     // latency异常限制，整数表示高于平均比例下线，负值表示低于平均比例下线
    uint32_t hold_time;         // 持续时间限制
};

// 节点实体的信息
struct CMISNode
{
    int32_t m_nodeID;
    int32_t m_relationid;
    int32_t m_clusterID;

    char m_ip[24];
    uint16_t m_port;
    eprotocol m_protocol;

    //enum: blender, merger, search, search_doc, search_mix
    enodetype m_type;
    char* m_clustername;

    int32_t m_localid;
    time_t m_startTime;
    time_t m_dead_begin;
    time_t m_dead_end;
    time_t m_dead_time;
    
    uint32_t m_cpu_busy;
    uint32_t m_load_one;
    uint32_t m_io_wait;
    uint32_t m_qps;
    uint32_t m_latency;
    uint32_t m_err_start;

    OfflineLimit m_limit;

    CSubInfo* sub_child;
    CSubInfo* sub_selfcluster;

    //节点状态，为了以后扩展，存储指针
    HostStatus m_state;

    CMISNode();
    ~CMISNode();
    int32_t nodeToBuf(char* buf);
    int32_t bufToNode(char* buf);
    int32_t getSize();
    bool isNodeStateNormal();
};

// Cluster实体的信息
struct CMCluster
{
    int32_t m_clusterID;
    char m_name[64];
    //enum: blender_cluster, merger_cluster, search_cluster
    eclustertype m_type;
    int level;
    bool docsep;

    // cluster 级别的下线设置，默认传给每个节点
    OfflineLimit limit;

    int32_t node_num;
    CMISNode* m_cmisnode;
    char* selfcluster_state;
    estate_cluster m_state_cluster;
    estate_cluster m_state_cluster_manual;
    time_t m_state_cluster_manual_time;

    uint32_t cluster_yellow_cpu;
    uint32_t cluster_yellow_load;
    uint32_t cluster_yellow_count;
    uint32_t cluster_yellow_percent;

    uint32_t cluster_red_cpu;
    uint32_t cluster_red_load;
    uint32_t cluster_red_count;
    uint32_t cluster_red_percent;

    CMCluster();
    ~CMCluster() {}
    int32_t nodeToBuf(char* buf);
    int32_t bufToNode(char* buf);
    int32_t getSize();
};

struct CMRelation
{
    CMCluster* cpCluster;

    CMRelation* parent;
    CMRelation* child;
    CMRelation* sibling;

    int32_t child_num;
    char* child_state;
    char* child_state_cluster;

    int32_t nodeToBuf(char* buf);
    int32_t getSize();
    CMRelation():cpCluster(NULL),parent(NULL),child(NULL),sibling(NULL),child_num(0) {}
};

struct TagName
{
    char* list_name;
    char* cluter_name;
};

}
#endif
