#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>

#include "util/py_code.h"

#define MAX_LEN 10240

int main(int argc, char** argv)
{
    setlocale(LC_ALL,"zh_CN.UTF-8");

    FILE* in = fopen(argv[1], "rb");
    FILE* out = fopen(argv[2], "wb");

    char buf1[MAX_LEN];
    char buf2[MAX_LEN];

    if(fgets(buf1, MAX_LEN, in) == NULL) {
        return -1;
    }

    wchar_t str[MAX_LEN];
    swprintf(str, MAX_LEN, L"%s", buf1);
    py_unicode_normalize(str);
    py_unicode_normalize(str);
    fprintf(out, "%ls", str);

    fclose(in);
    fclose(out);
    
    return 0;
}

