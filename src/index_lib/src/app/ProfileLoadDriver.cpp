#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "index_lib/ProfileManager.h"
using namespace index_lib;

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s -p [profile_path] [-h] \n", proc_name);
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
                if (valueNum == 0) {
                    printf("NULL\n");
                }
                else {
                    printf("%s\n", pDocAccessor->getString(docId, pField));
                }
                break;

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

    char *path = NULL;
    int   ret  = 0;

    // parse args
    for(int ch; -1 != (ch = getopt(argc, argv, "p:"));)
    {
        switch(ch)
        {
            case 'p':
                path = optarg;
            break;
            
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }

    ProfileManager *pManager = ProfileManager::getInstance();
    pManager->setProfilePath(path);

    int32_t docId;
    char    field_name[PATH_MAX] = {0};

    do {
        // load profile
        if ((ret = pManager->load(false)) != 0) {
            printf("Load profile failed!\n");
            break;
        }

        pManager->printProfileVersion();
        printf("doc count:%u\n", pManager->getProfileDocNum());

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
    while(0);

    ProfileManager::freeInstance();
    return ret;
}

