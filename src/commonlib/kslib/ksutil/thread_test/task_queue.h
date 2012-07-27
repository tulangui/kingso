//task_queue.h

#ifndef _TASK_QUEUE_H
#define _TASK_QUEUE_H 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "common.h"

#define PTHREAD_MUTEX_ERRORCHECK PTHREAD_MUTEX_ERRORCHECK_NP

struct task_info
{
    char *arg;
	struct task_info *next;
};

struct task_queue
{
	struct task_info *head;
	struct task_info *tail;
	pthread_mutex_t lock;
	int max_connections;
	int arg_size;
};

#ifdef __cplusplus
extern "C" {
#endif

int init_pthread_lock(pthread_mutex_t *pthread_lock);

int free_queue_init(const int max_connections, const int arg_size);
void free_queue_destroy();

int free_queue_push(struct task_info *pTask);
struct task_info *free_queue_pop();
int free_queue_count();


int task_queue_init(struct task_queue *pQueue);
int task_queue_push(struct task_queue *pQueue, \
		struct task_info *pTask);
struct task_info *task_queue_pop(struct task_queue *pQueue);
int task_queue_count(struct task_queue *pQueue);

#ifdef __cplusplus
}
#endif

#endif

