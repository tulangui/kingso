#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <stdint.h>
#include <limits.h>
#include <zlib.h>

#include "ks_build/ks_build.h"
#include "xml_parser.h"
#include "ib_index.h"
#include "ib_profile.h"
#include "ib_detail.h"

#define XP_FILES_SIZE 32
#define XP_GETS_SIZE (1024*1024)

pthread_mutex_t g_files_mutex = PTHREAD_MUTEX_INITIALIZER;
static char **g_files;
static uint32_t g_files_ptr;
static uint32_t g_files_count;
static uint32_t g_files_size;

void xp_cleanup_input_queue(ib_ctx_t *p_ibctx){
    for(unsigned int i = 0; i < g_files_count; i++) {
        free(g_files[i]);
    }
    free(g_files);
}

static const char *xp_pop_file(int pos = -1) {
    const char *ret_file = NULL;
    if (pos < 0) {
        pthread_mutex_lock(&g_files_mutex);
        if(g_files_ptr>=g_files_count){
            ret_file = NULL;
        }
        else{
            ret_file = g_files[g_files_ptr];
            g_files_ptr++;
        }
        pthread_mutex_unlock(&g_files_mutex);
    }
    else {
        if (pos >= (int)g_files_count) {
            ret_file = NULL;
        }
        else {
            ret_file = g_files[pos];
        }
    }
    return ret_file;
}

static int xp_push_file(const char *path, const char *filename){
    char fn[PATH_MAX];
    snprintf(fn,sizeof(fn),"%s/%s",path,filename);
    if(!g_files){
        g_files_size=XP_FILES_SIZE;
        g_files=(char**)malloc(sizeof(char*)*g_files_size);
    }
    if(g_files_count==g_files_size){
        char **tmp = NULL;
        tmp = (char**)malloc(sizeof(char*)*g_files_size*2);
        if(!tmp){
            return -1;
        }
        memcpy(tmp,g_files,sizeof(char*)*g_files_count);
        free(g_files);
        g_files = tmp;
        g_files_size*=2;
    }
    g_files[g_files_count]=strdup(fn);
    if(!g_files[g_files_count]){
        return -1;
    }
    fprintf(stderr,"DEBUG: file:%s added.\n",g_files[g_files_count]);
    ++g_files_count;
    return 0;
}

static int cmp_file_name(const void * p1, const void * p2)
{
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}

int xp_load_input_queue(ib_ctx_t *p_ibctx){
    DIR *dir=NULL;
    dir = opendir(p_ibctx->input_dir);
    if(!dir){
        return -1;
    }

    struct dirent *de = NULL;
    const char *pe = 0;
    uint32_t push_count=0;
    while((de=readdir(dir))!=NULL){
        if(de->d_type!=DT_REG && de->d_type!=DT_LNK){
            continue;
        }
        pe = rindex(de->d_name,p_ibctx->file_ext[0]);
        if(!pe){
            continue;
        }
        if(strcasecmp(pe,p_ibctx->file_ext)!=0){
            continue;
        }
        if(xp_push_file(p_ibctx->input_dir,de->d_name)!=0){
            closedir(dir);
            return -1;
        }
        push_count++;
    }
    closedir(dir);
    g_files_ptr=0;
    if(push_count==0){
        fprintf(stderr,"No input file!\n");
        return -1;
    }

    qsort(g_files, g_files_count, sizeof(char *), cmp_file_name);
    return 0;
}

void xp_close_output_files(xp_ctx_t *pctx){
    uint32_t i;
    uint32_t j;

    for(i=0;i<pctx->conf->index_count;i++){
        for(j = 0; j<INDEX_SPLIT_PARTS;j++){
            if(pctx->fp_index[i*INDEX_SPLIT_PARTS+j]){
                gzclose(pctx->fp_index[i*INDEX_SPLIT_PARTS+j]);
                pctx->fp_index[i*INDEX_SPLIT_PARTS+j] = NULL;
            }
        }
    }
    if(pctx->fp_profile){
        gzclose(pctx->fp_profile);
        pctx->fp_profile = NULL;
    }
    if(pctx->fp_detail){
        gzclose(pctx->fp_detail);
        pctx->fp_detail = NULL;
    }
    if(pctx->fp_detail_idx){
        gzclose(pctx->fp_detail_idx);
        pctx->fp_detail_idx = NULL;
    }
    return;
}

int xp_open_output_files(xp_ctx_t *pctx,int is_gzip){
    char fn[PATH_MAX];
    uint32_t i = 0;
    uint32_t j = 0;

    if((pctx->p_ibctx->mode& MODE_SEARCHER)!=0){
        for(i=0;i<pctx->conf->index_count;i++){
            if(pctx->conf->index[i].bind_to){
                fprintf(stderr,"Bind field found:%s. Skip!\n",pctx->conf->index[i].name);
                continue;
            }
            if(pctx->conf->index[i].package_only){
                fprintf(stderr,"Package only field found:%s. Skip!\n",pctx->conf->index[i].name);
                continue;
            }
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                snprintf(fn,sizeof(fn),"%s/"INDEX_FILENAME"%s",pctx->p_ibctx->tmp_dir,pctx->conf->index[i].name,pctx->id,j,is_gzip?".gz":"");
                truncate(fn,0);
                pctx->fp_index[i*INDEX_SPLIT_PARTS+j] = gzopen(fn,"w+b1");
                if(!pctx->fp_index){
                    fprintf(stderr,"Open Output Failed! FileIdx:%u,Open %s\n",i*INDEX_SPLIT_PARTS+j,fn);
                    goto XP_OPEN_OUTPUT_FAILED; 
                }
            }
        }

        snprintf(fn,sizeof(fn),"%s/"PROFILE_FILENAME"%s",pctx->p_ibctx->tmp_dir,pctx->id,is_gzip?".gz":"");
        truncate(fn,0);
        pctx->fp_profile = gzopen(fn,"w+b1");
        if(!pctx->fp_profile){
            fprintf(stderr,"Open Output Failed! %s\n",fn);
            goto XP_OPEN_OUTPUT_FAILED; 
        }
    }

    if((pctx->p_ibctx->mode& MODE_DETAIL)!=0){
        snprintf(fn,sizeof(fn),"%s/detail/"DETAIL_FILENAME"%s",pctx->p_ibctx->output_dir,pctx->id,is_gzip?".gz":"");
        truncate(fn,0);
        pctx->fp_detail = gzopen(fn,"w+b1");
        if(!pctx->fp_detail){
            fprintf(stderr,"Open Output Failed! %s\n",fn);
            goto XP_OPEN_OUTPUT_FAILED; 
        }

        snprintf(fn,sizeof(fn),"%s/"DETAIL_IDX_FILENAME"%s",pctx->p_ibctx->tmp_dir,pctx->id,is_gzip?".gz":"");
        truncate(fn,0);
        pctx->fp_detail_idx = gzopen(fn,"w+b1");
        if(!pctx->fp_detail_idx){
            fprintf(stderr,"Open Output Failed! %s\n",fn);
            goto XP_OPEN_OUTPUT_FAILED; 
        }
    }


    return 0;
XP_OPEN_OUTPUT_FAILED:
    xp_close_output_files(pctx);
    return -1;
}

static int xp_dump_fields(xp_ctx_t *pctx, ksb_result_t *presult){
    ksb_conf_t *conf = pctx->conf;
    int ret = 0;
    
    if(conf->index_count>0){
        ret = ib_write_index(pctx->fp_index, conf, presult);
        if(ret!=0){
            fprintf(stderr,"ib_write_index() failed!\n");
            return -1;
        }
    }
    if(conf->profile_count>0){
        ret = ib_write_profile(pctx->fp_profile, conf, presult);
        if(ret!=0){
            fprintf(stderr,"ib_write_profile() failed!\n");
            return -1;
        }
    }
    if(conf->detail_count>0){
        ret = ib_write_detail(pctx->fp_detail,pctx->fp_detail_idx, conf, presult);
        if(ret!=0){
            fprintf(stderr,"ib_write_detail() failed!\n");
            return -1;
        }
    }
    
    return 0;
}

int xp_parse_line(char *line, ksb_result_t *presult, ksb_auction_t *pauction,
                  xp_ctx_t *pctx, ksb_conf_t *conf)
{
    if(strncasecmp(line,"<doc>",sizeof("<doc>") - 1)==0){
        //doc start
        ksb_reset_result(presult);
        ksb_reset_auction(pauction);
    }
    else if(strncasecmp(line,"</doc>",sizeof("</doc>") - 1)==0){
        //doc end
        if (ksb_parse(pauction,conf,presult,0) != 0) {
            fprintf(stderr,"ksb_parse() failed.\n");
            return -1;
        }
        else{
            if (xp_dump_fields(pctx, presult) != 0) {
                fprintf(stderr,"xp_dump_fields() failed.\n");
                return -1;
            }
            pctx->doc_count++;
        }
    }
    else{
        char *pvalue = index(line,'=');
        if(!pvalue){
       	    return 0;
        } 
        *pvalue = 0;
        pvalue += 1;
        int len = strlen(pvalue);
        int i = 0;
        while (i < len) {
            if (1 == pvalue[len-1]){
                len--;
                continue;
            }
            else if (1 == pvalue[i])
            {
                pvalue[i] = ' ';
            }
            ++i;
        }
        pvalue[len] = 0;
        ksb_auction_insert_field(pauction,line,pvalue);
    }
    return 0;
}

void *xp_parse_thread(void *pdata){
    xp_ctx_t *pctx = (xp_ctx_t*)pdata; 
    ksb_conf_t *conf = pctx->conf;
    pctx->ret_val = -1;
    
    const char *cur_file = NULL;
    char line[XP_GETS_SIZE];
	line[0] = 0;

    gzFile fp;
    ksb_result_t *presult = NULL;
    ksb_auction_t *pauction = NULL;

    presult = ksb_create_result();
    pauction = ksb_create_auction();
    if(!presult||!pauction){
        fprintf(stderr,"xp_parse_thread() failed. line:%d\n",__LINE__);
        return NULL;
    }

    int need_break = 0;
    const static int max_read_bytes = 1 << 20;
    char buf[max_read_bytes + 1];
    buf[0] = 0;
    int read_bytes = 0;
	char *p = NULL;
  
    while(1){
        if (pctx->p_ibctx->is_fixseq) {
            cur_file = xp_pop_file(pctx->fcnt * pctx->p_ibctx->thread_count + pctx->id);
            if (cur_file != NULL) {
                printf("xp_pop_file [Thread:%lu] : %s\n", pthread_self(), cur_file);
            }
        }
        else {
            cur_file = xp_pop_file();
        }

        if(!cur_file){
            pctx->ret_val = 0;
            break;
        }

        pctx->fcnt++;
        fp = gzopen(cur_file,"rb");
        if(!fp){
            fprintf(stderr,"Can not open file %s for read.\n",cur_file);
            continue;
        }
		int leftsize = 0;
        while ((read_bytes = gzread(fp, buf, max_read_bytes)) > 0) {
			buf[read_bytes] = 0;
            char *buf2 = buf;
            while ((p = strchr(buf2, '\n')) != NULL) {
                *p = '\0';
                if (leftsize > 0) {
					strcat(line, buf2);
                    if (xp_parse_line(line, presult, pauction, pctx, conf) != 0) {
                        need_break = 1;
                        break;
                    }
                    leftsize = 0;					
                }
                else {
					if (xp_parse_line(buf2, presult, pauction, pctx, conf) != 0) {
                        need_break = 1;
                        break;
                    }
                }
                read_bytes -= ((p - buf2) + 1);
                buf2 = p + 1;
            }
			if (need_break) {
                break;
            }
            if ((leftsize = read_bytes) > 0) {
				strcpy(line, buf2);
            }
        }
        if(fp){
            gzclose(fp);
            fp = NULL;
        }
        if(need_break){
            break;
        }
    }

    ksb_destroy_auction(pauction);
    pauction = NULL;
    ksb_destroy_result(presult);
    presult = NULL;
    return NULL;

}


