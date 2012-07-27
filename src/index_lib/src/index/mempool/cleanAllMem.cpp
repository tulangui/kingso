#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <error.h>
#include <errno.h>

void usage(const char * bin)
{
    printf("usage:\n\t%s -p [mem_path] [-f(force clean)]\n", bin);
}

bool destroyShareMem(const char * path, int pos, bool force)
{
    key_t key;
    struct shmid_ds stat;

    key = ftok(path, pos);
    if (key == -1) {
        fprintf(stderr, "call ftok failed:%s,%d, error info:%s\n", path, pos, strerror(errno));
        return false;
    }

    int segId = shmget(key, getpagesize(), S_IRUSR | S_IWUSR);
    if (segId == -1) {
        return false;
    }

    if (!force) {
        if (shmctl(segId, IPC_STAT, &stat) == -1) {
            fprintf(stderr, "get status of key[0x%08x] error info:%s\n", key, strerror(errno));
            return false;
        }

        if (stat.shm_nattch != 0) {
            fprintf(stderr, "key[0x%08x] memory is attached by other process!\n", key);
            return true;
        }
    }

    if (shmctl(segId, IPC_RMID, 0) == -1) {
        fprintf(stderr, "delete memory segment [%d] failed!\n", pos);
        return false;
    }
    return true;
}

int main(int argc, char * argv[])
{
    bool force_clean  = false;
    char * path       = NULL;

    int i;
    while ((i = getopt(argc, argv, "p:f")) != EOF) {
        switch (i) {
            case 'p': 
                path = optarg;
                break;  

            case 'f': 
                force_clean = true;
                break;  

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
        }    
    }    

    if (path == NULL) {
        usage(argv[0]);
        return -1;
    }

    if (force_clean) {
        char input[10];
        printf("Are you sure to clean all shareMem of [%s] ?\n", path);
        do {
            printf("Enter:(yes / no)\n");
            fgets(input, 10, stdin);
            if (strncasecmp(input,"yes", 3) == 0) {
                break;
            }

            if (strncasecmp(input, "no", 2) == 0) {
                return 0;
            }
        }while(1);
    }
    
    int pos = 0;
    while(1)
    {
        if (!destroyShareMem(path, pos, force_clean)) {
            break;
        }
        pos++;
    };
    return 0;
}
