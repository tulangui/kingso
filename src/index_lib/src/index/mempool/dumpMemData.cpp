#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#include "ShareMemPool.h"

static const char * DUMP_OK_FLAG   = "dump.done";
static const char * DUMP_FAIL_FLAG = "dump.fail";

void usage(const char * bin)
{
    printf("usage:\n\t%s -m [mem_path] -d \n", bin);
}

void touch_flag(const char * path, bool status)
{
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    char flag_path[512];
    if (status) {
        snprintf(flag_path, 512, "%s/%s", path, DUMP_OK_FLAG);
    }
    else {
        snprintf(flag_path, 512, "%s/%s", path, DUMP_FAIL_FLAG);
    }
    int fd = creat(flag_path, mode);
    close(fd);
}

int main(int argc, char * argv[])
{
    char * mem_path = NULL;
    bool   destory_mempool = false;

    int i;
    while ((i = getopt(argc, argv, "m:d")) != EOF) {
        switch (i) {
            case 'm': 
                mem_path = optarg;
                break;  

            case 'd':
                destory_mempool = true;
                break;

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }    
    }    

    if (mem_path == NULL) {
        usage(argv[0]);
        return -1;
    }
	index_mem::ShareMemPool mempool;
	int ret = mempool.loadMem(mem_path);
	if (ret != 0) {
		printf("load mempool from [%s] error!\n", mem_path);
		return -1;
	}

    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;
    gettimeofday(&tv_begin, NULL);

    ret = mempool.dump(mem_path);
    if (ret != 0) {
        printf("dump mempool data to [%s] failed!\n", mem_path);
        mempool.detach();
        touch_flag(mem_path, false);
        return -1;
    }

    gettimeofday(&tv_end, NULL);
    timersub(&tv_end, &tv_begin, &tv_total);
    struct tm * p = NULL;
    p = localtime(&tv_end.tv_sec);

    printf("dump mempool data success at [%04d-%02d-%02d %02d:%02d:%02d]! cost: %ld.%06ld seconds\n",  (1900 + p->tm_year), (1 + p->tm_mon), p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv_total.tv_sec, tv_total.tv_usec);
    touch_flag(mem_path, true);

    if (destory_mempool) {
        mempool.destroy();
    }
    else {
        mempool.detach();
    }
    return 0;
}
