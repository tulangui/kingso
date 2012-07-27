#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/ProfileManager.h"
using namespace index_lib;


void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid avg] \n", proc_name);
}

void  process_str(char *str)
{
    int len = strlen(str);
    while (len != 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        len--;
    }
    str[len] = '\0';
}

int main(int argc, char* argv[])
{
    char *path       = NULL;
    char field_name[32];

    uint32_t doc_num  = 0;
    uint32_t doc_count = 0;
    int      ret      = 0;

    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;
    srand(time(NULL));
    // parse args

    if (argc < 6) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:f:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case 'd':
                doc_num = strtoul(optarg, NULL, 10);
                break;

            case 'f':
                snprintf(field_name, 32, "%s", optarg);
                break;

            case '?':
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }

    uint32_t *targetDocs = NULL;
    targetDocs = (uint32_t*)malloc(sizeof(uint32_t) * doc_num);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);
    const ProfileField * pField = NULL;
    uint32_t valueNum = 0;

    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        doc_count = pManager->getProfileDocNum();
        ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
        pField = pDocAccessor->getProfileField(field_name);
        if (pField == NULL) {
            printf("%s field not exist!\n", field_name);
            break;
        }

        for (uint32_t pos=0; pos< doc_num; pos++) {
            targetDocs[pos] = rand()%doc_count;
        }   

        printf("begin!****************************\n");
        gettimeofday(&tv_begin, NULL);

        // read doc information
        for (uint32_t pos=0; pos < doc_num; pos++) {
            valueNum = pDocAccessor->getValueNum(targetDocs[pos], pField);
            //printf("valueNum: %u\n", valueNum);
        }

        gettimeofday(&tv_end, NULL);
        timersub(&tv_end, &tv_begin, &tv_total);

        fprintf(stderr, "time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
    }
    while(0);

    printf("valueNum:%u\n", valueNum);
    ProfileManager::freeInstance();
    free(targetDocs);
    return ret;
}

