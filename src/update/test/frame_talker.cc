#include "update/update_api.h"
#include <stdio.h>
#include <string.h>

#define PRINT(fmt, arg...) fprintf(stderr, fmt"\n", ##arg)

#define UPDATER_MAX 64

int main()
{
    up_addr_t addrs[UPDATER_MAX] = {{"127.0.0.1", 9999},};
    update_api_t* api = update_api_create(addrs, 1);
    if(!api){
        PRINT("update_api_create error.");
        return -1;
    }
#pragma pack(1)
    struct{
        uint64_t id;
        char line[1024];
    } package = {1, {0}};
#pragma pack()
    PRINT("=============================");
    while(fgets(package.line, 1024, stdin)){
        PRINT("input : %s", package.line);
        if(strcmp(package.line, "exit")==0){
            break;
        }
        if(update_api_send(api, package.id, &package, sizeof(uint64_t)+strlen(package.line)+1, 'N')){
            PRINT("update_api_send error.");
            update_api_destroy(api);
            return -1;
        }
        PRINT("id=%llu, string=%s send done.", package.id, package.line);
        package.id ++;
        PRINT("=============================");
    }
    update_api_destroy(api);
    return 0;
}
