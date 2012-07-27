#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/LatLngProcessor.h"

using namespace index_lib;

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid] -l [lat] -n [lng] \n", proc_name);
}

int main(int argc, char* argv[])
{
    int      ret        = 0;

    char     *path      = NULL;
    uint32_t doc_id     = 0;
    uint32_t doc_count  = 0;
    float    lat        = 0.0f;
    float    lng        = 0.0f;

    // parse args
    if (argc < 9) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:l:n:"));)
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
                lat = strtof(optarg, NULL);
                break;

            case 'n':
                lng = strtof(optarg, NULL);
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
        pField = pDocAccessor->getProfileField("latlng");
        if (pField == NULL) {
            printf("latlng field not exist!\n");
            break;
        }

        if (pDocAccessor->getFieldFlag(pField) != F_LATLNG_DIST) {
            printf("field not LATLNG flag!\n");
            break;
        }

        if (pField->type != DT_UINT64) {
            printf("latlng field wrong type!\n");
            break;
        }

        LatLngProcessor process(pField);

        process.printLatLng(doc_id);
        float dist = process.getMinDistance(lat, lng, doc_id);
        printf("doc_id [%u] minDistance [%f]\n", doc_id, dist);
    }
    while(0);

    ProfileManager::freeInstance();
    LatLngManager::freeInstance();
    return ret;
}

