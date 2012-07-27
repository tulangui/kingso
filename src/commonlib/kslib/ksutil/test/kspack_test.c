#include "util/kspack.h"
#include <stdio.h>
#include <string.h>

int main()
{
    kspack_t* pack = 0;
    kspack_t* sub = 0;
    kspack_t* tmp = 0;
    void* data = 0;
    int size = 0;
    char* name = 0;
    int nlen = 0;
    void* value = 0;
    int vlen = 0;
    char vtype = 0;

    char str8k[8193];
    memset(str8k, 'a', 8192);
    str8k[8192] = 0;

    pack = kspack_open('w', 0, 0);
    if(!pack){
        fprintf(stderr, "kspack_open error, return![%d]\n", __LINE__);
        return -1;
    }

    sub = kspack_sub_open(pack, "xiaoming", 8);
    if(!sub){
        fprintf(stderr, "kspack_sub_open error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        return -1;
    }
    if(kspack_put(sub, "nianling", 8, "20", 3, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    if(kspack_put(sub, "shengao", 7, "170", 4, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    if(kspack_done(sub)){
        fprintf(stderr, "kspack_done error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    kspack_close(sub, 0);

    sub = kspack_sub_open(pack, "xiaoliang", 9);
    if(!sub){
        fprintf(stderr, "kspack_sub_open error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        return -1;
    }
    if(kspack_put(sub, "nianling", 8, "21", 3, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    if(kspack_put(sub, "shengao", 7, "180", 4, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    tmp = sub;
   
    sub = kspack_sub_open(pack, "xiaoqiang", 9);
    if(!sub){
        fprintf(stderr, "kspack_sub_open error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        return -1;
    }
    if(kspack_put(sub, "nianling", 8, "22", 3, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    if(kspack_put(sub, "shengao", 7, "190", 4, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    if(kspack_done(sub)){
        fprintf(stderr, "kspack_done error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        kspack_close(sub, 0);
        return -1;
    }
    kspack_close(sub, 0);
    kspack_close(tmp, 0);

    if(kspack_put(pack, "str8k", 5, str8k, 8193, KS_BYTE_VALUE)){
        fprintf(stderr, "kspack_put error, return![%d]\n", __LINE__);
        kspack_close(pack, 1);
        return -1;
    }

    kspack_done(pack);
    data = kspack_pack(pack);
    size = kspack_size(pack);
    kspack_close(pack, 0);

    printf("data size=%d\n", size);

    pack = kspack_open('r', data, size);
    if(!pack){
        fprintf(stderr, "kspack_open error, return![%d]\n", __LINE__);
        return -1;
    }
    kspack_first(pack);
    while(kspack_next(pack, &name, &nlen, &value, &vlen, &vtype)==0){
        printf("name=%s, nlen=%d, vtype=%d\n", name, nlen, vtype);
        if(vtype==KS_BYTE_VALUE){
            printf("name=%s, nlen=%d, value=%s, vlen=%d, vtype=%d\n", name, nlen, (char*)value, vlen, vtype);
            continue;
        }
        sub = kspack_sub_open(pack, name, nlen);
        if(!sub){
            fprintf(stderr, "kspack_sub_open error, return![%d]\n", __LINE__);
            kspack_close(pack, 0);
            free(data);
            return -1;
        }
        if(kspack_get(sub, "nianling", 8, &value, &vlen, &vtype)){
            fprintf(stderr, "kspack_get error, return![%d]\n", __LINE__);
            kspack_close(sub, 0);
            kspack_close(pack, 0);
            free(data);
            return -1;
        }
        printf("name=nianling, nlen=8, value=%s, vlen=%d, vtype=%d\n", (char*)value, vlen, vtype);
        if(kspack_get(sub, "shengao", 7, &value, &vlen, &vtype)){
            fprintf(stderr, "kspack_get error, return![%d]\n", __LINE__);
            kspack_close(sub, 0);
            kspack_close(pack, 0);
            free(data);
            return -1;
        }
        printf("name=shengao, nlen=8, value=%s, vlen=%d, vtype=%d\n", (char*)value, vlen, vtype);
        kspack_close(sub, 0);
    }
    kspack_close(pack, 0);
    free(data);

    return 0;
}
