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
 * $Id: CMClientRemote.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "CMClientRemote.h"

namespace clustermap {

CMClientRemote::CMClientRemote()
{
    //printf("CMClientRemote new\n");
}

CMClientRemote::~CMClientRemote()
{
    //printf("CMClientRemote del\n");
}

int32_t CMClientRemote::init(const char* server_ip, uint16_t udp_port, uint16_t tcp_port)
{
    strcpy(m_server_ip, server_ip);
    m_server_udp_port = udp_port;
    m_server_tcp_port = tcp_port;

    return 0;
}

int32_t CMClientRemote::Initialize(const MsgInput* in, MsgOutput* out)
{
    CMTCPClient cTcpClient;
    if(initTcpClient(&cTcpClient) != 0)
        return -1;

    InitInput* input = (InitInput*)in;
    InitOutput* output = (InitOutput*)out;

    if(input->nodeToBuf() <= 0)
        return -1;
    if(cTcpClient.send(input->buf, input->len) != 0)
        return -1;
    char* body;
    int32_t bodylen;
    if(cTcpClient.recv(body, bodylen) != 0)
        return -1;
    if(!out->isSuccess(body, bodylen))
        return -1;
    output->outlen = bodylen;
    output->outbuf = new char[bodylen];
    if(output->outbuf == NULL)
        return -1;
    memcpy(output->outbuf, body, bodylen);
    return 0;
}

int32_t CMClientRemote::Register(const MsgInput* in, MsgOutput* out)
{
    CMTCPClient cTcpClient;
    if(initTcpClient(&cTcpClient) != 0)
        return -1;
    if(m_cDGramClient.InitDGramClient(m_server_ip, m_server_udp_port) != 0)
        return -1;

    RegInput* input = (RegInput*)in;
    RegOutput* output = (RegOutput*)out;

    if(input->nodeToBuf() <= 0)
        return -1;
    if(cTcpClient.send(input->buf, input->len) != 0)
        return -1;
    char* body;
    int32_t bodylen;
    if(cTcpClient.recv(body, bodylen) != 0)
        return -1;
    if(!out->isSuccess(body, bodylen))
        return -1;
    output->outlen = bodylen;
    if(output->outbuf)
        delete[] output->outbuf;
    output->outbuf = new char[bodylen];
    if(output->outbuf == NULL)
        return -1;
    memcpy(output->outbuf, body, bodylen);
    return 0;
}

int32_t CMClientRemote::Subscriber(const MsgInput* in, MsgOutput* out)
{
    CMTCPClient cTcpClient;
    if(initTcpClient(&cTcpClient) != 0)
        return -1;
    SubInput* input = (SubInput*)in;
    SubOutput* output = (SubOutput*)out;

    if(input->nodeToBuf() <= 0)
        return -1;
    if(cTcpClient.send(input->buf, input->len) != 0)
        return -1;
    char* body;
    int32_t bodylen;
    if(cTcpClient.recv(body, bodylen) != 0)
        return -1;
    if(!out->isSuccess(body, bodylen))
        return -1;
    output->outlen = bodylen;
    output->outbuf = new char[bodylen];
    if(output->outbuf == NULL)
        return -1;
    memcpy(output->outbuf, body, bodylen);
    return 0;
}

int32_t CMClientRemote::reportState(const MsgInput*in, MsgOutput* out)
{
    StatInput* input = (StatInput*)in;
    m_cDGramClient.send((const char*)input->buf, input->len);
    return 0;
}

int32_t CMClientRemote::initTcpClient(CMTCPClient* cpClient)
{
    char server_spec[64];
    sprintf(server_spec, "%s:%s:%u", "tcp", m_server_ip, m_server_tcp_port);
    if(cpClient->setServerAddr(server_spec) != 0)
        return -1;
    if(cpClient->start() != 0)
        return -1;
    return 0;
}

}
