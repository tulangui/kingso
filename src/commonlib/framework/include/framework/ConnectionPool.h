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
 * $Id: ConnectionPool.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: connection pool $
 *
 *******************************************************************
 */

#ifndef _FRAMEWORK_CONNECTIONPOOL_H_
#define _FRAMEWORK_CONNECTIONPOOL_H_
#include "util/common.h"
#include "util/namespace.h"
#include "framework/namespace.h"
#include "util/ThreadLock.h"
#include <anet/anet.h>

FRAMEWORK_BEGIN;

class ConnectionPool {
    public:
        /**
         *@brief 连接池构造函数
         *@param Transport anet通信对象
         *@param connqueuelimit 连接队列最大请求数
         *@param timeout 超时时间
         *@return 连接池对象
         */
        ConnectionPool(anet::Transport &transport, 
                uint32_t connQueueLimit = 0, uint32_t timeout = 0);
        ~ConnectionPool();
    public:
        /**
         *@brief 创建连接、并返回连接对象
         *@param protocol 协议
         *@param server  服务器地址
         *@param port 服务端口号
         *@return 连接对象
         */
        anet::Connection * makeConnection(const char *protocol,
                const char *server, 
                uint16_t port);
        /**
         *@brief 创建tcp连接、并返回连接对象
         *@param spec 协议:服务器地址：服务端口号
         *@return 连接对象
         */
        anet::Connection * makeTcpConnection(const char *spec);
        /**
         *@brief 创建tcp连接、并返回连接对象
         *@param spec 协议:服务器地址：服务端口号
         *@return 连接对象
         */
        anet::Connection * makeHttpConnection(const char *spec);
    private:
        anet::Transport &_transport;
        uint32_t _connQueueLimit;
        uint32_t _timeout;
        std::map<std::string, anet::Connection*> _mp;
        UTIL::Mutex _lock;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_CONNECTIONPOOL_H_

