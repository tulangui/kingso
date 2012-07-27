#include <stdio.h>

#include "ks_build/ks_build.h"
#include "util/strfunc.h"

void print_result(ksb_result_t *presult, ksb_conf_t *conf){
    unsigned int i = 0;
    unsigned int j = 0;
    ksb_index_value_t *pvalue = NULL;
    for(i=0;i<conf->index_count;i++){
        pvalue = presult->index_fields[i].index_value;
        fprintf(stdout,"Field:%s\n",conf->index[i].name);
        for(j=0;j<presult->index_fields[i].v_len;j++){
            fprintf(stdout,"\tsign=%lu,occ=%u,weight=%lu\n",pvalue[j].sign,pvalue[j].occ,pvalue[j].sign);
        }
    }
    for(i=0;i<conf->profile_count;i++){
        fprintf(stdout,"Field:%s\n",conf->profile[i].name);
        fprintf(stdout,"Value:%s\n",presult->profile_fields[i].value);
    }
    
    for(i=0;i<conf->detail_count;i++){
        fprintf(stdout,"Field:%s\n",conf->detail[i].name);
        fprintf(stdout,"Value:%s\n",presult->detail_fields[i].value);
    }
}

int parse_xml(ksb_conf_t *conf, FILE *fp){
    ksb_auction_t *pauction = ksb_create_auction();
    ksb_result_t *presult = ksb_create_result();
    fprintf(stderr,"pauction=%p\npresult=%p\n",pauction,presult);

    char line[1024];
    char *value = NULL;
    char *name = NULL;
    int count = 0;
    int ret = 0;
    int value_len = 0;

    while(fgets(line,1024,fp)!=NULL){
        if(strncasecmp(line,"<doc>",5)==0){
            ksb_reset_auction(pauction);
            ksb_reset_result(presult);
            continue;
        }
        else if(strncasecmp(line,"</doc>",6)==0){
            ret = ksb_parse(pauction,conf,presult,0);
            print_result(presult,conf);
            count++;
            fprintf(stderr,"ksb_parse returned %d\n",ret);
            continue;
        }
        name = line;
        value = index(line,'=');
        if(!value){
            fprintf(stderr,"Error!\n");
            break;
        }
        *value = 0;
        value++;
        value_len=strlen(value);
        while((value[value_len]==0 || value[value_len]=='\n' ||value[value_len]==1)&&value_len>=0){
            value[value_len]=0;
            value_len--;
        }
        
        ksb_auction_insert_field(pauction,name,value);
    }
    fprintf(stdout,"%d docs\n",count);
    return 0;
}

int main(int argc, char *argv[]){
    ksb_conf_t *conf;
    conf = ksb_load_config("/home/cuilun/profile_msearch.xml",MODE_ALL);
    fprintf(stderr,"conf = %p\n",conf);
    if(!conf){
        return -1;
    }
    FILE *fp = fopen("2.xml","rb");
    parse_xml(conf,fp);
    fclose(fp);

    return 0;
}
