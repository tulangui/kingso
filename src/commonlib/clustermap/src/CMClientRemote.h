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
 * $Id: CMClientRemote.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#ifndef _CM_CLIENT_REMOTE_H_
#define _CM_CLIENT_REMOTE_H_

#include <anet/anet.h>
#include "CMClientInterface.h"
#include "DGramClient.h"
#include "CMTCPClient.h"

namespace clustermap {

class CMClientRemote : public CMClientInterface
{
    public:
        CMClientRemote();
        virtual ~CMClientRemote();

        int32_t init(const char* server_ip, uint16_t udp_port, uint16_t tcp_port);

        int32_t Initialize(const MsgInput* in, MsgOutput* out);
        int32_t Register(const MsgInput* in, MsgOutput* out);	
        int32_t Subscriber(const MsgInput* in, MsgOutput* out);

    private:
        int32_t reportState(const MsgInput*in, MsgOutput* out);
        int32_t initTcpClient(CMTCPClient* cpClient);

        char m_server_ip[24];
        uint16_t m_server_udp_port;
        uint16_t m_server_tcp_port;

        CDGramClient m_cDGramClient;
};

}
#endif
