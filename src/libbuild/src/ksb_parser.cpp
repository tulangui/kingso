#include <stdio.h>
#include <wchar.h>


#include "ksb_ws.h"
#include "ksb_helper.h"
#include "match_tree.h"
#include "ks_build/ks_build.h"
#include "index_lib/ProvCityManager.h"
#include "util/charsetfunc.h"
#include "util/strfunc.h"
#include "util/idx_sign.h"
#include "util/py_code.h"

#define STR_VALUE_LENGH 81920
#define DETAIL_STR_LEN  4096

/**
 * @brief qsort 的回调函数, 用于比较字符串
 * @param p1  元素指针
 * @param p2  元素指针
 * @return  0：相等 ; <0: p1小于p2;  >0: p1大于p2
 */
static int ksb_qsort_cmpstr(const void *p1, const void *p2) 
{
    return strcmp(* (char * const *) p1, * (char * const *) p2); 
}

// prop sort add by tiechou 
/**
 * @brief 对输入的字符串切分，排序，输入字符串将被修改！！！
 * @param value 待排序的字符串
 * @param size 数组的最大长度
 * @param delim 分割符字符
 * @return -1:fail  0:success
 */
int32_t ksb_sort_value(char* value, int32_t size, char delim)
{
    char *items[KSB_FIELD_MAX_SEGS] = {0};

    // split string with space
    const char delim_str[2] = {delim , '\0'};
    int32_t ret = split_multi_delims(value, items, KSB_FIELD_MAX_SEGS, delim_str);
    if (ret < 0) {
        fprintf(stderr, "split value error in ksb_sort_value ..\n");
        return -1;
    }

    // sort items using strcmp
    qsort(items, ret, sizeof(char * ),  ksb_qsort_cmpstr);

    // form sorted char* array  to string
    int32_t value_len = 0;
    char *tmp = (char *)malloc(size);
    if (tmp==NULL) return -1;
    memset(tmp, 0 , size);
    for (int32_t i=0; i<ret; i++){
        int32_t len = snprintf(tmp + value_len, size - value_len , "%s ", items[i] );
        value_len += len; 
        if (value_len > size) {
            fprintf(stderr, "value len:%d > size:%d in ksb_sort_value ..\n", value_len, size);
            free(tmp);
            return -1;
        }
    } 
    snprintf(value, value_len, "%s", tmp);
    free(tmp);

    // cut last space
    if (value_len > 0){
        value[value_len-1] = '\0';
    }
    
    return 0;
}

static int
ksb_set_value(ksb_result_t * presult, ksb_conf_t * conf, const char *value,
              int32_t index, int32_t index_type, uint32_t flags) {
    field_info_t *pinfo = NULL;

    if (FIELD_INDEX == index_type) {
        pinfo = &conf->index[index];
    }
    else if (FIELD_PROFILE == index_type) {
        pinfo = &conf->profile[index];
    }
    else if (FIELD_DETAIL == index_type) {
        pinfo = &conf->detail[index];
    }
    union {
        char str_nick_hash[MAX_NICK_HASH_LENGTH];
        char str_loc[MAX_NICK_HASH_LENGTH];
        char str_value[STR_VALUE_LENGH];
    };
    wchar_t wcs_value[STR_VALUE_LENGH];
    uint64_t nick_hash = 0;
    index_lib::ProvCityManager * pm =
        (index_lib::ProvCityManager *) conf->provcity;
    index_lib::ProvCityID pcid_s;
    uint16_t pcid;
    uint64_t cst_time;
    int i = 0;
    int ret = 0;
    uint64_t sign;
    ksb_result_field_t *pfield = NULL;
    ksb_result_field_t *pfieldbind = NULL;
    union{
        char *items[KSB_FIELD_MAX_SEGS];
        uint64_t latlng_fee_list[POSTFEE_MAX];
        uint64_t long_values[POSTFEE_MAX];
    };
    int item_len = 0;
    int item_count = 0;
    char tmp_str[32];

    //判断是否在package中
    if(FIELD_INDEX == index_type&& pinfo->package_ext_head) {//倒排

        if (pinfo->to_utf8 && (flags & KSB_FLAG_INPUT_UTF8) == 0) {
            code_gbk_to_utf8(str_value, sizeof(str_value), value,
                             CC_FLAG_IGNORE);
        }
        else {
            snprintf(str_value, sizeof(str_value), "%s", value);
        }
//        ksb_normalize(str_value, strlen(str_value));
        swprintf(wcs_value,STR_VALUE_LENGH,L"%s",str_value);
        py_unicode_normalize(wcs_value);
        snprintf(str_value,sizeof(str_value),"%ls",wcs_value);
        
        package_ext_t *p_ext = NULL;
        for(p_ext = pinfo->package_ext_head; p_ext; p_ext = p_ext->next){
            pfield = ksb_result_get_node(presult, index, FIELD_INDEX);
            pfieldbind = ksb_result_get_node(presult, p_ext->p_package_info->package_index, FIELD_INDEX);
            if(!pfieldbind->is_set){
               pfieldbind->is_set = 1;
               ++presult->index_count;
            }
            ksb_do_ws(str_value, pfield, conf, pfieldbind, p_ext->p_package_info->fields[p_ext->index_in_package].occ,
                    p_ext->p_package_info->fields[p_ext->index_in_package].real_occ, index);
        }
        //clear pfield result
        free(pfield->index_value);
        pfield->index_value = NULL;
        pfield->v_len = 0; 
        if(pinfo->package_only){
            pfield->is_set=1;
            ++presult->index_count;
            return 0;
        }
    }

    switch (pinfo->index_type) {
    case FIELD_TYPE_LATLNG:
        if(value[0]==0){
            str_value[0]=0;
            ksb_result_set(presult,str_value,index,index_type);
            break;
        }
        ret = conf->latlng->encode(value,latlng_fee_list,POSTFEE_MAX);
        if(ret<=0){
            fprintf(stderr,"LatLngManager::encode() failed. ret=%d\n",ret);
            str_value[0]=0;
            ksb_result_set(presult,str_value,index,index_type);
        }
        else{
            str_value[0]=0;
            item_count =ret;
            item_len = 0;
            //POSTFEE_MAX=128 str_value 4096bits
            for(i=0;i<item_count;i++){
                if(i<item_count-1){
                    snprintf(tmp_str,sizeof(tmp_str),"%llu ",(unsigned long long)latlng_fee_list[i]);
                }
                else{
                    snprintf(tmp_str,sizeof(tmp_str),"%llu",(unsigned long long)latlng_fee_list[i]);
                }
                item_len+=strlen(tmp_str);
                if(item_len<STR_VALUE_LENGH){
                    strcat(str_value,tmp_str);
                }
                else{
                    fprintf(stderr,"Postfee field is too long! count:%d\n",item_count);
                    break;
                }
            }
            //fprintf(stderr,"Real post fee:count:%d, %s\n",item_count,str_value);
            ksb_result_set(presult,str_value,index,index_type);
        }
        break;
    case FIELD_TYPE_POSTFEE:
        if(value[0]==0){
            str_value[0]=0;
            ksb_result_set(presult,str_value,index,index_type);
            break;
        }
        ret = conf->postfee->encode(value,latlng_fee_list,POSTFEE_MAX);
        if(ret<=0){
            fprintf(stderr,"RealPostFeeManager::encode() failed. ret=%d\n",ret);
            str_value[0]=0;
            ksb_result_set(presult,str_value,index,index_type);
        }
        else{
            str_value[0]=0;
            item_count =ret;
            item_len = 0;
            //POSTFEE_MAX=128 str_value 4096bits
            for(i=0;i<item_count;i++){
                if(i<item_count-1){
                    snprintf(tmp_str,sizeof(tmp_str),"%llu ",(unsigned long long)latlng_fee_list[i]);
                }
                else{
                    snprintf(tmp_str,sizeof(tmp_str),"%llu",(unsigned long long)latlng_fee_list[i]);
                }
                item_len+=strlen(tmp_str);
                if(item_len<STR_VALUE_LENGH){
                    strcat(str_value,tmp_str);
                }
                else{
                    fprintf(stderr,"Postfee field is too long! count:%d\n",item_count);
                    break;
                }
            }
            //fprintf(stderr,"Real post fee:count:%d, %s\n",item_count,str_value);
            ksb_result_set(presult,str_value,index,index_type);
        }
        break;
    case FIELD_TYPE_LOC:
        str_loc[0] = 0;
        if ((flags & KSB_FLAG_INPUT_UTF8) == 0) {
            code_gbk_to_utf8(str_loc, sizeof(str_loc), value,
                             CC_FLAG_IGNORE);
        }
        else {
            snprintf(str_loc, sizeof(str_loc), "%s", value);
        }

        swprintf(wcs_value,STR_VALUE_LENGH,L"%s",str_loc);
        py_unicode_normalize(wcs_value);
        snprintf(str_loc,sizeof(str_loc),"%ls",wcs_value);
        

        if (pm->seek(str_loc, ' ', &pcid_s) == 0) {
            pcid = pcid_s.pc_id;
        }
        else {
            pcid = 0;
        }
        snprintf(str_loc, sizeof(str_loc), "%u", uint32_t(pcid));
        ksb_result_set(presult, str_loc, index, index_type);
        break;
    case FIELD_TYPE_NICK:
        if ((flags & KSB_FLAG_INPUT_UTF8) != 0) {
            code_utf8_to_gbk(str_value,sizeof(str_value), value,CC_FLAG_IGNORE);
            nick_hash = get_nick_hash(str_value);
        }
        else{
            nick_hash = get_nick_hash(value);
        }
        snprintf(str_nick_hash, sizeof(str_nick_hash), "%llu",
                 (unsigned long long) nick_hash);
        ksb_result_set(presult, str_nick_hash, index, index_type);
        break;
    case FIELD_TYPE_TEXT:
        if (FIELD_INDEX != index_type) {
            fprintf(stderr, "text type must be index.\n");
            return -1;
        }
        if (pinfo->to_utf8 && (flags & KSB_FLAG_INPUT_UTF8) == 0) {
            code_gbk_to_utf8(str_value, sizeof(str_value), value,
                             CC_FLAG_IGNORE);
        }
        else {
            snprintf(str_value, sizeof(str_value), "%s", value);
        }
//        ksb_normalize(str_value, strlen(str_value));
        swprintf(wcs_value,STR_VALUE_LENGH,L"%s",str_value);
        py_unicode_normalize(wcs_value);
        snprintf(str_value,sizeof(str_value),"%ls",wcs_value);

        pfield = ksb_result_get_node(presult, index, index_type); //index_type must be FIELD_INDEX
        if (!pfield) {
            //
            return -1;
        }
        pfieldbind = NULL;
        if(pinfo->bind_to){
            pfieldbind = ksb_result_get_node(presult, pinfo->bind_to-FIELD_INDEX_ADD, index_type);
            if(!pfieldbind){
                return -1;
            }
        }
        if (ksb_do_ws(str_value, pfield, conf, pfieldbind, pinfo->bind_occ, pinfo->bind_real_occ, index) != 0) {
            //
            return -1;
        }
        ++presult->index_count;
        break;
    case FIELD_TYPE_CSTTIME:
        if (value[0] == '\0') {
            str_value[0] = '\0';
        }
        else {
            snprintf(str_value, sizeof(str_value), "%s", value);

            item_len =
                split_multi_delims(str_value, items, KSB_FIELD_MAX_SEGS,
                                   " ");
            ret = 0;
            if (item_len > 0) {
                for (i = 0; i < item_len; i++) {
                    cst_time = static_cast<uint64_t>(strtoul(items[i], NULL, 10));
                    if (cst_time == 0) {
                        long_values[i] = 0;
                    }
                    else {
                        long_values[i] = cst_time + timezone;
                    }
                }
                for (i = 0; i < item_len; i++) {
                    ret += snprintf(str_value + ret, sizeof(str_value) - ret, \
                            "%lu ", long_values[i]);
                }
            }
            if (ret > 0) {
                str_value[ret - 1] = '\0';
            }
            else {
                str_value[0] = '\0';
            }
        }

        ksb_result_set(presult, str_value, index, index_type);
        break;
    default:
        if (!pinfo->to_utf8 && pinfo->value_delim == ' ') {
            //ksb_result_set(presult,value,index,index_type);
            snprintf(str_value, sizeof(str_value), "%s", value);
        }
        else {
            if (pinfo->to_utf8 && (flags & KSB_FLAG_INPUT_UTF8) == 0) {
                code_gbk_to_utf8(str_value, sizeof(str_value), value,
                                 CC_FLAG_IGNORE);
            }
            else {
                snprintf(str_value, sizeof(str_value), "%s", value);
            }
            if (pinfo->value_delim != ' ') {
                for (i = 0; str_value[i]; i++) {
                    if (str_value[i] == pinfo->value_delim) {
                        str_value[i] = ' ';
                    }
                }
            }
        }
        if(pinfo->output_delim!=' '){
            for(i=0;str_value[i];++i){
                if(str_value[i] == ' '){
                    str_value[i] = pinfo->output_delim;
                }
            }
        }
        // value need_sort add by tiechou
        if (pinfo->need_sort == 1){
            ksb_sort_value(str_value, STR_VALUE_LENGH, pinfo->output_delim);
        }

        if (FIELD_PROFILE == index_type) {  // profile 不截断
            ksb_result_set(presult, str_value, index, index_type);
        }  
        else if (FIELD_DETAIL == index_type) {  // detail 截断到4K
            str_value[DETAIL_STR_LEN] = '\0';
            ksb_result_set(presult, str_value, index, index_type);
        }
        else {
            swprintf(wcs_value,STR_VALUE_LENGH,L"%s",str_value);
            py_unicode_normalize(wcs_value);
            snprintf(str_value,sizeof(str_value),"%ls",wcs_value);
        	
            pfield = ksb_result_get_node(presult, index, index_type);
            ret =
                split_multi_delims(str_value, items, KSB_FIELD_MAX_SEGS,
                                   " ");
            if (ret > 0) {
                item_count = ret;
                for (i = 0; i < ret; i++) {
                    item_len = strlen(items[i]);
                    if (item_len == 0) {
                        continue;
                    }
                    sign = idx_sign64(items[i], strlen(items[i]));
                    ksb_insert_result(pfield, sign, 0, 0);
                }
                if(pfield->v_len>0){
                    ksb_ws_sort_uniq(pfield);
                }
            }
            ++presult->index_count;
        }

        break;
    }
    
    if (FIELD_INDEX == index_type) {
        pfield = &presult->index_fields[index];
        pfield->is_set=1;
    }
    else if (FIELD_PROFILE == index_type) {
        pfield = &presult->profile_fields[index];
        pfield->is_set=1;
    }
    else if (FIELD_DETAIL == index_type) {
        pfield = &presult->detail_fields[index];
        pfield->is_set=1;
    }

    return 0;
}

int
ksb_parse(ksb_auction_t * auction, ksb_conf_t * conf,
          ksb_result_t * presult, uint32_t flags) {
    uint32_t i = 0;
    uint64_t index = 0;
    int ret = 0;
    for (i = 0; i < auction->count; i++) {
        index =
            (uint64_t) match_tree_find((match_tree_t*)conf->index_map,
                                    auction->fields[i].name);
        if (index) {
            ret =
                ksb_set_value(presult, conf, auction->fields[i].value,
                              index - FIELD_INDEX_ADD, FIELD_INDEX, flags);
            if (ret != 0) {
                fprintf(stderr,"ksb_set_value failed. INDEX.\n");
                return -1;
            }
        }
        index =
            (uint64_t) match_tree_find((match_tree_t*)conf->profile_map,
                                    auction->fields[i].name);
        if (index) {
            ret =
                ksb_set_value(presult, conf, auction->fields[i].value,
                              index - FIELD_INDEX_ADD, FIELD_PROFILE,
                              flags);
            if (ret != 0) {
                fprintf(stderr,"ksb_set_value failed. PROFILE.\n");
                return -1;
            }
        }
        index =
            (uint64_t) match_tree_find((match_tree_t*)conf->detail_map,
                                    auction->fields[i].name);
        if (index) {
            ret =
                ksb_set_value(presult, conf, auction->fields[i].value,
                              index - FIELD_INDEX_ADD, FIELD_DETAIL,
                              flags);
            if (ret != 0) {
                fprintf(stderr,"ksb_set_value failed. DETAIL.\n");
                return -1;
            }
        }
    }

    if (conf->index_count != presult->index_count ||
        conf->profile_count != presult->profile_count ||
        conf->detail_count != presult->detail_count) {
        fprintf(stderr,"No enough fields.%u:%u,%u:%u,%u:%u\n",
                conf->index_count,presult->index_count,
                conf->profile_count,presult->profile_count,
                conf->detail_count,presult->detail_count
                );
        for(i=0;i<conf->index_count;i++){
            if(presult->index_fields[i].is_set == 0){
                fprintf(stderr,"No field:%s\n",conf->index[i].name);
            }
        }
        for(i=0;i<conf->profile_count;i++){
            if(presult->profile_fields[i].is_set == 0){
                fprintf(stderr,"No field:%s\n",conf->profile[i].name);
            }
        }
        for(i=0;i<conf->detail_count;i++){
            if(presult->detail_fields[i].is_set == 0){
                fprintf(stderr,"No field:%s\n",conf->detail[i].name);
            }
        }
        return -1;
    }
/*    fprintf(stderr, "%u:%u,%u:%u,%u:%u\n", conf->index_count,
            presult->index_count, conf->profile_count,
            presult->profile_count, conf->detail_count,
            presult->detail_count);*/

    return 0;
}

int
ksb_parse_str(const char *pbuf, uint32_t size, ksb_conf_t * conf,
              ksb_result_t * presult, uint32_t flags) {
    return -1;
}
