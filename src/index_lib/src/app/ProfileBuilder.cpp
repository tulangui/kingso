#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <error.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <mxml.h>
#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"

/**
 * 读取单个doc属性信息的buffer长度
 */
#define MAX_DOC_LEN  (1024*1024*2)

/** 属性字段类型定义 */
#define TYPE_STR_INT8            "AT_INT8"
#define TYPE_STR_UINT8           "AT_UINT8"
#define TYPE_STR_INT16           "AT_INT16"
#define TYPE_STR_UINT16          "AT_UINT16"
#define TYPE_STR_INT32           "AT_INT32"
#define TYPE_STR_UINT32          "AT_UINT32"
#define TYPE_STR_INT64           "AT_INT64"
#define TYPE_STR_UINT64          "AT_UINT64"
#define TYPE_STR_FLOAT           "AT_FLOAT"
#define TYPE_STR_DOUBLE          "AT_DOUBLE"
#define TYPE_STR_STRING          "AT_STRING"
#define TYPE_STR_KV32            "AT_KV32"

/** 字段FLAG类型定义 */
#define BIT_FILTER_FLAG_STR      "F_FILTER_BIT"
#define PROVCITY_FLAG_STR        "F_PROVCITY"
#define REALPOSTFEE_FLAG_STR     "F_REALPOSTFEE"
#define TIME_FLAG_STR            "F_TIME"
#define PROMOTION_FLAG_STR       "F_PROMOTION"
#define LATLNG_FLAG_STR          "F_LATLNG"
#define NO_FLAG_STR              "F_NO_FLAG"

/**
 * BitRecord字段的相关信息结构
 */
typedef struct _bit_params
{
    uint32_t              field_num;
    char*                 fieldNameArr [MAX_BITFIELD_NUM];
    char*                 fieldAliasArr[MAX_BITFIELD_NUM];
    uint32_t              bitLenArr    [MAX_BITFIELD_NUM];
    PF_DATA_TYPE          typeArr      [MAX_BITFIELD_NUM];
    bool                  encodeFlagArr[MAX_BITFIELD_NUM];
    PF_FIELD_FLAG         fieldFlagArr [MAX_BITFIELD_NUM];
    index_lib::EmptyValue defaultArr   [MAX_BITFIELD_NUM];
}BitRecordParam;


/**
 * package对应的单个字段信息节点
 */
typedef struct _package_node
{
    char     name[MAX_FIELD_NAME_LEN];
    uint32_t groupPos;
}PackageNode;


/**
 * 字段package信息结构体定义
 */
typedef struct _package_info
{
    bool        package;
    PackageNode fieldArr[MAX_PROFILE_FIELD_NUM];
    uint32_t    fieldNum;
    uint32_t    groupNum;
}PackageInfo;

/**
 * 打印使用说明
 */
void usage(char *name)
{
    printf("usage:\n\t%s -c [conf_file] -t [profile.title] -d [profile.dat] [-h]\n", basename(name));
}

/**
 * 将package中的单个group中的字段名信息加入到PackageInfo结构中
 */
int addPackageGroup(const char *fieldStr, PackageInfo &pack)
{
    if (fieldStr == NULL) {
        return KS_EINVAL;
    }

    const char *begin = fieldStr;
    const char *end   = NULL;
    uint32_t    len   = 0;

    while((end = strchr(begin, ' ')) != NULL) {
        len = end - begin;
        if (len >= MAX_FIELD_NAME_LEN) {
            return -1;
        }

        if (len == 0) {
            begin = end + 1;
            continue;
        }

        snprintf(pack.fieldArr[pack.fieldNum].name, len + 1, "%s", begin);
        for(uint32_t pos = 0; pos < pack.fieldNum; pos++) {
            if (strcmp(pack.fieldArr[pos].name, pack.fieldArr[pack.fieldNum].name) == 0) {
                TERR("ProfileBuilder add package group error, duplicate field name [%s] !", pack.fieldArr[pack.fieldNum].name);
                return KS_EINTR;
            }
        }

        pack.fieldArr[pack.fieldNum].groupPos = pack.groupNum;
        pack.fieldNum++;        
        begin = end + 1;
    }

    if (*begin != '\0') {
        snprintf(pack.fieldArr[pack.fieldNum].name, MAX_FIELD_NAME_LEN, "%s", begin);
        for(uint32_t pos = 0; pos < pack.fieldNum; pos++) {
            if (strcmp(pack.fieldArr[pos].name, pack.fieldArr[pack.fieldNum].name) == 0) {
                TERR("ProfileBuilder add package group error, duplicate field name [%s] !", pack.fieldArr[pack.fieldNum].name);
                return KS_EINTR;
            }
        }
        pack.fieldArr[pack.fieldNum].groupPos = pack.groupNum;
        pack.fieldNum++;
    }

    pack.groupNum++;
    return 0;
}

/**
 * 根据字段名获取目标字段的分组信息
 */
uint32_t getFieldGroupPosition(const char *fieldName, PackageInfo &pack)
{
    bool     match    = false;
    uint32_t groupPos = 0;

    for(uint32_t pos = 0; pos < pack.fieldNum; pos++) {
        if (strcmp(fieldName, pack.fieldArr[pos].name) == 0) {
            match    = true;
            groupPos = pack.fieldArr[pos].groupPos;
            break;
        }
    }

    if (!match) {
        if (pack.package) {
            groupPos = pack.groupNum;
        }
        else {
            groupPos = pack.groupNum++;
        }
    }

    return groupPos;
}

/**
 * 获取字符串对应的字段值类型
 */
PF_DATA_TYPE getType(const char* type_str) {
    if (strcmp(type_str, TYPE_STR_INT8) == 0) {
        return DT_INT8;
    }

    if (strcmp(type_str, TYPE_STR_UINT8) == 0) {
        return DT_UINT8;
    }

    if (strcmp(type_str, TYPE_STR_INT16) == 0) {
        return DT_INT16;
    }

    if (strcmp(type_str, TYPE_STR_UINT16) == 0) {
        return DT_UINT16;
    }

    if (strcmp(type_str, TYPE_STR_INT32) == 0) {
        return DT_INT32;
    }

    if (strcmp(type_str, TYPE_STR_UINT32) == 0) {
        return DT_UINT32;
    }

    if (strcmp(type_str, TYPE_STR_INT64) == 0) {
        return DT_INT64;
    }

    if (strcmp(type_str, TYPE_STR_UINT64) == 0) {
        return DT_UINT64;
    }

    if (strcmp(type_str, TYPE_STR_FLOAT) == 0) {
        return DT_FLOAT;
    }

    if (strcmp(type_str, TYPE_STR_DOUBLE) == 0) {
        return DT_DOUBLE;
    }

    if (strcmp(type_str, TYPE_STR_STRING) == 0) {
        return DT_STRING;
    }

    if (strcmp(type_str, TYPE_STR_KV32) == 0) {
        return DT_KV32;
    }

    return DT_UNSUPPORT;
}

/**
 * 根据解析获得的配置文件中default标签内容设定单值字段的空值
 */
bool setBitRecordDefaultValue(PF_DATA_TYPE type, index_lib::EmptyValue &value, const char *str, uint32_t len, bool encode)
{
    memset(&value, 0, sizeof(index_lib::EmptyValue));
    if (str == NULL) {
        switch (type) {
            case DT_INT8   :
                value.EV_INT8 = INVALID_INT8;
                break;

            case DT_INT16  :
                value.EV_INT16 = INVALID_INT16;
                break;

            case DT_INT32  :
                value.EV_INT32 = INVALID_INT32;
                break;

            case DT_INT64  :
                value.EV_INT64 = INVALID_INT64;
                break;

            case DT_UINT8  :
                if (encode) {
                    // UINT类型且编码的字段
                    value.EV_UINT8 = INVALID_UINT8;
                }
                else {
                    // UINT类型且存储原始值的字段
                    value.EV_UINT8 = (uint8_t)MAX_BITFIELD_VALUE(len);
                }
                break;

            case DT_UINT16 :
                if (encode) {
                    value.EV_UINT16 = INVALID_UINT16;
                }
                else {
                    value.EV_UINT16 = (uint16_t)MAX_BITFIELD_VALUE(len);
                }
                break;

            case DT_UINT32 :
                if (encode) {
                    value.EV_UINT32 = INVALID_UINT32;
                }
                else {
                    value.EV_UINT32 = (uint32_t)MAX_BITFIELD_VALUE(len);
                }
                break;

            case DT_UINT64 :
                if (encode) {
                    value.EV_UINT64 = INVALID_UINT64;
                }
                else {
                    value.EV_UINT64 = (uint64_t)MAX_BITFIELD_VALUE(len);
                }
                break;

            case DT_FLOAT  :
                value.EV_FLOAT = INVALID_FLOAT;
                break;

            case DT_DOUBLE :
                value.EV_DOUBLE = INVALID_DOUBLE;
                break;

            case DT_STRING :
                value.EV_STRING = NULL;
                break;

            default:
                printf("unknown type!\n");
                return false;
        }
    }
    else {
        if (strlen(str) == 0) {
            TERR("Empty default attribute value in configure file");
            return false;
        }
        switch (type) {
            case DT_INT8   :
                value.EV_INT8 = (int8_t)strtol(str, NULL, 10);
                break;

            case DT_INT16  :
                value.EV_INT16 = (int16_t)strtol(str, NULL, 10);
                break;

            case DT_INT32  :
                value.EV_INT32 = (int32_t)strtol(str, NULL, 10);
                break;

            case DT_INT64  :
                value.EV_INT64 = (int64_t)strtoll(str, NULL, 10);
                break;

            case DT_UINT8  :
                if (encode) {
                    value.EV_UINT8 = (uint8_t)strtoul(str, NULL, 10);
                }
                else {
                    uint32_t tarValue = (uint32_t)strtoul(str, NULL, 10);
                    if (tarValue > MAX_BITFIELD_VALUE(len)) {
                        TERR("Default attribute value [%s] over limitation indicated by bit_len attribute [%u]", str, len);
                        return false;
                    }
                    value.EV_UINT8 = (uint8_t)tarValue;
                }
                break;

            case DT_UINT16 :
                if (encode) {
                    value.EV_UINT16 = (uint16_t)strtoul(str, NULL, 10);
                }
                else {
                    uint32_t tarValue = (uint32_t)strtoul(str, NULL, 10);
                    if (tarValue > MAX_BITFIELD_VALUE(len)) {
                        TERR("Default attribute value [%s] over limitation indicated by bit_len attribute [%u]", str, len);
                        return false;
                    }
                    value.EV_UINT16 = (uint16_t)tarValue;
                }
                break;

            case DT_UINT32 :
                if (encode) {
                    value.EV_UINT32 = (uint32_t)strtoul(str, NULL, 10);
                }
                else {
                    uint32_t tarValue = (uint32_t)strtoul(str, NULL, 10);
                    if (tarValue > MAX_BITFIELD_VALUE(len)) {
                        TERR("default attribute value [%s] over limitation indicated by bit_len attribute [%u]", str, len);
                        return false;
                    }
                    value.EV_UINT32 = (uint32_t)tarValue;
                }
                break;

            case DT_UINT64 :
                if (encode) {
                    value.EV_UINT64 = (uint64_t)strtoull(str, NULL, 10);
                }
                else {
                    uint32_t tarValue = (uint32_t)strtoul(str, NULL, 10);
                    if (tarValue > MAX_BITFIELD_VALUE(len)) {
                        TERR("default attribute value [%s] over limitation indicated by bit_len attribute [%u]", str, len);
                        return false;
                    }
                    value.EV_UINT64 = (uint64_t)tarValue;
                }
                break;

            case DT_FLOAT  :
                value.EV_FLOAT = (float)strtof(str, NULL);
                break;

            case DT_DOUBLE :
                value.EV_DOUBLE = (double)strtod(str, NULL);
                break;

            case DT_STRING :
                value.EV_STRING = NULL;
                // STRING字段不支持default参数
                TERR("Configure failed: field of DT_STRING do not support default attribute!");
                return false;
                break;

            default:
                printf("unknown type!\n");
                return false;
        }
    }
    return true;
}

/**
 * 根据解析获得的配置文件中default标签内容设定单值字段的空值
 */
bool setDefaultValue(PF_DATA_TYPE type, index_lib::EmptyValue &value, const char *str)
{
    memset(&value, 0, sizeof(index_lib::EmptyValue));
    if (str == NULL) {
        switch (type) {
            case DT_INT8   :
                value.EV_INT8 = INVALID_INT8;
                break;

            case DT_INT16  :
                value.EV_INT16 = INVALID_INT16;
                break;

            case DT_INT32  :
                value.EV_INT32 = INVALID_INT32;
                break;

            case DT_INT64  :
                value.EV_INT64 = INVALID_INT64;
                break;

            case DT_UINT8  :
                value.EV_UINT8 = INVALID_UINT8;
                break;

            case DT_UINT16 :
                value.EV_UINT16 = INVALID_UINT16;
                break;

            case DT_UINT32 :
                value.EV_UINT32 = INVALID_UINT32;
                break;

            case DT_UINT64 :
                value.EV_UINT64 = INVALID_UINT64;
                break;

            case DT_FLOAT  :
                value.EV_FLOAT = INVALID_FLOAT;
                break;

            case DT_DOUBLE :
                value.EV_DOUBLE = INVALID_DOUBLE;
                break;

            case DT_STRING :
                value.EV_STRING = NULL;
                break;

            case DT_KV32:
                value.EV_KV32.key   = INVALID_INT32;
                value.EV_KV32.value = INVALID_INT32;
                break;

            default:
                printf("unknown type!\n");
                return false;
        }
    }
    else {
        if (strlen(str) == 0 || ! isdigit(str[0])) {
            TERR("Invalid default attribute value in configure file");
            return false;
        }
        switch (type) {
            case DT_INT8   :
                value.EV_INT8 = (int8_t)strtol(str, NULL, 10);
                break;

            case DT_INT16  :
                value.EV_INT16 = (int16_t)strtol(str, NULL, 10);
                break;

            case DT_INT32  :
                value.EV_INT32 = (int32_t)strtol(str, NULL, 10);
                break;

            case DT_INT64  :
                value.EV_INT64 = (int64_t)strtoll(str, NULL, 10);
                break;

            case DT_UINT8  :
                value.EV_UINT8 = (uint8_t)strtoul(str, NULL, 10);
                break;

            case DT_UINT16 :
                value.EV_UINT16 = (uint16_t)strtoul(str, NULL, 10);
                break;

            case DT_UINT32 :
                value.EV_UINT32 = (uint32_t)strtoul(str, NULL, 10);
                break;

            case DT_UINT64 :
                value.EV_UINT64 = (uint64_t)strtoull(str, NULL, 10);
                break;

            case DT_FLOAT  :
                value.EV_FLOAT = (float)strtof(str, NULL);
                break;

            case DT_DOUBLE :
                value.EV_DOUBLE = (double)strtod(str, NULL);
                break;

            case DT_STRING :
                value.EV_STRING = NULL;
                // STRING字段不支持default参数
                TERR("Configure failed: field of DT_STRING do not support default attribute!");
                return false;
                break;

            case DT_KV32:
                {
                    char * delim = strchr(str, ':');
                    index_lib::KV32 kv;
                    if (delim == NULL) {
                        kv.key   = INVALID_INT32;
                        kv.value = INVALID_INT32;
                    }
                    else {
                        kv.key   = strtol(str, NULL, 10);
                        kv.value = strtol(delim+1, NULL, 10);
                    }
                    value.EV_KV32 = kv;
                }
                break;

            default:
                printf("unknown type!\n");
                return false;
        }
    }
    return true;
}

/**
 * 加载配置文件，初始化profile路径和相关字段
 */
int load_conf(const char *conf_file) {
    char  full_path[PATH_MAX];
    char  command[1024];
    FILE *conf_fd = NULL;
    int   ret     = 0;
    mxml_node_t *tree = NULL;
    conf_fd = fopen(conf_file, "r");
    do {
        if (conf_fd == NULL) {
            TERR("INDEXLIB: Create profile error, [%s]!", strerror(errno));
            ret = -1;
            break;
        }

        tree = mxmlLoadFile(NULL, conf_fd, MXML_NO_CALLBACK);
        mxml_node_t *profile_node = NULL;
        mxml_node_t *global_node  = NULL;
        mxml_node_t *dict_node    = NULL;

        global_node = mxmlFindElement(tree, tree, "globals", "root", NULL, MXML_DESCEND);
        if (global_node == NULL) {
            TERR("INDEXLIB: Create profile error, no <globals> element with <root> attribute!");
            ret = -1;
            break;
        }
        char *root_path = (char *)mxmlElementGetAttr(global_node, "root");

        dict_node = mxmlFindElement(tree, tree, "dict", "sub_dir", NULL, MXML_DESCEND);
        if (dict_node == NULL) {
            TERR("INDEXLIB: Create profile error, no <dict> elemment with <sub_dir> attribute!");
            ret = -1;
            break;
        }
        char *dict_path = (char *)mxmlElementGetAttr(dict_node, "sub_dir");
        snprintf(full_path, PATH_MAX, "%s/%s", root_path, dict_path);
        snprintf(command, 1024, "rm -rf %s; mkdir -p %s", full_path, full_path);
        index_lib::ProfileManager *pManager = index_lib::ProfileManager::getInstance();
        if (pManager->setDocIdDictPath(full_path)) {
            TERR("INDEXLIB: Create profile error, set docId dict path [%s] failed!", full_path);
            ret = -1;
            break;
        }
        system(command);

        profile_node = mxmlFindElement(tree, tree, "profile", "sub_dir", NULL, MXML_DESCEND);
        if (profile_node == NULL) {
            TERR("INDEXLIB: Create profile error, no <profile> elemment with <sub_dir> attribute!");
            ret = -1;
            break;
        }

        // 读取profile元素的sub_dir属性
        char *profile_path = (char *)mxmlElementGetAttr(profile_node, "sub_dir");
        snprintf(full_path, PATH_MAX, "%s/%s", root_path, profile_path);
        snprintf(command,   1024, "rm -rf %s; mkdir -p %s", full_path, full_path);
        if (pManager->setProfilePath(full_path)) {
            TERR("INDEXLIB: Create profile error, set profile path [%s] failed!", full_path);
            ret = -1;
            break;
        }
        system(command);

        // 读取profile元素的package属性
        PackageInfo pack;
        memset(&pack, 0, sizeof(PackageInfo));
        char *package_str = (char *)mxmlElementGetAttr(profile_node, "package");
        if (package_str == NULL || strcasecmp(package_str, "true") == 0) {
            pack.package = true;
        }
        else {
            pack.package = false;
        }

        // 解析package元素，获取分组配置信息
        mxml_node_t *package_node = NULL;
        package_node = mxmlFindElement(profile_node, tree, "package", NULL, NULL, MXML_DESCEND);
        if (package_node != NULL) {
            mxml_node_t *group_node = NULL;
            for (group_node = mxmlFindElement(package_node, tree, "group", "fields", NULL, MXML_DESCEND_FIRST);
                 group_node != NULL;
                 group_node = mxmlFindElement(group_node, tree, "group", "fields", NULL, MXML_NO_DESCEND))
            {
                char *fieldList = (char *)mxmlElementGetAttr(group_node, "fields");
                if (addPackageGroup(fieldList, pack) < 0) {
                    TERR("INDEXLIB: Create profile error, add package group [%s] failed!", fieldList);
                    ret = -1;
                    break;
                }
            }
        }

        if (ret == -1) {
            break;
        }

        mxml_node_t *bit_node = NULL;
        mxml_node_t *field_node = NULL;
        mxml_node_t *bit_field_node = NULL;
        int bit_record_num   = 0;
        int common_field_num = 0;

        for (bit_node = mxmlFindElement(profile_node, tree, "bit_record", NULL, NULL, MXML_DESCEND);
                bit_node != NULL; bit_node = mxmlFindElement(bit_node, tree, "bit_record", NULL, NULL, MXML_NO_DESCEND))
        {
            BitRecordParam bitParam;
            memset(&bitParam, 0, sizeof(BitRecordParam));
            char * bitRecordName = (char *)mxmlElementGetAttr(bit_node, "name");
            if (bitRecordName == NULL) {
                TERR("<name> attribute of <bit_record> element do not exitst!");
                ret = -1;
                break;
            }

            uint32_t groupPos    = getFieldGroupPosition(bitRecordName, pack);
            for (bit_field_node = mxmlFindElement(bit_node, tree, "field", NULL, NULL, MXML_DESCEND_FIRST);
                    bit_field_node != NULL; bit_field_node = mxmlFindElement(bit_field_node, tree, "field", NULL, NULL, MXML_NO_DESCEND))
            {
                char *name_str = (char*)mxmlElementGetAttr(bit_field_node, "name");
                if (name_str == NULL) {
                    TERR("<name> attribute of bit_record <field> element do not exist!");
                    ret = -1;
                    break;
                }

                if (strlen(name_str) >= MAX_FIELD_NAME_LEN) {
                    TERR("<name> attribute value length over %u !", MAX_FIELD_NAME_LEN);
                    ret = -1;
                    break;
                }

                bitParam.fieldNameArr[bitParam.field_num] = name_str;

                char * alias_str = (char *)mxmlElementGetAttr(bit_field_node, "alias");
                if (alias_str != NULL && strlen(alias_str) >= MAX_FIELD_NAME_LEN) {
                    TERR("<alias> attribute value length over %u !", MAX_FIELD_NAME_LEN);
                    ret = -1;
                    break;
                }
                bitParam.fieldAliasArr[bitParam.field_num] = alias_str;

                char *len_str = (char*)mxmlElementGetAttr(bit_field_node, "bit_len");
                if (len_str == NULL) {
                    TERR("<bit_len> attribute of bit_record <field> element do not exist!");
                    ret = -1;
                    break;
                }
                bitParam.bitLenArr[bitParam.field_num]    =  (uint32_t)strtoul(len_str, NULL, 10);

                char *type_str = (char*)mxmlElementGetAttr(bit_field_node, "value_type");
                if (type_str == NULL) {
                    TERR("<value_type> attribute of bit_record <field> element do not exist!");
                    ret = -1;
                    break;
                }

                PF_DATA_TYPE  field_type = getType(type_str);
                if (field_type == DT_UNSUPPORT || field_type == DT_KV32) {
                    ret = -1;
                    break;
                }
                bitParam.typeArr[bitParam.field_num]      =  field_type;

                char *encodeFlag_str = (char*)mxmlElementGetAttr(bit_field_node, "encode");
                if (encodeFlag_str == NULL || strcasecmp(encodeFlag_str, "true") != 0) {
                    bitParam.encodeFlagArr[bitParam.field_num] = false;
                }
                else {
                    bitParam.encodeFlagArr[bitParam.field_num] = true;
                }

                char *fieldFlag_str = (char*)mxmlElementGetAttr(bit_field_node, "flag");
                if (fieldFlag_str == NULL) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_NO_FLAG;
                }
                else if (strcmp(fieldFlag_str, BIT_FILTER_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_FILTER_BIT;
                }
                else if (strcmp(fieldFlag_str, PROVCITY_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_PROVCITY;
                }
                else if (strcmp(fieldFlag_str, REALPOSTFEE_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_REALPOSTFEE;
                }
                else if (strcmp(fieldFlag_str, PROMOTION_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_PROMOTION;
                }
                else if (strcmp(fieldFlag_str, TIME_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_TIME;
                }
                else if (strcmp(fieldFlag_str, LATLNG_FLAG_STR) == 0) {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_LATLNG_DIST;
                }
                else {
                    bitParam.fieldFlagArr[bitParam.field_num] = F_NO_FLAG;
                }

                char *default_str = (char*)mxmlElementGetAttr(field_node, "default");
                if (!setBitRecordDefaultValue(field_type, bitParam.defaultArr[bitParam.field_num], default_str, bitParam.bitLenArr[bitParam.field_num], bitParam.encodeFlagArr[bitParam.field_num])) {
                    ret = -1;
                    break;
                }

                bitParam.field_num++;
                if (bitParam.field_num >= MAX_BITFIELD_NUM) {
                    break;
                }
            }

            if (ret == -1) {
                break;
            }
            ret = pManager->addBitRecordField(bitParam.field_num, bitParam.fieldNameArr, bitParam.fieldAliasArr, bitParam.bitLenArr,
                                              bitParam.typeArr, bitParam.encodeFlagArr, bitParam.fieldFlagArr,
                                              bitParam.defaultArr, groupPos);

            if (ret != 0) {
                TERR("INDEXLIB: Create profile error, add bit record field error!");
                break;
            }
            bit_record_num++;
            printf("加载配置文件:成功添加第 %d 组,共 %u 个BitRecord字段", bit_record_num, bitParam.field_num);
        }
        
        if (ret != 0) {
            break;
        }

        for (field_node = mxmlFindElement(profile_node, tree, "field", NULL, NULL, MXML_DESCEND_FIRST);
                field_node != NULL; field_node = mxmlFindElement(field_node, tree, "field", NULL, NULL, MXML_NO_DESCEND))
        {
            char *name_str = (char*)mxmlElementGetAttr(field_node, "name");
            if (name_str == NULL) {
                TERR("<name> attribute of <field> element do not exist!");
                ret = -1;
                break;
            }
            if (strlen(name_str) >= MAX_FIELD_NAME_LEN) {
                TERR("<name> attribute value length over %u !", MAX_FIELD_NAME_LEN);
                ret = -1;
                break;
            }

            char * alias_str = (char*)mxmlElementGetAttr(field_node, "alias");
            if (alias_str != NULL && strlen(alias_str) >= MAX_FIELD_NAME_LEN) {
                TERR("<alias> attribute value length over %u !", MAX_FIELD_NAME_LEN);
                ret = -1;
                break;
            }
 
            uint32_t groupPos = getFieldGroupPosition(name_str, pack);
            char *multiVal_str = (char*)mxmlElementGetAttr(field_node, "multi_num");
            if (multiVal_str == NULL) {
                TERR("<multi_num> attribute of <field> element do not exist!");
                ret = -1;
                break;
            }
            uint32_t multiVal = (uint32_t)strtoul(multiVal_str, NULL, 10);

            char *type_str = (char*)mxmlElementGetAttr(field_node, "value_type");
            if (type_str == NULL) {
                TERR("<value_type> attribute of <field> element do not exist!");
                ret = -1;
                break;
            }
            PF_DATA_TYPE dType = getType(type_str);

            bool encode_flag = false;
            char *encodeFlag_str = (char*)mxmlElementGetAttr(field_node, "encode");
            if (encodeFlag_str == NULL || strcasecmp(encodeFlag_str, "true") != 0) {
                encode_flag = false;
            }
            else {
                encode_flag = true;
            }

            bool compress_flag = false;
            char *compress_str = (char*)mxmlElementGetAttr(field_node, "compress");
            if (compress_str == NULL || strcasecmp(compress_str, "Y") != 0) {
                compress_flag = false;
            }
            else {
                compress_flag = true;
            }

            char *default_str = (char*)mxmlElementGetAttr(field_node, "default");
            if ((multiVal != 1 || dType == DT_STRING) && default_str != NULL) {
                TERR("Parse config file error, <default> attribute is only used by single & no_string value field!");
                ret = -1;
                break;
            }

            index_lib::EmptyValue defaultValue;
            if (!setDefaultValue(dType, defaultValue, default_str)) {
                printf("set DefaultValue of field [ %s ] failed!\n", name_str);
                ret = -1;
                break;
            }

            PF_FIELD_FLAG field_flag = F_NO_FLAG;
            char *fieldFlag_str = (char*)mxmlElementGetAttr(field_node, "flag");
            if (fieldFlag_str == NULL) {
                field_flag = F_NO_FLAG;
            }
            else if (strcmp(fieldFlag_str, BIT_FILTER_FLAG_STR) == 0) {
                field_flag = F_FILTER_BIT;
            }
            else if (strcmp(fieldFlag_str, PROVCITY_FLAG_STR) == 0) {
                field_flag = F_PROVCITY;
            }
            else if (strcmp(fieldFlag_str, REALPOSTFEE_FLAG_STR) == 0) {
                field_flag = F_REALPOSTFEE;
            }
            else if (strcmp(fieldFlag_str, PROMOTION_FLAG_STR) == 0) {
                field_flag = F_PROMOTION;
            }
            else if (strcmp(fieldFlag_str, TIME_FLAG_STR) == 0) {
                field_flag = F_TIME;
            }
            else if (strcmp(fieldFlag_str, LATLNG_FLAG_STR) == 0) {
                field_flag = F_LATLNG_DIST;
            }
            else {
                field_flag = F_NO_FLAG;
            }

            bool updateOverlap = true;
            char *overlap_str = (char*)mxmlElementGetAttr(field_node, "update_overlap");
            if (overlap_str != NULL && strcasecmp(overlap_str, "false") == 0) {
                updateOverlap = false;
            }

            ret = pManager->addField(name_str, alias_str, dType, multiVal, encode_flag, field_flag, compress_flag, defaultValue, groupPos, updateOverlap);
            if (ret != 0) {
                TERR("INDEXLIB: Create profile error, add field [%s] error!", name_str);
                break;
            }
            common_field_num++;
        }

        if (ret != 0) {
            break;
        }
        printf("加载配置文件：成功添加 %d 非BitRecord字段\n", common_field_num);

    }while(0);

    if (conf_fd != NULL) {
        fclose(conf_fd);
    }
    if (tree != NULL) {
        mxmlDelete(tree);
    }
    return ret;
}

/**
 * 处理字符串, 去除\n
 */
void process_str(char *str)
{
    uint32_t len = strlen(str);
    while(len != 0 && (str[len - 1] == '\n' || str[len - 1] == '\r'))
    {
        len--;
    }

    str[len] = '\0';
}

int main(int argc, char * argv[])
{
    alog::Configurator::configureRootLogger();

    char conf_file[PATH_MAX] = {0};
    char head_file[PATH_MAX] = {0};
    char data_file[PATH_MAX] = {0};

    char *data_str  = NULL; 
    FILE *header_fd = NULL;
    FILE *data_fd   = NULL;
    int   doc_num   = 0;
    int   ret       = 0;

    int i;
    while ((i = getopt(argc, argv, "c:t:d:h")) != EOF) {
        switch (i) {
            case 'c':
                snprintf(conf_file, PATH_MAX, "%s", optarg);
                break;  

            case 't':
                snprintf(head_file, PATH_MAX, "%s", optarg);
                break;  

            case 'd':
                snprintf(data_file, PATH_MAX, "%s", optarg);
                break;  

            case 'h':
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;     

            default:
                usage(argv[0]);
                exit(EXIT_FAILURE);
                break;
        }
    }

    if (conf_file[0] == '\0') {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (head_file[0] == '\0') {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    struct timeval tv_begin;
    struct timeval tv_end;
    struct timeval tv_total;

    printf("********************************\n");
    gettimeofday(&tv_begin, NULL);

    // load conf
    ret = load_conf(conf_file);
    if (ret != 0) {
        TERR("INDEXLIB: Create profile error, load conf file [%s] failed!", conf_file);
        return ret;
    }
    
    index_lib::ProfileManager *pManager = index_lib::ProfileManager::getInstance();
    header_fd = fopen(head_file, "r");
    if (data_file[0] == '\0') {
        data_fd = stdin;
    }
    else {
        data_fd = fopen(data_file, "r");
    }

    do
    {
        if (header_fd == NULL || data_fd == NULL) {
            TERR("INDEXLIB: Create profile error:%s", strerror(errno));
            ret = KS_EIO;
            break;
        }

        data_str = (char *)malloc(MAX_DOC_LEN * sizeof(char));
        if (data_str == NULL) {
            TERR("INDEXLIB: Create profile error:%s", strerror(errno));
            ret = KS_ENOMEM;
            break;
        }

        if (fgets(data_str, PATH_MAX, header_fd) == NULL) {
            TERR("INDEXLIB: Create profile error, read field header information from %s failed!", head_file);
            ret = KS_EIO;
            break;
        }

        printf("*********加载Profile索引字段的头文件!********\n");
        process_str(data_str);
        if ((ret = pManager->setDocHeader(data_str)) != 0) {
            TERR("INDEXLIB: Create profile error, set Doc header failed!");
            break;
        }

        printf("*********开始解析文本索引文件!*********\n");
        while(fgets(data_str, MAX_DOC_LEN, data_fd) != NULL) {
            process_str(data_str);
            if ((ret = pManager->addDoc(data_str)) != 0) {
                TERR("INDEXLIB: Create profile error, add doc [%d] failed!", doc_num);
                break;
            }
            doc_num++;
            if (doc_num % 500000 == 0) {
                printf("Create profile: already parse %d doc\n", doc_num);
            }
        }

        if (ret != 0) {
            break;
        }

        printf("*********解析文本索引完毕，开始dump二进制索引数据*********\n");
        if (pManager->dump() != 0) {
            TERR("INDEXLIB: Create profile error, dump data failed!");
            break;
        }
    }
    while(0);

    gettimeofday(&tv_end, NULL);
    timersub(&tv_end, &tv_begin, &tv_total);

    printf("创建Profile索引耗时: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);
    if (ret != 0) {
        printf("创建Profile索引失败\n");
    }
    else {
        printf("成功创建的doc数量:%d\n", doc_num);
    }

    if (header_fd != NULL) {
        fclose(header_fd);
    }

    if (data_fd != NULL && data_fd != stdin) {
        fclose(data_fd);
    }

    if (data_str != NULL) {
        free(data_str);
    }

    index_lib::ProfileManager::freeInstance();
    index_lib::DocIdManager::freeInstance();
    return 0;
}
