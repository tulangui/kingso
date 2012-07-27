/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 167 $
 *
 * $LastChangedDate: 2011-03-25 11:13:57 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: Request.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: request object which send request packet to server 
 *         and recieve response $
 *
 *******************************************************************
 */

#ifndef __FRAMEWORK_REQUEST_H__
#define __FRAMEWORK_REQUEST_H__
#include "util/namespace.h"
#include "framework/namespace.h"
#include "util/ThreadLock.h"
#include "clustermap/CMTree.h"
#include <anet/anet.h>
#include <string>

FRAMEWORK_BEGIN;

class Request;
class ANETRequest;
class ANETResponse;
class ConnectionPool;
class Session;
class TaskQueue;

//packet information
struct _packet_info_t {
    char addr[32];
    _packet_info_t() { addr[0] = '\0'; }
};

//status
typedef enum {
    rt_unknown, 
    rt_sended,     //sended but not return
    rt_responsed   //response have return
} request_status_t;

//anet callback object 
class ANETRequest : public anet::IPacketHandler {
    public:
        ANETRequest(ConnectionPool &conn_pool);
        virtual ~ANETRequest();
        /**
         *@brief anet callback function
         *@param packet pointer to anet packet
         *@param args argument which bandle to the packet
         *@return 0,success  other, fail
         */
        virtual anet::IPacketHandler::HPRetCode 
            handlePacket(anet::Packet *packet, void *args);
        /**
         *@brief send request to destination
         *@return 0,success  other, fail
         */
        int32_t request();
        request_status_t getStatus() const { return _status; }
        void setOwner(Request *owner) { _owner = owner; }
        /**
         *@brief set request query 
         *@param data query
         *@param size query size
         *@return 0,success  other, fail
         */
        int32_t setQuery(const char *data, uint32_t size);

        const char *getQuery() {return _query;}

        uint32_t getQuerySize() {return _querySize;}

        /**
         *@brief set destination node
         *@param master master destination node
         *@param slave slave destination node
         */
        void setRequestNode(clustermap::CMISNode *master, 
                clustermap::CMISNode *slave = NULL) {
            _node = master;
            _standby = slave;
        }
        /**
         *@brief get destination nodes
         *@return destination nodes object
         */
        clustermap::CMISNode * getRequestNode() const { return _node; }

        /**
         *@brief get Response object
         *@return Response object
         */
        ANETResponse * getResponse() const { return _res; }
        /**
         *@brief set packet infomation
         *@param args packet infomation
         */
        void setArgs(void *args) { _args = args; }

        void setSerial(uint32_t serial) { _serial = serial; }

        uint32_t getSerial() { return _serial; }
        /**
         *@brief get packet infomation
         *@return packet infomation
         */
        void * getArgs() const { return _args; }
    public:
        ANETRequest *_next;
    private:
        request_status_t _status;
        ANETResponse *_res;
        Request *_owner;
        char *_query;
        uint32_t _querySize;
        uint32_t _serial;
        clustermap::CMISNode *_node;
        clustermap::CMISNode *_standby;
        void *_args; //do not free
        ConnectionPool & _connPool;
};

class ANETResponse {
    public:
        ANETResponse(const char *data, uint32_t size);
        ~ANETResponse();
    public:
        /**
         *@brief get response data
         *@return response data
         */
        const char * getData() const { return _data; }
        /**
         *@brief get data size
         *@return data size
         */
        uint32_t getSize() const { return _size; }
    private:
        char *_data;
        uint32_t _size;
};

class Request {
    public:
        Request(Session &owner, TaskQueue &taskQueue);
        ~Request();
    public:
        /**
         *@brief send request to server
         *@return 0,success  other, fail
         */
        int32_t request();
        /**
         *@brief recieve reaponse from server
         *@param req Request object
         */
        void response(ANETRequest *req);
        /**
         *@brief get the size of Request list
         *@return size
         */
        uint32_t size() const { return _reqSize; }
        /**
         *@brief get Request list
         *@return Request list
         */
        ANETRequest * getRequestList() const { return _reqList; }
    public:
        /**
         *@brief add ANETRequest object to Request list
         *@param req ANETRequest object
         */
        int32_t addANETRequest(ANETRequest *req);
        /**
         *@brief add Request list
         *@param req Request object
         */
        int32_t add(Request & req); 
        /**
         *@brief get the ANETRequest at idx position
         *@param idx ANETRequest offset
         */
        ANETRequest * getANETRequest(uint32_t idx);
        /**
         *@brief get status
         *@return status
         */
        request_status_t getStatus() const { return _status; }
    private:
	//status
        request_status_t _status;
	//Owner session
        Session &_owner;
        TaskQueue &_taskQueue;
	//Request list
        ANETRequest *_reqList;
        uint32_t _reqSize;
        uint32_t _sendFail;
        uint32_t _return;
        UTIL::Mutex _lock;
};

FRAMEWORK_END;

#endif //__FRAMEWORK_REQUEST_H__
