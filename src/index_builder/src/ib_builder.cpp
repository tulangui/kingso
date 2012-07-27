
#include <pthread.h>
#include <limits.h>
#include <stdlib.h>

#include "ib_builder.h"
#include "ib_ctx.h"
#include "ib_index.h"
#include "ib_detail.h"
#include "index_lib/IndexBuilder.h"
#include "profile_helper.h"


using namespace index_lib;

#define IB_TYPE_PROFILE      0
#define IB_TYPE_INDEX_TEXT   1
#define IB_TYPE_INDEX_NUM    2
#define IB_TYPE_DETAIL       3

#define IB_TEXT_INIT_SIZE  (1048576)
#define IB_TEXT_READ_COUNT 64

#define IB_NUM_INIT_SIZE  (1048576)
#define IB_NUM_READ_COUNT 64
#define IB_DETAIL_READ_COUNT 512

typedef struct bq_node_s{
    uint32_t type;
    char fn_pattern[PATH_MAX];
    char field_name[KSB_FIELDNAME_MAX];
}bq_node_t;

static bq_node_t ib_build_queue[KSB_FIELD_MAX];
static uint32_t ib_build_queue_size = KSB_FIELD_MAX;
static uint32_t ib_build_queue_ptr = 0;
static uint32_t ib_build_queue_count = 0;
static pthread_mutex_t ib_build_queue_mutex = PTHREAD_MUTEX_INITIALIZER;


typedef struct ib_build_thread_ctx_s{
    ib_builder_ctx_t *p_builder_ctx;
    pthread_t tid;
    int retval;
}ib_build_thread_ctx_t;


static int ib_build_create_queue(ib_builder_ctx_t *pctx){
    ksb_conf_t *conf = pctx->conf;

    if((pctx->p_ibctx->mode&MODE_DETAIL)!=0){
        //detail
        ib_build_queue[ib_build_queue_count].type = IB_TYPE_DETAIL;
        snprintf(ib_build_queue[ib_build_queue_count].fn_pattern,
                sizeof(ib_build_queue[ib_build_queue_count].fn_pattern),
                "%s/%s.gz",pctx->p_ibctx->tmp_dir,DETAIL_IDX_FILENAME);
        ib_build_queue[ib_build_queue_count].field_name[0]=0;
        ib_build_queue_count++;
    }

    if((pctx->p_ibctx->mode&MODE_SEARCHER)!=0){
        fprintf(stderr,"HASPROFILE!!! %d\n",pctx->p_ibctx->mode);
        //profile
        ib_build_queue[ib_build_queue_count].type = IB_TYPE_PROFILE;
        snprintf(ib_build_queue[ib_build_queue_count].fn_pattern,
                sizeof(ib_build_queue[ib_build_queue_count].fn_pattern),
                "%s/%s.gz",pctx->p_ibctx->tmp_dir,PROFILE_FILENAME_BP);
        ib_build_queue[ib_build_queue_count].field_name[0]=0;
        ib_build_queue_count++;

        //index
        uint32_t i;
        field_info_t *pinfo;
        for(i=0;i<conf->index_count;i++){
            if(ib_build_queue_count>=ib_build_queue_size){
                return -1;
            }
            pinfo = &conf->index[i];
            if(pinfo->bind_to){
                fprintf(stderr,"Skip bind field:%s.\n",pinfo->name);
                continue;
            }
            if(pinfo->package_only){
                fprintf(stderr,"Skip package only field:%s. \n",pinfo->name);
                continue;
            }
            if(pinfo->index_type == FIELD_TYPE_TEXT){
                ib_build_queue[ib_build_queue_count].type = IB_TYPE_INDEX_TEXT;
            }
            else{
                ib_build_queue[ib_build_queue_count].type = IB_TYPE_INDEX_NUM;
            }
            snprintf(ib_build_queue[ib_build_queue_count].fn_pattern,
                    sizeof(ib_build_queue[ib_build_queue_count].fn_pattern),
                    "%s/"INDEX_FILENAME_BP".gz",pctx->p_ibctx->tmp_dir,pinfo->name);
            snprintf(ib_build_queue[ib_build_queue_count].field_name,
                    sizeof(ib_build_queue[ib_build_queue_count].field_name),
                    "%s",pinfo->name);
            //        fprintf(stderr,"DEBUG: %s\n%s\n\n",
            //                ib_build_queue[ib_build_queue_count].fn_pattern,
            //                ib_build_queue[ib_build_queue_count].field_name);

            ib_build_queue_count++;
        }
    }
    fprintf(stderr,"%u fields added.\n",ib_build_queue_count);
    ib_build_queue_ptr = 0;
    return 0;
}

static IndexBuilder *ib_init_index_builder(ib_builder_ctx_t *pctx){
    IndexBuilder *pb = NULL;
    pb = IndexBuilder::getInstance();
    if(!pb){
        return NULL;
    }

    pb->setMaxDocNum(pctx->p_ibctx->doc_total);

    uint32_t i;
    int ret = 0;
    for(i = 0; i<pctx->conf->index_count;i++){
        if(pctx->conf->index[i].bind_to){
            continue;
        }  
        if(pctx->conf->index[i].package_only){
            continue;
        }
        ret = pb->addField(pctx->conf->index[i].name,
                (pctx->conf->index[i].index_type == FIELD_TYPE_TEXT)?TEXT_OCC_COUNT:0);
        if(ret<0){
            fprintf(stderr,"addField() failed. Field:%s\n",pctx->conf->index[i].name);
            return NULL;
        }
    }

    char path[PATH_MAX];
    snprintf(path,sizeof(path),"%s/index/",pctx->p_ibctx->output_dir);
    ret = pb->open(path);
    if(ret<0){
        pb = NULL;
        return NULL;
    }
    return pb;
}

static int ib_num_sn_compare(const void* t1, const void *t2){
    ib_num_sn_t *v1 = (ib_num_sn_t*)t1;
    ib_num_sn_t *v2 = (ib_num_sn_t*)t2;
    if(v1->node.sign > v2->node.sign){
        return 1;
    }
    if(v1->node.sign < v2->node.sign){
        return -1;
    }
    return 0;
}

static int ib_text_sn_compare(const void* t1, const void *t2){
    ib_text_sn_t *v1 = (ib_text_sn_t*)t1;
    ib_text_sn_t *v2 = (ib_text_sn_t*)t2;
    
    if(v1->node.sign>v2->node.sign){
        return 1;
    }
    if(v1->node.sign<v2->node.sign){
        return -1;
    }
    //sign equal
    if(v1->docid > v2->docid){
        return 1;
    }
    if(v1->docid < v2->docid){
        return -1;
    }
    if(v1->node.occ > v2->node.occ){
        return 1;
    }
    if(v1->node.occ < v2->node.occ){
        return -1;
    }
    return 0;

}

static int ib_do_build_index_text(bq_node_t *pnode,ib_build_thread_ctx_t *pctx,uint32_t split_part){
    //IndexBuilder *pib = ib_init_index_builder(pctx->p_builder_ctx);
    IndexBuilder *pib = IndexBuilder::getInstance();
    if(!pib){
        return -1;
    }

    ib_text_sn_t *plist = NULL;
    uint64_t list_count = 0;
    uint64_t list_size = IB_TEXT_INIT_SIZE;
    plist = (ib_text_sn_t*)malloc(sizeof(ib_text_sn_t)*list_size);
    if(!plist){
        return -1;
    }
    uint32_t docid = 0;
    uint32_t file_index = 0;
    gzFile fp = NULL;
    char filename[PATH_MAX];
    uint32_t term_count = 0;
    int failed = 0;
    uint32_t i = 0;
    int ret = 0;
    for(file_index = 0; file_index<pctx->p_builder_ctx->p_ibctx->thread_count; file_index++){
        snprintf(filename,sizeof(filename),pnode->fn_pattern,file_index,split_part);
//        fprintf(stderr,"OPENING:%s, DOCID=%u\n",filename,docid);
        fp = gzopen(filename,"rb");
        if(!fp){
            fprintf(stderr,"gzopen(\"%s\") failed,\n",filename);
            return -1;
        }
        while(1){
            ret = gzread(fp,&term_count,sizeof(term_count));
            if(ret==0){
                break;
            }
            if(ret!=sizeof(term_count)){
                fprintf(stderr,"file broken! 0\n");
                failed = 1;
                break;
            }
            if(list_count + term_count >= list_size) {
                ib_text_sn_t *ptmplist = plist;
                while (list_size < list_count + term_count) {
                    list_size*=2;
                }
                plist=(ib_text_sn_t*)realloc(ptmplist,list_size*sizeof(ib_text_sn_t));
                if(!plist){
                    fprintf(stderr,"Out of memory!\n");
                    return -1;
                }
            }
            ib_text_t terms[term_count];
            if (gzread(fp, terms, sizeof(terms)) != (int)sizeof(terms)) {
                fprintf(stderr,"file broken! 1\n");
                failed = 1;
                break;
            }
            for(i=0;i<term_count;i++){
                plist[list_count].docid = docid;
                plist[list_count].node = terms[i];
                ++list_count;
            }
            ++docid;
        }
        gzclose(fp);
        fp = NULL;
        if(failed){
            free(plist);
            plist = NULL;
            return -1;
        }
    }

    if(!list_count){
        free(plist);
        plist = NULL;
        return 0;
    }
    
    qsort(plist,list_count,sizeof(ib_text_sn_t),ib_text_sn_compare);

    DocListUnit *pdoclist = NULL;
    uint64_t doclist_count = 0;
    uint64_t doclist_size = IB_TEXT_INIT_SIZE;
    pdoclist = (DocListUnit*)malloc(sizeof(DocListUnit)*doclist_size);
    if(!pdoclist){
        free(plist);
        plist = NULL;
        return -1;
    }
    uint64_t sign;
    uint64_t last_sign = plist[0].node.sign;
    doclist_count = 1;
    pdoclist[0].doc_id = plist[0].docid;
    pdoclist[0].occ = plist[0].node.occ;
//    uint64_t ii;

    for(i = 1; i<list_count; i++){
        sign = plist[i].node.sign;
        if(sign!=last_sign){
            //call addTerm
            ret = pib->addTerm(pnode->field_name,last_sign,pdoclist,doclist_count);
            if(ret<=0){
                fprintf(stderr,"addTerm failed. line:%d\n",__LINE__);
/*                fprintf(stderr,"sign=%llu doclist_count = %lu\n",last_sign,doclist_count);
                for(ii=0;ii<doclist_count;ii++){
                    fprintf(stderr,"%d.%u ",pdoclist[ii].doc_id,(unsigned int)pdoclist[ii].occ);
                }
                fprintf(stderr,"\n\n");*/
            }
            else{

            }
            //fprintf(stderr,"field:%s sign:%llu count:%u\n",pnode->field_name,last_sign,doclist_count);

            doclist_count = 0;
            last_sign = sign;
        }
        if(doclist_count>=doclist_size){
            doclist_size*=2;
            DocListUnit * newdoclist = (DocListUnit*)realloc(pdoclist,sizeof(DocListUnit)*doclist_size);
            if (newdoclist == NULL) {
                fprintf(stderr, "realloc memory failed:%d\n", __LINE__);
                break;
            }
            else {
                pdoclist = newdoclist;
            }
        }
        pdoclist[doclist_count].doc_id = plist[i].docid;
        pdoclist[doclist_count].occ = plist[i].node.occ;
        //fprintf(stderr,"id:%d occ:%d\n",plist[i].docid,plist[i].node.occ);
        doclist_count++;
    }

    if(doclist_count>0){
        ret = pib->addTerm(pnode->field_name,last_sign,pdoclist,doclist_count);
        if(ret<=0){
            fprintf(stderr,"addTerm failed. line:%d\n",__LINE__);
/*            fprintf(stderr,"sign=%llu doclist_count = %lu\n",last_sign,doclist_count);
            for(ii=0;ii<doclist_count;ii++){
                fprintf(stderr,"%d.%u ",pdoclist[ii].doc_id,(unsigned int)pdoclist[ii].occ);
            }
            fprintf(stderr,"\n\n");*/
        }
    }
    free(plist);
    plist = NULL;
    free(pdoclist);
    pdoclist = NULL;

//    fprintf(stderr,"Index field:%s . %u docs, %d nodes\n",pnode->field_name,docid,list_count);
    return 0;
}

static int ib_do_build_detail(bq_node_t *pnode,ib_ctx_t *pctx){
    char filename[PATH_MAX];
    uint32_t i;
    gzFile fp = NULL;
    gzFile fp_out = NULL;
    uint64_t real_offset = 0;

    snprintf(filename,sizeof(filename),"%s/detail/detail.idx.gz",pctx->output_dir);
    fp_out = gzopen(filename,"w+b1");
    if(!fp_out){
        fprintf(stderr,"Can not open %s for write.\n",filename);
        return -1;
    }
    for(i=0;i<pctx->thread_count;i++){
        snprintf(filename,sizeof(filename),pnode->fn_pattern,i);
        fp = gzopen(filename,"rb");
        if(!fp){
            fprintf(stderr,"Can not open %s for read.\n",filename);
            gzclose(fp_out);
            fp_out = NULL;
            return -1;
        }

        int max_read_bytes = IB_DETAIL_READ_COUNT * sizeof(ib_detail_idx_t);
        char buf[max_read_bytes];
        int read_bytes = 0;
        while ((read_bytes = gzread(fp, (void *) buf, max_read_bytes)) > 0) {
            int size = read_bytes / sizeof(ib_detail_idx_t);
            ib_detail_idx_t *idxs = (ib_detail_idx_t *) buf;
            for (int j = 0; j < size; j++) {
                if (gzprintf(fp_out,"%lld %llu %llu\n", idxs[j].nid, real_offset, idxs[j].len) <= 0) {
                    return -1;
                }
                real_offset += idxs[j].len;
            }
        }
        gzclose(fp);
        fp = NULL;
    }
    gzclose(fp_out);
    return 0;
}

static int ib_do_build_index_str(bq_node_t *pnode,ib_build_thread_ctx_t *pctx, uint32_t split_part){
    //IndexBuilder *pib = ib_init_index_builder(pctx->p_builder_ctx);
    IndexBuilder *pib = IndexBuilder::getInstance();
    if(!pib){
        return -1;
    }

    ib_num_sn_t *plist = NULL;
    uint64_t list_count = 0;
    uint64_t list_size = IB_NUM_INIT_SIZE;
    plist = (ib_num_sn_t*)malloc(sizeof(ib_num_sn_t)*list_size);
    if(!plist){
        return -1;
    }
    uint32_t docid = 0;
    uint32_t file_index = 0;
    gzFile fp = NULL;
    char filename[PATH_MAX];
    uint32_t term_count = 0;
    int failed = 0;
    uint32_t i = 0;
    int ret = 0;
    for(file_index = 0; file_index<pctx->p_builder_ctx->p_ibctx->thread_count; file_index++){
        snprintf(filename,sizeof(filename),pnode->fn_pattern,file_index,split_part);
//        fprintf(stderr,"OPENING:%s, DOCID=%u\n",filename,docid);
        fp = gzopen(filename,"rb");
        if(!fp){
            fprintf(stderr,"gzopen(\"%s\") failed,\n",filename);
            free(plist);
            plist = NULL;
            return -1;
        }
        while(1){
            ret = gzread(fp,&term_count,sizeof(term_count));
            if(ret==0){
                break;
            }
            if(ret!=sizeof(term_count)){
                fprintf(stderr,"file broken! 0\n");
                failed = 1;
                break;
            }
            if(list_count + term_count >= list_size) {
                ib_num_sn_t *ptmplist = plist;
                while (list_size < list_count + term_count) {
                    list_size*=2;
                }
                plist=(ib_num_sn_t*) realloc(ptmplist, list_size*sizeof(ib_num_sn_t));
                if(!plist){
                    fprintf(stderr,"Out of memory!\n");
                    return -1;
                }
            }
            ib_num_t terms[term_count];
            if (gzread(fp, terms, sizeof(terms)) != (int)sizeof(terms)) {
                failed = 1;
                break;
            }
            for(i=0;i<term_count;i++){
                plist[list_count].docid = docid;
                plist[list_count].node = terms[i];
                ++list_count;
            }
            ++docid;
        }
        gzclose(fp);
        fp = NULL;
        if(failed){
            free(plist);
            plist = NULL;
            return -1;
        }
    }

    if(!list_count){
        free(plist);
        plist = NULL;
        return 0;
    }
    
    qsort(plist,list_count,sizeof(ib_num_sn_t),ib_num_sn_compare);

    uint32_t *pdoclist = NULL;
    uint64_t doclist_count = 0;
    uint64_t doclist_size = IB_NUM_INIT_SIZE;
    pdoclist = (uint32_t*)malloc(sizeof(uint32_t)*doclist_size);
    if(!pdoclist){
        free(plist);
        plist = NULL;
        return -1;
    }
    uint64_t sign;
    uint64_t last_sign = plist[0].node.sign;
    doclist_count = 1;
    pdoclist[0] = plist[0].docid;
    uint32_t ii;

    for(i = 1; i<list_count; i++){
        sign = plist[i].node.sign;
        if(sign!=last_sign){
            //call addTerm
            ret = pib->addTerm(pnode->field_name,last_sign,pdoclist,doclist_count);
            if(ret<=0){
                fprintf(stderr,"addTerm failed no occ.\n");
                fprintf(stderr,"sign=%lu doclist_count = %lu\n",last_sign,doclist_count);
                for(ii=0;ii<doclist_count;ii++){
                    fprintf(stderr,"%d ",pdoclist[ii]);
                }
                fprintf(stderr,"\n\n");
            }
//            fprintf(stderr,"field:%s sign:%llu count:%u\n",pnode->field_name,last_sign,doclist_count);

            doclist_count = 0;
            last_sign = sign;
        }
        if(doclist_count>=doclist_size){
            doclist_size*=2;
            uint32_t * newDoclist = (uint32_t*)realloc(pdoclist,sizeof(uint32_t)*doclist_size);
            if (newDoclist == NULL) {
                fprintf(stderr, "realloc memory failed:%d\n", __LINE__);
                break;
            }
            else {
                pdoclist = newDoclist;           
            }
        }
        pdoclist[doclist_count] = plist[i].docid;
        doclist_count++;
    }

    if(doclist_count>0){
//        fprintf(stderr,"call addTerm. last_sign=%llu\n",last_sign);
        ret = pib->addTerm(pnode->field_name,last_sign,pdoclist,doclist_count);
        if(ret<=0){
            fprintf(stderr,"addTerm failed. line:%d\n",__LINE__);
            fprintf(stderr,"sign=%lu doclist_count = %lu\n",last_sign,doclist_count);
            for(ii=0;ii<doclist_count;ii++){
                fprintf(stderr,"%d ",pdoclist[ii]);
            }
            fprintf(stderr,"\n\n");
        }
    }
    free(pdoclist);
    pdoclist=NULL;
    free(plist);
    plist = NULL;
    fprintf(stderr,"Index field:%s . %u docs, %lu nodes\n",pnode->field_name,docid,list_count);
    return 0;
}

static void *ib_build_thread(void *pdata){
    ib_build_thread_ctx_t *pctx = (ib_build_thread_ctx_t*)pdata;
    pctx->retval = 0;
    int failed = 0;
    bq_node_t *pnode = NULL;
    int ret = 0;
    uint32_t j;
    while(1){
        pthread_mutex_lock(&ib_build_queue_mutex);
        if(ib_build_queue_ptr>=ib_build_queue_count){
            pthread_mutex_unlock(&ib_build_queue_mutex);
            break;
        }
        pnode = &ib_build_queue[ib_build_queue_ptr];
        ib_build_queue_ptr++;
        pthread_mutex_unlock(&ib_build_queue_mutex);
//        fprintf(stderr,"FIELD:%s\n",pnode->field_name);
        if(pnode->type == IB_TYPE_PROFILE){
            ret = do_build_profile(pctx->p_builder_ctx->p_ibctx);
            if(ret !=0){
                fprintf(stderr,"do_build_profile() failed.\n");
                pctx->retval=-1;
                break;
            }
            fprintf(stderr,"do_build_profile() finished.\n");
        }
        else if(pnode->type == IB_TYPE_INDEX_TEXT){
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                ret = ib_do_build_index_text(pnode,pctx,j);
                if(ret!=0){
                    fprintf(stderr,"ib_do_build_index_text() failed.\n");
                    pctx->retval=-1;
                    failed =1;
                    break;
                }
            }
            if(!failed){
                fprintf(stderr,"ib_do_build_index_text() finished.\n");
            }
            else{
                break;
            }
        }
        else if(pnode->type == IB_TYPE_INDEX_NUM){
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                ret = ib_do_build_index_str(pnode,pctx,j);
                if(ret!=0){
                    fprintf(stderr,"ib_do_build_index_str() failed.\n");
                    pctx->retval=-1;
                    failed = 1;
                    break;
                }
            }
            if(!failed){
                fprintf(stderr,"ib_do_build_index_str() finished.\n");
            }
            else{
                break;
            }
        }
        else if(pnode->type == IB_TYPE_DETAIL){
            ret = ib_do_build_detail(pnode,pctx->p_builder_ctx->p_ibctx);
            if(ret !=0){
                fprintf(stderr,"ib_do_build_detail() failed.\n");
                pctx->retval=-1;
                break;
            }
            fprintf(stderr,"ib_do_build_detail() finished.\n");
        }
        else{
            fprintf(stderr,"Invalid type! %s:%u\n",pnode->field_name,pnode->type);
            continue;
        }
    }

    return NULL;
}



int ib_build(ib_builder_ctx_t *pctx){
    int ret = 0;
    
    ib_build_queue_count = 0;
    ib_build_queue_ptr = 0;
    ret = ib_build_create_queue(pctx);
    if(ret!=0){
        fprintf(stderr,"ib_build_create_queue() failed.\n");
        return -1;
    }

    ib_build_thread_ctx_t *ctxs = NULL;
    ctxs = (ib_build_thread_ctx_t*)malloc(sizeof(ib_build_thread_ctx_t)*pctx->p_ibctx->thread_count);
    if(!ctxs){
        fprintf(stderr,"Out of memory.\n");
        return -1;
    }
    memset(ctxs,0,sizeof(ib_build_thread_ctx_t)*pctx->p_ibctx->thread_count);

    IndexBuilder *pib = NULL;
    if (pctx->p_ibctx->mode != MODE_DETAIL) {
        pib = ib_init_index_builder(pctx);
        if(!pib){
            fprintf(stderr,"ib_init_index_builder() failed.\n");
            free(ctxs);
            return -1;
        }
    }


    uint32_t i;
    for(i = 0; i < pctx->p_ibctx->thread_count; i++){

        ctxs[i].p_builder_ctx=pctx;
        ret = pthread_create(&ctxs[i].tid,NULL,ib_build_thread,&ctxs[i]);
        if(ret!=0){
            fprintf(stderr,"pthread_create() failed\n");
            return -1;
        }
    }

    int int_ret_val = 0;
    void *ret_val = NULL;
    for(i = 0; i < pctx->p_ibctx->thread_count; i++){
        ret_val = NULL;
        ret = pthread_join(ctxs[i].tid,&ret_val);
        if(ret!=0){
            fprintf(stderr,"pthread_join() failed.\n");
            continue;
        }
        if(ctxs[i].retval!=0){
            int_ret_val = -1;
        }
    }

    if (pctx->p_ibctx->mode != MODE_DETAIL) {
        ret = pib->dump();
        if(ret<0){
            fprintf(stderr,"IndexBuilder dump() failed.\n");
            return -1;
        }
        pib->close();
    }
    return int_ret_val;    
}

