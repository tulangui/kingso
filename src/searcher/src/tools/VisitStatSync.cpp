#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "VisitStat.h"
#include "VisitStatSync.h"

VisitStatAccpet::VisitStatAccpet(VisitResult& result, int port) : _result(result) 
{
    _port = port;
    _stop = true;
}

VisitStatAccpet::~VisitStatAccpet()
{
}
            
int VisitStatAccpet::start()
{
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    _address.sin_addr.s_addr = htonl(INADDR_ANY);

    _socketHandle = socket(AF_INET, SOCK_STREAM, 0);
    if(_socketHandle == -1) return -1;

    int flags = fcntl(_socketHandle, F_GETFD);
    if(flags == -1) return -1;
    flags |= FD_CLOEXEC;
    if(fcntl(_socketHandle, F_SETFD, flags) == -1) return -1;

    if(bind(_socketHandle, (struct sockaddr *)&_address, sizeof(_address)) < 0) {
        return -1;
    }
        
    if(listen(_socketHandle, 256) < 0) {
        return -1;
    }
    _stop  = false;

    pthread_create(&_threadId, NULL, work, this);
    return 0;
}

void VisitStatAccpet::stop()
{
    if(_stop) return;
    
    while(!_result.isOk()) {
        sleep(1);
    }

    _stop = true;
    
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) return;

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(_port);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(fd, (struct sockaddr *)&address, sizeof(address));
    close(fd);

    pthread_join(_threadId, NULL);
    close(_socketHandle);
}


int VisitStatAccpet::recvResult()
{
    struct sockaddr_in addr;
    int len = sizeof(addr);
             
    int fd = accept(_socketHandle, (struct sockaddr *) & addr, (socklen_t*) & len);
    if (fd < 0) {
        TERR("accpet error\n");
        return -1;
    }
    unsigned char* ptr = (unsigned char*)&(addr.sin_addr.s_addr);
    TLOG("accept %u.%u.%u.%u", ptr[0], ptr[1], ptr[2], ptr[3]);

    if(_stop) return 0;

    char header[16];
    header[0] = 0;
    len = recv(fd, header, 16, MSG_WAITALL);
    if(len != 16 || memcmp(header+8, "visitfre", 8)) {
        close(fd);
        TERR("accpet header error, len=%d, str=%s", len, header);
        return -1;
    }
    int size = *(int32_t*)header;
    char* buf = new char[size + sizeof(VisitInfo)];

    len = 0;
    int ret = 1;
    while(ret > 0 && len < size) {
        ret = recv(fd, buf + len, size - len, MSG_WAITALL);
        if(ret > 0) len += ret;
    }
    
    if(len < size) {
        close(fd);
        delete buf;
        TERR("accpet body error, %d(%d)", len, size);
        return -1;
    }
    _result.setMulResult((VisitInfo*)buf, size / sizeof(VisitInfo));
    delete buf;

    char responses[128];
    strcpy(responses, "success");

    send(fd, responses, 8, MSG_WAITALL);
    close(fd);
    
    TLOG("recv %u.%u.%u.%u success, len=%d", ptr[0], ptr[1], ptr[2], ptr[3], size);

    return 0;
}

void * VisitStatAccpet::work(void* para)
{
    VisitStatAccpet* st = (VisitStatAccpet*)para;

    while(st->_stop == false) {
        st->recvResult();
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////
VisitStatClient::VisitStatClient(VisitResult& result,
        VisitStatArgs& args) : _result(result), _args(args)
{
}
VisitStatClient::~VisitStatClient()
{

}

int VisitStatClient::sendResult() 
{
    char message[1024];
    char flag[_args.getColNum()];

    int okNum = 0;
    memset(flag, 0, _args.getColNum());

    while(okNum < _args.getColNum()) {
        okNum = 0;
        for(int i = 0; i < _args.getColNum(); i++) {
            if(flag[i]) {
                okNum++;
                continue;
            }
            if(sendPacket(i, message) < 0) {
                TERR("send Packet %d error, %s", i, message);
            } else {
                TLOG("send Packet %d success", i);
                okNum++;
                flag[i] = 1;
            }
        }
        sleep(5);
    }

    return 0;
}

int VisitStatClient::sendPacket(int no, char* message)
{
    int port = 0;
    char* ip = NULL;
    if(_args.getColInfo(no, ip, port)) {
        sprintf(message, "get col info error");
        return -1;
    }

    int num = 0;
    VisitInfo* info = NULL;
    if(_result.getColResult(no, info, num) < 0) {
        sprintf(message, "get %d result error", no);
        return -1;
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd == -1) {
        sprintf(message, "socket error");
        return -1;
    }

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = inet_addr(ip);
    if(connect(fd, (struct sockaddr *)&address, sizeof(address)) == -1) {
        close(fd);
        sprintf(message, "connect error, %s %d", ip, port);
        return -1;
    }

    char header[16];
    *(int32_t*)header = num * sizeof(VisitInfo);
    strcpy(header+8, "visitfre");

    int len = send(fd, header, 16, 0);
    if(len != 16) {
        close(fd);
        sprintf(message, "send header error");
        return -1;
    }

    len = num * sizeof(VisitInfo);
    int ret = 1;
    int finish = 0;
    while(ret > 0 && finish < len) {
        ret = send(fd, info + finish, len - finish, 0);
        if(ret > 0) finish += ret;
    }
    if(finish < len) {
        sprintf(message, "send body error, len=%d(%d)", finish, len);
        close(fd);
        return -1;
    }

    header[0] = 0;
    len = recv(fd, header, 8, 0);
    if(len != 8 || memcmp(header, "success", 7)) {
        sprintf(message, "recv tail error, len=%d, %s", len, header);
        close(fd);
        return -1;
    }
    close(fd);
    TLOG("send %s %d success, num=%d", ip, port, num);
    return 0;
}

