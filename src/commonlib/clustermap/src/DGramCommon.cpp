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
 * $Id: DGramCommon.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <stdio.h>
#include <sys/ioctl.h>   
#include <net/if.h>   
#include <net/if_arp.h>   
#include <errno.h>   
#include "DGramCommon.h"

namespace clustermap {

int CDGramCommon::make_dgram_server_socket(int portnum)
{
    int	sock_id;	       /* the socket             */
    sockaddr_in saddr; /* build our address here */

    sock_id = socket(AF_INET, SOCK_DGRAM, 0);  /* get a socket */
    if (sock_id < 0){
        //TERR("CDGramCommon: make_dgram_server_socket failed! %s\n", strerror(errno));
        return -1;
    }

    /** build address and bind it to socket **/
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(portnum);
    saddr.sin_family = AF_INET;

    if (bind(sock_id, (sockaddr *)&saddr, sizeof(saddr)) != 0){
        //TERR("CDGramCommon: make_dgram_server_socket failed! %s\n", strerror(errno));
        return -2;
    }
    return sock_id;
}

int CDGramCommon::make_dgram_client_socket()
{
    return socket(AF_INET, SOCK_DGRAM, 0);
}

/*
 * constructor for an Internet socket address, uses hostname and port
 *   (host,port) -> *addrp
 */
int CDGramCommon::make_internet_address(const char *hostname, int port, sockaddr_in *addrp)
{
    hostent	*hp;

    bzero((void *)addrp, sizeof(sockaddr_in));
    hp = gethostbyname(hostname);
    if ( hp == NULL ) return -1;
    bcopy((void *)hp->h_addr, (void *)&addrp->sin_addr, hp->h_length);
    addrp->sin_port = htons(port);
    addrp->sin_family = AF_INET;
    return 0;
}

/*
 * extracts host and port from an internet socket address
 * *addrp -> (host,port)
 */
int CDGramCommon::get_internet_address(char *host, int nMaxLen, int *portp, sockaddr_in *addrp)
{
    char *pAddr = inet_ntoa(addrp->sin_addr);
    int nAddrLen = strlen(pAddr);
    if(nAddrLen >= nMaxLen)
        return -1;
    strcpy(host, pAddr);
    *portp = ntohs(addrp->sin_port);
    return 0;
}

/*
 * function：取得本地的IP地址字符串表示，因为可能有多个网址，所以对应。
 * para: nSize返回数组的长度
 * return： 成功返回多个IP地址，失败返回NULL，另外返回的地址由调用者释放
 */
char** CDGramCommon::get_local_ipaddr(int &nSize) 
{	
    int sd;
    struct ifconf conf;
    struct ifreq *ifr;
    char buff[BUFSIZ];
    nSize = 0;

    if((sd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
        return NULL;
    conf.ifc_len = BUFSIZ;
    conf.ifc_buf = buff;
    if(ioctl(sd, SIOCGIFCONF, &conf) < 0)
        return NULL;

    nSize = conf.ifc_len / sizeof(struct ifreq);
    ifr = conf.ifc_req;
    char ** ppIpAddrs = new char *[nSize];
    if(ppIpAddrs == NULL)
        return NULL;

    for(int i = 0; i < nSize; i++, ifr++){
        ppIpAddrs[i] = NULL;
        struct sockaddr_in *sin = (struct sockaddr_in *)(&ifr->ifr_addr);
        if(ioctl(sd, SIOCGIFFLAGS, ifr) < 0)
            continue;
        if(((ifr->ifr_flags & IFF_LOOPBACK) == 0) && (ifr->ifr_flags & IFF_UP)){
            char *pIp = inet_ntoa(sin->sin_addr);
            ppIpAddrs[i] = new char[strlen(pIp) + 1];
            if(ppIpAddrs[i] != NULL){
                strcpy(ppIpAddrs[i], pIp);
                (ppIpAddrs[i])[strlen(pIp)] = '\0';
            }
        }			
    }
    return ppIpAddrs;
}

/* 删除本地ip地址数组 */
void CDGramCommon::del_local_ipaddr(char **ppIpAddrs, int nSize)
{
    if(ppIpAddrs == NULL || nSize == 0)
        return;
    for(int i = 0; i < nSize; i++){
        if(ppIpAddrs[i] != NULL){
            delete[] ppIpAddrs[i];
            ppIpAddrs[i] = NULL;
        }
    }	
    delete[] ppIpAddrs;
    ppIpAddrs = NULL;
}

}
