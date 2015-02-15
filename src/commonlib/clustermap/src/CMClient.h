/** \file
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 11573 $
 *
 * $LastChangedDate: 2012-04-25 15:55:14 +0800 (Wed, 25 Apr 2012) $
 *
 * $Id: CMClient.h 11573 2012-04-25 07:55:14Z pujian $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_CLIENT_H_
#define _CM_CLIENT_H_

#include <stdio.h>
#include <string>
#include <mxml.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <linux/limits.h>
#include "DGramClient.h"
#include "DGramServer.h"
#include "CMWatchPoint.h"
#include "CMClientInterface.h"

namespace clustermap {

class CMClient
{
    public:
        CMClient();
        ~CMClient();

        /**
         *@brief:初始化clustermap客户端, 获取自身信息（id等等)
         *@ app_spec: 应用程序信息，格式: 协议:ip:端口，如果ip为空，则自动探测, 例如: tcp::8888，如果为NULL，则只能订阅全部节点
         *@ map_server: clustermap server信息，多个server用,号分隔 ip:tcpPort:udpPort, 192.168.1.1:8888:9999
         *@ local_cfg: 本地配置文件全路径
         *@return: 0 is success.
         */
        int32_t Initialize(const char* app_spec, std::vector<std::string>& map_server, const char* local_cfg = NULL);

        /**
         *@brief: 向cm server注册,并开始发送心跳报文
         *@param: 
         *@return: 0 is success.
         */
        int32_t Register();

        /**
         *@brief:向cm server进行节点信息订阅，获取相应的信息，例如: 订阅类型为子节点，则获取子节点信息，每节点只能订阅一次
         *@param: 订阅类型,0,子节点信息.1,兄弟节点信息.2,全部节点信息
         *@return: 0 is success.
         */
        int32_t Subscriber(int32_t subtype = sub_child);

        /**
         *@brief:增加当前服务状况观测,接口内部根据server来配置具体增加项
         */
        int32_t addWatchPoint(WatchPoint* cpWatchPonit);

        /**
         *@brief:设置发送状态时间间隔, 单位为毫秒
         */
        int32_t setReportPeriod(int32_t report_period = 50);

        /**
         *@brief: 获取一行子节点信息
         *@param: ids,子节点id的指针
         *@param: size,子节点的数量
         *@param: nodetype 节点类型, search_mix search merger 
         *@return: 0 is success.
         */
        int32_t allocSubRow(int32_t*& ids, int32_t &size, enodetype nodetype=search_mix, int level=1, LogicOp logic_op=GE, uint32_t locating_signal = 0);
        int32_t freeSubRow(int32_t* ids);

        /**
         * 获取自身列中所有符合条件的节点信息，需要freeSubRow()释放ids。
         * @param ids 节点数组，每个节点都需要getNodeInfo()获得节点指针。
         * @param size 数组维数。
         * @param nodetype 节点类型。
         * @param level 所在列的level。
         * @param logic_op 对列level的操作符。
         * @param locating_signal 选择行的依据（例如：按q哈希）。
         * @return 成功时返回0，失败时返回-1。
         */
        int32_t allocSelfCluster(int32_t*& ids, int32_t &size, enodetype nodetype=search_mix, int level=1, LogicOp logic_op=GE, uint32_t locating_signal = 0);

        /**
         *@brief: 获取一行子节点信息，不判断节点状态
         *@param: ids,子节点id的指针
         *@param: size,子节点的数量
         *@param: nodetype 节点类型, search_mix search merger 
         *@return: 0 is success.
         */
        int32_t allocSubCluster(int32_t*& ids, int32_t &size, enodetype nodetype=search_mix, int level=1, LogicOp logic_op=GE, uint32_t locating_signal = 0);

        /**
         *@brief: 获取指定节点信息
         *@param: 节点id.为 -1 表示当前节点
         *@return: 0 is success.
         */
        CMISNode *getNodeInfo(int32_t nodeid = -1);

        /**
         *@brief: 根据node的信息，获取第二阶段请求的节点信息，如果Node是blender、merger, doc不分离
         *@param: node,子节点信息的指针
         *@param: typeid,子节点的类型, doc: doc_mix, merger (search 、blender)
         *@return: NULL is failed, other success.
         */
        CMISNode *allocNode(CMISNode *node, enodetype type_id);

        /**
         *@brief: 获取nodeid的子节点信息
         *		  如果Node是blender,merger,doc不分离，
         *		  则返回自身信息（ip转为127.0.0.1）
         *@param: node,子节点信息的指针, 如果为节点信息, 格式为 协议:ip:端口
         *@param: typeid,子节点的类型, doc: doc_mix
         *@return: 0 is success.
         */
        CMISNode *allocNode(const char * subclustername, enodetype type_id);

        void setState(char state) { m_state = state;}

        int32_t getTree(void *&tree, time_t& update_time);

        int32_t getCMTCPServer(std::vector<std::string> &server_spec);	

        uint32_t getClusterCountByStatus(estate_cluster status);    // ClusterState: GREEN, YELLOW, RED

        std::string getGlobalEnvVar(const char *EnvVar_Key);

        /**
         * @brief:  获取子列的个数，不区分子列的类型和状态。
         * @return: 子列个数，可能为0.
         */
        int32_t getSubClusterCount();

        /**
         * @brief: 根据类型，获取一列的全部节点；后续通过getNodeInfo()获取具体的节点信息.
         * @param: subcluster_num[in]    子列号，0 ~ getSubClusterCount()-1
         * @param: ids[out]              子节点id的指针，使用后需要用freeSubNodes()释放
         * @param: size[out]             子节点的数量，可能为0.
         * @param: nodetype[in]          节点类型, search_mix search merger 
         * @return: 0 is success.
         */
        int32_t getAllNodesInOneSubCluster(int32_t subcluster_num, int32_t*& ids, int32_t &size, enodetype nodetype=search_mix);

        /**
         * @brief: 根据类型，获取一列的全部可用节点；后续通过getNodeInfo()获取具体的节点信息.
         * @param: subcluster_num[in]    子列号，0 ~ getSubClusterCount()-1
         * @param: ids[out]              子节点id的指针，使用后需要用freeSubNodes()释放
         * @param: size[out]             子节点的数量，可能为0.
         * @param: nodetype[in]          节点类型, search_mix search merger 
         * @return: 0 is success.
         */
        int32_t getNormalNodesInOneSubCluster(int32_t subcluster_num, int32_t*& ids, int32_t &size, enodetype nodetype=search_mix);
        int32_t freeSubNodes(int32_t* ids);

    private:
        int32_t m_stop_flag;
        int32_t m_thread_num;
        int32_t m_report_period;
        bool m_thread_udp_recv_isStart;

        //int32_t m_subtype;

        char m_state;
        eprotocol m_app_pro;
        char m_app_ip[24];
        uint16_t m_app_port;
        uint16_t m_sub_port;

        int32_t m_server_num;
        char m_server_ip[2][24];
        uint16_t m_server_udp_port[2];
        uint16_t m_server_tcp_port[2];
        char m_local_cfg[PATH_MAX+1];

        uint32_t m_tree_time;
        uint32_t m_tree_no;
        time_t m_update_time;
        pthread_mutex_t m_lock;

        CDGramServer m_udpserver;
        CMClientProxy m_cClient;

        WatchPoint* m_cpWatchPonit;
        CPUWatchPoint m_cCPUWatchPoint;
        SysloadWatchPoint m_cSysloadWatchPoint;
        CMClientInterface* m_cpClientHead;

        int32_t display();
        int32_t parseAppSpec(const char* app_spec);
        int32_t parseServer(const char* map_server);
        int32_t setSubClusterStation(char* stateBitmap, int32_t stateBitmapLen, int32_t node_count);

    public:	
        static void* thread_report(void* vpPara);
        static void* thread_udp_recv(void* vpPara);
        static void* thread_monitor(void* vpPara);
};

}
#endif
