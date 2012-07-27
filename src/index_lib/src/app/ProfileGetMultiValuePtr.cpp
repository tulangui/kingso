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
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -o [output] -f [field file] \n", proc_name);
}

int main(int argc, char* argv[])
{
    char *path       = NULL;
    char field_name[100];
    char *output    = NULL;

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
    const char* value_str = NULL;

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

    for(int ch; -1 != (ch = getopt(argc, argv, "p:o:f:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case 'o':
                output = optarg;
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

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);
    const ProfileField * pField = NULL;

    ProfileMultiValueIterator iterator;
    FILE * fd = NULL;

    do {
        fd = fopen(output, "w");
        if (fd == NULL) {
            fprintf(stderr, "open %s failed!\n", output);
            break;
        }

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

        printf("begin!****************************\n");
        gettimeofday(&tv_begin, NULL);

        switch(pField->type) {
            case DT_INT8:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int8 = iterator.nextInt8();
                        fprintf(fd, "%d ", value_int8);
                        ++call_num;
                    }
                    //printf("%d\n", value_int8);
                    fprintf(fd, "\n");
                }
                break;

            case DT_INT16:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int16 = iterator.nextInt16();
                        fprintf(fd, "%d ", value_int16);
                        ++call_num;
                    }
                    //printf("%d\n", value_int16);
                    fprintf(fd, "\n");
                }
                break;

            case DT_INT32:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int32 = iterator.nextInt32();
                        fprintf(fd, "%d ", value_int32);
                        ++call_num;
                    }
                    //printf("%d\n", value_int32);
                    fprintf(fd, "\n");
                }
                break;

            case DT_INT64:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_int64 = iterator.nextInt64();
                        fprintf(fd, "%ld ", value_int64);
                        ++call_num;
                    }
                    //printf("%ld\n", value_int64);
                    fprintf(fd, "\n");
                }
                break;

            case DT_UINT8:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint8 = iterator.nextUInt8();
                        fprintf(fd, "%u ", value_uint8);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint8);
                    fprintf(fd, "\n");
                }
                break;

            case DT_UINT16:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint16 = iterator.nextUInt16();
                        fprintf(fd, "%u ", value_uint16);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint16);
                    fprintf(fd, "\n");
                }
                break;

            case DT_UINT32:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint32 = iterator.nextUInt32();
                        fprintf(fd, "%u ", value_uint32);
                        ++call_num;
                    }
                    //printf("%u\n", value_uint32);
                    fprintf(fd, "\n");
                }
                break;

            case DT_UINT64:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_uint64 = iterator.nextUInt64();
                        fprintf(fd, "%lu ", value_uint64);
                        ++call_num;
                    }
                    //printf("%lu\n", value_uint64);
                    fprintf(fd, "\n");
                }
                break;

            case DT_FLOAT:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_float = iterator.nextFloat();
                        fprintf(fd, "%f ", value_float);
                        ++call_num;
                    }
                    //printf("%f\n", value_float);
                    fprintf(fd, "\n");
                }
                break;

            case DT_DOUBLE:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_double = iterator.nextDouble();
                        fprintf(fd, "%lf ", value_double);
                        ++call_num;
                    }
                    //printf("%lf\n", value_double);
                    fprintf(fd, "\n");
                }
                break;

            case DT_STRING:
                for(uint32_t pos = 0; pos < doc_count; ++pos) {
                    pDocAccessor->getRepeatedValue(pos, pField, iterator);
                    value_num = iterator.getValueNum();
                    fprintf(fd, "%u ", value_num);
                    for(uint32_t fpos = 0; fpos < value_num; ++fpos) {
                        value_str = iterator.nextString();
                        fprintf(fd, "%s ", value_str);
                        ++call_num;
                    }
                    //printf("%s\n", value_str);
                    fprintf(fd, "\n");
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

    if (fd != NULL) {
        fclose(fd);
    }

    ProfileManager::freeInstance();
    return ret;
}

