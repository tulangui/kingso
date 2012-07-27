#include "util/charsetfunc.h"
#include <iconv.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>

#define MAX_CODENAME_LEN 32

struct _cc_struct {
    iconv_t cd;
};

code_conv_t
code_conv_open(uint32_t to, uint32_t from, uint32_t flag) {
    char tocode[MAX_CODENAME_LEN];
    char fromcode[MAX_CODENAME_LEN];
    char str_flag[MAX_CODENAME_LEN];

    if (!flag) {
        str_flag[0] = 0;
    }
    else if (flag == CC_FLAG_IGNORE) {
        snprintf(str_flag, sizeof(str_flag), "//IGNORE");
    }
    else if (flag == CC_FLAG_TRANSLIT) {
        snprintf(str_flag, sizeof(str_flag), "//TRANSLIT");
    }
    else {
        return NULL;
    }

    if (to == CC_GBK) {
        snprintf(tocode, sizeof(tocode), "GB18030%s", str_flag);
    }
    else if (to == CC_UTF8) {
        snprintf(tocode, sizeof(tocode), "UTF-8%s", str_flag);
    }
    else {
        return NULL;
    }

    if (from == CC_GBK) {
        snprintf(fromcode, sizeof(fromcode), "GB18030");
    }
    else if (from == CC_UTF8) {
        snprintf(fromcode, sizeof(fromcode), "UTF-8");
    }
    else {
        return NULL;
    }


    iconv_t cd = iconv_open(tocode, fromcode);
    if ((iconv_t) - 1 == cd) {
        return NULL;
    }

    struct _cc_struct *cc = NULL;

    cc = (struct _cc_struct *) malloc(sizeof(struct _cc_struct));
    if (!cc) {
        return NULL;
    }

    cc->cd = cd;

    return (code_conv_t) cc;

}



ssize_t
code_conv(code_conv_t cc, char *out, ssize_t size, const char *in) {

    if (!in || !out || size <= 0) {
        return -1;
    }

    struct _cc_struct *pcc = (struct _cc_struct *) cc;

    if (pcc->cd == (iconv_t) - 1) {
        return -1;
    }

    char *pin = (char *) in;
    char *pout = (out);
    size_t in_size = strlen(in);
    size_t out_size = size - 1;
    size_t ret = 0;

    ret = iconv(pcc->cd, &pin, &in_size, &pout, &out_size);

    *pout = 0;

    if (ret == (size_t) - 1) {
        return -1;
    }

    return (ssize_t) (pout - out);
}

void
code_conv_close(code_conv_t cc) {
    struct _cc_struct *pcc = (struct _cc_struct *) cc;
    if (!pcc) {
        return;
    }

    iconv_close(pcc->cd);
    free(pcc);
    return;
}

ssize_t
code_gbk_to_utf8(char *out, ssize_t size, const char *in, uint32_t flag) {
    code_conv_t cc = code_conv_open(CC_UTF8, CC_GBK, flag);
    if (!cc) {
        return -1;
    }
    ssize_t ret = 0;

    ret = code_conv(cc, out, size, in);

    code_conv_close(cc);

    return ret;
}

ssize_t
code_utf8_to_gbk(char *out, ssize_t size, const char *in, uint32_t flag) {
    code_conv_t cc = code_conv_open(CC_GBK, CC_UTF8, flag);
    if (!cc) {
        return -1;
    }
    ssize_t ret = 0;

    ret = code_conv(cc, out, size, in);

    code_conv_close(cc);

    return ret;
}
