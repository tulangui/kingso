#include <mxml.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>

#include "util/crc.h"
#include "util/strfunc.h"
#include "ks_build/ks_build.h"
#include "index_lib/ProvCityManager.h"
#include "match_tree.h"
#include "ksb_stopword.h"

typedef struct ksb_on_node_ctx_s {
    field_info_t *pvector;
    void *pmap;
    uint32_t *pcount;
    ITokenizer **pTokenizer;
    uint32_t size;
} ksb_on_node_ctx_t;

const conf_node_t ksb_profile_conf_path[] = { {"ks"}, {"profile"} };
const conf_node_t ksb_detail_conf_path[] = { {"ks"}, {"detail"} };
const conf_node_t ksb_index_conf_path[] = { {"ks"}, {"index"} };
const conf_node_t ksb_aliws_path[] = { {"ks"}, {"tokenizer"} };
const conf_node_t ksb_provcity_path[] = { {"ks"}, {"provcity"} };
const conf_node_t ksb_stopword_path[] = { {"ks"}, {"stopword"} };

const char *ksb_aliws_path_conf_name = "conf_path";
const char *ksb_aliws_ws_conf_name = "ws";
const char *ksb_provcity_dict_conf_name = "dict";
const char *ksb_stopword_dict_conf_name = "dict";

static pthread_mutex_t ksb_provcity_mutex = PTHREAD_MUTEX_INITIALIZER;

static CTokenizerFactory ksb_tokenizer_factory;

static field_type_t
ksb_type_names[] = { {"text", FIELD_TYPE_TEXT},
{"str", FIELD_TYPE_STR},
{"nick", FIELD_TYPE_NICK},
{"loc", FIELD_TYPE_LOC},
{"postfee",FIELD_TYPE_POSTFEE},
{"csttime", FIELD_TYPE_CSTTIME},
{"latlng", FIELD_TYPE_LATLNG}
};

static uint32_t
    ksb_type_names_count = sizeof(ksb_type_names) / sizeof(field_type_t);

typedef int (*FUNC_ON_NODE) (void *ctx, mxml_node_t * pnode);

/*
static uint64_t
ksb_fieldname_hash(const void *key) {
    assert(key);
    return get_crc64((const char *) key, strlen((const char *) key));
}

static int32_t
ksb_fieldname_compare(const void *key1, const void *key2) {
    assert(key1 && key2);
    return strcmp((const char *) key1, (const char *) key2);
}
*/

static ksb_conf_t *
ksb_init_conf() {
    ksb_conf_t *
        conf;
    conf = (ksb_conf_t *) malloc(sizeof(ksb_conf_t));
    if (!conf) {
        return NULL;
    }
    memset(conf, 0, sizeof(ksb_conf_t));

    conf->index_map = match_tree_create();
    conf->profile_map = match_tree_create();
    conf->detail_map = match_tree_create();

    if (!conf->index_map || !conf->profile_map || !conf->detail_map) {
        return NULL;
    }
    return conf;
}

static mxml_node_t *
ksb_get_conf_node(mxml_node_t * xml, const conf_node_t * path, int levels) {
    int
        i = 0;
    mxml_node_t *
        pnode = NULL;
    int
        found = 0;
    pnode = xml;
    for (i = 0; i < levels; i++) {
        found = 0;
        for (; pnode; pnode = pnode->next) {
            if (pnode->type != MXML_ELEMENT) {
                continue;
            }
            if (strcasecmp(path[i].name, pnode->value.element.name) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return NULL;
        }
        if (i < levels - 1) {   //最后一个了
            pnode = pnode->child;
        }
    }
    return pnode;
}

static int
ksb_get_items_by_name(mxml_node_t * pnode, const char *name,
                      FUNC_ON_NODE cb, void *ctx) {
    if (!pnode || !name || !cb) {
        return -1;
    }

    mxml_node_t *
        pcur = NULL;
    for (pcur = pnode; pcur; pcur = pcur->next) {
        if (pcur->child) {
            ksb_get_items_by_name(pcur->child, name, cb, ctx);
        }
        if (pcur->type != MXML_ELEMENT) {
            continue;
        }
        if (strcasecmp(name, pcur->value.element.name) == 0) {
            if (cb(ctx, pcur) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

static int
ksb_cb_on_package(void *data, mxml_node_t * pnode) {
    ksb_on_node_ctx_t *
        pctx = (ksb_on_node_ctx_t *) data;
    if (!pctx) {
        return -1;
    }

    if (pctx->size <= (*pctx->pcount)) { //full
        return -1;
    }

    const char *
        name = mxmlElementGetAttr(pnode, "name");

    const char *
        fields = mxmlElementGetAttr(pnode,"fields");
    const char *
        tokenizerType = mxmlElementGetAttr(pnode,"tokenizer_type");
    if(!name || !fields){
        return -1;
    }
    tokenizerType = (tokenizerType == NULL) ? TOKENIZER_TYPE_METACHAR : tokenizerType; 
    char *str_fields = strdup(fields);
    int ret = 0;
    int f_count = 0;
    int i = 0;
    char *items[PACKAGE_FIELD_MAX];
    char *infos[PACKAGE_FIELD_INFOS];
    f_count = split_multi_delims(str_fields,items,PACKAGE_FIELD_MAX,",");
    if(f_count<=0){
        free(str_fields);
        return 0;
    }

    package_info_t* p_pack_info = NULL;
    p_pack_info = (package_info_t*) malloc(sizeof(package_info_t));
    if(!p_pack_info){
        fprintf(stderr,"Out of memory!");
        free(str_fields);
        return -1;
    }
    memset(p_pack_info,0,sizeof(package_info_t));

    snprintf(p_pack_info->name,KSB_FIELDNAME_MAX,PACKAGE_NAME_PREFIX"%s",name);

    for(i = 0; i<f_count; i++){
        ret = split_multi_delims(items[i],infos,PACKAGE_FIELD_INFOS,":");
        if(ret != PACKAGE_FIELD_INFOS){
            fprintf(stderr,"Invalid package field info.\n");
            free(str_fields);
            free(p_pack_info);
            return -1;
        }
        snprintf(p_pack_info->fields[i].name,KSB_FIELDNAME_MAX,"%s",infos[0]);
        p_pack_info->fields[i].occ = strtoul(infos[1],NULL,10);
        p_pack_info->fields[i].real_occ = strtoul(infos[2],NULL,10);
    }
    p_pack_info->fields_count = f_count;
    field_info_t *pinfo;

    uint64_t field_index;
    for(i = 0; i<(int)(p_pack_info->fields_count); i++){
        field_info_t *pinfo = NULL;
        fprintf(stderr,"PACKAGE:%s FIELD:%s\n",p_pack_info->name,p_pack_info->fields[i].name);
        field_index = (uint64_t)match_tree_find((match_tree_t*)pctx->pmap,p_pack_info->fields[i].name);
        if(field_index==0){//索引中无此字段，需要添加
            pinfo = &pctx->pvector[(*pctx->pcount)];
            snprintf(pinfo->name,KSB_FIELDNAME_MAX,"%s",p_pack_info->fields[i].name);
            pinfo->index_type = FIELD_TYPE_TEXT;
            pinfo->allow_null = 1;
            pinfo->to_utf8 = 1;
            pinfo->value_delim=' ';
            pinfo->output_delim=' ';
            pinfo->package_only = 1;
            pctx->pTokenizer[(*pctx->pcount)] = ksb_tokenizer_factory.getTokenizer(tokenizerType);
            match_tree_insert((match_tree_t*)pctx->pmap,pinfo->name,(void *) (uint64_t) ((*pctx->pcount) + FIELD_INDEX_ADD));
            (*pctx->pcount)++;
        }
        else{
            pinfo = &pctx->pvector[field_index-FIELD_INDEX_ADD];
        }
        package_ext_t *p_ext = NULL;
        p_ext = (package_ext_t*)malloc(sizeof(package_ext_t));
        if(!p_ext){
            fprintf(stderr,"Out of memory\n");
            free(str_fields);
            return -1;
        }
        p_ext->p_package_info = p_pack_info;
        p_ext->index_in_package = i;
        p_ext->next = pinfo->package_ext_head;
        pinfo->package_ext_head = p_ext;
    }
    free(str_fields);

    pinfo = &pctx->pvector[(*pctx->pcount)];
    snprintf(pinfo->name,KSB_FIELDNAME_MAX,"%s",p_pack_info->name);
    pinfo->index_type = FIELD_TYPE_TEXT;
    pinfo->allow_null = 1;
    pinfo->to_utf8 = 1;
    pinfo->value_delim=' ';
    pinfo->output_delim=' ';
    pinfo->is_package = 1;
    p_pack_info->package_index = (*pctx->pcount);
    (*pctx->pcount)++;
    return 0;
}

static int
ksb_cb_on_node(void *data, mxml_node_t * pnode) {
//    fprintf(stdout,"Field:%s\n",mxmlElementGetAttr(pnode,"name"));

    ksb_on_node_ctx_t *
        pctx = (ksb_on_node_ctx_t *) data;
    if (!pctx) {
        return -1;
    }

    if (pctx->size <= (*pctx->pcount)) { //full
        return -1;
    }

    uint32_t
        index = (*pctx->pcount);
    field_info_t *
        pinfo = &pctx->pvector[index];
    unsigned int
        i = 0;
    int
        found = 0;

    const char *
        name = mxmlElementGetAttr(pnode, "name");
    const char *
        index_type = mxmlElementGetAttr(pnode, "index_type");
//    const char *trans_type = mxmlElementGetAttr(pnode,"trans_type");
//    const char *indexType = mxmlElementGetAttr(pnode,"index_type");
    const char *
        allow_null = mxmlElementGetAttr(pnode, "allow_null");
    const char *
        value_delim = mxmlElementGetAttr(pnode, "value_delim");
    const char *
        to_utf8 = mxmlElementGetAttr(pnode, "gbk2utf8");
    const char *
        need_sort = mxmlElementGetAttr(pnode, "need_sort"); // prop sort add by tiechou
    const char *
        str_bind_to = mxmlElementGetAttr(pnode, "bind_to");
    const char *
        str_bind_occ = mxmlElementGetAttr(pnode, "bind_occ");
    const char *
        str_bind_real_occ = mxmlElementGetAttr(pnode, "bind_real_occ");
    const char *
        output_delim = mxmlElementGetAttr(pnode, "output_delim");

//    const char *group = mxmlElementGetAttr(pnode,"group");
    const char *
        tokenizerType = mxmlElementGetAttr(pnode,"tokenizer_type");
    tokenizerType = (tokenizerType == NULL) ? TOKENIZER_TYPE_METACHAR : tokenizerType; 
    snprintf(pinfo->name, KSB_FIELDNAME_MAX, "%s", name);

    if (index_type) {
        found = 0;
        for (i = 0; i < ksb_type_names_count; i++) {
            if (strcasecmp(index_type, ksb_type_names[i].type_name) == 0) {
                found = 1;
                pinfo->index_type = ksb_type_names[i].type;
                break;
            }
        }
        if (!found) {
            return -1;
        }
    }
    else {
        pinfo->index_type = FIELD_TYPE_STR;
    }
    if (pctx->pTokenizer != NULL) {
        pctx->pTokenizer[index] = ksb_tokenizer_factory.getTokenizer(tokenizerType);
    }
    if (allow_null && (allow_null[0] == 'n' || allow_null[0] == 'N')) {
        pinfo->allow_null = 0;
    }
    else {
        pinfo->allow_null = 1;
    }

    if (to_utf8 && (to_utf8[0] == 'y' || to_utf8[0] == 'Y')) {
        pinfo->to_utf8 = 1;
    }
    else {
        pinfo->to_utf8 = 0;
    }

    // prop sort add by tiechou
    if (need_sort && (need_sort[0] == 'y' || need_sort[0] == 'Y')) {
        pinfo->need_sort = 1;
    }
    else {
        pinfo->need_sort = 0;
    }

    if (value_delim && value_delim[0] != 0) {
        pinfo->value_delim = value_delim[0];
    }
    else {
        pinfo->value_delim = ' ';
    }

    if(output_delim && output_delim[0] != 0) {
        pinfo->output_delim = output_delim[0];
    }
    else{
        pinfo->output_delim = ' ';
    }


    if(str_bind_to){
        if(!str_bind_occ || pinfo->index_type!=FIELD_TYPE_TEXT ){
            return -1;
        }
        pinfo->bind_to_name=strdup(str_bind_to);
        pinfo->bind_occ = strtol(str_bind_occ,NULL,10);
        if(str_bind_real_occ){
            pinfo->bind_real_occ = strtol(str_bind_real_occ, NULL, 10);
        }
    }

    if (match_tree_insert((match_tree_t*)pctx->pmap,pinfo->name,(void *) (uint64_t) (index + FIELD_INDEX_ADD))!=0){
        return -1;
    }
/*    if (hashmap_insert
        (pctx->pmap, pinfo->name,
         (void *) (uint64_t) (index + FIELD_INDEX_ADD)) != 0) {
        return -1;
    }*/
    ++(*pctx->pcount);
    return 0;
}

static int
ksb_init_stopword(mxml_node_t * xml, ksb_conf_t * conf) {
    const char *
        stopword_path = NULL;
    mxml_node_t *
        pnode_stopword = NULL;
    pnode_stopword =
        ksb_get_conf_node(xml->child, ksb_stopword_path,
                          sizeof(ksb_stopword_path) / sizeof(conf_node_t));
    if (!pnode_stopword) {
        fprintf(stderr, "Can not find stopword in xml.\n");
        return -1;
    }

    stopword_path =
        mxmlElementGetAttr(pnode_stopword, ksb_stopword_dict_conf_name);
    if (!stopword_path) {
        fprintf(stderr, "Get stopword dict path failed.\n");
        return -1;
    }

    if (ksb_load_stopword(stopword_path, conf) != 0) {
        fprintf(stderr, "ksb_load_stopword() failed.\n");
        return -1;
    }

    return 0;
}

static int
ksb_init_postfee(ksb_conf_t * conf) {
    conf->postfee = index_lib::RealPostFeeManager::getInstance();
    if(!conf->postfee){
        return -1;
    }
    return 0;
}

static int
ksb_init_latlng(ksb_conf_t * conf) {
    conf->latlng = index_lib::LatLngManager::getInstance();
    if (!conf->latlng) {
        return -1;
    }
    return 0;
}

static int
ksb_init_provcity(mxml_node_t * xml, ksb_conf_t * conf) {
    const char *
        provcity_path = NULL;
    mxml_node_t *
        pnode_provcity = NULL;
    pnode_provcity =
        ksb_get_conf_node(xml->child, ksb_provcity_path,
                          sizeof(ksb_provcity_path) / sizeof(conf_node_t));
    if (!pnode_provcity) {
        fprintf(stderr, "Can not find provcity in xml.\n");
        return -1;
    }

    provcity_path =
        mxmlElementGetAttr(pnode_provcity, ksb_provcity_dict_conf_name);
    if (!provcity_path) {
        fprintf(stderr, "Get provcity dict path failed.\n");
        return -1;
    }

    pthread_mutex_lock(&ksb_provcity_mutex);
    conf->provcity = index_lib::ProvCityManager::getInstance();
    if (!conf->provcity) {
        fprintf(stderr, "ProvCityManager::getInstance() failed.\n");
        //delete conf->provcity;
        //index_lib::ProvCityManager::destroy();
        conf->provcity = NULL;
        pthread_mutex_unlock(&ksb_provcity_mutex);
        return -1;
    }

    if (!conf->provcity->load(provcity_path)) {
        fprintf(stderr, "ProvCityManager load dict failed. path:%s\n",
                provcity_path);
        //delete conf->provcity;
        //index_lib::ProvCityManager::destroy();
        conf->provcity = NULL;
        pthread_mutex_unlock(&ksb_provcity_mutex);
        return -1;
    }
    pthread_mutex_unlock(&ksb_provcity_mutex);

    return 0;
}

static int
ksb_init_tokenizer(mxml_node_t * xml, ksb_conf_t * conf) {
    const char *
        ws_conf = NULL;
    const char *
        ws_type = NULL;
    mxml_node_t *
        pnode_aliws = NULL;
    pnode_aliws =
        ksb_get_conf_node(xml->child, ksb_aliws_path,
                          sizeof(ksb_aliws_path) / sizeof(conf_node_t));
    if (!pnode_aliws) {
        fprintf(stderr, "Can not find aliws conf in xml.\n");
        return -1;
    }

    ws_conf = mxmlElementGetAttr(pnode_aliws, ksb_aliws_path_conf_name);
    if (!ws_conf) {
        fprintf(stderr, "Aliws conf path not found.\n");
        return -1;
    }

    ws_type = mxmlElementGetAttr(pnode_aliws, ksb_aliws_ws_conf_name);
    if (!ws_type) {
        fprintf(stderr, "Aliws conf ws not found.\n");
        return -1;
    }

    if (ksb_tokenizer_factory.init(ws_conf, ws_type) < 0) {
        fprintf(stderr, "Can not init aliws.\n");
        return -1;
    }
    return 0;
}

ksb_conf_t *
ksb_load_config(const char *conf_file,uint32_t mode) {
    FILE *
        xml_fp = fopen(conf_file, "rb");
    if (!xml_fp) {
        fprintf(stderr, "Can not open %s for read.\n", conf_file);
        return NULL;
    }

    uint32_t detail_nid_index = 0;

    mxml_node_t *
        xml = mxmlLoadFile(NULL, xml_fp, MXML_NO_CALLBACK);
    if (!xml || !xml->child) {
        fprintf(stderr, "mxmlLoadFile() failed.\n");
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }

    ksb_conf_t *
        conf = ksb_init_conf();
    if (!conf) {
        fprintf(stderr, "ksb_init_conf() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }

    if (ksb_init_stopword(xml, conf) != 0) {
        fprintf(stderr, "ksb_init_stopword() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }


    if (ksb_init_postfee(conf) != 0) {
        fprintf(stderr, "ksb_init_postfee() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }

    if (ksb_init_latlng(conf) != 0) {
        fprintf(stderr, "ksb_init_latlng() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }

    if (ksb_init_provcity(xml, conf) != 0) {
        fprintf(stderr, "ksb_init_provcity() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }

    if (ksb_init_tokenizer(xml, conf) != 0) {
        fprintf(stderr, "ksb_init_tokenizer() failed.\n");
        mxmlRelease(xml);
        xml = NULL;
        fclose(xml_fp);
        xml_fp = NULL;
        return NULL;
    }
    fprintf(stderr, "Aliws Inited.\n");

    // set time zone
    tzset();

    mxml_node_t *
        pnode_profile = NULL;
    mxml_node_t *
        pnode_index = NULL;
    mxml_node_t *
        pnode_detail = NULL;

    if((mode&MODE_SEARCHER)!=0){
            //index
            pnode_index =
            ksb_get_conf_node(xml->child, ksb_index_conf_path,
                sizeof(ksb_index_conf_path) /
                sizeof(conf_node_t));

            //profile
            pnode_profile =
            ksb_get_conf_node(xml->child, ksb_profile_conf_path,
                sizeof(ksb_profile_conf_path) /
                sizeof(conf_node_t));
    }

    if((mode&MODE_DETAIL)!=0){
        //detail
        pnode_detail =
            ksb_get_conf_node(xml->child, ksb_detail_conf_path,
                    sizeof(ksb_detail_conf_path) /
                    sizeof(conf_node_t));
    }

    ksb_on_node_ctx_t
        ctx;

    uint32_t i;
    uint64_t bind_index = 0;
    int32_t ret = 0;

    if (pnode_index) {
        ctx.pvector = conf->index;
        ctx.pmap = conf->index_map;
        ctx.pcount = &conf->index_count;
        ctx.pTokenizer = conf->pTokenizer;
        ctx.size = KSB_FIELD_MAX;
        ret = ksb_get_items_by_name(pnode_index->child, "field", ksb_cb_on_node,
                              &ctx);
        if(ret!=0){
            return NULL;
        }
        for(i=0;i<conf->index_count;i++){
            if(conf->index[i].bind_to_name){
                bind_index = (uint64_t)match_tree_find((match_tree_t*)conf->index_map,
                                    conf->index[i].bind_to_name);
                if(bind_index==0){
                    fprintf(stderr,"Can not find bind field %s.\n",conf->index[i].bind_to_name);
                    mxmlRelease(xml);
                    xml = NULL;
                    fclose(xml_fp);
                    xml_fp = NULL;
                    return NULL;
                }
                if(bind_index == i){
                    fprintf(stderr,"Can not find bind to itself! %s\n",conf->index[i].bind_to_name);
                    mxmlRelease(xml);
                    xml = NULL;
                    fclose(xml_fp);
                    xml_fp = NULL;
                    return NULL;
                }
                free(conf->index[i].bind_to_name);
                conf->index[i].bind_to=bind_index;
                conf->index_bind_fields[conf->index_bind_count]=i;
                conf->index_bind_count++;
            }
        }
        ret = ksb_get_items_by_name(pnode_index->child, "package", ksb_cb_on_package,
                              &ctx);
        if(ret!=0){
            return NULL;
        }
    }

    if (pnode_profile) {
        ctx.pvector = conf->profile;
        ctx.pmap = conf->profile_map;
        ctx.pcount = &conf->profile_count;
        ctx.pTokenizer = NULL;
        ctx.size = KSB_FIELD_MAX;
        ksb_get_items_by_name(pnode_profile->child, "field",
                              ksb_cb_on_node, &ctx);
    }

    if (pnode_detail) {
        ctx.pvector = conf->detail;
        ctx.pmap = conf->detail_map;
        ctx.pcount = &conf->detail_count;
        ctx.pTokenizer = NULL;
        ctx.size = KSB_FIELD_MAX;
        ksb_get_items_by_name(pnode_detail->child, "field", ksb_cb_on_node,
                              &ctx);
        for(detail_nid_index=0;detail_nid_index<conf->detail_count;detail_nid_index++){
            if(strcasecmp(conf->detail[detail_nid_index].name,"nid")==0){
                break;
            }
        }
        if(detail_nid_index>=conf->detail_count){
            mxmlRelease(xml);
            xml = NULL;
            fclose(xml_fp);
            xml_fp = NULL;
            ksb_destroy_config(conf);
            conf = NULL;
            fprintf(stderr,"Detail must have field 'nid'\n");
            return NULL;
        }
        else{
            conf->detail_nid_index = detail_nid_index;
        }
    }
    mxmlRelease(xml);
    xml = NULL;
    fclose(xml_fp);
    xml_fp = NULL;

    return (ksb_conf_t *) conf;
}

void
ksb_destroy_config(ksb_conf_t * conf) {
    ksb_conf_t *
        pconf = (ksb_conf_t *) conf;
    // conf->tokenizer->ReleaseSegResult(conf->token_result);
    for (int i = 0; i < KSB_FIELD_MAX; ++i)
    {
        if (conf->pTokenizer[i] != NULL)
        {
            delete conf->pTokenizer[i];
            conf->pTokenizer[i] = NULL;
        }
    }
    match_tree_destroy((match_tree_t*)pconf->index_map);
    match_tree_destroy((match_tree_t*)pconf->profile_map);
    match_tree_destroy((match_tree_t*)pconf->detail_map);
    free(pconf);
    return;
} 
