#include "update/plugin.h"
#include "util/Log.h"
#include <unistd.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"{ 
#endif

static const char* plugin_name = "test";

#pragma pack(1)
typedef struct{
    uint64_t id; 
    char line[0];
} package_t;
#pragma pack()

extern const char* up_name()
{
    return plugin_name;
}

extern void* up_init(const mxml_node_t* root)
{
    return (void*)plugin_name;
}

extern void up_dest(void* handler)
{
}

extern int up_select(thread_handler_t* handler)
{
    package_t* package = 0;
    StorableMessage msg;
    int ret = 0;
    if(!handler){
        TWARN("select argument error, return!");
        return -1; 
    }
    while(*(handler->running) && (ret=handler->upstream->pop(msg))==0){
        usleep(1000);
    }
    if(*(handler->running)){
        if(ret<0){
            TWARN("select thread pop from upsteam error, return!");
            return -1; 
        }
        package = (package_t*)msg.ptr;
        if(package->id&0x1LL){
            TNOTE("current seq=%llu id=%llu, choose it", msg.seq, package->id);
            while(*(handler->running) && (ret=handler->downstreams[0]->push(msg))==0){
                usleep(1000);
            }
            if(*(handler->running) && ret<0){
                TWARN("select thread push to downstreams[0] error, return!");
                return -1;
            }
        }
        else{
            TNOTE("current seq=%llu id=%llu, discard it", msg.seq, package->id);
        }
    }
    return 0;
}

extern int up_process(thread_handler_t* handler)
{
    package_t* package = 0;
    StorableMessage msg;
    int ret = 0;
    if(!handler){
        TWARN("process argument error, return!");
        return -1; 
    }
    while(*(handler->running) && (ret=handler->upstream->pop(msg))==0){
        usleep(1000);
    }
    if(*(handler->running)){
        if(ret<0){
            TWARN("process thread %d pop from upstream queue error, return!", handler->id);
            return -1;
        }
        package = (package_t*)msg.ptr;
        package->line[0] = '#';
        while(*(handler->running) && (ret=handler->downstreams[0]->push(msg))==0){
            usleep(1000);
        }
        if(*(handler->running) && ret<0){
            TWARN("process thread %d push from upstream queue error, return!", handler->id);
            return -1;
        }
    }
    return 0;
}

extern int up_send(thread_handler_t* handler)
{
    package_t* package = 0;
    StorableMessage msg;
    int ret = 0;
    if(!handler){
        TWARN("send argument error, return!");
        return -1;
    }
    while(*(handler->running) && (ret=handler->upstream->pop(msg))==0){
        usleep(1000);
    }
    if(*(handler->running)){
        if(ret<0){
            TWARN("send thread %d pop from upstream queue error, return!", handler->id);
            return -1;
        }
        package = (package_t*)msg.ptr;
        TNOTE("seq=%llu, id=%llu, string=%s", msg.seq, package->id, package->line);
    }
    fseek(*(handler->seqbak_file), 0, SEEK_SET);
    fprintf(*(handler->seqbak_file), "%llu\n", msg.seq);
    fflush(*(handler->seqbak_file));
    return 0;
}

#ifdef __cplusplus
}
#endif
