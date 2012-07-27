#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include "index_lib/ProfileStruct.h"
#include "index_lib/ProfileEncodeFile.h"

using namespace index_lib;

enum FileType{
    GROUP,
    ENCODE,
    BITRECORD,
    HASHTABLE
};

void show_usage(const char* proc_name)
{
    fprintf(stderr, "Usage:\n\t%s [-g/-e/-h] [file_name]\n", proc_name);
}

void print_group_desc(FILE *fd)
{
    int      rnum      = 0;
    uint32_t u32_value = 0;
    uint32_t versions[3];

    // read version information
    rnum = fread(versions, sizeof(uint32_t), 3, fd);
    if (rnum != 3) {
        printf("read version information in profile description failed!\n");
        return;
    }
    printf("Data version:%u.%u.%u\n", versions[0], versions[1], versions[2]);

    // read doc number
    rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
    if (rnum != 1) {
        printf("read docNum in profile description failed!\n");
        return;
    }
    printf("docNum:\t%u\n", u32_value);
    printf("****************************************\n");

    // read group number
    rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
    if (rnum != 1) {
        printf("read groupNum in profile description failed!\n");
        return;
    }
    printf("groupNum:\t%u\n", u32_value);

    uint32_t groupNum = u32_value;
    // read each group size
    for(uint32_t pos = 0; pos < groupNum; pos++) {
        rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
        if (rnum != 1) {
            printf("group[%u] unitsize read failed!\n", pos);
            return;
        }
        printf("group[%u] unitSize:\t%u\n", pos, u32_value);
    }
    printf("****************************************\n");

    // read field number
    rnum = fread(&u32_value, sizeof(uint32_t), 1, fd);
    if (rnum != 1) {
        TERR("read field number failed!\n");
        return;
    }
    printf("fieldNum:\t%u\n", u32_value);

    uint32_t fieldNum = u32_value;

    ProfileField        field_struct;
    BitRecordDescriptor bit_struct;
    // read each field description information
    for(uint32_t pos = 0; pos < fieldNum; pos++) {
        // read ProfileField structure
        rnum = fread(&field_struct, sizeof(ProfileField), 1, fd);
        if (rnum != 1) {
            printf("read profile field [pos: %u] information failed!\n", pos);
            return;
        }

        printf("*************************************\n");
        printf("fieldPos:\t%u\n",  pos);
        printf("fieldName:\t%s\n", field_struct.name);
        printf("fieldType:\t");
        switch(field_struct.type) {
            case DT_UINT8:
                printf("UINT8");
                break;

            case DT_INT8:
                printf("INT8");
                break;

            case DT_INT16:
                printf("INT16");
                break;

            case DT_UINT16:
                printf("UINT16");
                break;

            case DT_INT32:
                printf("INT32");
                break;

            case DT_UINT32:
                printf("UINT32");
                break;

            case DT_INT64:
                printf("INT64");
                break;

            case DT_UINT64:
                printf("UINT64");
                break;

            case DT_FLOAT:
                printf("FLOAT");
                break;

            case DT_DOUBLE:
                printf("DOUBLE");
                break;

            case DT_STRING:
                printf("STRING");
                break;

            default:
                printf("UNKNOWN");

        }
        printf("\n");
        printf("fieldFlag:\t");
        if (field_struct.flag == F_PROVCITY) {
            printf("F_PROVCITY\n");
        }
        else if (field_struct.flag == F_FILTER_BIT) {
            printf("F_FILTER_BIT\n");
        }
        else if (field_struct.flag == F_REALPOSTFEE) {
            printf("F_REALPOSTFEE\n");
        }
        else if (field_struct.flag == F_PROMOTION) {
            printf("F_PROMOTION\n");
        }
        else if (field_struct.flag == F_TIME) {
            printf("F_TIME\n");
        }
        else {
            printf("F_NO_FLAG\n");
        }
        printf("multiValNum:\t%u\n", field_struct.multiValNum);
        printf("groupId:\t%u\n",     field_struct.groupID);
        printf("unitLen:\t%u\n",     field_struct.unitLen);
        printf("offset:\t%u\n",      field_struct.offset);
        printf("isEncoded:\t%s\n",   (field_struct.isEncoded)?"true":"false");
        printf("isBitField:\t%s\n",  (field_struct.isBitRecord)?"true":"false");
        printf("isCompress:\t%s\n",  (field_struct.isCompress)?"true":"false");
        printf("storeType:\t");
        if (field_struct.storeType == MT_ONLY) {
            printf("MT_ONLY\n");
        }
        else if (field_struct.storeType == MT_BIT) {
            printf("MT_BIT\n");
        }
        else if (field_struct.storeType == MT_ENCODE) {
            printf("MT_ENCODE\n");
        }
        else if (field_struct.storeType == MT_BIT_ENCODE) {
            printf("MT_BIT_ENCODE\n");
        }

        if (field_struct.multiValNum == 1) {
            printf("EmptyValue:\t");
            switch(field_struct.type) {
                case DT_UINT8:
                    printf("%u\n", field_struct.defaultEmpty.EV_UINT8);
                    break;

                case DT_INT8:
                    printf("%d\n", field_struct.defaultEmpty.EV_INT8);
                    break;

                case DT_INT16:
                    printf("%d\n", field_struct.defaultEmpty.EV_INT16);
                    break;

                case DT_UINT16:
                    printf("%u\n", field_struct.defaultEmpty.EV_UINT16);
                    break;

                case DT_INT32:
                    printf("%d\n", field_struct.defaultEmpty.EV_INT32);
                    break;

                case DT_UINT32:
                    printf("%u\n", field_struct.defaultEmpty.EV_UINT32);
                    break;

                case DT_INT64:
                    printf("%ld\n", field_struct.defaultEmpty.EV_INT64);
                    break;

                case DT_UINT64:
                    printf("%lu\n", field_struct.defaultEmpty.EV_UINT64);
                    break;

                case DT_FLOAT:
                    printf("%f\n", field_struct.defaultEmpty.EV_FLOAT);
                    break;

                case DT_DOUBLE:
                    printf("%lf\n", field_struct.defaultEmpty.EV_DOUBLE);
                    break;

                case DT_STRING:
                    printf("NULL\n");
                    break;

                default:
                    printf("UNKNOWN");

            }
        }

        printf("------------------------------------\n");
        // read BitRecordDescriptor structure if needed
        if (field_struct.isBitRecord) {
            rnum = fread(&bit_struct, sizeof(BitRecordDescriptor), 1, fd);
            if (rnum != 1) {
                printf("read bitRecord information failed!\n");
                return;
            }
            printf("bitLen:\t%u\n",       bit_struct.field_len);
            printf("u32_pos:\t%u\n",      bit_struct.u32_offset);
            printf("bits_move:\t%u\n",    bit_struct.bits_move);
            printf("read_mask:\t0x%x\n",  bit_struct.read_mask);
            printf("write_mask:\t0x%x\n", bit_struct.write_mask);
        }
    }
}

void print_encode_desc(FILE *fd)
{
    encode_desc_cnt_info  cnt_desc;
    encode_desc_ext_info  ext_desc;
    encode_desc_meta_info meta_desc;
    size_t rnum = fread(&cnt_desc, sizeof(encode_desc_cnt_info), 1, fd);
    if (rnum != 1) {
        fprintf(stderr, "read encode file description info error!\n");
        return;
    }
    rnum = fread(&ext_desc, sizeof(encode_desc_ext_info), 1, fd);
    if (rnum != 1) {
        fprintf(stderr, "read encode file description info error!\n");
        return;
    }
    rnum = fread(&meta_desc, sizeof(encode_desc_meta_info), 1, fd);
    if (rnum != 1) {
        fprintf(stderr, "read encode file description info error!\n");
        return;
    }

    printf("DataType:\t");
    switch(meta_desc.type) {
        case DT_UINT8:
            printf("UINT8");
            break;

        case DT_INT8:
            printf("INT8");
            break;

        case DT_INT16:
            printf("INT16");
            break;

        case DT_UINT16:
            printf("UINT16");
            break;

        case DT_INT32:
            printf("INT32");
            break;

        case DT_UINT32:
            printf("UINT32");
            break;

        case DT_INT64:
            printf("INT64");
            break;

        case DT_UINT64:
            printf("UINT64");
            break;

        case DT_FLOAT:
            printf("FLOAT");
            break;

        case DT_DOUBLE:
            printf("DOUBLE");
            break;

        case DT_STRING:
            printf("STRING");
            break;

        default:
            printf("UNKNOWN");

    }

    printf("\nEncodeNum:\t");
    printf("%u", cnt_desc.cur_encode_value);

    printf("\nContentSize:\t");
    printf("%lu", cnt_desc.cur_content_size);

    printf("\nUnitSize:\t");
    printf("%lu", meta_desc.unit_size);
    printf("\nCompress:\t");
    printf("%s", meta_desc.varintEncode?"true":"false");
    printf("\nOverlap:\t");
    printf("%s\n", meta_desc.update_overlap?"true":"false");
}

void print_hashtable_desc(FILE *fd)
{
}

int main(int argc, char* argv[])
{
    char     *file_name = NULL;
    FileType ftype      = GROUP;
    FILE     *fd        = NULL;

    if (argc != 3) {
        show_usage(basename(argv[0]));
        return -1;
    }

    for(int ch; -1 != (ch = getopt(argc, argv, "g:e:h"));)
    {
        switch(ch)
        {
            case 'g':
            file_name = optarg;
            ftype     = GROUP;
            break;
            
            case 'e':
            file_name = optarg;
            ftype     = ENCODE;
            break;

            case 'h':
            file_name = optarg;
            ftype     = HASHTABLE;
            break;
            
            default:
                show_usage(basename(argv[0]));
                return -1;
        }
    }
    
    fd = fopen(file_name, "r");
    if (fd == NULL) {
        fprintf(stderr, "open file [%s] error!\n", file_name);
        return -1;
    }
    
    printf("\033[32m*******************************************\n\033[0m");
    switch(ftype) {
        case GROUP:
            print_group_desc(fd);
            break;

        case ENCODE:
            print_encode_desc(fd);
            break;

        case HASHTABLE:
            break;

        default:
            fclose(fd);
            return -1;
    }
    printf("\033[32m*******************************************\n\033[0m");
    
    fclose(fd);
    return 0;
}


