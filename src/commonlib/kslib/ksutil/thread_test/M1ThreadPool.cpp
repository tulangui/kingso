#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include "common.h"
#include "task_queue.h"
#include "ThreadPool.h"
#include "Thread.h"

using namespace util;

int continue_flag;
int generate_count = 0;
int deal_count = 0;
struct task_queue task_queue;

pthread_mutex_t worker_lock;
pthread_cond_t worker_cond;

ThreadPool *threadPool = NULL;
static void *generater_thread(void* arg);
static void *dispatcher_thread(void* arg);

int main(int argc, char **argv)
{
    int result;
    pthread_t generator_tid;
    pthread_t dispatcher_tid;

    if ((result=free_queue_init(MAX_CONNECTIONS, ARG_SIZE)) != 0) {
        return result;
    }

    if ((result=task_queue_init(&task_queue)) != 0) {
        return result;
    }

	if ((result=init_pthread_lock(&worker_lock)) != 0) {
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"init_pthread_lock fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
    }

    if ((result=pthread_cond_init(&worker_cond, NULL)) != 0) {
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"pthread_cond_init fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
        return result;
    }

    continue_flag = 1;
    if ((result=pthread_create(&generator_tid, NULL, generater_thread, 
                    NULL)) != 0) {
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"pthread_create fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
    }

    threadPool = new ThreadPool(WORKER_COUNT);

    if ((result=pthread_create(&dispatcher_tid, NULL, dispatcher_thread, 
                    NULL)) != 0) {
        fprintf(stderr, "file: "__FILE__", line: %d, " \
                "pthread_create fail, errno: %d, error info: %s", \
                __LINE__, result, strerror(result));
        return result;
    }

    pthread_join(generator_tid, NULL);

    continue_flag = 0;
    pthread_join(dispatcher_tid, NULL);
    printf("dispatcher done, dispatch count: %d\n", generate_count);
    threadPool->join();

    printf("thread done, deal count: %d\n", deal_count);

    return 0;
}

static void *generater_thread(void* arg)
{
    int result;
    struct task_info *pTask;

    generate_count = 0;
    while (generate_count < TASK_COUNT) {
        pTask = free_queue_pop();
        if (pTask == NULL) {
            pthread_cond_signal(&worker_cond);
            usleep(100);
            continue;
        }

        task_queue_push(&task_queue, pTask);
	if (task_queue.head != NULL && task_queue.head == task_queue.tail) {
        if ((result=pthread_cond_signal(&worker_cond)) != 0) {
		    fprintf(stderr, "file: "__FILE__", line: %d, " \
		    	"pthread_cond_signal fail, errno: %d, error info: %s", \
		    	__LINE__, result, strerror(result));
            return NULL;
        }
	}

        generate_count++;
    }

    return NULL;
}

class Worker : public Runnable {
	public:
		Worker(struct task_info *pTask) {
            _task = pTask;
        }
	public:
		int32_t run() {
            deal_count++;
            return 0;
        }

		void free() {
            free_queue_push(_task);
            delete this;
        }
    private:
        struct task_info *_task;
};

static int dispatch(struct task_info *pTask) {
    Thread *thread;
    int result;

    const bool block = true;
    if ((result=threadPool->newThread(&thread, new Worker(pTask), block)) != 0) {
		    fprintf(stderr, "file: "__FILE__", line: %d, " \
		    	"newThread fail, errno: %d, error info: %s", \
		    	__LINE__, result, strerror(result));
    }

    return result;
}

static void *dispatcher_thread(void* arg)
{
    int result;
    struct task_info *pTask;

    while (continue_flag) {
        pTask = task_queue_pop(&task_queue);
        if (pTask != NULL) {
            dispatch(pTask);
            continue;
        }

        if ((result=pthread_mutex_lock(&worker_lock)) != 0) {
		    fprintf(stderr, "file: "__FILE__", line: %d, " \
		    	"pthread_mutex_lock fail, errno: %d, error info: %s", \
		    	__LINE__, result, strerror(result));
            break;
        }

        if ((result=pthread_cond_wait(&worker_cond, &worker_lock)) != 0) {
            pthread_mutex_unlock(&worker_lock);

		    fprintf(stderr, "file: "__FILE__", line: %d, " \
		    	"pthread_cond_wait fail, errno: %d, error info: %s", \
		    	__LINE__, result, strerror(result));
            break;
        }

        pTask = task_queue_pop(&task_queue);

        if ((result=pthread_mutex_unlock(&worker_lock)) != 0) {
		    fprintf(stderr, "file: "__FILE__", line: %d, " \
		    	"pthread_mutex_lock fail, errno: %d, error info: %s", \
		    	__LINE__, result, strerror(result));
            break;
        }

        if (pTask != NULL) {
            dispatch(pTask);
            continue;
        }
    }

    return NULL;
}

