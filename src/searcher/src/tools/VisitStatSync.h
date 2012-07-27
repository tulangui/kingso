#ifndef _VISIT_STAT_SYNC_H_
#define _VISIT_STAT_SYNC_H_

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "VisitResult.h"
#include "VisitStatArgs.h"

class VisitStatAccpet {
public:
    VisitStatAccpet(VisitResult& result, int port);
    ~VisitStatAccpet();

    int start();
    void stop();

private:
    bool _stop;
    pthread_t _threadId;

    int _port;
    VisitResult& _result;

    int _socketHandle;
    struct sockaddr_in  _address;

private:    

    int recvResult(); 

public:
    static void * work(void* para);
};

class VisitStatClient {
public:
    VisitStatClient(VisitResult& result, VisitStatArgs& args);
    ~VisitStatClient();

    int sendResult();

private:
    int sendPacket(int no, char* message);

private:
    
    VisitResult& _result;
    VisitStatArgs& _args;
};

#endif
