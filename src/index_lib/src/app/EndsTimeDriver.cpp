#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/EndsTimeProcessor.h"
using namespace index_lib;

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid] \n", proc_name);
}

int main(int argc, char* argv[])
{
    int      ret        = 0;

    char     *path      = NULL;
    uint32_t doc_id     = 0;
    uint32_t doc_count  = 0;

    // parse args
    if (argc < 4) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case 'd':
                doc_id = strtoul(optarg, NULL, 10);
                break;

            case '?':
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);
    const ProfileField * pField = NULL;

    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        doc_count = pManager->getProfileDocNum();
        if (doc_id >= doc_count) {
            printf("target docid not exist!\n");
            break;
        }

        ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
        pField = pDocAccessor->getProfileField("ends");
        if (pField == NULL) {
            printf("ends field not exist!\n");
            break;
        }

        if (pDocAccessor->getFieldFlag(pField) != F_TIME) {
            printf("field not ends flag!\n");
            break;
        }

        if (pField->type != DT_UINT64) {
            printf("ends field wrong type!\n");
            break;
        }

        EndsTimeProcessor process(pField);
        uint32_t curTime = time(NULL);
        uint32_t ends = process.getValidEndTime(doc_id, curTime);
        printf("doc_id [%u] cur_time [%u]:\t ends [%u]\n", doc_id, curTime, ends);
    }
    while(0);

    ProfileManager::freeInstance();
    EndsTimeManager::freeInstance();
    return ret;
}

