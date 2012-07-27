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
    fprintf(stderr, "Usage:\n\t%s [srcLat:srcLng] [dstLat:dstLng] \n", proc_name);
}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        show_usage(argv[0]);
        return -1;
    }

    const char * srcLoc = NULL;
    const char * dstLoc = NULL;
    srcLoc = argv[1];
    dstLoc = argv[2];

    float srclat, srclng, dstlat, dstlng = 0.0f;
    char * srcDelim = NULL;
    char * dstDelim = NULL;

    srcDelim = strchr(srcLoc, ':');
    dstDelim = strchr(dstLoc, ':');

    if (srcDelim == NULL || dstDelim == NULL) {
        show_usage(argv[0]);
        return -1;
    }

    srclat = strtof(srcLoc, NULL);
    srclng = strtof(srcDelim + 1, NULL);

    dstlat = strtof(dstLoc, NULL);
    dstlng = strtof(dstDelim + 1, NULL);

    LatLngProcessor obj(NULL);
    printf("distance is %f\n", obj.getDistance(srclat, srclng, dstlat, dstlng));
    LatLngManager::freeInstance();
    return 0;
}

