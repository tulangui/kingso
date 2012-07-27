#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/RealPostFeeProcessor.h"
using namespace index_lib;

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid] -l [postnum] \n", proc_name);
}

int main(int argc, char* argv[])
{
    int      ret        = 0;

    char     *path      = NULL;
    uint32_t doc_id     = 0;
    uint32_t doc_count  = 0;
    uint32_t usr_loc    = 0;

    // parse args
    if (argc < 6) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:l:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case 'd':
                doc_id = strtoul(optarg, NULL, 10);
                break;

            case 'l':
                usr_loc = strtoul(optarg, NULL, 10);
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
        pField = pDocAccessor->getProfileField("real_post_fee");
        if (pField == NULL) {
            printf("real_post_fee field not exist!\n");
            break;
        }

        if (pDocAccessor->getFieldFlag(pField) != F_REALPOSTFEE) {
            printf("field not realpostfee flag!\n");
            break;
        }

        if (pField->type != DT_UINT64) {
            printf("realpostfee field wrong type!\n");
            break;
        }

        RealPostFeeProcessor process(pField);

        process.printRealPostFee(doc_id);
        float fee = process.getRealPostFee(usr_loc, doc_id);
        printf("doc_id [%u] usr_loc [%u]:\t fee [%f]\n", doc_id, usr_loc, fee);
    }
    while(0);

    ProfileManager::freeInstance();
    RealPostFeeManager::freeInstance();
    return ret;
}

