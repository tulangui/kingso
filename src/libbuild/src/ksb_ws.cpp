
#include <wchar.h>
#include <stdio.h>
#include "ksb_ws.h"
#include "util/charsetfunc.h"
#include "util/idx_sign.h"
#include "ksb_stopword.h"

static int
ksb_ws_cmp(const void *t1, const void *t2) {
    ksb_index_value_t *tt1 = (ksb_index_value_t *) t1;
    ksb_index_value_t *tt2 = (ksb_index_value_t *) t2;
    if (tt1->sign > tt2->sign) {
        return 1;
    }
    if (tt1->sign < tt2->sign) {
        return -1;
    }
    if (tt1->occ > tt2->occ) {
        return 1;
    }
    if (tt1->occ < tt2->occ) {
        return -1;
    }
    return 0;
}

void
ksb_ws_sort_uniq(ksb_result_field_t * pfield) {
    unsigned int i = 0;
    unsigned int j = 0;
    qsort(pfield->index_value, pfield->v_len, sizeof(ksb_index_value_t),
          ksb_ws_cmp);
    ksb_index_value_t *pvalue = pfield->index_value;
    for (i = pfield->v_len - 1; i > 0; i--) {
        if (pvalue[i].sign == pvalue[i - 1].sign
            && pvalue[i].occ == pvalue[i - 1].occ) {
            for (j = i; j < pfield->v_len - 1; j++) {
                memcpy(&pvalue[j], &pvalue[j + 1],
                       sizeof(ksb_index_value_t));
            }
            pfield->v_len--;
        }
    }
}

int
ksb_insert_result(ksb_result_field_t * pfield, uint64_t sign, uint32_t occ,
                  uint32_t weight) {
    ksb_index_value_t *pvalue;
    if (!pfield->index_value || pfield->v_size == 0) {
        pfield->v_len = 0;
        pfield->v_size = 8;
        pfield->index_value =
            (ksb_index_value_t *) malloc(sizeof(ksb_index_value_t) *
                                         pfield->v_size);
        if (!pfield->index_value) {
            return -1;
        }
    }
    else {
        if (pfield->v_len == pfield->v_size) {
            pvalue =
                (ksb_index_value_t *) malloc(sizeof(ksb_index_value_t) *
                                             pfield->v_size * 2);
            if (!pvalue) {
                return -1;
            }
            memcpy(pvalue, pfield->index_value,
                   sizeof(ksb_index_value_t) * pfield->v_size);
            free(pfield->index_value);
            pfield->index_value = pvalue;
            pfield->v_size *= 2;
        }
    }
    int index = pfield->v_len;
    pfield->index_value[index].sign = sign;
    pfield->index_value[index].occ = occ;
    pfield->index_value[index].weight = weight;
    pfield->v_len++;
    return 0;
}

inline int
ws_check_stopword(const char *word, int len, ksb_conf_t * conf) {
    if (len > 3) {
        return 0;
    }
    char tword[4];
    wchar_t termUcs[4];
    int i;
    for (i = 0; i < len; i++) {
        tword[i] = word[i];
    }
    tword[i] = 0;
    swprintf(termUcs, sizeof(termUcs) / sizeof(wchar_t), L"%s", tword);
    if (termUcs[1] == 0 && ksb_is_stopword(termUcs[0], conf)) {
        return 1;
    }
    return 0;
}

int
ksb_do_ws(const char *str, ksb_result_field_t * pfield, ksb_conf_t * conf, ksb_result_field_t *pfield_bind_to, uint32_t bind_occ, uint32_t bind_real_occ, int index) {
    int ret = 0;
    uint64_t sign = 0;
    int i = 0;
    int term_count = 0;
    conf->pTokenizer[index]->load(str);
    const char *pTokenText = NULL;
    int32_t iTextLen = 0;
    int32_t iTokenSerial = 0;
    while (conf->pTokenizer[index]->next(pTokenText, iTextLen, iTokenSerial) >= 0) {
        if (ws_check_stopword(pTokenText, iTextLen, conf)) {
            continue;
        }
        sign = idx_sign64(pTokenText, iTextLen);
        ret = ksb_insert_result(pfield, sign, iTokenSerial, 1);
        if (ret != 0) {
            fprintf(stderr, "ksb_insert_result() failed!\n");
            return -1;
        }
    }

    term_count = pfield->v_len;

    if(pfield_bind_to){
        term_count = pfield->v_len;
        for(i=0;i<term_count;i++){
            if(bind_real_occ){
                ksb_insert_result(pfield_bind_to,pfield->index_value[i].sign,bind_occ+pfield->index_value[i].occ,1);
            }
            else{
                ksb_insert_result(pfield_bind_to,pfield->index_value[i].sign,bind_occ,1);
            }
        }
        if(term_count>0 && pfield_bind_to->v_len>0){
            ksb_ws_sort_uniq(pfield_bind_to);
        }
    }
    else{
        if(pfield->v_len>0){
            ksb_ws_sort_uniq(pfield);
        }
    }
    return 0;
}
