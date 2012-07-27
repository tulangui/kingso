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
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid avg] -f [field file] \n", proc_name);
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
    char field_name[100];

    uint32_t doc_num  = 0;
    uint32_t doc_count  = 0;
    int8_t   value_int8 = 0;
    int16_t  value_int16 = 0;
    int32_t  value_int32 = 0;
    int64_t  value_int64 = 0;
    uint8_t  value_uint8 = 0;
    uint16_t value_uint16 = 0;
    uint32_t value_uint32 = 0;
    uint64_t value_uint64 = 0;
    float    value_float = 0;
    double   value_double = 0;
    const char* value_str = 0;
    int      ret      = 0;
    register uint32_t value_num = 0;
    register uint32_t call_num = 0;

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
                snprintf(field_name, 100, "%s", optarg);
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

    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        doc_count = pManager->getProfileDocNum();
        for (uint32_t pos=0; pos< doc_num; ++pos) {
            targetDocs[pos] = rand()%doc_count;
        }   

        ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
        pField = pDocAccessor->getProfileField(field_name);
        if (pField == NULL) {
            printf("%s field not exist!\n", field_name);
            break;
        }

        printf("begin!****************************\n");
        gettimeofday(&tv_begin, NULL);


        switch(pField->type) {
            case DT_INT8:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int8 = pDocAccessor->getRepeatedInt8(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%d\n", value_int8);
                }
                break;

            case DT_INT16:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int16 = pDocAccessor->getRepeatedInt16(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%d\n", value_int16);
                }
                break;

            case DT_INT32:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int32 = pDocAccessor->getRepeatedInt32(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%d\n", value_int32);
                }
                break;

            case DT_INT64:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int64 = pDocAccessor->getRepeatedInt64(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%ld\n", value_int64);
                }
                break;

            case DT_UINT8:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint8 = pDocAccessor->getRepeatedUInt8(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint8);
                }
                break;

            case DT_UINT16:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint16 = pDocAccessor->getRepeatedUInt16(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint16);
                }
                break;

            case DT_UINT32:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint32 = pDocAccessor->getRepeatedUInt32(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint32);
                }
                break;

            case DT_UINT64:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint64 = pDocAccessor->getRepeatedUInt64(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%lu\n", value_uint64);
                }
                break;

            case DT_FLOAT:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_float = pDocAccessor->getRepeatedFloat(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%f\n", value_float);
                }
                break;

            case DT_DOUBLE:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_double = pDocAccessor->getRepeatedDouble(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%lf\n", value_double);
                }
                break;

            case DT_STRING:
                for(uint32_t pos = 0; pos < doc_num; ++pos) {
                    value_num = pDocAccessor->getValueNum(targetDocs[pos], pField);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_str = pDocAccessor->getRepeatedString(targetDocs[pos], pField, fpos);
                        ++call_num;
                    }
                    //printf("%s\n", value_str);
                }
                break;

            default:
                printf("invalid type!\n");
        }

        gettimeofday(&tv_end, NULL);
        timersub(&tv_end, &tv_begin, &tv_total);
        fprintf(stderr, "time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
        fprintf(stderr, "total call times: %u\n", call_num);
    }
    while(0);

    printf("int8:%d\n", value_int8);
    printf("int16:%d\n", value_int16);
    printf("int32:%d\n", value_int32);
    printf("int64:%ld\n", value_int64);
    printf("uint8:%u\n", value_uint8);
    printf("uint16:%u\n", value_uint16);
    printf("uint32:%u\n", value_uint32);
    printf("uint64:%lu\n", value_uint64);
    printf("float:%f\n", value_float);
    printf("double:%lf\n", value_double);
    printf("string:%s\n", value_str);

    ProfileManager::freeInstance();
    free(targetDocs);
    return ret;
}

