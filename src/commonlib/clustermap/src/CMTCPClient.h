/** \file
 *******************************************************************
 * $Author: santai.ww $
 *
 * $LastChangedBy: santai.ww $
 *
 * $Revision: 516 $
 *
 * $LastChangedDate: 2011-04-08 23:43:26 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: CMTCPClient.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_TCP_CLIENT_H_
#define _CM_TCP_CLIENT_H_

#include <stdint.h>
#include <string>
#include <anet/anet.h>
#include "CMTree.h"

namespace clustermap {

class CMTCPClient
{
    public:
        CMTCPClient();
        ~CMTCPClient();

        /**
         *@brief: 设定监听地址和端口
         *@param: 配置文件路径
         *@return: 0 is success.
         */
        int32_t setServerAddr(const char *spec);

        /**
         *@brief: 设定超时时间
         *@param: 超时时间
         *@return: 0 is success.
         */
        int32_t setTimeout(int32_t timeout);

        /**
         *@brief:启动client服务,连接server
         *@param: 
         *@return: 0 is success.
         */
        int32_t start();

        /**
         *@brief: 发送
         *@param: 发送缓冲区中的报文
         *@return: 0 is success.
         */
        int32_t send(const char *bufp, int32_t size);

        /**
         *@brief: 接收
         *@param: 接收报文到缓冲区
         *@return: 0 is success.
         */
        int32_t recv(char*& bufp, int32_t& size);

        char m_spec[64];
    private:
        anet::DefaultPacketFactory*  m_factory;
        anet::DefaultPacketStreamer* m_streamer;
        anet::Transport* m_transport;
        anet::Connection* m_connection;
        anet::Packet* m_packet;

        bool m_start;
        int32_t m_sendFailNum;
        int32_t m_timeout;
        int32_t clear();
};

}
#endif
