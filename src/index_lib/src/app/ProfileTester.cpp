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
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [docid avg] -q [query num] -f [field file] \n", proc_name);
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
    char *field_path = NULL;

    uint32_t doc_avg  = 0;
    uint32_t line_num = 0;
    uint32_t pro_num  = 0;
    uint64_t call_num = 0;
    uint32_t doc_num  = 0;
    int      ret      = 0;

    int8_t   value_int8;
    int16_t  value_int16;
    int32_t  value_int32;
    int64_t  value_int64;
    uint8_t  value_uint8;
    uint16_t value_uint16;
    uint32_t value_uint32;
    uint64_t value_uint64;
    float    value_float;
    double   value_double;
    const char* value_str;

    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;
    srand(time(NULL));
    // parse args

    if (argc < 8) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:q:f:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
                break;
            
            case 'd':
                doc_avg = strtoul(optarg, NULL, 10);
                break;

            case 'q':
                line_num = strtoul(optarg, NULL, 10);
                break;

            case 'f':
                field_path = optarg;
                break;

            case '?':
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }

    uint32_t *targetDocs = NULL;
    targetDocs = (uint32_t*)malloc(sizeof(uint32_t) * line_num * doc_avg);

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);
    const ProfileField * pFieldArr[256] = {0};
    uint32_t field_num = 0;
    uint32_t valueNum = 0;

    FILE *field_fd = NULL;
    field_fd = fopen(field_path, "r");

    char buf[2048];
    char field_name[100];

    do {
        if (field_fd == NULL) {
            fprintf(stderr, "open data file failed!\n");
            break;
        }

        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        doc_num = pManager->getProfileDocNum();

        for (uint64_t pos=0; pos< (uint64_t)line_num * doc_avg; ++pos) {
            targetDocs[pos] = rand()%doc_num;
        }   

        ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();
        printf("begin!****************************\n");
        gettimeofday(&tv_begin, NULL);

        register uint64_t doc_pos = 0;
        uint64_t          doc_start = 0;
        ProfileMultiValueIterator iterator;

        // read doc information
        while(fgets(buf, 2048, field_fd) != NULL && pro_num < line_num) {
            process_str(buf);
            field_num = 0;

            const char *begin = buf;
            const char *end   = NULL;
            int         len   = 0; 

            while(*begin == '\0' || (end = strchr(begin, ' ')) != NULL) {
                len = end - begin;
                snprintf(field_name, (len + 1), "%s", begin);
                if ((pFieldArr[field_num] = pDocAccessor->getProfileField(field_name)) != NULL) {
                    ++field_num;
                }
                begin = end + 1; 
            }    

            if (*begin != '\0') {
                if ((pFieldArr[field_num] = pDocAccessor->getProfileField(begin)) != NULL) {
                    ++field_num;
                }
            }

            doc_start = (uint64_t)pro_num * doc_avg;
            for (uint32_t docNum = 0; docNum < doc_avg; ++docNum) {
                doc_pos = doc_start + docNum;
                for (uint32_t fnum = 0; fnum < field_num; ++fnum) {
                    if (pFieldArr[fnum]->multiValNum == 1) {
                        switch(pFieldArr[fnum]->type) {
                            case DT_INT8:  
                                value_int8 = pDocAccessor->getInt8(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;
                            case DT_INT16:
                                value_int16 = pDocAccessor->getInt16(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;
                            case DT_INT32:
                                value_int32 = pDocAccessor->getInt32(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_INT64: 
                                value_int64 = pDocAccessor->getInt64(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_UINT8:
                                value_uint8 = pDocAccessor->getUInt8(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_UINT16:
                                value_uint16 = pDocAccessor->getUInt16(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_UINT32:
                                value_uint32 = pDocAccessor->getUInt32(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_UINT64:
                                value_uint64 = pDocAccessor->getUInt64(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_FLOAT: 
                                value_float = pDocAccessor->getFloat(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_DOUBLE: 
                                value_double = pDocAccessor->getDouble(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            case DT_STRING: 
                                value_str =  pDocAccessor->getString(targetDocs[doc_pos], pFieldArr[fnum]);
                                call_num++;
                                break;

                            default:
                                printf("invalid type\n");
                        }
                    }
                    else {
                        pDocAccessor->getRepeatedValue(targetDocs[doc_pos], pFieldArr[fnum], iterator);
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
            } // for
            pro_num++;
        } // while

        gettimeofday(&tv_end, NULL);
        timersub(&tv_end, &tv_begin, &tv_total);

        fprintf(stderr, "time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
        fprintf(stderr, "total call get fuction times: %lu\n", call_num);
        fprintf(stderr, "total process query: %u\n", line_num);
    }
    while(0);

    ProfileManager::freeInstance();
    free(targetDocs);
    fclose(field_fd);
    return ret;
}

