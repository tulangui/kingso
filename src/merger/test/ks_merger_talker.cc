#include <stdio.h>
#include "util/kspack.h"
#include <anet/anet.h>
#include "util/MemPool.h"
#include "sort/SortResult.h"
#include "framework/Compressor.h"

void print_detail_result(kspack_t* pack);
void print_search_result(kspack_t* pack);

int main(int argc, char* argv[])
{
    static anet::DefaultPacketFactory SG_DF_PF;
    static anet::DefaultPacketStreamer SG_DF_PS(&SG_DF_PF);

    anet::Transport transport;
    anet::Connection* connection = 0;
    anet::DefaultPacket* packet = 0;
    anet::Packet* rcvp = 0;

    kspack_t* pack = 0;
    char* name = 0;
    int name_len = 0;
    void* value = 0;
    int value_len = 0;
    char value_type = 0;

    char addr[64];

    FRAMEWORK::Compressor* compr = 0;
    MemPool mempool;
    MemPool* pHeap = &mempool;
    char* resData = 0;
    uint32_t resSize = 0;
    int ret = 0;

    if(argc<3){
        fprintf(stderr, "Fuck, why didn't you input query!\n");
        return -1;
    }
    else{
        printf("merger : %s\n", argv[1]);
        printf("query : %s\n", argv[2]);
        snprintf(addr, 64, "tcp:%s", argv[1]);
    }

    transport.start(true);
    connection = transport.connect(addr, &SG_DF_PS, false);
    if(!connection){
        fprintf(stderr, "Fuck, connecting merger error!\n");
        return -1;
    }

    packet = new anet::DefaultPacket();
    if(!packet){
        fprintf(stderr, "Fuck, create new packet error!\n");
        connection->subRef();
        return -1;
    }

    packet->setBody(argv[2], strlen(argv[2]));
    rcvp = connection->sendPacket(packet);
    if(!rcvp){
        fprintf(stderr, "Fuck, send request to merger error!\n");
        connection->subRef();
        packet->free();
        return -1;
    }
    if(!rcvp->isRegularPacket()){
        fprintf(stderr, "Fuck, receive response packet error!\n");
        connection->subRef();
        rcvp->free();
        return -1;
    }

    connection->subRef();

    packet = dynamic_cast<anet::DefaultPacket*>(rcvp);

    compr = FRAMEWORK::CompressorFactory::make(FRAMEWORK::ct_zlib, pHeap);
    if(!compr){
        fprintf(stderr, "Fuck, make compressor error!\n");
        packet->free();
        return -1;
    }

    ret = compr->uncompress(packet->getBody(), packet->getBodyLen(), resData, resSize);
    if(ret!=KS_SUCCESS || resData==0 || resSize==0){
        FRAMEWORK::CompressorFactory::recycle(compr);
        fprintf(stderr, "Fuck, uncompress error!\n");
        packet->free();
        return -1;
    }

    pack = kspack_open('r', resData, resSize);
    if(!pack){
        fprintf(stderr, "Fuck, kspack_open error!\n");
        packet->free();
        return -1;
    }

    if(kspack_get(pack, "field_name", 10, &value, &value_len, &value_type)==0){
        print_detail_result(pack);
    }
    else{
        print_search_result(pack);
    }

    kspack_close(pack, 0);
    packet->free();
    FRAMEWORK::CompressorFactory::recycle(compr);
    transport.stop();
    transport.wait();

    return 0;
}

void replace_string(char* str, int len, char c, char d)
{
    int i = 0;
    for(; i<len; i++){
        if(str[i]==c){
            str[i] = d;
        }
    }
}

void print_detail_result(kspack_t* pack)
{
    char* name = 0;
    int name_len = 0;
    void* value = 0;
    int value_len = 0;
    char value_type = 0;

    kspack_first(pack);
    while(kspack_next(pack, &name, &name_len, &value, &value_len, &value_type)==0){
        if(strncmp(name, "field_count", 11)==0){
            printf("%s : %d\n", name, *((int*)value));
        }
        else if(strncmp(name, "data_count", 10)==0){
            printf("%s : %d\n", name, *((int*)value));
        }
        else{
            replace_string((char*)value, value_len, 0x01, '|');
            printf("%s : %s\n", name, (char*)value);
        }
    }
}

void print_search_result(kspack_t* pack)
{
    char* name = 0;
    int name_len = 0;
    void* value = 0;
    int value_len = 0;
    char value_type = 0;

    if(kspack_get(pack, "DocsSearch", 10, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get DocsSearch from kspack error!\n");
    }
    else{
        printf("DocsSearch : %u\n", *((uint32_t*)value));
    }

    if(kspack_get(pack, "DocsFound", 9, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get DocsFound from kspack error!\n");
    }
    else{
        printf("DocsFound : %u\n", *((uint32_t*)value));
    }

    if(kspack_get(pack, "EstimateDocsFound", 17, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get EstimateDocsFound from kspack error!\n");
    }   
    else{
        printf("EstimateDocsFound : %u\n", *((uint32_t*)value));
    } 

    if(kspack_get(pack, "DocsRestrict", 12, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get DocsRestrict from kspack error!\n");
    }   
    else{
        printf("DocsRestrict : %u\n", *((uint32_t*)value));
    }

    if(kspack_get(pack, "DocsReturn", 10, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get DocsReturn from kspack error!\n");
    }   
    else{
        printf("DocsReturn : %u\n", *((uint32_t*)value));
    }

    if(kspack_get(pack, "SortResult", 10, &value, &value_len, &value_type)){
        fprintf(stderr, "Fuck, get SortResult from kspack error!\n");
    }   
    else{
        int64_t* nids = 0;
        int nid_count = 0;
        MemPool mempool;
        MemPool* pHeap = &mempool;
        sort_framework::SortResult* sr = NEW(pHeap, sort_framework::SortResult)(pHeap);
        if(!sr){
            fprintf(stderr, "Fuck, NEW SortResult error!\n");
            return;
        }
        if(!sr->deserialize((char *)value, value_len)){
            fprintf(stderr, "Fuck, deserialize SortResult error!\n");
            return;
        }
        nid_count = sr->getNidList(nids);
        for(int i=0; i<nid_count; i++){
            printf("nids[%d] : %lld\n", i, nids[i]);
        }
    }
}
