#ifndef _UPDATE_PROCESSOR_H_
#define _UPDATE_PROCESSOR_H_

#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "netdef.h"
#include "FileQueue.h"
#include "common/Config.h"
#include "util/ThreadLock.h"
#include "update/Updater.h"

#define UPDATE update
#define ACCEPT_FD_MAX 1024

namespace update {

// 建立链接后报告自身信息

#pragma pack(1) 
struct UpdateRes {
    net_head_t head;
    char orgbuf[DEFAULT_MESSAGE_SIZE];    // 原始消息 
    char tmpbuf[DEFAULT_MESSAGE_SIZE];    // 解压缩中间内存
    char* message;   // 消息，可能指向 orgbuf或者tmpbuf
    int32_t size;    // 消息大小
};

struct ClusterMessage { // 用于自身信息描述
    int port;                   // 服务端口
    char ip[INET_ADDRSTRLEN];   // 服务ip
    uint32_t colNum;     // 总列数
    uint32_t colNo;      // 当前列号，暂时分析配置文件获取
    ROLE role;           // 角色
    uint64_t seq;        // 已完成的序列号
};

struct MessageIdx {     // 消息索引
    uint64_t nid;       // nid
    uint8_t  fileno;     // 文件号
    uint32_t offset;    // 消息开始偏移
    uint32_t size;      // 消息大小
    uint32_t lseq;      // 本机消息序号,用于确认消息顺序
    uint8_t action;     // add or del 
    uint8_t cmpr;       // 压缩标志
    uint64_t seq;       // dispatcher的上的序号
};

struct UnionInfo {      // 消息合并
    uint16_t no;        // update server 编号
    uint32_t bak;       // 上一个有效消息
    MessageIdx idx;     // 索引信息 
};

struct ThreadInfo {
    int port;                   // update server 的端口
    char ip[INET_ADDRSTRLEN];   // update server 的ip
    ClusterMessage msg;         // search、或detail自身信息
    
    pthread_t id;               // 线程句柄
    int32_t isActive;           // 线程是否活跃或者有效

    int fdIdx;                  // 索引句柄
    int fdDat;                  // 数据句柄

    uint32_t datOffset;         // 数据文件偏移
    uint8_t  datFileNo;         // 数据文件号
};
#pragma pack() 

class UpdateProcessor 
{ 
public:
    UpdateProcessor();
    virtual ~UpdateProcessor();

    /*
     * 初始化函数，按照框架的要求，完成自身初始化，增量数据恢复
     * @param pConf 配置文件路径
     * @return 操作是否成功，0为成功，<0失败
     */
    int init(const char* conf_path, Updater* up, ROLE role);

    /*
     * 启动服务
     * @param arg: UpdateProcessor对象实例
     * @return 操作是否成功, NULL 失败
     */ 
    void* start(void* arg);

    /*
     * 停止服务
     */
    void* stop();
    
    /*
     * 增量恢复操作
     * @return success: >= 0, failed: other
     */
    int recovery(uint32_t msgCount);

protected:
    /*
     * 备份消息
     */
    int bakMessage(ThreadInfo* info, UpdateRes* res, int threadNo);

    /*
     * 合并相同nid的消息
     */
    int unionMessage(UnionInfo* messageList, int msgCount);
   
    /*
     * 合成文件名
     */
    void idxFileName(char* name, int updaterSeq);
    void datFileName(char* name, int updaterSeq, int fileNo);
    
private:
    bool _bStartSimon;          //是否启动了simon服务
    uint32_t _incMsgCount;      //本地消息序号
    
    int _running;               // 运行状态
    util::Mutex _lock;          // 分配序列号等

    Config _cfg;                // 配置信息
    Updater* _updater;          // 实现index_lib、libdetail 底层调用 
    
    int _threadNum;             // 线程数
    ThreadInfo _thread_list[MAX_UPDATE_SERVER_NUM];

    int _fdSeq;                 // 当前更新序号

public:
    static void* update_work(void* para);
};

}


#endif 
