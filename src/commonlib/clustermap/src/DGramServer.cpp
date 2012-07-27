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
 * $Id: DGramServer.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "DGramServer.h"

namespace clustermap {

CDGramServer::CDGramServer()
{
    m_sock = -1;
}

CDGramServer::~CDGramServer()
{
    if(m_sock != -1){
        //shutdown(m_sock, 2);
        close(m_sock);
        m_sock = -1;
    }
}

int32_t CDGramServer::InitDGramServer(int nPort)
{
    /* get a datagram socket */
    if((m_sock = m_cDGramComm.make_dgram_server_socket(nPort)) < 0){
        //TERR("CDGramServer: can't create server. %d\n", m_sock);
        return -1;
    }

    return 0;
}

/* receive messaages on the socket(m_sock) */
int32_t CDGramServer::receive(char* buf, int buf_size)
{
    sockaddr_in saddr;	/* put sender's address here	*/
    socklen_t saddrlen = sizeof(saddr);

    int32_t nLen = recvfrom(m_sock, buf, buf_size, 0, (sockaddr *) &saddr, &saddrlen);

    return nLen;
}

}
