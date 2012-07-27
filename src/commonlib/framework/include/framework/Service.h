/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 167 $
 *
 * $LastChangedDate: 2011-03-25 11:13:57 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: Service.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: service object which is anet call back handle $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_SERVICE_H_
#define _FRAMEWORK_SERVICE_H_
#include "framework/namespace.h"
#include "framework/ParallelCounter.h"
#include "framework/WebProbe.h"
#include "framework/Compressor.h"
#include <anet/anet.h>

FRAMEWORK_BEGIN;

class TaskQueue;
class Session;
class CacheHandle;

//anet的回调句柄类
class Service : public anet::IServerAdapter {
    public:
        Service(TaskQueue &tq, ParallelCounter &pc, WebProbe *wp);
        virtual ~Service();
    public:
        /**
         *@brief anet回调函数
         *@param connection anet连接对象
         *@param packet 收到的报文包
         *@return 0,成功 非0,失败
         */
        anet::IPacketHandler::HPRetCode
            handlePacket(anet::Connection *connection, anet::Packet *packet);
    public:
        /**
         *@brief 启动服务
         *@param transport anet通信对象
         *@param server 服务名称
         *@param port 服务端口
         */
        virtual int32_t start(anet::Transport &transport, 
                const char *server, uint16_t port) = 0;
        /**
         *@brief 设定查询串前缀
         *@param prefix 前缀
         */
        int32_t setPrefix(const char *prefix);
        /**
         *@brief 设定每个链接的最大服务数
         *@param maxrequest 最大服务数
         */
        void setMaxRequest(uint32_t maxrequest) { 
            _maxRequestPerConnection = (maxrequest>0)?maxrequest:0; 
        }
        /**
         *@brief 设定到达最大服务数后的缓冲数量
         *@param maxoverflow 缓冲数量
         */
        void setMaxOverFlow(uint32_t maxoverflow) { 
            _maxRequestOverflow = (maxoverflow>0)?maxoverflow:0; 
        }
        /**
         *@brief 获取每个链接的最大服务数
         *@return 最大服务数
         */
        uint32_t getMaxRequest(){ return _maxRequestPerConnection; }
        /**
         *@brief 获取到达最大服务数后的缓冲数量
         *@return 缓冲数量
         */
        uint32_t getMaxOverFlow(){ return _maxRequestOverflow; }
        /**
         *@brief 设置Cache的操作句柄类
         *@param cache Cache的操作句柄类
         */
        void setCacheHandle(CacheHandle * cache){ _cache = cache; }
        /**
         *@brief 存储键值对到cache中
         */
        bool putCache(const char * key_data, uint32_t key_size,
                const char * val_data, uint32_t val_size, int nDocsFound);

        /**
         *@brief 返回应答报文给调用方
         *@param conn 当前的连接
         *@param str  应答的信息、size 应答信息大小
         *@param channelid 连接号
         *@param keepalive 是否长连接
         *@param cost 耗时
         *@param outfmt 显示格式
         *@param compress_type_t 使用哪种压缩算法
         *@return 0,成功 非0,失败
         */
        int32_t response(anet::Connection *conn, 
                char *str, uint32_t size, 
                uint32_t channelid, bool keepalive, 
                uint64_t cost, char *outfmt, bool error = false, 
                bool notexist = false, 
                compress_type_t comType= ct_none, bool isDec = true);
        /**
         *@brief 获取spec(协议：ip：端口号)
         *@return spec
         */
        const char * getSpec() const { return _spec; }
        /*        bool putCache(const char *key_data, uint32_t key_size,
                  const char *val_data, uint32_t val_size, int nDocsFound);
                  void setCacheHandle(CacheHandle *cache) { _cache = cache; }
                  */
    protected:
        virtual int32_t parseQueryString(anet::Packet *packet, 
                Session &session) = 0;
        /**
         *@brief 创建packet包
         *@param str 报文的信息、size 报文信息大小
         *@param channelid 连接号
         *@param keepalive 是否长连接
         *@param error 是否出错
         *@return 0,成功 非0,失败
         */
        virtual anet::Packet * createResponsePacket(char *str, uint32_t size,
                uint32_t channelid, bool keepalive, 
                bool error) = 0;
    protected:
        anet::IOComponent *_listener;
        ParallelCounter &_counter;
        TaskQueue &_taskQueue;
        WebProbe *_webProbe;
        char _spec[64];            //store protocol:ip:port
        char *_prefix;
        CacheHandle *_cache;
        uint32_t _maxRequestPerConnection;
        uint32_t _maxRequestOverflow;
    public:
        Service *next;
};

//tcp服务对象
class TCPService : public Service {
    public:
        TCPService(TaskQueue &tq, ParallelCounter &pc, WebProbe *wp) 
            : Service(tq, pc, wp) { }
        virtual ~TCPService() { }
    public:
        /**
         *@brief 启动服务
         *@param transport anet通信对象
         *@param server 服务名称
         *@param port 服务端口
         */
        virtual int32_t start(anet::Transport &transport, 
                const char *server, uint16_t port);
    protected:
        virtual int32_t parseQueryString(anet::Packet *packet, 
                Session &session);
        /**
         *@brief 创建TCP packet包
         *@param str 报文的信息、size 报文信息大小
         *@param channelid 连接号
         *@param keepalive 是否长连接
         *@param error 是否出错
         *@return 0,成功 非0,失败
         */
        virtual anet::Packet * createResponsePacket(char *str, uint32_t size,
                uint32_t channelid, bool keepalive, 
                bool error);
};

//http服务对象, 会创建http协议格式的包
class HTTPService : public Service {
    public:
        HTTPService(TaskQueue &tq, ParallelCounter &pc, WebProbe *wp) 
            : Service(tq, pc, wp) { }
        virtual ~HTTPService() { }
    public:
        /**
         *@brief 启动服务
         *@param transport anet通信对象
         *@param server 服务名称
         *@param port 服务端口
         */
        virtual int32_t start(anet::Transport &transport, 
                const char *server, uint16_t port);
    protected:
        virtual int32_t parseQueryString(anet::Packet *packet, 
                Session &session);
        /**
         *@brief 创建HTTP packet包
         *@param str 报文的信息、size 报文信息大小
         *@param channelid 连接号
         *@param keepalive 是否长连接
         *@param error 是否出错
         *@return 0,成功 非0,失败
         */
        virtual anet::Packet * createResponsePacket(char *str, uint32_t size,
                uint32_t channelid, bool keepalive, 
                bool error);
};

/************************* ServiceFactory **************************/
class ServiceFactory {
    public:
        ServiceFactory() { }
        ~ServiceFactory() { }
    public:
        /**
         *@brief 创建服务对象
         *@param protocol 服务使用的通信协议
         *@param tq 服务使用的任务队列
         *@param pc 服务的并发控制
         *@param wp 服务的探测对象
         *@return 服务对象
         */
        static Service * makeService(const char *protocol, 
                TaskQueue &tq, ParallelCounter &pc,
                WebProbe *wp);
};

FRAMEWORK_END;

#endif //_FRAMEWORK_SERVICE_H_

