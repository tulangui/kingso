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
 * $Id: CMTree.h 11792 2012-05-03 06:20:42Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_TREE_H_
#define _CM_TREE_H_

#include <ctime>
#include <string>
#include <map>
#include <stdint.h>
#include <mxml.h>
#include <linux/limits.h>
#include "util/ThreadLock.h"
#include "util/ScopedLock.h"
#include "util/Log.h"
#include "CMNode.h"

namespace clustermap {

struct MsgInput {
    MsgInput() : type(msg_unvalid) {};
    eMessageType type;
};
struct MsgOutput {
    char* outbuf;
    int32_t outlen;
    eMessageType type;

    MsgOutput() : outbuf(NULL), outlen(0) {} 
    virtual ~MsgOutput() {if(outbuf) delete[] outbuf; outbuf = NULL;}
    bool isSuccess(char* buf, int32_t len) {
        if(len <= 0 || *buf == msg_unvalid)
            return false;
        return true;
    };
};

struct ComInput : public MsgInput {
    char* buf;
    int32_t len;

    ComInput() : buf(NULL) {}
    virtual ~ComInput() {if(buf) delete[] buf; buf=NULL;}
    virtual int32_t bufToNode(char* in_buf, int32_t in_len);
    virtual int32_t nodeToBuf();
};
struct ComOutput : public MsgOutput {
};

struct InitInput : public ComInput {
    char ip[24];
    uint16_t port;
    eprotocol protocol;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct InitOutput : public ComOutput {
};

struct RegInput : public ComInput {
    char ip[24];
    uint16_t port;
    eprotocol protocol;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct RegOutput : public ComOutput {
    time_t reg_time;
    uint32_t tree_no;
};

struct SubInput : public ComInput {
    char ip[24];
    uint16_t port;
    eprotocol protocol;
    uint16_t sub_port;
    time_t op_time;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct SubOutput : public ComOutput {
    uint32_t tree_time;
    uint32_t tree_no;
};

struct StatInput : public ComInput {
    char ip[24];
    uint16_t port;
    eprotocol protocol;
    char state;
    uint32_t tree_time;
    uint32_t tree_no;
    struct timeval op_time;
    uint32_t cpu_busy;
    uint32_t load_one;
    uint32_t io_wait;
    uint32_t latency;
    uint32_t qps; 

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct StatOutput : public ComOutput {
    CSubInfo* sub;
    StatOutput* next;
    char* send_buf;
    int32_t send_len;

    StatOutput() : sub(NULL), next(NULL), send_buf(NULL) {}
    virtual ~StatOutput() {
        StatOutput* p = next, *bak;
        while(p) {
            bak = p;
            p = p->next;
            delete[] bak->outbuf;
        }
    }
};

struct ClusterStateInput : public ComInput {
    char clustername[64];
    char sClusterState[32];
    char ClusterState;
    struct timeval op_time;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct ClusterStateOutput : public ComOutput {
};

struct EnvVarInput : public ComInput {
    int32_t EnvVarCommand;
    char EnvVar_Key[32];
    char EnvVar_Value[32];
    struct timeval op_time;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct EnvVarOutput : public ComOutput {
    char EnvVar_Key[32];
    char EnvVar_Value[32];

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};

struct ValidInput : public ComInput {
    char ip[24];
    uint16_t port;
    eprotocol protocol;
    char valid;
    struct timeval op_time;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct ValidOutput : public ComOutput {
};

struct getNodeInput : public ComInput {
    union {
        struct {
            char ip[24];
            uint16_t port;
            eprotocol protocol;
        };
        char name[64];
        int32_t local_id;
    };
    int32_t nodeToBuf();
    int32_t bufToNode(char* in_buf, int32_t in_len);
};
struct getNodeOutput : public ComOutput {
    int32_t node_num;
    CMISNode* cpNode;
    CMCluster* cpCluster;

    getNodeOutput() : cpNode(NULL), cpCluster(NULL) {}
    ~getNodeOutput() {
        if(cpNode) delete[] cpNode;
        if(cpCluster) delete cpCluster;
    }
    int32_t bufToNode(char* in_buf, int32_t in_len);
};

struct checkInput : public MsgInput {
    struct timeval op_time;
    struct timeval minUpdateTime;
};
struct checkOutput : public ComOutput {
};

struct setmapInput : public ComInput {
    time_t op_time;
    char* mapbuf;
    int32_t maplen;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct setmapOutput : public ComOutput {
    char* filename;

    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};

struct getmapInput : public ComInput {
    time_t op_time;
    char* mapbuf;
    int32_t maplen;
    int32_t bufToNode(char* in_buf, int32_t in_len);
    int32_t nodeToBuf();
};
struct getmapOutput : public ComOutput {
};

class CMTree
{
    public:
        CMTree();
        ~CMTree();

        int32_t init(const void* cpInfo, uint32_t tree_time, uint32_t tree_no);
        int32_t init(const char *conffile, char type, uint32_t tree_time, uint32_t tree_no); // type: 0:master, 1:slave 
        int32_t load();

        int32_t process(const MsgInput* input, MsgOutput* output);

        //	private:
        int32_t getNode(int32_t clusterid, int32_t nodeid, CMISNode *&cmisnode);
        int32_t getNode(char* ip, int32_t port, eprotocol proto, CMISNode *&cmisnode);

        int32_t getCluster(int32_t clusterid, CMCluster *&cmcluster);
        int32_t getCluster(const char *clustername, CMCluster *&cmcluster);

        int32_t getSubCluster(int32_t relationid, CMCluster **&cmcluster, int32_t &size);
        int32_t freeSubCluster(CMCluster** cmcluster);


        int32_t travelTree(FILE* fp = NULL);
        int32_t getRootRelation(CMRelation *&pRoot) {pRoot = &m_cRoot; return 0;}

        int32_t getRelationArray(CMRelation**& cppRelation, int32_t& relation_num, int32_t& all_node_num);
        int32_t getRelation(int32_t relationid, CMRelation*& cpRelation);
        int32_t getAvgValue(CMISNode* cur, int32_t& qps, int32_t& latency, int32_t& offNum, int32_t& limit);

    private:
        int32_t get_info_by_url(MsgInput* input, MsgOutput* output);

        int32_t getOneList(mxml_node_t* root, const char* list_name, const char* cluster_name, std::map<std::string, CMRelation*>& root_map, const char type);
        int32_t getOneCluster(mxml_node_t* root, CMCluster* cpCluster);
        int32_t getOneRelation(mxml_node_t* root, CMRelation* cpRelation, std::map<std::string, CMRelation*>& root_map);
        int32_t parseClusterState(uint32_t &limit_value, const char *str_limit_value, const char *limit_name, bool negative = false);

        int32_t clientInit(MsgInput* input, MsgOutput* output);
        int32_t clientReg(MsgInput* input, MsgOutput* output);
        int32_t clientSubChild(MsgInput* input, MsgOutput* output);
        int32_t clientSubAll(MsgInput* input, MsgOutput* output);
        int32_t clientSubSelfCluster(MsgInput* input, MsgOutput* output);

        int32_t getStateUpdate(MsgInput* input, MsgOutput* output);
        int32_t checkTimeOut(MsgInput* input, MsgOutput* output);
        int32_t setNodeValid(MsgInput* input, MsgOutput* output);
        int32_t setNodeState(MsgInput* input, MsgOutput* output);
        int32_t getClustermap(MsgInput* input, MsgOutput* output);
        int32_t setClustermap(MsgInput* input, MsgOutput* output);
        int32_t getNodeInfo(MsgInput* input, MsgOutput* output);
        int32_t getNodeInfo_auto(MsgInput* input, MsgOutput* output);
        int32_t setClusterState(MsgInput* input, MsgOutput* output);
        int32_t setEnvVarCommand(MsgInput* input, MsgOutput* output);

        int32_t parseSpec(const char* spec, char* ip, uint16_t& port, eprotocol& proto);
        int32_t readFiletoSysMap(std::map<std::string, std::string> &sys, const char* filename);

        char* allocBuf(int buf_size);
        CMISNode* allocMISNode(int num = 1);
        CMCluster* allocMClusterNode(int num = 1);
        CMRelation* allocRelationNode(int num = 1);

        int32_t strToProtocol(const char* str, eprotocol& protocol);
        int32_t strToNodeType(const char* str, enodetype& type);
        int32_t strToClusterType(const char* str, eclustertype& type);
        int32_t getProtocolIpPortKey(eprotocol protocol, const char* ip, uint16_t port, char* key);

        int32_t clearTree();
        int32_t recordNodeInfo(CMISNode* cpNode);
        int32_t travelTree(CMRelation* root, int indent, FILE* fp);
        int32_t statRelationNum(CMRelation* cpRelation, int32_t relation_id);
        estate_cluster getClusterState(CMCluster *cpCluster);

        char m_tree_type;	  // 0:master 1:slave 
        uint32_t m_tree_no; 
        uint32_t m_tree_time;

        char* m_sys_file;
        char* m_sys_transin_file;
        char* m_sys_bak_file;
        char m_cfg_file_name[PATH_MAX+1];
        pthread_mutex_t m_file_lock;
        pthread_mutex_t m_mem_lock;
        pthread_mutex_t m_state_lock;

        // 以cluster name为key的map
        std::map<std::string, CMCluster*> m_clustermap;
        // 以cluster name为key的map
        std::map<std::string, CMRelation*> m_relationMap;
        // ip port protocol为key
        std::map<std::string, CMISNode*> m_misnodeMap;
        //
        std::map<std::string, CSubInfo*> m_subAllMap;

        // merger_list -> merger cluster name
        std::map<std::string, std::vector<std::string> > _listTable;


        //以 nodeid 为offset的数组
        CMISNode** m_cppMISNode;
        CMCluster** m_cppCluster;
        CMRelation** m_cppRelation;
        int32_t m_max_node_num;
        int32_t m_max_cluster_num;
        int32_t m_max_relation_num;

        CMRelation m_cRoot;
        CProtocol m_cprotocol;

        CSubInfo* m_cpSubAll;
        char* m_cLastAllState;
        char* m_cLastAllState_Cluster;

        char* m_szpBuf;
        char* m_szpHead;
        int32_t m_left_buf_size;

        TagName m_cTagCluster[TAG_CLUSTER_NUM];
        TagName m_cTagRelation[TAG_RELATION_NUM];
        char* m_cTagMISNode[TAG_MISNODE_NUM];

        int32_t m_watchOnOff;        // 检测开关， 0: 关闭 other: 打开
        std::map<std::string, std::string> m_EnvVarMap;
};

class CMTreeProxy
{
    public: 
        CMTreeProxy(); 
        ~CMTreeProxy();

    public: 
        int32_t init(const char *conffile, char type);
        int32_t process(const MsgInput* input, MsgOutput* output);
        int32_t loadSyncInfo();
        bool reload();
        int32_t getversion(char* md5, time_t& up_time);
    private:
        CMTree* _res;
        char _type;
        char _tree_md5[16];
        time_t _tree_time;
        char _path[PATH_MAX + 1];
        uint32_t _update_count;
        UTIL::RWLock _lock;

        int32_t getTreeVersion(char* cfg_file, char* md5, time_t& up_time);
};

}
#endif
