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
 * $Id: DGramCommon.h 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

/********************************************************************
 * 
 * File: udp通信的公共代码，c代码
 * Desc: 主要是创建socket和socket地址
 * Log:
 *       Create by weilei, 2008/5/21
 * 
 *********************************************************************/

#ifndef __DGRAM_COMMON_H__
#define __DGRAM_COMMON_H__

#include <netinet/in.h>

/* hostname的最大长度 */
//const static int HOST_SIZE = 256;  
/* udp消息的最大长度 */
//const static int BUF_SIZE = 1500; 

namespace clustermap {

const int32_t MAX_UDP_PACKET_SIZE = 4096;

class CDGramCommon
{
    public:
        /* 创建服务端socket，输入参数是端口号 */
        int make_dgram_server_socket(int);

        /* 创建客户端socket */
        int make_dgram_client_socket(void);

        /* 根据ip地址和端口，创建socket地址 */
        int make_internet_address(const char *, int , sockaddr_in *);

        /* 根据socket地址，获取ip地址和端口 */
        int get_internet_address(char *, int , int *, sockaddr_in *);

        /*
         * function：取得本地的IP地址字符串表示，因为可能有多个网址，所以对应。
         * para: nSize返回数组的长度
         * return： 成功返回多个IP地址，失败返回NULL，另外返回的地址由调用者释放
         */ 
        char** get_local_ipaddr(int &nSize);

        /* 删除本地ip地址数组 */
        void del_local_ipaddr(char **ppIpAddrs, int nSize);
};

}
#endif
