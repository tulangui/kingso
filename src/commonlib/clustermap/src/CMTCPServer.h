/** \file
 *******************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 10439 $
 *
 * $LastChangedDate: 2012-02-22 14:01:58 +0800 (Wed, 22 Feb 2012) $
 *
 * $Id: CMTCPServer.h 10439 2012-02-22 06:01:58Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_TCP_SERVER_H_
#define _CM_TCP_SERVER_H_

#include <stdint.h>
#include <string>
#include <anet/anet.h>
#include "CMTree.h"

namespace clustermap {

class CMTCPAdapter : public anet::IServerAdapter
{
    public:
        CMTCPAdapter(CMTreeProxy& cTree): m_cTree(cTree) {};
        virtual ~CMTCPAdapter() {};

        anet::IPacketHandler::HPRetCode	handlePacket(anet::Connection *connection, anet::Packet *packet);

    private:
        int32_t handleMessage(ComInput* in, ComOutput* out, anet::DefaultPacket *&request);
        int32_t handleSlave(char* buf, int32_t buf_len, anet::DefaultPacket*& packet);

        // 响应url方式的状态请求
        int32_t handleUrl(char* body, int32_t bodyLength, anet::DefaultPacket*& request);

        CMTreeProxy& m_cTree;
};

class CMTCPServer
{
    public:
        CMTCPServer(CMTreeProxy& cmtree) : m_ppacketstreamer(NULL), m_timeout(3000), m_adapter(cmtree) {}
        virtual ~CMTCPServer();

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
         *@brief:启动server服务，开始监听端口
         *@param: 
         *@return: 0 is success.
         */
        int32_t start();

    private:
        anet::DefaultPacketFactory m_packetfactory;
        anet::DefaultPacketStreamer *m_ppacketstreamer;
        int32_t m_timeout;
        CMTCPAdapter m_adapter;
        anet::Transport m_transport;
        std::string m_spec;
};

}
#endif
