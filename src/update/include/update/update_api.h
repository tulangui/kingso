#ifndef _UPDATE_API_H_
#define _UPDATE_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum
{
    UP_SUCCESS = 0,
    UP_EARGUMENT = -1,
    UP_ENOMEM = -2,
    UP_EFUNCERR = -3,
    UP_EUNKOWN = -4,
};

typedef struct
{
    char ip[16];
    unsigned short port;
}
up_addr_t;

typedef struct update_api update_api_t;

extern update_api_t* update_api_create(up_addr_t addrs[], int addr_count);

extern void update_api_destroy(update_api_t* api);

extern int update_api_send(update_api_t* api, uint64_t id, void* data, int size, char cmpr, char cmd);

#ifdef __cplusplus
}
#endif

#endif
