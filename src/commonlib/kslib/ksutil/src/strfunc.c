#include <stdlib.h>
#include <string.h>
#include "util/strfunc.h"

char *
str_bin2hex(const char *s, const int len, char *hex_buff) {
    const unsigned char *p;
    const unsigned char *end_ptr;
    int nLen;

    nLen = 0;
    end_ptr = (const unsigned char *) s + len;
    for (p = (const unsigned char *) s; p < end_ptr; p++) {
        nLen += sprintf(hex_buff + nLen, "%02x", *p);
    }

    hex_buff[nLen] = '\0';
    return hex_buff;
}

char *
str_hex2bin(const char *s, char *bin_buff, int *dest_len) {
    char buff[3];
    const char *src;
    int src_len;
    char *dest;
    char *dest_end;

    src_len = strlen(s);
    if (src_len == 0) {
        *dest_len = 0;
        bin_buff[0] = '\0';
        return bin_buff;
    }

    *dest_len = src_len / 2;
    src = s;
    buff[2] = '\0';

    dest_end = bin_buff + (*dest_len);
    for (dest = bin_buff; dest < dest_end; dest++) {
        buff[0] = *src++;
        buff[1] = *src++;
        *dest = (char) strtol(buff, NULL, 16);
    }

    *dest = '\0';
    return bin_buff;
}

void
str_print_buffer(const char *s, const int len) {
    unsigned char *p;
    int i;

    p = (unsigned char *) s;
    for (i = 0; i < len; i++) {
        printf("%02X", *p);
        p++;
    }
    printf("\n");
}

char *
str_ltrim(char *str) {
    char *p;
    char *end_ptr;
    int dest_len;

    end_ptr = str + strlen(str);
    for (p = str; p < end_ptr; p++) {
        if (!(' ' == *p || '\n' == *p || '\r' == *p || '\t' == *p)) {
            break;
        }
    }

    if (p == str) {
        return str;
    }

    dest_len = (end_ptr - p) + 1; //including \0
    memmove(str, p, dest_len);

    return str;
}

char *
str_rtrim(char *str) {
    int len;
    char *p;
    char *last_ptr;

    len = strlen(str);
    if (len == 0) {
        return str;
    }

    last_ptr = str + len - 1;
    for (p = last_ptr; p >= str; p--) {
        if (!(' ' == *p || '\n' == *p || '\r' == *p || '\t' == *p)) {
            break;
        }
    }

    if (p != last_ptr) {
        *(p + 1) = '\0';
    }

    return str;
}

char *
str_trim(char *str) {
    str_rtrim(str);
    str_ltrim(str);
    return str;
}

char *
str_lowercase(char *src) {
    char *p;

    p = src;
    while (*p != '\0') {
        if (*p >= 'A' && *p <= 'Z') {
            *p += 32;
        }

        p++;
    }

    return src;
}

char *
str_uppercase(char *src) {
    char *p;

    p = src;
    while (*p != '\0') {
        if (*p >= 'a' && *p <= 'z') {
            *p -= 32;
        }

        p++;
    }

    return src;
}

char **
str_split(char *src, const char seperator,
          const int max_cols, int *col_count) {
    char **cols;
    char **current;
    char *p;
    int i;
    int last_index;

    if (src == NULL) {
        *col_count = 0;
        return NULL;
    }

    *col_count = 1;
    p = strchr(src, seperator);

    while (p != NULL) {
        (*col_count)++;
        p = strchr(p + 1, seperator);
    }

    if (max_cols > 0 && (*col_count) > max_cols) {
        *col_count = max_cols;
    }

    current = cols = (char **) malloc(sizeof(char *) * (*col_count));
    if (cols == NULL) {
        *col_count = 0;
        return NULL;
    }

    p = src;
    last_index = *col_count - 1;
    for (i = 0; i < *col_count; i++) {
        *current = p;
        current++;

        p = strchr(p, seperator);
        if (i != last_index) {
            *p = '\0';
            p++;                //skip seperator
        }
    }

    return cols;
}

void
str_free_split(char **p) {
    if (p != NULL) {
        free(p);
    }
}

int
str_split_ex(char *src, const char seperator,
             char **cols, const int max_cols) {
    char *p;
    char **current;
    int count = 0;

    if (max_cols <= 0) {
        return 0;
    }

    p = src;
    current = cols;

    while (true) {
        *current = p;
        current++;

        count++;
        if (count >= max_cols) {
            break;
        }

        p = strchr(p, seperator);
        if (p == NULL) {
            break;
        }

        *p = '\0';
        p++;
    }

    return count;
}



int
split_multi_delims(char *str, char **items, int item_size, const char *delim) {
    char *begin = NULL;
    char *end = NULL;
    int cnt = 0;
    int finish = 0;


    begin = NULL;
    while (!finish) {

        if (!begin) {
            begin = str;
        }

        // find the word end.
        end = begin;
        while (*end && strchr(delim, *end) == NULL) {
            end++;
        }
        if (!(*end)) {
            finish = 1;
        }

        if (cnt >= item_size) {
            return -1;
        }

        items[cnt++] = begin;
        if (*end) {
            *end = 0;
            begin = end + 1;
        }
        else {
            break;
        }
    }
    return cnt;
}
