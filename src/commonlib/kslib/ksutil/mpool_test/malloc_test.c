#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include "common.h"

int worker_count;

static char cmd[256];
pthread_mutex_t worker_lock;

static void *worker_thread(void* arg);

int main(int argc, char **argv)
{
    int i;
    int result;
    pthread_t tid;

	if ((result=pthread_mutex_init(&worker_lock, NULL)) != 0) {
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"init_pthread_lock fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
    }

    snprintf(cmd, sizeof(cmd), "ps auxww | grep %s | grep -v grep", argv[0]);

    worker_count = WORKER_COUNT;
    for (i=0; i<WORKER_COUNT; i++) {
        if ((result=pthread_create(&tid, NULL, worker_thread, 
                        NULL)) != 0) {
            fprintf(stderr, "file: "__FILE__", line: %d, " \
                    "pthread_create fail, errno: %d, error info: %s", \
                    __LINE__, result, strerror(result));
            return result;
        }
    }

    pthread_join(tid, NULL);

    while (worker_count > 0) {
        usleep(100);
    }

    //system(cmd);

    return 0;
}

static void *worker_thread(void* arg)
{
    int i;
    int k;
    char *p;

    for(i=1; i<=TASK_COUNT; i++) {
        for(k=1024; k<=4 * 1024; k++) {
            p = (char *)malloc(k * 10);
            assert(p != NULL);

            free(p);
        }
    }

    pthread_mutex_lock(&worker_lock);
    worker_count--;

    //system(cmd);

    pthread_mutex_unlock(&worker_lock);

    return NULL;
}

