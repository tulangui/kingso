#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char** argv)
{
    if(argc != 5) {
        fprintf(stderr, "fomat:%s infile 年 月 日\n", argv[0]);
        return -1;
    }

    int year = atoi(argv[2]);
    int mon = atoi(argv[3]);
    int day = atoi(argv[4]);
    fprintf(stdout, "to %d-%d-%d\n", year, mon, day);
    
    char buf1[102400];
    FILE * fp1 = fopen(argv[1], "rb");
    if(NULL == fp1) return -1;
    sprintf(buf1, "%s.new", argv[1]);
    FILE * fp2 = fopen(buf1, "wb");
    if(NULL == fp2) return -1;

    while(fgets(buf1, 102400, fp1)) {
        char* p1 = strstr(buf1, "!et=");
        time_t tt = 0;
        if(p1) {
            tt = atoi(p1+4);
            struct tm pt;
            localtime_r(&tt, &pt);
            pt.tm_year = year - 1900; 
            pt.tm_mon = mon - 1; 
            pt.tm_mday = day; 
            tt = mktime(&pt);
            *p1 = 0;

            p1 += 4;
            while(*p1 && *p1 != '&') p1++;
            fprintf(fp2, "%s!et=%d%s", buf1, tt, p1); 
        } else {
            fprintf(fp2, "%s", buf1); 
        }
    }
    fclose(fp1);
    fclose(fp2);

    return 0;
}

