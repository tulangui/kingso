#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include "ks_build/ks_build.h"


ksb_auction_t *
ksb_create_auction() {
    ksb_auction_t *p = (ksb_auction_t *) malloc(sizeof(ksb_auction_t));
    if (!p) {
        return NULL;
    }
    memset(p, 0, sizeof(ksb_auction_t));

    return p;
}

int
ksb_auction_insert_field(ksb_auction_t * auction, const char *key,
                         const char *value) {
    assert(key && value);
    if (auction->count >= KSB_FIELD_MAX) {
        return -1;
    }
    int index = auction->count;
    ksb_auction_field_t *pfield = &auction->fields[index];
    unsigned int n_len = strlen(key);
    unsigned int v_len = strlen(value);
    if (pfield->n_size >= n_len) {
        snprintf(pfield->name, pfield->n_size + 1, "%s", key);
        pfield->n_len = n_len;
    }
    else {
        free(pfield->name);
        pfield->name = strdup(key);
        if (!pfield->name) {
            return -1;
        }
        pfield->n_size = n_len;
        pfield->n_len = n_len;
    }

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
    ++auction->count;
    return 0;
}

void
ksb_reset_auction(ksb_auction_t * auction) {
    assert(auction);
    auction->count = 0;
}

void
ksb_destroy_auction(ksb_auction_t * auction) {
    int i = 0;
    for (i = 0; i < KSB_FIELD_MAX; i++) {
        free(auction->fields[i].name);
        free(auction->fields[i].value);
    }
    free(auction);
}
