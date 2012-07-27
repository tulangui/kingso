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
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] \n", proc_name);
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

    uint64_t call_num = 0;
    int      ret      = 0;

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
    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;

    // parse args

    if (argc < 2) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case '?':
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }


    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);
    const ProfileField * pFieldArr[256] = {0};
    uint32_t field_num = 0;
    uint32_t doc_num  = 0;
    uint32_t valueNum = 0;
    ProfileMultiValueIterator iterator;


    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
        field_num = pManager->getProfileFieldNum();
        for(uint32_t pos = 0; pos < field_num; pos++) {
            pFieldArr[pos] = pManager->getProfileFieldByIndex(pos);
        }

        doc_num = pManager->getProfileDocNum();
        printf("begin!****************************\n");
        gettimeofday(&tv_begin, NULL);


        for(uint32_t docNum = 0; docNum < doc_num; docNum++) {
            for (uint32_t fnum = 0; fnum < field_num; fnum++) {
                if (pFieldArr[fnum]->multiValNum == 1) {
                    switch(pFieldArr[fnum]->type) {
                        case DT_INT8:{
                                           value_int8 = pDocAccessor->getInt8(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_INT16:{
                                           value_int16 = pDocAccessor->getInt16(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_INT32:{
                                           value_int32 = pDocAccessor->getInt32(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_INT64: {
                                           value_int64 = pDocAccessor->getInt64(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;

                        case DT_UINT8:{
                                           value_uint8 = pDocAccessor->getUInt8(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_UINT16:{
                                           value_uint16 = pDocAccessor->getUInt16(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_UINT32:{
                                           value_uint32 = pDocAccessor->getUInt32(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;


                        case DT_UINT64:{
                                           value_uint64 = pDocAccessor->getUInt64(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;

                        case DT_FLOAT: {
                                           value_float = pDocAccessor->getFloat(docNum, pFieldArr[fnum]);
                                           call_num++;
                                       }
                                       break;

                        case DT_DOUBLE: {
                                            value_double = pDocAccessor->getDouble(docNum, pFieldArr[fnum]);
                                            call_num++;
                                        }
                                        break;

                        case DT_STRING: {
                                            value_str = pDocAccessor->getString(docNum, pFieldArr[fnum]);
                                            call_num++;
                                        }
                                        break;

                        default:
                                        printf("invalid type\n");
                    }
                }
                else {
                    pDocAccessor->getRepeatedValue(docNum, pFieldArr[fnum], iterator);
                    valueNum = iterator.getValueNum();
                    for (uint32_t valuePos = 0; valuePos < valueNum; valuePos++) {
                        switch(pFieldArr[fnum]->type) {
                            case DT_INT8:
                                value_int8 = iterator.nextInt8();
                                call_num++;
                                break;

                            case DT_INT16:
                                value_int16 = iterator.nextInt16();
                                call_num++;
                                break;

                            case DT_INT32:
                                value_int32 = iterator.nextInt32();
                                call_num++;
                                break;

                            case DT_INT64:
                                value_int64 = iterator.nextInt64();
                                call_num++;
                                break;

                            case DT_UINT8:
                                value_uint8 = iterator.nextUInt8();
                                call_num++;
                                break;

                            case DT_UINT16:
                                value_uint16 = iterator.nextUInt16();
                                call_num++;
                                break;

                            case DT_UINT32:
                                value_uint32 = iterator.nextUInt32();
                                call_num++;
                                break;

                            case DT_UINT64:
                                value_uint64 = iterator.nextUInt64();
                                call_num++;
                                break;

                            case DT_FLOAT:
                                value_float = iterator.nextFloat();
                                call_num++;
                                break;

                            case DT_DOUBLE:
                                value_double = iterator.nextDouble();
                                call_num++;
                                break;

                            case DT_STRING:
                                value_str = iterator.nextString();
                                call_num++;
                                break;

                            default:
                                printf("invalid type\n");
                        } // switch
                    } // for
                } // else
            } //for
            if (docNum % 1000000 == 0) {
                printf("%u\n", docNum);
            }
        } // for

        gettimeofday(&tv_end, NULL);
        timersub(&tv_end, &tv_begin, &tv_total);

        fprintf(stderr, "time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
        fprintf(stderr, "total call get fuction times: %lu\n", call_num);
        fprintf(stderr, "total doc num: %u\n", doc_num);
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
    return ret;
}

