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
 * $Id: DGramClient.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

/********************************************************************
 * 
 * File: udp通信的客户端
 * Desc: 发送消息给服务端
 *       usage: dgclient hostname portnum message
 * Log:
 *       Create by weilei, 2008/5/21
 * 
 ********************************************************************/

#ifndef __DGRAM_CLIENT_H__
#define __DGRAM_CLIENT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include "DGramCommon.h"

namespace clustermap {

class CDGramClient
{ 
    public:
        /* 构造函数,配置参数 */
        CDGramClient();

        /* 析构函数 */
        virtual ~CDGramClient(void);

            int32_t InitDGramClient(const char *pIp, uint16_t nPort);	

            /* 发送消息，输入参数是消息指针和长度 */
            int send(const char *pMsg, int pLen);

    private:	
        int m_sock; // use this socket to send
        sockaddr_in m_saddr; // server's address	
        CDGramCommon m_cDGramComm; // udp的公共类
};

}
#endif
