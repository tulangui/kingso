//task_queue.c

#include <errno.h>
#include <pthread.h>
#include "task_queue.h"

static struct task_queue g_free_queue;

static struct task_info *g_mpool = NULL;

int init_pthread_lock(pthread_mutex_t *pthread_lock)
{
	pthread_mutexattr_t mat;
	int result;

	if ((result=pthread_mutexattr_init(&mat)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutexattr_init fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutexattr_settype(&mat, \
			PTHREAD_MUTEX_ERRORCHECK)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutexattr_settype fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutex_init(pthread_lock, &mat)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_init fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}
	if ((result=pthread_mutexattr_destroy(&mat)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call thread_mutexattr_destroy fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}

	return 0;
}

int task_queue_init(struct task_queue *pQueue)
{
	int result;

	if ((result=init_pthread_lock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"init_pthread_lock fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}

	pQueue->head = NULL;
	pQueue->tail = NULL;

	return 0;
}

int free_queue_init(const int max_connections, const int arg_size)
{
	struct task_info *pTask;
	char *p;
	char *pCharEnd;
	int block_size;
	int alloc_size;
	int result;

	if ((result=init_pthread_lock(&(g_free_queue.lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"init_pthread_lock fail, errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}

	block_size = sizeof(struct task_info) + arg_size;
	alloc_size = block_size * max_connections;

	g_mpool = (struct task_info *)malloc(alloc_size);
	if (g_mpool == NULL)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"malloc %d bytes fail, " \
			"errno: %d, error info: %s", \
			__LINE__, alloc_size, errno, strerror(errno));
		return errno != 0 ? errno : ENOMEM;
	}
	memset(g_mpool, 0, alloc_size);

	pCharEnd = ((char *)g_mpool) + alloc_size;
	for (p=(char *)g_mpool; p<pCharEnd; p += block_size)
	{
		pTask = (struct task_info *)p;
		pTask->arg = p + sizeof(struct task_info);
	}

	g_free_queue.tail = (struct task_info *)(pCharEnd - block_size);
	for (p=(char *)g_mpool; p<(char *)g_free_queue.tail; p += block_size)
	{
		pTask = (struct task_info *)p;
		pTask->next = (struct task_info *)(p + block_size);
	}

	g_free_queue.max_connections = max_connections;
	g_free_queue.arg_size = arg_size;
	g_free_queue.head = g_mpool;
	g_free_queue.tail->next = NULL;

	return 0;
}

void free_queue_destroy()
{
	if (g_mpool == NULL)
	{
		return;
	}

	free(g_mpool);
	g_mpool = NULL;

	pthread_mutex_destroy(&(g_free_queue.lock));
}

struct task_info *free_queue_pop()
{
	return task_queue_pop(&g_free_queue);;
}

int free_queue_push(struct task_info *pTask)
{
	int result;

	if ((result=pthread_mutex_lock(&g_free_queue.lock)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
	}

	pTask->next = g_free_queue.head;
	g_free_queue.head = pTask;
	if (g_free_queue.tail == NULL)
	{
		g_free_queue.tail = pTask;
	}

	if ((result=pthread_mutex_unlock(&g_free_queue.lock)) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
	}

	return result;
}

int free_queue_count()
{
	return task_queue_count(&g_free_queue);
}

int task_queue_push(struct task_queue *pQueue, \
		struct task_info *pTask)
{
	int result;

	if ((result=pthread_mutex_lock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return result;
	}

	pTask->next = NULL;
	if (pQueue->tail == NULL)
	{
		pQueue->head = pTask;
	}
	else
	{
		pQueue->tail->next = pTask;
	}
	pQueue->tail = pTask;

	if ((result=pthread_mutex_unlock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
	}

	return 0;
}

struct task_info *task_queue_pop(struct task_queue *pQueue)
{
	struct task_info *pTask;
	int result;

	if ((result=pthread_mutex_lock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return NULL;
	}

	pTask = pQueue->head;
	if (pTask != NULL)
	{
		pQueue->head = pTask->next;
		if (pQueue->head == NULL)
		{
			pQueue->tail = NULL;
		}
	}

	if ((result=pthread_mutex_unlock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
	}

	return pTask;
}

int task_queue_count(struct task_queue *pQueue)
{
	struct task_info *pTask;
	int count;
	int result;

	if ((result=pthread_mutex_lock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_lock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
		return 0;
	}

	count = 0;
	pTask = pQueue->head;
	while (pTask != NULL)
	{
		pTask = pTask->next;
		count++;
	}

	if ((result=pthread_mutex_unlock(&(pQueue->lock))) != 0)
	{
		fprintf(stderr, "file: "__FILE__", line: %d, " \
			"call pthread_mutex_unlock fail, " \
			"errno: %d, error info: %s", \
			__LINE__, result, strerror(result));
	}

	return count;
}

