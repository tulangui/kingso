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
 * $Id: DGramServer.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

/********************************************************************
 * 
 * File: udp通信的服务端
 * Desc: 监听特定端口的消息，并解析客户端地址
 *       usage: dgserver portnum
 * Log:
 *       Create by weilei, 2008/5/21
 * 
 ********************************************************************/

#ifndef __DGRAM_SERVER_H__
#define __DGRAM_SERVER_H__

#include <sys/socket.h>
#include "DGramCommon.h"

namespace clustermap {

class CDGramServer
{
    public:
        /* 构造函数,配置参数 */
        CDGramServer();

        /* 析构函数 */
        virtual ~CDGramServer(void);

        int32_t InitDGramServer(int port);

        /* 接收消息，返回的是消息内容及长度(pMsg, nLen)，同时返回主机名和端口 */
        int32_t receive(char* buf, int buf_size);

        /* 获得DgramServer用的socked标识 */
        inline int getSocked() { return m_sock;	}

    private:

        int m_sock;	// use this socket to receive
        CDGramCommon m_cDGramComm; // udp的公共类
};

}
#endif
