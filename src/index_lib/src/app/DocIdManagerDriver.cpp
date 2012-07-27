#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"

using namespace index_lib;

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] -d [dict_path] [-h] \n", proc_name);
}

void  process_str(char *str)
{
    int len = strlen(str);
    while (len != 0 && (str[len - 1] == '\r' || str[len - 1] == '\n')) {
        len--;
    }
    str[len] = '\0';
}

void  print_field_info(uint32_t docId, const ProfileField *pField)
{
    if (pField == NULL) {
        printf("Field not exist!\n");
        return;
    }

    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();

    printf("FieldName:\t%s\n", pField->name);
    printf("FieldType:\t");
    switch(pField->type) {
        case DT_UINT8:
            printf("UINT8\n");
            break;

        case DT_INT8:
            printf("INT8\n");
            break;

        case DT_INT16:
            printf("INT16\n");
            break;

        case DT_UINT16:
            printf("UINT16\n");
            break;

        case DT_INT32:
            printf("INT32\n");
            break;

        case DT_UINT32:
            printf("UINT32\n");
            break;

        case DT_INT64:
            printf("INT64\n");
            break;

        case DT_UINT64:
            printf("UINT64\n");
            break;

        case DT_FLOAT:
            printf("FLOAT\n");
            break;

        case DT_DOUBLE:
            printf("DOUBLE\n");
            break;

        case DT_STRING:
            printf("STRING\n");
            break;

        case DT_KV32:
            printf("KV32\n");
            break;

        default:
            printf("UNKNOWN\n");
    }
    printf("FieldFlag:\t");
    switch(pField->flag) {
        case F_NO_FLAG:
            printf("NO_FLAG\n");
            break;

        case F_FILTER_BIT:
            printf("FILTER_BIT\n");
            break;

        case F_PROVCITY:
            printf("PROVCITY\n");
            break;

        case F_REALPOSTFEE:
            printf("REALPOSTFEE\n");
            break;

        case F_PROMOTION:
            printf("PROMOTION\n");
            break;

        case F_TIME:
            printf("TIME\n");
            break;

        case F_LATLNG_DIST:
            printf("LATLNG\n");
            break;

        default:
            printf("UNKNOWN\n");
    }
    uint32_t valueNum = 0;
    if (pField->multiValNum == 1) {
        printf("FieldNum :\t1\n");
        printf("FieldValue:\t");

        switch(pField->type) {
            case DT_INT8:
            case DT_INT16:
            case DT_INT32:
            case DT_INT64:
                printf("%ld\n", pDocAccessor->getInt(docId, pField));
                break;

            case DT_UINT8:
            case DT_UINT16:
            case DT_UINT32:
            case DT_UINT64:
                printf("%lu\n", pDocAccessor->getUInt(docId, pField));
                break;

            case DT_FLOAT:
                printf("%f\n", pDocAccessor->getFloat(docId, pField));
                break;

            case DT_DOUBLE:
                printf("%lf\n", pDocAccessor->getDouble(docId, pField));
                break;

            case DT_STRING:
                printf("%s\n", pDocAccessor->getString(docId, pField));
                break;

            case DT_KV32: {
                              KV32 kv = pDocAccessor->getKV32(docId, pField);
                              printf("%d:%d\n", kv.key, kv.value);
                          }
                          break;

            case DT_UNSUPPORT:
            default:
                          printf("invalid type\n");
        }
    }
    else {
        ProfileMultiValueIterator iterator;
        pDocAccessor->getRepeatedValue(docId, pField, iterator);
        valueNum = iterator.getValueNum();
        printf("FieldNum :\t%u\n", valueNum);

        for (uint32_t valuePos = 0; valuePos < valueNum; valuePos++) {
            printf("FieldVal_%u:\t", valuePos);
            switch(pField->type) {
                case DT_INT8:
                    printf("%d\n", iterator.nextInt8());
                    break;
                case DT_INT16:
                    printf("%d\n", iterator.nextInt16());
                    break;
                case DT_INT32:
                    printf("%d\n", iterator.nextInt32());
                    break;

                case DT_INT64:
                    printf("%ld\n", iterator.nextInt64());
                    break;

                case DT_UINT8:
                    printf("%u\n", iterator.nextUInt8());
                    break;

                case DT_UINT16:
                    printf("%u\n", iterator.nextUInt16());
                    break;

                case DT_UINT32:
                    printf("%u\n", iterator.nextUInt32());
                    break;

                case DT_UINT64:
                    printf("%lu\n", iterator.nextUInt64());
                    break;

                case DT_FLOAT:
                    printf("%f\n", iterator.nextFloat());
                    break;

                case DT_DOUBLE:
                    printf("%lf\n", iterator.nextDouble());
                    break;

                case DT_STRING:
                    printf("%s\n", iterator.nextString());
                    break;

                case DT_KV32:
                    {
                        KV32 kv = iterator.nextKV32();
                        printf("%d:%d %ld\n", kv.key, kv.value, ConvFromKV32(kv));
                    }
                    break;
                default:
                    printf("invalid type\n");
            }
        }
    }

    if (pField->flag == F_TIME) {
        int32_t validTime = 0;
        int32_t validPos  = 0;
        int32_t curTime = time(NULL);
        validTime = pDocAccessor->getValidEndTime(docId, pField, curTime, validPos);
        printf("curTime:\t%d\n", curTime);
        printf("validEnds(pos:%d):\t%d\n", validPos, validTime);
    }
}

void  print_doc_info(uint32_t docId, const char* field_name)
{
    printf("doc id is %u\n", docId);
    printf("----------------------------------------------\n");

    ProfileManager     *pManager     = ProfileManager::getInstance();
    ProfileDocAccessor *pDocAccessor = ProfileManager::getDocAccessor();

    const ProfileField *pField       = NULL;
    if (field_name[0] == '\0' || field_name[0] == '\n' || field_name[0] == '\r') {
        uint32_t fieldNum = pManager->getProfileFieldNum();
        for (uint32_t pos = 0; pos < fieldNum; pos++) {
            printf("**** print Field [index:%u] information ****\n", pos);
            pField = pManager->getProfileFieldByIndex(pos);
            print_field_info(docId, pField);
        }
    }
    else {
        pField = pDocAccessor->getProfileField(field_name);
        print_field_info(docId, pField);
    }
}

int main(int argc, char* argv[])
{
    alog::Configurator::configureRootLogger();

    char * profile_path = NULL;
    char * dict_path    = NULL;
    int    ret  = 0;

    // parse args
    for(int ch; -1 != (ch = getopt(argc, argv, "p:d:"));)
    {
        switch(ch)
        {
            case 'p':
                profile_path = optarg;
            break;
            
            case 'd':
                dict_path = optarg;
            break;

            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }

    if (profile_path == NULL || dict_path == NULL) {
        show_usage(basename(argv[0]));
        return -1;
    }

    ProfileManager *pManager = ProfileManager::getInstance();
    DocIdManager   *pDocMgr  = DocIdManager::getInstance();

    pManager->setProfilePath(profile_path);
    pManager->setDocIdDictPath(dict_path);

    int32_t docId;
    char    field_name[PATH_MAX] = {0};

    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        if (!pDocMgr->load(dict_path)) {
            printf("Load docid dict and delMap failed!\n");
            break;
        }

        pManager->printProfileVersion();
        printf("profile doc num:%u\n", pManager->getProfileDocNum());
        printf("docid cnt:%u, \tdoc cnt:%u, \tdel doc cnt:%u\n",
                pDocMgr->getDocIdCount(), pDocMgr->getDocCount(), pDocMgr->getDelCount());

        int targetFunc = 0;
        do {
            printf("Input target function (0 for profile, 1 for docIdManager) :");
            fgets(field_name, PATH_MAX, stdin);
            targetFunc = strtol(field_name, NULL, 10);
            printf("targetFunc: %d", targetFunc);
            if (targetFunc == 0 || targetFunc == 1) {
                break;
            }
        }while(1);

        if (targetFunc == 0) {
            // read doc information
            do {
                printf("Input DocId to read (-1 to exit):");
                fgets(field_name, PATH_MAX, stdin);
                docId = strtol(field_name, NULL, 10);
                if (docId <= -1) {
                    break;
                }

                printf("Input target field name (direct \"Enter\" to print all information):");
                fgets(field_name, PATH_MAX, stdin);
                process_str(field_name);
                printf("\033[32m*******************************************\n\033[0m");
                print_doc_info((uint32_t)docId, field_name);
                printf("\033[32m*******************************************\n\033[0m");

            }while(1);
        }
        else {
            do {
                printf("\033[32m*******************************************\n\033[0m");
                printf("1. docId check in delmap\n");
                printf("2. get docid by nid\n");
                printf("3. list all deleted docid\n");
                printf("4. exit\n");
                printf("\033[32m*******************************************\n\033[0m");
                printf("Input your choice:");
                fgets(field_name, PATH_MAX, stdin);
                int ch = strtol(field_name, NULL, 10);
                if (ch == 1) {
                    printf("Input docid:");
                    fgets(field_name, PATH_MAX, stdin);
                    docId = strtoul(field_name, NULL, 10);
                    if (pDocMgr->isDocIdDel(docId)) {
                        printf("%u is Deleted!\n", docId);
                    }
                    else {
                        printf("%u is ok!\n", docId);
                    }
                }

                if (ch == 2) {
                    printf("Input nid:");
                    fgets(field_name, PATH_MAX, stdin);
                    int64_t nid = strtoll(field_name, NULL, 10);
                    printf("%ld docid :%u\n", nid, pDocMgr->getDocId(nid));
                }

                if (ch == 3) {
                    for(uint32_t pos = 0; pos < pDocMgr->getDocIdCount(); pos++) {
                        if (pDocMgr->isDocIdDel(pos)) {
                            printf("del docid:%u\n", pos);
                        }
                    }
                }

                if (ch == 4) {
                    break;
                }
            }while(1);
        }
    }
    while(0);

    ProfileManager::freeInstance();
    DocIdManager::freeInstance();
    return ret;
}

