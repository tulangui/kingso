#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "util/charsetfunc.h"

#define SIZE_1 (1024) //转换前的缓冲区长度
#define SIZE_2 (1024) //转换后的缓冲区长度

bool test_open_close(int argc, char *argv[]){
    code_conv_t ct;

    ct = code_conv_open(CC_UTF8, CC_GBK, CC_FLAG_IGNORE);
    if (!ct) {
        return false;
    }

    code_conv_close(ct);

    return true;
}

// [NULL,0-n] [NULL or string]
bool test_code_conv(int argc, char *argv[]){

    if (argc != 2) {
        fprintf(stderr,"%s: 2 params required.\n",__FUNCTION__);
        return false;
    }

    code_conv_t ct;
    int ret = 0;

    fprintf(stderr,"Call code_conv_open(CC_UTF8, CC_GBK, CC_FLAG_IGNORE))\n");
    ct = code_conv_open(CC_UTF8, CC_GBK, CC_FLAG_IGNORE);
    if (!ct) {
        fprintf(stderr,"code_conv_open() failed.\n");
        return false;
    }

    char *out = NULL;
    char *in = NULL;
    ssize_t out_len = 0;

    if (strcasecmp(argv[0],"NULL") == 0) {
        out = NULL;
        out_len = 0;
    }
    else{
        out_len = atol(argv[0]);
        if(out_len == 0){
            out = (char*)malloc(1);
        }
        else{
            out = (char*)malloc(out_len);
        }
    }

    in = argv[1];

    fprintf(stderr, "Call code_conv(ct, %p, %lld, \"%s\")\n", out, (long long int)out_len, in);
    ret = code_conv(ct, out, out_len, in);

    fprintf(stderr, "code_conv() returned %d\n", ret);


    code_conv_close(ct);

    free(out);
    out = NULL;

    return true;
    
}

bool test_code_conv_stream(int argc, char *argv[]){
    code_conv_t ct;

    //打开一个code_conv句柄
    ct = code_conv_open(CC_UTF8, CC_GBK, CC_FLAG_IGNORE); 
    if (!ct) {
        fprintf(stderr, "句柄打开失败\n");
        return false;
    }

    int line_len = SIZE_1; //输入的缓冲区长度
    int out_len = SIZE_2;  //输出的缓冲区长度

    char *line = new char[line_len];
    char *out = new char[out_len];

    int ret = 0;

    while (fgets(line, line_len, stdin) != NULL) {
        ret = code_conv(ct, out, out_len, line);
        fprintf(stderr, "ret=%d, out:%s", ret, out);
        fprintf(stdout, "%s", out);
    }

    code_conv_close(ct);

    return true;
}


bool test_quick_conv(int argc, char *argv[]){
    if (argc != 1) {
        fprintf(stderr, "test_code_conv(): 1 param required.\n");
        return false;
    }

    char *test_str = argv[0];
    int src_len = strlen(argv[0]);
    int dest_len = src_len * 2;
    char *dest_str = (char*)malloc(dest_len + 1);
    char *back_str = (char*)malloc(src_len + 1);
    if (!dest_str || !back_str) {
        fprintf(stderr, "Out of memory!\n");
        free(dest_str);
        free(back_str);
        return false;
    }
    
    code_utf8_to_gbk(dest_str, dest_len, test_str, CC_FLAG_IGNORE);
    code_gbk_to_utf8(back_str, src_len, dest_str, CC_FLAG_IGNORE);

    fprintf(stdout, "%s", back_str);

    free(dest_str);
    dest_str = NULL;
    free(back_str);
    back_str = NULL;

    return true;
}

void print_usage(const char *argv0){

    fprintf(stderr, "Usage: %s (tool) [params]\n\n", argv0);

    fprintf(stderr, "tool:\n\n");

    fprintf(stderr, "\tcode_conv [OUTPUT_LENGTH] [INPUT_STRING]\n");
    fprintf(stderr, "\t\tOUTPUT_LENGTH: NULL或>=0整数。\n");
    fprintf(stderr, "\t\tINPUT_STRING: UTF-8字符串\n\n");

    fprintf(stderr, "\tquick_conv [UTF8_STRING]\n\n");
    fprintf(stderr, "\t\t测试快速转换函数，将进行UTF8 => GBK => UTF8转换\n");
    fprintf(stderr, "\t\t输出到标准输出\n\n");

    fprintf(stderr, "\tcode_conv_stream\n");
    fprintf(stderr, "\t\t读取标准输入，转换为GBK写入标准输出\n\n");

    fprintf(stderr, "\topen_close\n\n");

    return;
}

int
main(int argc, char *argv[]) {

    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }

    bool ret = false;

    if (strcmp(argv[1], "quick_conv") == 0) {
        ret = test_quick_conv(argc-2, &argv[2]);
    }
    else if (strcmp(argv[1], "code_conv_stream") == 0) {
        ret = test_code_conv_stream(argc-2, &argv[2]);
    }
    else if (strcmp(argv[1], "code_conv") == 0) {
        ret = test_code_conv(argc-2, &argv[2]);
    }
    else{
        print_usage(argv[0]);
        return -1;
    }

    if(ret){
        return 0;
    }
    return -1;

}
