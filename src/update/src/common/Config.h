#ifndef _COMMON_CONFIG_H_
#define _COMMON_CONFIG_H_

#include <mxml.h>
#include <limits.h>
#include <arpa/inet.h>
#include "update/Updater.h"
#include "update/update_def.h"

namespace update { 

class Config
{ 
public:
    Config();
    ~Config();
    
    /*
     * success: 0 failed: other
     */
    int init(const char* cfgFile, ROLE role);

public:
    char _cluster_conf[PATH_MAX];     // cluster.xml
    char _simon_conf[PATH_MAX];       // simon 配置文件
    char _build_conf[PATH_MAX];       // libbuild 的配置文件
    char _dispatcher_field[PATH_MAX]; // 宝贝按这个字段分发到不同列中
    
    // 自身信息，确认角色后获取自身信息
    int  _selfPort;                   
    char _selfIp[INET_ADDRSTRLEN];
    int  _colNum;       // 总列数
    int  _colNo;        // 当前列号
    char* _data_path;   // 数据备份路径

    // enter_updater
    char _enter_data_path[PATH_MAX]; // 数据备份路径 
    char _dotey_sign_path[PATH_MAX]; // 检测nid是否真正更新字典路径,全量签名应该考到此目录下
                                  
    int _enter_port;                 // 监听端口, 监听更新数据到来
    int _enter_send_timeout;         // socket接收发送超时

    // 
    int _cluster_port;               // 监听端口, 监听search更新请求
    int _cluster_send_timeout;       // socket接收发送超时
    int _cluster_thread_num;         // 工作线程数量
    uint64_t _flux_limit;            // 流量限制,点对点直接的流量控制

    int  _detail_server_port;         // detail 服务端口，用于自身信息确认 
    int  _search_server_port;         // search 服务端口，用于自身信息确认
    char _detail_data_path[PATH_MAX]; // 数据备份路径
    char _search_data_path[PATH_MAX];
    

    int  _updater_server_num;                                        // dispatcher 的数量   
    int  _updater_server_port[MAX_UPDATE_SERVER_NUM];                // port   
    char _updater_server_ip[MAX_UPDATE_SERVER_NUM][INET_ADDRSTRLEN]; // ip 

private:
    /*
     * 获取updateserver 的信息
     */
    int parseDispatcherField(mxml_node_t *pNode);

    /*
     * 获取自身信息
     */
    int parseSelfField(mxml_node_t *pNode, ROLE role);

    /*
     * 查找自身信息
     */
    int findNode(mxml_node_t *pNode, const char* ip, int port);

};

}

#endif //COMMON_CONFIG_H_
