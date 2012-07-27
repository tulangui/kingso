#ifndef _UTIL_TIME_HELPER_H_
#define _UTIL_TIME_HELPER_H_
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>

#ifndef __cplusplus
extern "C"{
#endif
extern uint64_t now_time(){
    struct timeval t;
    gettimeofday(&t, NULL);
    return (static_cast<uint64_t>(t.tv_sec) * 1000000ul + 
            static_cast<uint64_t>(t.tv_usec));
}

#ifndef __cplusplus
};
#endif 

#endif //_UTIL_TIME_HELPER_H_
