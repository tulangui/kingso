#include "KeyvalueFilterParser.h"
#include "StringUtil.h"
#include "util/strfunc.h"

namespace queryparser {
KeyvalueFilterParser::KeyvalueFilterParser()
{
}
KeyvalueFilterParser::~KeyvalueFilterParser()
{
}


int KeyvalueFilterParser::doFilterParse(MemPool *mempool, FilterList *list, char *name, int nlen, char *string, int slen)
{
    static char* default_field_type = "number";
    static int default_field_tlen = 6;

    char** subs = 0;
    int sub_cnt = 0;
    char** values = 0;
    int value_cnt = 0;
    char* field_name = 0;
    int field_nlen = 0;
    char* field_value = 0;
    int field_vlen = 0;
    char* field_type = 0;
    int field_tlen = 0;
    int i = 0;
    int j = 0;
    bool prohibite = false;

    char field[MAX_FIELD_VALUE_LEN+1];

    if(!mempool  || !list || !name || nlen<=0 || !string || slen<=0){
        TNOTE("qp_filter_keyvalue_parse argument error, return!");
        return -1;
    }
    char *equal = NULL;
    char *flag = NULL;
    char *value = str_trim(string);


    // 为按';'或".and"分割参数值申请缓存数组
    subs = NEW_VEC(mempool, char*, slen);
    if(!subs){
        TNOTE("NEW_VEC subs error, return!");
        return -1;
    }

    // 为按','分割参数值子句值申请缓存数组
    values = NEW_VEC(mempool, char*, slen);
    if(!values){
        TNOTE("NEW_VEC values error, return!");
        return -1;
    }

    // 按';'分割参数值
    sub_cnt =  str_split_ex(value, ';', subs, strlen(value));
    if(sub_cnt<=0){
        TNOTE("_split_keyvalue_string error, return!");
        return -1;
    }

    // 循环处理每个参数值子句
    for(i=0; i<sub_cnt; i++){
        FilterNode node;
        memset(&node, 0, sizeof(node));
        node.type = ESingleType; 
        if(parseName2Node(&node, name, strlen(name)) < 0) {
            return -1;
        }
        node.item_size = 0;
        subs[i] = str_trim(subs[i]);
        if(subs[i][0]==0){
            continue;
        }
        // 将每个子句分析成type,name,value
        if(_parse_keyvalue_string(subs[i], strlen(subs[i]), 
                    &field_name, &field_nlen, &field_value, &field_vlen, &field_type, &field_tlen)){
            TNOTE("_parse_keyvalue_string error, return!");
            return -1;
        }
        if(field_nlen<=0 || field_vlen<=0){
            TNOTE("field name or value error, return![name:%s][value:%s]", field_name, field_value);
            return -1;
        }
        if(!field_type || field_tlen==0){
            field_type = default_field_type;
            field_tlen = default_field_tlen;
        } else{
            field_type = str_trim(field_type);
            field_tlen = strlen(field_type);
        }
        field_name = str_trim(field_name);
        field_nlen = strlen(field_name);
        field_value = str_trim(field_value);
        field_vlen = strlen(field_value);
        if(field_nlen<=0 || field_vlen<=0){
            TNOTE("field name or value error, return![name:%s][value:%s]", field_name, field_value);
            return -1;
        }
        // 当前子句是否取非
        if(field_name[0]=='_'){
            prohibite = true;
            field_name ++;
            field_nlen --;
            if(field_nlen<=0){
                TNOTE("field name error, return!");
                return -1;
            }
        } else{
            prohibite = false;
        }
        if(prohibite){
            node.relation = false;
        }
        // 将子句值按','分割成多值
        value_cnt = str_split_ex(field_value, ',', values, slen);
        if(value_cnt<=0){
            TNOTE("str_split_ex \',\' error, return!");
            return -1;
        }
        // 将每个值添加入过滤数据结构中
        for(j=0; j<value_cnt; j++){
            values[j] = str_trim(values[j]);
            if(values[j][0]==0){
                continue;
            }
            snprintf(field, MAX_FIELD_VALUE_LEN+1, "%s:%s", field_name, values[j]);
            if(addFiltItem2Node(mempool, &node, field, strlen(field), true, 0, 0, false)){
                TNOTE("qp_filter_data_insert error, return!");
                return -1;
            }
        }
        list->addFiltNode(node);

        return 0;
    }
}
}
