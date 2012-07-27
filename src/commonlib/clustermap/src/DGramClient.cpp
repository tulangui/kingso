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
 * $Id: DGramClient.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "DGramClient.h"

namespace clustermap {

CDGramClient::CDGramClient()
{
    m_sock = -1;
}

CDGramClient::~CDGramClient()
{ 
    if(m_sock != -1){
        close(m_sock);
        m_sock = -1;
    }
}

int32_t CDGramClient::InitDGramClient(const char *pIp, uint16_t nPort)
{
    if(m_sock != -1)
        close(m_sock);

    /* get a datagram socket */
    if((m_sock = m_cDGramComm.make_dgram_client_socket()) < 0){
        //TERR("CDGramClient: make_dgram_client_socket failed! -1 \n");
        return -1;
    }

    /* combine hostname and portnumber of destination into an address */
    memset(&m_saddr, 0, sizeof(m_saddr));
    if(m_cDGramComm.make_internet_address(pIp, nPort, &m_saddr) < 0){
        //TERR("CDGramClient: make_internet_address failed! -2\n");
        return -1;
    }

    return 0;
}

/* send a string through the socket to that address */
int CDGramClient::send(const char *pMsg, int nLen)
{
    return sendto(m_sock, pMsg, nLen, 0, (struct sockaddr *)&m_saddr, sizeof(m_saddr));
}

}
