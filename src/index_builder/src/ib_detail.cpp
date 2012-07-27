#include "ib_detail.h"

int ib_write_detail(gzFile fp, gzFile fp_idx, ksb_conf_t *conf, ksb_result_t *presult){
    unsigned int i = 0;
    unsigned int mark = conf->detail_count -1;
    uint64_t off_start;
    uint64_t off_end;
    uint32_t nid_index = conf->detail_nid_index;
    off_start = gzseek(fp,0L,SEEK_CUR);
    int size = 1024 * 1024;
    char buf[size];
    int len = 0;
    for(i=0;i<conf->detail_count;++i){
        snprintf(buf+len, size - len, "%s%s", presult->detail_fields[i].value,(i < mark)?"\001":"");
        len += presult->detail_fields[i].v_len + 1;
    }

/*    if (gzprintf(fp, "%s", buf) <= 0) {*/
    if( gzwrite(fp,buf,strlen(buf)) <= 0) {
        fprintf(stderr,"gzwrite failed. %d,%p\n",__LINE__,fp);
        return -1;
    }

    off_end = gzseek(fp,0L,SEEK_CUR);
    ib_detail_idx_t idx(atol(presult->detail_fields[nid_index].value), off_start, off_end-off_start);
    if (gzwrite(fp_idx, &idx, sizeof(ib_detail_idx_t)) <= 0) {
        fprintf(stderr,"gzwrite failed. %d\n",__LINE__);
        return -1;
    }
    return 0;
}
