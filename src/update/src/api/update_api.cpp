#include "update/update_api.h"
#include <pthread.h>
#include <stdint.h>
#include <malloc.h>
#include "conhash.h"
#include <unistd.h>
#include "netdef.h"
#include "util/Log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "netfunc.h"
#include "../common/StorableMessage.h"

#define RECONNECT_TIME 3

using namespace update;

typedef struct
{
    int fd; 
    up_addr_t addr;
}
machine_t;

struct update_api
{
    int updater_count;
    machine_t updaters[64];
    uint64_t updater_status;
    conhash_t* updater_conhash;
};

extern update_api_t* update_api_create(up_addr_t addrs[], int addr_count)
{
    update_api_t* api = 0;
    int i = 0;

    if(!addrs || addr_count<=0 || addr_count>64){
        return 0;
    }

    api = (update_api_t*)malloc(sizeof(update_api_t));
    if(!api){
        return 0;
    }

    api->updater_count = addr_count;
    for(i=0; i<addr_count; i++){
        api->updaters[i].addr = addrs[i];
        api->updaters[i].fd = -1;
    }
    api->updater_status = 0ll;
    api->updater_conhash = conhash_create(addr_count);
    if(!api->updater_conhash){
        free(api);
        return 0;
    }

    return api;
}

extern void update_api_destroy(update_api_t* api)
{
    if(api){
        int i = 0;
        for(i=0; i<api->updater_count; i++){
            if(api->updaters[i].fd>=0){
                close(api->updaters[i].fd);
            }
        }
        if(api->updater_conhash){
            conhash_dest(api->updater_conhash);
        }
        free(api);
    }
}

static int reconnect(update_api_t* api, int id);
extern int update_api_send(update_api_t* api, uint64_t id, void* data, int size, char cmpr, char cmd)
{
    StorableMessage msg;
    conhash_result_t conhash_result;
    net_head_t* head = 0;
    machine_t* updater = 0;
    int no = 0;
    int i = 0;

    if(!api || !data || size<=0){
        return UP_EARGUMENT;
    }

    // 填充msg
    msg.ptr = (char*)malloc(sizeof(net_head_t)+size);
    if(!msg.ptr){
        TWARN("malloc StorableMessage buffer error, return!");
        return UP_ENOMEM;
    }
    head = (net_head_t*)(msg.ptr);
    head->seq = id;
    head->size = size;
    head->cmpr = cmpr;
    head->cmd  = cmd;
    memcpy(head+1, data, size);
    msg.seq = id;
    msg.max = msg.size = sizeof(net_head_t)+size;
    msg.freeFn = free;
    TDEBUG("make msg done, id=%llu, size=%d", id, msg.size);

    // 选择节点
    bool isFirst = true;
    conhash_result.intv = conhash_find(api->updater_conhash, id, api->updater_status);
    while(conhash_result.real_hash==NODE_COUNT_MAX && conhash_result.valid_hash==NODE_COUNT_MAX){
        if(isFirst == false) {
            TNOTE("no node ready, wait!");
            usleep(1000);
        }
        isFirst = false;
        for(i=0; i<api->updater_count; i++){
            reconnect(api, i);
        }
        conhash_result.intv = conhash_find(api->updater_conhash, id, api->updater_status);
    }
    if(conhash_result.real_hash!=conhash_result.valid_hash){
        if(reconnect(api, conhash_result.real_hash)==0){
            conhash_result.valid_hash = conhash_result.real_hash;
        }
    }
    no = conhash_result.valid_hash;
    updater = &(api->updaters[no]);
    TDEBUG("choose %d node, real_hash=%d, valid_hash=%d", no, conhash_result.real_hash, conhash_result.valid_hash);

    if(write_check(updater->fd, msg.ptr, msg.size)!=msg.size){
        TWARN("write update data error, return!");
        close(updater->fd);
        updater->fd = -1; 
        api->updater_status &= (0xFFFFFFFFFFFFFFFFLL^(0x1LL<<no));
        return UP_EFUNCERR; 
    }   
    TDEBUG("send msg done, seq=%llu, size=%d", msg.seq, msg.size);
    if(read_wait(updater->fd, head, sizeof(net_head_t), 0)!=sizeof(net_head_t)){
        TWARN("read response data error, return!");
        close(updater->fd);
        updater->fd = -1; 
        api->updater_status &= (0xFFFFFFFFFFFFFFFFLL^(0x1LL<<no));
        return UP_EFUNCERR; 
    }   
    TDEBUG("receive response done, seq=%llu, cmd=%d", head->seq, head->cmd);
    if(head->seq!=msg.seq){
        TWARN("response id not match data id, return!");
        close(updater->fd);
        updater->fd = -1; 
        api->updater_status &= (0xFFFFFFFFFFFFFFFFLL^(0x1LL<<no));
        return UP_EFUNCERR; 
    }
    switch(head->cmd){
        case UPCMD_ACK_SUCCESS :
            return UP_SUCCESS;
        case UPCMD_ACK_EFORMAT :
            return UP_EARGUMENT;
        case UPCMD_ACK_EUNRECOGNIZED :
            return UP_EARGUMENT;
        case UPCMD_ACK_EPROCESS :
            return UP_EFUNCERR;
        case UPCMD_ACK_EUNKOWN :
        default :
            return UP_EUNKOWN;
    }   
}

static int reconnect(update_api_t* api, int id) 
{
    if(api->updaters[id].fd<0){
        net_head_t head;
        struct sockaddr_in addr;
        int keepalive = 1;
        int fd = socket(AF_INET, SOCK_STREAM, 0); 
        if(fd<0){
            TWARN("updater %d create socket to %s:%d error, return!", id, api->updaters[id].addr.ip, api->updaters[id].addr.port);
            return -1; 
        }   
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(int));
        bzero(&addr, sizeof(struct sockaddr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(api->updaters[id].addr.port);
        inet_aton(api->updaters[id].addr.ip, &(addr.sin_addr));
        if(connect_times(fd, (struct sockaddr*)(&addr), sizeof(addr), RECONNECT_TIME)){
            TWARN("updater %d connect to %s:%d error, return!", id, api->updaters[id].addr.ip, api->updaters[id].addr.port);
            close(fd);
            return -1; 
        }   
        TDEBUG("updater %d connect to %s:%d done.", id, api->updaters[id].addr.ip, api->updaters[id].addr.port);
        if(read_wait(fd, &head, sizeof(head), 0)!=sizeof(head)){
            TWARN("updater %d receive first ack for %s:%d error, return!", id, api->updaters[id].addr.ip, api->updaters[id].addr.port);
            close(fd);
            return -1; 
        }   
        TDEBUG("updater %d receive first head from %s:%d done.", id, api->updaters[id].addr.ip, api->updaters[id].addr.port);
        api->updaters[id].fd = fd; 
        api->updater_status |= (0x1LL<<id);
    }   
    else{
        api->updater_status |= (0x1LL<<id);
        return 0;
    }

    return -1;
}

