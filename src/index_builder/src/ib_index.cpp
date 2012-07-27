#include "ib_index.h"


int ib_write_index(gzFile *fps, ksb_conf_t *conf, ksb_result_t *presult){
    uint32_t i = 0;
    uint32_t j = 0;
    uint32_t value_count = 0;
    ib_text_t text;
    ib_num_t num;
    ksb_index_value_t *pvalue = NULL;
    uint32_t counts[INDEX_SPLIT_PARTS];
    for(i=0;i<conf->index_count;++i){
        pvalue = presult->index_fields[i].index_value;
        if(conf->index[i].index_type == FIELD_TYPE_TEXT){
//            fprintf(stderr,"WRITE field:%s\n",conf->index[i].name);
            value_count = presult->index_fields[i].v_len;
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                counts[j]=0;
            }
            for(j=0;j<value_count;j++){
                counts[pvalue[j].sign%INDEX_SPLIT_PARTS]++;
            }
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                gzwrite(fps[i*INDEX_SPLIT_PARTS + j],&counts[j],sizeof(counts[j]));
            }
            for(j=0;j<value_count;j++){
                text.sign=pvalue[j].sign;
                text.occ=pvalue[j].occ;
//                text.weight = 1;
                gzwrite(fps[i*INDEX_SPLIT_PARTS+text.sign%INDEX_SPLIT_PARTS],&text,sizeof(text));
//                fprintf(stderr,"WRITE file:%u\n",i*INDEX_SPLIT_PARTS+text.sign%INDEX_SPLIT_PARTS);
            }
        }
        else{
//            fprintf(stderr,"WRITE field:%s\n",conf->index[i].name);
            value_count = presult->index_fields[i].v_len;
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                counts[j]=0;
            }
            for(j=0;j<value_count;j++){
                counts[pvalue[j].sign%INDEX_SPLIT_PARTS]++;
            }
            for(j=0;j<INDEX_SPLIT_PARTS;j++){
                gzwrite(fps[i*INDEX_SPLIT_PARTS+j],&counts[j],sizeof(counts[j]));
            }
            for(j=0;j<value_count;j++){
                num.sign=pvalue[j].sign;
                gzwrite(fps[i*INDEX_SPLIT_PARTS+num.sign%INDEX_SPLIT_PARTS],&num,sizeof(num));
//                fprintf(stderr,"WRITE:sign=%llu\n",num.sign);
//                fprintf(stderr,"WRITE file:%u\n",i*INDEX_SPLIT_PARTS+text.sign%INDEX_SPLIT_PARTS);
            }
        }
    }
    return 0;
}
