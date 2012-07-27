/** \file
 *******************************************************************
 * $Author: xiaojie.lgx $
 *
 * $LastChangedBy: xiaojie.lgx $
 *
 * $Revision: 11792 $
 *
 * $LastChangedDate: 2012-05-03 14:20:42 +0800 (Thu, 03 May 2012) $
 *
 * $Id: CMTCPServer.cpp 11792 2012-05-03 06:20:42Z xiaojie.lgx $
 *
 * $Brief: ClusterMap库的功能类 $
 *******************************************************************
 */

#include <stdio.h>
#include "kingso_cm.pb.h"
#include "CMTCPServer.h"

namespace clustermap {

anet::IPacketHandler::HPRetCode CMTCPAdapter::handlePacket(anet::Connection *connection, anet::Packet *packet)
{
    if (packet->isRegularPacket())
    {
        //handle request
        int32_t ret;
        anet::DefaultPacket *request = dynamic_cast<anet::DefaultPacket*>(packet);

        ComInput tmp;
        size_t bodyLength = 0;
        char* body = request->getBody(bodyLength);
        tmp.bufToNode(body, bodyLength);

        switch(tmp.type)
        {
            case msg_init:
                {
                    InitInput input;
                    InitOutput output; 
                    input.bufToNode(body, bodyLength);
                    ret = handleMessage(&input, &output, request);
                }
                break;	
            case msg_register:
                {
                    RegInput input;
                    RegOutput output;
                    input.bufToNode(body, bodyLength);
                    ret = handleMessage(&input, &output, request);
                }
                break;
            case msg_sub_child:
            case msg_sub_all:
            case msg_sub_selfcluster:
                {
                    SubInput input;
                    SubOutput output;
                    input.bufToNode(body, bodyLength);
                    ret = handleMessage(&input, &output, request);
                }
                break;
            case cmd_get_clustermap:
                {
                    getmapInput input;
                    getmapOutput output;
                    input.bufToNode(body, bodyLength);
                    ret = handleMessage(&input, &output, request);
                }
                break;
            case cmd_set_clustermap:
                {
                    setmapInput input;
                    setmapOutput output;
                    input.bufToNode(body, bodyLength);
                    ret = handleMessage(&input, &output, request);
                    if(ret == 0 && m_cTree.reload())
                        ret = 0;
                    else
                        ret = -1;
                }
                break;
            case cmd_get_info_by_name:
            case cmd_get_info_by_spec:
            case cmd_get_info_allbad:
            case cmd_get_info_watchpoint:
            case cmd_get_info_allauto:
                {
                    getNodeInput in;
                    getNodeOutput out;
                    in.bufToNode(body, bodyLength);
                    ret = handleMessage(&in, &out, request);
                }
                break;
            case msg_valid:
                {
                    ValidInput in;
                    ValidOutput out;
                    in.bufToNode(body, bodyLength);
                    ret = handleMessage(&in, &out, request);
                }
                break;
            case msg_ClusterState:
                {
                    ClusterStateInput in;
                    ClusterStateOutput out;
                    in.bufToNode(body, bodyLength);
                    ret = handleMessage(&in, &out, request);
                }
                break;
            case msg_EnvVarCommand:
                {
                    EnvVarInput in;
                    EnvVarOutput out;
                    in.bufToNode(body, bodyLength);
                    ret = handleMessage(&in, &out, request);
                }
                break;
            case msg_active:
                ret = handleSlave(body, bodyLength, request);
                break;
            case cmd_by_url:
                ret = handleUrl(body, bodyLength, request);
                break;
            default:
                request->free();
                ret = -1;
        }		

        if(ret != 0) {
            return anet::IPacketHandler::FREE_CHANNEL;
        }
        if(!connection->postPacket(request)) {
            request->free();
        }
    } else {
        //control command received
        packet->free(); //free packet if finished
    }
    return anet::IPacketHandler::FREE_CHANNEL;
}

int32_t CMTCPAdapter::handleMessage(ComInput* in, ComOutput* out, anet::DefaultPacket *&request)
{
    uint32_t chid = request->getChannelId();
    request->free();
    request = new anet::DefaultPacket;
    if(request == NULL)
        return -1;
    request->setChannelId(chid);
    if(m_cTree.process(in, out) != 0) {
        ComInput tmp;
        tmp.type = msg_unvalid;
        if(tmp.nodeToBuf() <= 0) {
            request->free();
            return -1;
        }
        request->setBody(tmp.buf, tmp.len);
    } else {
        request->setBody(out->outbuf, out->outlen);
    }
    return 0;
}

int32_t CMTCPAdapter::handleSlave(char* body, int32_t bodyLength, anet::DefaultPacket*& request)
{
    uint32_t chid = request->getChannelId();
    if(bodyLength < 20 + 1) {
        request->free();
        return -1;
    }
    char* p = body + 1;
    time_t slave_time = 0, master_time = 0;
    char master_md5[16], slave_md5[16];

    m_cTree.getversion(master_md5, master_time);

    memcpy(slave_md5, p, 16);
    p += 16;
    slave_time = ntohl(*(int32_t*)p);
    p += 4; 

    request->free();
    request = new anet::DefaultPacket;
    if(request == NULL)
        return -1;
    request->setChannelId(chid);

    if(memcmp(master_md5, slave_md5, 16) && master_time >= slave_time){
        setmapInput in;
        setmapOutput out;
        in.type = cmd_get_clustermap;
        in.op_time = master_time;
        if(m_cTree.process(&in, &out) != 0) {
            TLOG("get clustermap error");
            request->free();
            return -1;
        }
        request->setBody(out.outbuf, out.outlen);
    } else {
        char send_buf[64];
        p = send_buf;
        *p = (char)msg_active;
        p += 1;
        memcpy(p, master_md5, 16);
        p += 16;
        *(int32_t*)p = htonl(master_time);
        p += 4;
        request->setBody(send_buf, p - send_buf);
    }
    return 0;
}

int32_t CMTCPAdapter::handleUrl(char* body, int32_t bodyLength, anet::DefaultPacket*& request)
{
    ComInput in; 
    ComOutput out; 

    in.buf = new char[bodyLength+1];
    if(in.buf == NULL) {
        return -1;
    }
    memcpy(in.buf, body, bodyLength);
    in.buf[bodyLength] = 0;
    in.len = bodyLength;
    in.type = cmd_by_url;

    kingso_cm::CMResponse res;
    if(m_cTree.process(&in, &out) != 0) {
        kingso_cm::NodeInfo* node = res.add_nodes();
        node->set_node_ip("0.0.0.0");
        node->set_node_port(0);
        node->set_protocol(tcp);
        node->set_state(kingso_cm::unvalid);
        node->set_col_id(0);
        res.set_cluster_num(0);
        res.set_response_type(kingso_cm::CM_QUERY_FAILED);
    } else {
        int num = out.outlen / sizeof(ClusterInfo);
        ClusterInfo* info = (ClusterInfo*)out.outbuf;
        for(int i = 0; i < num; i++) {
            kingso_cm::NodeInfo* node = res.add_nodes();
            node->set_node_ip(info[i].ip);
            node->set_node_port(info[i].port);
            node->set_protocol(info[i].protocol);
            if(info[i].valid) {
                switch (info[i].state) {
                    case normal:
                        node->set_state(kingso_cm::Normal);
                        break;
                    case timeout:
                        node->set_state(kingso_cm::timeout);
                        break;
                    case uninit:
                        node->set_state(kingso_cm::uninit);
                        break;
                    default:
                        node->set_state(kingso_cm::abnormal);
                        break;
                }
            } else {
                node->set_state(kingso_cm::unvalid);
            }
            node->set_col_id(info[i].col_id);
        }
        res.set_cluster_num(num);
        res.set_response_type(kingso_cm::CM_SUCCEED);
    }
                  
    std::string data;
    if(res.SerializeToString(&data) == false) {
        TERR("SerializeToString error");
        return -1;
    }
        
    uint32_t chid = request->getChannelId();
    request->free();
    request = new anet::DefaultPacket;
    if(request == NULL) {
        return -1;
    }
    request->setChannelId(chid);
    request->setBody(data.c_str(), data.size());

    return 0;
}

CMTCPServer::~CMTCPServer()
{
    m_transport.stop();
    m_transport.wait();
    if(m_ppacketstreamer)
        delete m_ppacketstreamer;
}

int32_t CMTCPServer::setServerAddr(const char *spec)
{
    if (spec == NULL)	{
        return -1;
    }
    m_spec = spec;
    return 0;
}

int32_t CMTCPServer::setTimeout(int32_t timeout)
{
    m_timeout = timeout;
    return 0;
}

int32_t CMTCPServer::start()
{
    if (m_spec.empty())	{
        return -1;
    }
    m_ppacketstreamer = new anet::DefaultPacketStreamer(&m_packetfactory);
    anet::IOComponent *listener = m_transport.listen(m_spec.c_str(), m_ppacketstreamer, &m_adapter, m_timeout);
    if(listener == NULL)
        return -1;
    m_transport.start();
    return 0;
}

}
