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
 * $Id: CMTCPClient.cpp 516 2011-04-08 15:43:26Z santai.ww $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include "CMTCPClient.h"

namespace clustermap {

CMTCPClient::CMTCPClient()
{
    m_factory = NULL;
    m_streamer = NULL;
    m_transport = NULL;
    m_connection = NULL;
    m_start = false;
    m_packet = NULL;
    m_sendFailNum = 0;
}

CMTCPClient::~CMTCPClient()
{
    m_start = false;
    clear();
}

int32_t CMTCPClient::setServerAddr(const char *spec)
{
    strcpy(m_spec, spec);
    setTimeout(3000);

    return 0;
}

int32_t CMTCPClient::setTimeout(int32_t timeout)
{
    m_timeout = timeout;
    return 0;
}

int32_t CMTCPClient::start()
{
    clear();
    m_factory = new anet::DefaultPacketFactory;
    if(m_factory == NULL) {
        clear();
        return -1;
    }
    m_streamer = new anet::DefaultPacketStreamer(m_factory);
    if(m_streamer == NULL) {
        clear();
        return -1;
    }
    m_transport = new anet::Transport;
    if(m_transport == NULL) {
        clear();
        return -1;
    }
    m_transport->start(); //Using anet in multithreads mode
    m_connection = m_transport->connect(m_spec, m_streamer, true);
    if(m_connection == NULL) {
        clear();
        return -1;
    }
    m_start = true;

    return 0;
}

int32_t CMTCPClient::send(const char *bufp, int32_t size)
{
    anet::DefaultPacket *request = NULL;
    if(m_connection == NULL)
        goto RECONNECT;
    request = new anet::DefaultPacket();
    if(request == NULL)
        return -1;
    request->setBody(bufp, size);
    if(m_packet) m_packet->free();
    m_packet = m_connection->sendPacket(request);
    if (!m_packet) {
        request->free();
        goto RECONNECT;
    }
    return 0;
RECONNECT:	
    start();
    m_sendFailNum = 0;
    return -1;
}

int32_t CMTCPClient::recv(char*& bufp, int32_t& size)
{
    if(m_packet == NULL || !m_packet->isRegularPacket())
        return -1;

    size_t bodyLength = 0;
    anet::DefaultPacket *reply = dynamic_cast<anet::DefaultPacket*>(m_packet);

    bufp = reply->getBody((size_t&)bodyLength);
    size = (int32_t)bodyLength;

    return 0;
}

int32_t CMTCPClient::clear()
{
    if(m_connection) {
        m_connection->close();//Close this connection.
        m_connection->subRef();//Do not use this connection any more.
        m_connection = NULL;
    }
    if(m_transport) {
        m_transport->stop();
        m_transport->wait();
        delete m_transport;
        m_transport = NULL;
    }
    if(m_streamer) {
        delete m_streamer;
        m_streamer = NULL;
    }
    if(m_packet) {
        m_packet->free();
        m_packet = NULL;
    }
    if(m_factory) {
        delete m_factory;
        m_factory = NULL;
    }
    return 0;
}

}
