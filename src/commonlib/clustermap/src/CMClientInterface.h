/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 8984 $
 *
 * $LastChangedDate: 2011-12-16 16:27:22 +0800 (Fri, 16 Dec 2011) $
 *
 * $Id: CMClientInterface.h 8984 2011-12-16 08:27:22Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_CLIENT_INTERFACE_H_
#define _CM_CLIENT_INTERFACE_H_

#include "CMTree.h"

namespace clustermap {

enum LogicOp {
    EQ,		// µÈÓÚ
    GE,		// ´óÓÚµÈÓÚ
    LE,		// Ð¡ÓÚµÈÓÚ
    UNKNOWN
};

struct allocSubInput : public MsgInput {
    enodetype node_type;
    int level;
    LogicOp logic_op;
    uint32_t locating_signal;
};
struct allocSubOutput : public MsgOutput {
    int32_t* ids;
    int32_t size;
};

struct allocNodeInput : public MsgInput {
    enodetype node_type;
    union {
        CMISNode* cpNode;
        char* name;
    };
};
struct allocNodeOutput : public MsgOutput {
    CMISNode* cpNode;
};

struct displayInput : public MsgInput {
};
struct displayOutput : public MsgOutput {
    displayOutput() : cpNode(NULL), node_num(0) {};
    ~displayOutput() {if(cpNode) delete[] cpNode;};

    CMISNode* cpNode;
    int32_t node_num;
};

struct setStatInput : public MsgInput {
    char* stateBitmap;
    int32_t node_num;
};
struct setStatOutput : public MsgOutput {
};

struct getTreeInput : public MsgInput {
    int32_t tree_no;
    int32_t tree_time;
    CMTree* tree;
};
struct getTreeOutput : public MsgOutput {
    CMTree* tree;
};

struct getClusterCountByStatusInput : public MsgInput {
    estate_cluster state_cluster;
};
struct getClusterCountByStatusOutput : public MsgOutput {
    uint32_t ClusterCount;
};

struct getGlobalEnvVarInput : public MsgInput {
    std::string EnvVar_Key;
};          
struct getGlobalEnvVarOutput : public MsgOutput {
    std::string EnvVar_Value;
};      

struct getNodesInput : public MsgInput {
    enodetype node_type;
    int32_t subcluster_num;
};
struct getNodesOutput : public MsgOutput {
    int32_t* ids;
    int32_t size;
};

class CMClientInterface
{
    public:
        CMClientInterface();
        virtual ~CMClientInterface();

        virtual int32_t Initialize(const MsgInput* in, MsgOutput* out) = 0;
        virtual int32_t Register(const MsgInput* in, MsgOutput* out) = 0;	
        virtual int32_t Subscriber(const MsgInput* in, MsgOutput* out) = 0;
        virtual int32_t reportState(const MsgInput*in, MsgOutput* out) = 0;

        CMClientInterface* next;

    protected:
        char m_server_ip[24];
        uint16_t m_server_udp_port;
        uint16_t m_server_tcp_port;
};

class ClientNodeInfo 
{
    friend class CMTree;
    public:
    ClientNodeInfo(CMClientInterface* head);
    ~ClientNodeInfo();
    int32_t process(const MsgInput* in, MsgOutput* out);

    ClientNodeInfo* next;
    private:
    int32_t max_node_num;
    CMISNode* cpMISNode;

    int32_t max_relation_num;
    CMRelation* cpRelation;

    int32_t max_col_num;
    int32_t _max_row_num;
    CMCluster* cpCluster;
    CMISNode** cppNodeCol;
    int32_t* max_row_num;
    int32_t* cur_row_use[2];

    pthread_mutex_t lock;
    pthread_rwlock_t _lock_EnvVar;
    std::map<std::string, CMISNode*> clusterNameTable;
    std::map<std::string, std::string> m_EnvVarMap;

    CMCluster self_cluster;
    CMISNode self_node;

    CMClientInterface* _head;

    int _min_level;
    int _max_level;
    char type_para_table[6];

    bool _isSubscribered;   // flag if Subscriber has completed

    int32_t _init(const MsgInput* in, MsgOutput* out);
    int32_t _register(const MsgInput* in, MsgOutput* out);
    int32_t _subscriber(const MsgInput* in, MsgOutput* out);

    int32_t getTree(const MsgInput* in, MsgOutput* out);	
    int32_t allocSubRow(const MsgInput* in, MsgOutput* out);
    int32_t allocSelfCluster(const MsgInput* in, MsgOutput* out);
    int32_t allocSubCluster(const MsgInput* in, MsgOutput* out);
    int32_t getNodeInfo(const MsgInput* in, MsgOutput* out);
    int32_t allocNode(const MsgInput* in, MsgOutput* out);
    int32_t setSubClusterStation(const MsgInput* in, MsgOutput* out);
    int32_t display(const MsgInput* in, MsgOutput* out);
    int32_t setAllStateNormal(const MsgInput* in, MsgOutput* out);
    int32_t getClusterCountByStatus(const MsgInput* in, MsgOutput* out);
    int32_t getGlobalEnvVar(const MsgInput* in, MsgOutput* out);
    int32_t getSubClusterCount(const MsgInput* in, MsgOutput* out);
    int32_t getAllNodesInOneSubCluster(const MsgInput* in, MsgOutput* out);
    int32_t getNormalNodesInOneSubCluster(const MsgInput* in, MsgOutput* out);

    bool isMatchNodeType(enodetype ExpectedType, enodetype CurrentType);
};

class CMClientProxy
{
    public: 
        CMClientProxy(); 
        ~CMClientProxy();

        int32_t Initialize(const CMClientInterface* cpHead, const MsgInput* in);
        int32_t Register(const MsgInput* in);
        int32_t Subscriber(const MsgInput* in);

        bool reload();
        int32_t process(const MsgInput* input, MsgOutput* output);

    private:
        InitInput _init_in;
        InitOutput _init_out;

        RegInput _reg_in;
        RegOutput _reg_out;

        SubInput _sub_in;
        SubOutput _sub_out;

        ClientNodeInfo* _res;
        CMClientInterface* _head;

        uint32_t _update_count;
        UTIL::RWLock _lock;
};

}
#endif
