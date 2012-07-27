#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "ksb_stopword.h"

int
ksb_load_stopword(const char *stopword_dict, ksb_conf_t * conf) {
    conf->stopword = (stopword_map_t *) malloc(sizeof(stopword_map_t));
    if (!conf->stopword) {
        fprintf(stderr, "Out of memory.\n");
        return -1;
    }
    memset(conf->stopword, 0, sizeof(stopword_map_t));
    setlocale(LC_ALL, "zh_CN.UTF-8");
    FILE *fp = NULL;
    fp = fopen(stopword_dict, "rb");
    if (!fp) {
        fprintf(stderr, "Can not open %s for read.\n", stopword_dict);
        return -1;
    }
    char line[128];
    int word;
    int pos;
    int bit;
    while (fgets(line, sizeof(line), fp) != NULL) {
        word = strtol(line, NULL, 10);
        if (word <= 0 || word >= 65536) {
            fprintf(stderr, "Invalid stopword %d. Skip!\n", word);
            continue;
        }
        pos = word / 64;
        bit = word % 64;
        conf->stopword->map[pos] = conf->stopword->map[pos] | (1 << bit);
    }
    fclose(fp);
    fp = NULL;
    return 0;
}
