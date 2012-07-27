#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <stdint.h>

#define MAX_CONNECTIONS  1024
#define ARG_SIZE         1024
#define TASK_COUNT      2000000
#define WORKER_COUNT    8

extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);

#endif

