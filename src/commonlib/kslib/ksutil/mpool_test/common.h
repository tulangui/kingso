#ifndef _TEST_COMMON_H
#define _TEST_COMMON_H

#include <stdint.h>

#define TASK_COUNT      50000
#define WORKER_COUNT    8

extern int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int kind);

#endif

