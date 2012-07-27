#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#include "util/MD5.h"
#include "util/HashMap.hpp"

struct Value {
    uint64_t high;
    uint64_t low;
};

int main(int argc, char* argv[])
{
    if(argc != 3) {
        printf("format:%s in1 in2\n", argv[0]);
        return -1;
    }

    Value v1, v2;
    util::HashMap<uint64_t, Value> hash(10<<20);

    size_t maxLen1, maxLen2;
    ssize_t len1, len2;
    char* line1 = NULL, *line2 = NULL;

    FILE* fp = fopen(argv[1], "rb");
    if(fp == NULL) return -1;
    
    int no = 1;
    int begin = time(NULL);
    while((len1 = getline(&line1, &maxLen1, fp)) > 0) {
        if(len1 <= 0) break;
        if((len2 = getline(&line2, &maxLen2, fp)) < 0) {
            printf("read 1 error, %d\n", no);
            break;
        }
       
        while(len1 > 0 && isspace(line1[--len1]));
        while(len2 > 0 && isspace(line2[--len2]));
        
        line1[++len1] = 0;
        line2[++len2] = 0;

        util::MD5(line1, (unsigned char*)&v1);
        util::MD5(line2, (unsigned char*)&v2);

        hash.insert(v1.high, v2);
        no += 2;
    }
    fclose(fp);
    printf("read %s finish\n", argv[1]);

    fp = fopen(argv[2], "rb");
    if(fp == NULL) return -1;
    
    no = 1;
    begin = time(NULL);
    while((len1 = getline(&line1, &maxLen1, fp)) > 0) {
        if((len2 = getline(&line2, &maxLen2, fp)) < 0) {
            printf("read 1 error\n");
            break;
        }
       
        while(len1 > 0 && isspace(line1[--len1]));
        while(len2 > 0 && isspace(line2[--len2]));
        
        line1[++len1] = 0;
        line2[++len2] = 0;

        util::MD5(line1, (unsigned char*)&v1);
        util::MD5(line2, (unsigned char*)&v2);
        
        util::HashMap<uint64_t, Value>::iterator it = hash.find(v1.high);
        if(it == hash.end()) {
            printf("not find line = %d\n", no);
            return -1;
        }
        if(it->value.high != v2.high || it->value.low != v2.low) {
            printf("not same = %d\n", no);
            return -1;
        }
        no += 2;
    }
    fclose(fp);

    printf("success %s %s \n", argv[1], argv[2]);
    return 0;
}

