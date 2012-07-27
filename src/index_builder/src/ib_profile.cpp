#include "ib_profile.h"

int ib_write_profile(gzFile fp, ksb_conf_t *conf, ksb_result_t *presult){
    if(conf->profile_count<=0){
        return 0;
    }
    unsigned int i = 0;
    unsigned int sign = conf->profile_count -1;
    int size = 1024 * 1024;
    int len = 0;
    char buf[size];
    buf[0] = 0;
    for(i=0;i<conf->profile_count;++i){
        snprintf(buf+len, size - len, "%s%s", presult->profile_fields[i].value,(i < sign)?"\001":"\n");
        len += presult->profile_fields[i].v_len + 1;
    }
//    if (gzprintf(fp, "%s", buf) <= 0) {
    if(gzwrite(fp,buf,strlen(buf)) <= 0) {
        return -1;
    }
    return 0;
}
