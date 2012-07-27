#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>
#include <locale.h>
#include <stdlib.h>

#include "alog/Logger.h"
#include "ks_build/ks_build.h"
#include "ib_ctx.h"
#include "xml_parser.h"
#include "ib_builder.h"
#include "util/Timer.h"

static void print_usage(){
    fprintf(stderr,"Usage:\n");
    fprintf(stderr,"\tindex_builder [options]\n");
    fprintf(stderr,"\tOptions:\n");
    fprintf(stderr,"\t\t-c\t[index config] 配置文件路径\n");
    fprintf(stderr,"\t\t-t\t[threads]\t线程数\n");
    fprintf(stderr,"\t\t-i\t[input directory]\t源文件路径(.gz或.xml)\n");
    fprintf(stderr,"\t\t-z\t无参数\t输入是否为gzip压缩\n");
    fprintf(stderr,"\t\t-o\t[output directory]\t输出路径\n");
    fprintf(stderr,"\t\t-T\t[tmp directory]\t临时目录\n");
    fprintf(stderr,"\t\t-m\t[mode] detail,searcher\n");
    fprintf(stderr,"\t\t-f\t无参数\t是否固定docid分配顺序(for QA)\n");
    fprintf(stderr,"\n");
}

static int ib_write_title_file(ib_ctx_t *ctx, ksb_conf_t *conf){
    char filename[PATH_MAX];
    snprintf(filename,sizeof(filename),"%s/index.mdl",ctx->output_dir);
    FILE *fp = fopen(filename,"w+b");
    if(!fp){
        return -1;
    }
    uint32_t i;
    if(conf->index_count>0){
        for(i=0;i<conf->index_count;i++){
            if(i<conf->index_count-1){
                fprintf(fp,"%s\001",conf->index[i].name);
            }
            else{
                fprintf(fp,"%s",conf->index[i].name);
            }
        }
    }
    fclose(fp);
    snprintf(filename,sizeof(filename),"%s/profile.mdl",ctx->output_dir);
    fp = fopen(filename,"w+b");
    if(!fp){
        return -1;
    }

    if(conf->profile_count>0){
        fprintf(fp,"vdocid\001");
        for(i=0;i<conf->profile_count;i++){
            if(i<conf->profile_count-1){
                fprintf(fp,"%s\001",conf->profile[i].name);
            }
            else{
                fprintf(fp,"%s",conf->profile[i].name);
            }
        }
    }
    fclose(fp);
    snprintf(filename,sizeof(filename),"%s/detail/detail.mdl",ctx->output_dir);
    fp = fopen(filename,"w+b");
    if(!fp){
        return -1;
    }
    if(conf->detail_count>0){
        for(i=0;i<conf->detail_count;i++){
            if(i<conf->detail_count-1){
                fprintf(fp,"%s\001",conf->detail[i].name);
            }
            else{
                fprintf(fp,"%s",conf->detail[i].name);
            }
        }
    }
    fclose(fp);
    return 0;
}

static int check_ctx(ib_ctx_t *ctx){
    if(!ctx->input_dir || access(ctx->input_dir,R_OK)!=0){
        fprintf(stderr,"Can not read input dir %s\n",ctx->input_dir);
        return -1;
    }

    if(!ctx->output_dir || access(ctx->output_dir,W_OK)!=0){
        fprintf(stderr,"Can not write in output dir %s\n",ctx->output_dir);
        return -1;
    }

    char path[PATH_MAX];
    snprintf(path,sizeof(path),"%s/index/",ctx->output_dir);
    if(access(path,W_OK)!=0){
        fprintf(stderr,"Can not write dir %s\n",path);
        return -1;
    }

    snprintf(path,sizeof(path),"%s/profile/",ctx->output_dir);
    if(access(path,W_OK)!=0){
        fprintf(stderr,"Can not write dir %s\n",path);
        return -1;
    }

    snprintf(path,sizeof(path),"%s/detail/",ctx->output_dir);
    if(access(path,W_OK)!=0){
        fprintf(stderr,"Can not write dir %s\n",path);
        return -1;
    }

    /** 清空原先的detail数据 **/
    char command[PATH_MAX];
    snprintf(command, sizeof(command), "rm -rf %s/*", path);
    system(command);

    if(!ctx->tmp_dir || access(ctx->tmp_dir,W_OK|R_OK)!=0){
        fprintf(stderr,"Can not read/write in temp dir %s\n",ctx->tmp_dir);
        return -1;
    }

    if(!ctx->conf_file || access(ctx->conf_file,R_OK)!=0){
        fprintf(stderr,"Can not read config file:%s\n",ctx->conf_file);
        return -1;
    }

    if(ctx->thread_count<=0 || ctx->thread_count > 255){
        fprintf(stderr,"Thread count must between 1 to 255.\n");
        return -1;
    }
    return 0;

}

static int parse_xml(ib_ctx_t *ctx){
    unsigned int i = 0;
    int ret = 0;

    xp_ctx_t *ctxs;
    ctxs = (xp_ctx_t *)malloc(sizeof(xp_ctx_t)*ctx->thread_count);
    if(!ctxs){
        return -1;
    }
    memset(ctxs,0,sizeof(xp_ctx_t)*ctx->thread_count);

    ret = xp_load_input_queue(ctx);
    if(ret!=0){
        free(ctxs);
        return -1;
    }



    for(i = 0; i < ctx->thread_count; i++){
        ctxs[i].p_ibctx = ctx;
        ctxs[i].doc_count = 0;
        ctxs[i].ret_val = 0;
        ctxs[i].id = i;
        ctxs[i].fcnt = 0;
        ctxs[i].conf = ksb_load_config(ctx->conf_file,ctx->mode);
        if(!ctxs[i].conf){
            fprintf(stderr,"ksb_load_config failed!. line:%d\n",__LINE__);
            return -1;
        }
        if(xp_open_output_files(&ctxs[i],1/*always gzip*/)!=0){
            fprintf(stderr,"xp_open_output_files() failed! line:%d\n",__LINE__);
            return -1;
        }
    }

    for(i = 0; i < ctx->thread_count; i++){
        ret = pthread_create(&ctxs[i].tid,NULL,xp_parse_thread,&ctxs[i]);
        if(ret!=0){
            fprintf(stderr,"pthread_create() failed. id=%u\n",i);
            return -1;
        }
        fprintf(stderr,"parse_xml(): Thread %u created.\n",i);
    }

    int func_return=0;
    void *ret_val = NULL;
    ctx->doc_total = 0;
    for(i = 0; i < ctx->thread_count; i++){
        ret_val = NULL;
        ret = pthread_join(ctxs[i].tid,&ret_val);
        if(ret!=0){
            fprintf(stderr,"pthread_join() failed. id = %u\n",i);
        }

        xp_close_output_files(&ctxs[i]);
        ksb_destroy_config(ctxs[i].conf);
        ctxs[i].conf=NULL;
        if(ctxs[i].ret_val!=0){
            fprintf(stderr,"parse_xml(): Thread %u exited. ret_val = %d. DOC:%d\n",i,ctxs[i].ret_val,ctxs[i].doc_count);
            func_return=-1;
        }
        else{
            fprintf(stderr,"parse_xml(): Thread %u exited. ret_val = %d. DOC:%d\n",i,ctxs[i].ret_val,ctxs[i].doc_count);
        }

        ctx->doc_total+=ctxs[i].doc_count;
    }


    free(ctxs);
    return func_return;
}

int qsort_int_cmp(const void *a,const void *b){
    return 0;
}

int main(int argc, char *argv[]){
    uint64_t nBeginTime = UTIL::Timer::getMicroTime();

    setlocale(LC_ALL,"zh_CN.UTF-8");

    alog::Appender *p_alog_appender = alog::ConsoleAppender::getAppender();
    if(!p_alog_appender){
        fprintf(stderr,"Get alog::ConsoleAppender::getAppender() failed.\n");
        return -1;
    }
    alog::PatternLayout alog_layout;
    alog_layout.setLogPattern("[%%d] [%%F:%%n:%%f()] [%%l] :: %%m");
    p_alog_appender->setLayout(&alog_layout);

    alog::Configurator::configureRootLogger();
    alog::Logger::MAX_MESSAGE_LENGTH = 10240;
    alog::Logger::getRootLogger()->setAppender(p_alog_appender);
    //TERR("FSDFDSFAFDA");
    //

    //patch for qsort bug
    int qsort_tmp[2048]={0};
    qsort(qsort_tmp,2048,sizeof(int),qsort_int_cmp);

    const char *str_conf_file = NULL;
    const char *str_threads = NULL;
    const char *str_input_dir = NULL;
    const char *str_output_dir = NULL;
    const char *str_tmp_dir = NULL;
    const char *str_mode = NULL;
    int is_gzip   = 0;
    int is_fixseq = 0;

    int ret = 0;
    int opt = 0;
    while((opt=getopt(argc,argv,"c:t:i:o:T:m:zf"))!=-1){
        switch(opt){
            case 'c':
                str_conf_file = optarg;
                break;
            case 't':
                str_threads = optarg;
                break;
            case 'i':
                str_input_dir = optarg;
                break;
            case 'o':
                str_output_dir = optarg;
                break;
            case 'T':
                str_tmp_dir = optarg;
                break;
            case 'z':
                is_gzip = 1;
                break;
            case 'm':
                str_mode = optarg;
                break;
            case 'f':
                is_fixseq = 1;
                break;
            default:
                fprintf(stderr,"Invalid arg: '-%c'.\n",(char)opt);
                print_usage();
                return -1;
        }
    }

    if(!str_conf_file || !str_threads || !str_input_dir || !str_output_dir ||
            !str_tmp_dir){
        fprintf(stderr,"No enough args.\n");
        print_usage();
        return -1;
    }



    ib_ctx_t parser_ctx;
    memset(&parser_ctx,0,sizeof(ib_ctx_t));
    parser_ctx.input_dir = str_input_dir;
    parser_ctx.output_dir = str_output_dir;
    parser_ctx.tmp_dir = str_tmp_dir;
    parser_ctx.conf_file = str_conf_file;
    parser_ctx.thread_count = strtoul(str_threads,NULL,10);
    parser_ctx.is_fixseq = is_fixseq;
    if(is_gzip){
        parser_ctx.file_ext = EXT_GZIP;
        parser_ctx.is_gzip = 1;
    }
    else{
        parser_ctx.file_ext = EXT_XML;
        parser_ctx.is_gzip = 0;
    }

    if(!str_mode){
        parser_ctx.mode = MODE_ALL;
    }
    else{
        if(strcasecmp(str_mode,"detail")==0){
            parser_ctx.mode = MODE_DETAIL;
        }
        else if(strcasecmp(str_mode,"searcher")==0){
            parser_ctx.mode = MODE_SEARCHER;
        }
        else{
            fprintf(stderr,"Invalid -m (%s)\n",str_mode);
            return -1;
        }
    }

    if(check_ctx(&parser_ctx)!=0){
        fprintf(stderr,"Check args failed. Abort!\n");
        return -1;
    }

    ret = parse_xml(&parser_ctx);
    if(ret!=0){
        fprintf(stderr,"parse_xml failed. Abort!\n");
        return -1;
    }

    ib_builder_ctx_t builder_ctx;
    memset(&builder_ctx,0,sizeof(builder_ctx));

    builder_ctx.p_ibctx = &parser_ctx;
    builder_ctx.conf = ksb_load_config(parser_ctx.conf_file,builder_ctx.p_ibctx->mode);
    if(!builder_ctx.conf){
        fprintf(stderr,"ksb_load_config() failed. in %s.\n",__FUNCTION__);
        return -1;
    }

    if(ib_write_title_file(&parser_ctx,builder_ctx.conf)!=0){
        fprintf(stderr,"ib_write_title_file() failed,\n");
        return -1;
    }

    fprintf(stderr,"Calling ib_build()\n");

    ret = ib_build(&builder_ctx);
    if(ret!=0){
        fprintf(stderr,"ib_build() failed.\n");
        return -1;
    }
    char tmp_max_doc[64];
    snprintf(tmp_max_doc,sizeof(tmp_max_doc),"echo %u > %s/max_doc_id",parser_ctx.doc_total,parser_ctx.output_dir);
    system(tmp_max_doc);
    uint64_t nEndTime = UTIL::Timer::getMicroTime();
    fprintf(stdout, "total_cost=%lu", nEndTime - nBeginTime);
    return 0;
}

