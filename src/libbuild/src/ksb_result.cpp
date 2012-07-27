#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "ks_build/ks_build.h"


ksb_result_t *
ksb_create_result() {
    ksb_result_t *p = (ksb_result_t *) malloc(sizeof(ksb_result_t));
    if (!p) {
        return NULL;
    }
    memset(p, 0, sizeof(ksb_result_t));
    return p;
}

ksb_result_field_t *
ksb_result_get_node(ksb_result_t * result, int index, uint32_t index_type) {
    if (index >= KSB_FIELD_MAX) {
        return NULL;
    }

    ksb_result_field_t *pfield;
    if (FIELD_INDEX == index_type) {
        pfield = &result->index_fields[index];
    }
    else if (FIELD_PROFILE == index_type) {
        pfield = &result->profile_fields[index];
    }
    else if (FIELD_DETAIL == index_type) {
        pfield = &result->detail_fields[index];
    }
    else {
        return NULL;
    }

    return pfield;
}

int
ksb_result_set(ksb_result_t * result, const char *value, int index,
               uint32_t index_type) {
    assert(value);
    if (index >= KSB_FIELD_MAX) {
        return -1;
    }

    uint32_t *pcount = NULL;
    ksb_result_field_t *pfield;
    if (FIELD_INDEX == index_type) {
        pfield = &result->index_fields[index];
        pcount = &result->index_count;
    }
    else if (FIELD_PROFILE == index_type) {
        pfield = &result->profile_fields[index];
        pcount = &result->profile_count;
    }
    else if (FIELD_DETAIL == index_type) {
        pfield = &result->detail_fields[index];
        pcount = &result->detail_count;
    }
    else {
        return -1;
    }
    unsigned int v_len = strlen(value);

    if (pfield->v_size >= v_len && pfield->v_size > 0) {
        snprintf(pfield->value, pfield->v_size + 1, "%s", value);
        pfield->v_len = v_len;
    }
    else {
        free(pfield->value);
        pfield->value = strdup(value);
        if (!pfield->value) {
            return -1;
        }
        pfield->v_size = v_len;
        pfield->v_len = v_len;
    }
    pfield->index = index;
    ++(*pcount);
    pfield->is_set = 1;
    return 0;
}

void
ksb_reset_result(ksb_result_t * result) {
    assert(result);
    unsigned int i;
    for (i = 0; i < KSB_FIELD_MAX; i++) {
        result->index_fields[i].v_len = 0;
        result->profile_fields[i].v_len = 0;
        result->detail_fields[i].v_len = 0;
        result->index_fields[i].is_set = 0;
        result->profile_fields[i].is_set = 0;
        result->detail_fields[i].is_set = 0;
    }
    result->index_count = 0;
    result->profile_count = 0;
    result->detail_count = 0;
}

void
ksb_destroy_result(ksb_result_t * result) {
    int i = 0;
    for (i = 0; i < KSB_FIELD_MAX; i++) {
        free(result->index_fields[i].value);
    }
    for (i = 0; i < KSB_FIELD_MAX; i++) {
        free(result->profile_fields[i].value);
    }
    for (i = 0; i < KSB_FIELD_MAX; i++) {
        free(result->detail_fields[i].value);
    }
    free(result);
}
