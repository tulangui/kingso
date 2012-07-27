#include "../src/commen/FileQueue.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "util/Log.h"

using namespace update;

int run_push(FileQueue* fq);
int run_pop(FileQueue* fq);

int main(int argc, char* argv[])
{
    FileQueue fq;

    if(argc!=3){
        fprintf(stderr, "no enough argument, return!\n");
        return -1;
    }

    if(fq.open(argv[2])){
        fprintf(stderr, "open FileQueue error, return!\n");
        return -1;
    }

    alog::Configurator::configureRootLogger();

    if(argv[1][0]=='0'){
        run_push(&fq);
    }
    else{
        run_pop(&fq);
    }

    fq.close();

    return 0;
}

int run_push(FileQueue* fq)
{
    uint64_t seq = 0;
    StorableMessage msg;
    char line[1024];
    int ret = 0;
    while(fgets(line, 1024, stdin)){
        msg.seq = ++seq;
        msg.ptr = line;
        msg.size = strlen(line)+1;
        while((ret=fq->push(msg))!=1){
            if(ret<0){
                fprintf(stderr, "push FileQueue error, return!\n");
                return -1;
            }
            else{
                usleep(1000);
            }
        }
    }
    return 0;
}

int run_pop(FileQueue* fq)
{
    StorableMessage msg;
    int ret = 0;
    while(1){
        while((ret=fq->pop(msg))!=1){
            if(ret<0){
                fprintf(stderr, "pop FileQueue error, return!\n");
                return -1;
            }
            else{
                usleep(1000);
            }
        }
        printf("string : %s\n", msg.ptr);
    }
    return 0;
}
