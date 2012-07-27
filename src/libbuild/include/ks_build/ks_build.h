
/*********************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ks_build.h 2577 2011-03-09 01:50:55Z gongyi.cl@taobao.com $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef INCLUDE_KS_BUILD_H_
#define INCLUDE_KS_BUILD_H_

#include <stdint.h>
#include "commdef/TokenizerFactory.h"
#include "index_lib/ProvCityManager.h"
#include "index_lib/RealPostFeeManager.h"
#include "index_lib/LatLngManager.h"
#define KSB_FIELD_MAX 256
#define KSB_FIELDNAME_MAX 64
#define KSB_TYPENAME_MAX 32
#define KSB_FIELD_VALUE_MAX 1024
#define KSB_FIELD_MAX_SEGS  1024
#define POSTFEE_MAX 1024

#define KSB_FLAG_INPUT_UTF8 1

#define MAX_NICK_LENGTH 64
#define MAX_NICK_HASH_LENGTH 64

#define FIELD_TYPE_TEXT 1
#define FIELD_TYPE_STR  2
#define FIELD_TYPE_NICK 3
#define FIELD_TYPE_LOC  4
#define FIELD_TYPE_POSTFEE 5
#define FIELD_TYPE_CSTTIME 6
#define FIELD_TYPE_LATLNG  7

#define FIELD_INDEX_ADD 1000

#define PACKAGE_FIELD_MAX 8
#define PACKAGE_FIELD_INFOS 3
#define PACKAGE_NAME_PREFIX ""

#define MODE_DETAIL 1
#define MODE_SEARCHER (1<<1)
#define MODE_ALL (MODE_DETAIL|MODE_SEARCHER)

enum {
    FIELD_INDEX = 0,
    FIELD_PROFILE,
    FIELD_DETAIL,
};

typedef struct field_type_s {
    char type_name[32];
    uint32_t type;
} field_type_t;


typedef struct package_field_s{
    char name[KSB_FIELDNAME_MAX];
    uint32_t occ;
    uint32_t real_occ;
}package_field_t;

typedef struct package_info_s{
    char name[KSB_FIELDNAME_MAX];
    package_field_t fields[PACKAGE_FIELD_MAX];
    uint32_t fields_count;
    uint32_t package_index;
}package_info_t;

typedef struct package_ext_s{
    package_info_t *p_package_info;
    uint32_t index_in_package;
    struct package_ext_s *next;
}package_ext_t;

typedef struct field_info_s {
    char name[KSB_FIELDNAME_MAX];
    uint32_t index_type;
    uint32_t allow_null;
    uint32_t to_utf8;
    uint32_t need_sort; // prop_vid sorted add by tiechou  need_sort=1：表示需要排序
    union {
        uint32_t unused;
        struct{
            char value_delim;
            char output_delim;
        };
    };
    union{
        char *bind_to_name;
        uint64_t bind_to;
    };
    uint32_t bind_occ;
    uint32_t bind_real_occ;
    uint32_t package_only:1,is_package:1;
    package_ext_t *package_ext_head;
} field_info_t;

typedef struct ksb_auction_field_s {
    char *name;
    char *value;
    uint32_t n_size;
    uint32_t v_size;
    uint32_t n_len;
    uint32_t v_len;
} ksb_auction_field_t;

typedef struct ksb_auction_s {
    ksb_auction_field_t fields[KSB_FIELD_MAX];
    uint32_t count;
} ksb_auction_t;

typedef struct ksb_index_value_s {
    uint64_t sign;
    uint32_t occ;
    uint32_t weight;
    char term[32];
} ksb_index_value_t;

typedef struct ksb_result_field_s {
    union {
        char *value;
        ksb_index_value_t *index_value;
    };
    uint32_t v_size;
    uint32_t v_len;
    int index;
    uint32_t is_set;
} ksb_result_field_t;

typedef struct ksb_result_s {
    ksb_result_field_t index_fields[KSB_FIELD_MAX];
    ksb_result_field_t profile_fields[KSB_FIELD_MAX];
    ksb_result_field_t detail_fields[KSB_FIELD_MAX];
    uint32_t index_count;
    uint32_t profile_count;
    uint32_t detail_count;
} ksb_result_t;

typedef struct conf_node_s {
    char name[64];
} conf_node_t;

typedef struct stopword_map_s {
    uint64_t map[1024];
} stopword_map_t;

typedef struct ksb_conf_s {
    uint32_t index_count;
    uint32_t profile_count;
    uint32_t detail_count;
    uint32_t detail_nid_index;
    field_info_t index[KSB_FIELD_MAX];
    field_info_t profile[KSB_FIELD_MAX];
    field_info_t detail[KSB_FIELD_MAX];
    ITokenizer *pTokenizer[KSB_FIELD_MAX];
    void* index_map;
    void* profile_map;
    void* detail_map;
    index_lib::ProvCityManager * provcity;
    index_lib::RealPostFeeManager *postfee;
    index_lib::LatLngManager * latlng;
    // ws::AliTokenizer * tokenizer;
    // ws::SegResult * token_result;
    stopword_map_t *stopword;
    uint32_t index_bind_fields[KSB_FIELD_MAX];
    uint32_t index_bind_count;
} ksb_conf_t;

#ifdef __cplusplus
extern "C" {
#endif


    extern ksb_conf_t *ksb_load_config(const char *conf_file, uint32_t mode);
    extern void ksb_destroy_config(ksb_conf_t * conf);

    extern ksb_auction_t *ksb_create_auction();
    extern int ksb_auction_insert_field(ksb_auction_t * auction,
                                        const char *key,
                                        const char *value);
    extern void ksb_reset_auction(ksb_auction_t * auction);
    extern void ksb_destroy_auction(ksb_auction_t * auction);

    extern ksb_result_t *ksb_create_result();
    extern int ksb_result_set(ksb_result_t * result, const char *value,
                              int index, uint32_t index_type);
    extern ksb_result_field_t *ksb_result_get_node(ksb_result_t * result,
                                                   int index,
                                                   uint32_t index_type);
    extern void ksb_reset_result(ksb_result_t * result);
    extern void ksb_destroy_result(ksb_result_t * result);

    extern int ksb_parse_str(const char *pbuf, uint32_t size,
                             ksb_conf_t * conf, ksb_result_t * presult,
                             uint32_t flags);
    extern int ksb_parse(ksb_auction_t * auction, ksb_conf_t * conf,
                         ksb_result_t * presult, uint32_t flags);


#ifdef __cplusplus
}
#endif
#endif                          //INCLUDE_KS_BUILD_H_
