#include <stdio.h>

int
main(int argc, char *argv) {
    //生成gbk
    //
    int i;
    int j;

    unsigned char c[3];

    //一区符号

/*    for(i=0xA1;i<=0xA9;i++){
        for(j=0xA1;j<=0xFE;j++){
            c[0]=(unsigned char)i;
            c[1]=(unsigned char)j;
            c[2]=0;
            fprintf(stdout,"%s",c);
        }
        fprintf(stdout,"\n");
    }*/
    //二区 
    for (i = 0xB0; i <= 0xF7; i++) {
        for (j = 0xA1; j <= 0xF2; j++) {
            c[0] = (unsigned char) i;
            c[1] = (unsigned char) j;
            c[2] = 0;
            fprintf(stdout, "%s", c);
        }
        fprintf(stdout, "\n");
    }


    //三区
    for (i = 0x81; i <= 0xA0; i++) {
        for (j = 0x40; j <= 0x7E; j++) {
            c[0] = (unsigned char) i;
            c[1] = (unsigned char) j;
            c[2] = 0;
            fprintf(stdout, "%s", c);
        }
        fprintf(stdout, "\n");
    }
    for (i = 0x81; i <= 0xA0; i++) {
        for (j = 0x80; j <= 0xFE; j++) {
            c[0] = (unsigned char) i;
            c[1] = (unsigned char) j;
            c[2] = 0;
            fprintf(stdout, "%s", c);
        }
        fprintf(stdout, "\n");
    }
    //四区 
    for (i = 0xAA; i <= 0xFE; i++) {
        for (j = 0x40; j <= 0x7E; j++) {
            c[0] = (unsigned char) i;
            c[1] = (unsigned char) j;
            c[2] = 0;
            fprintf(stdout, "%s", c);
        }
        fprintf(stdout, "\n");
    }
    for (i = 0xAA; i <= 0xFE; i++) {
        for (j = 0x80; j <= 0xA0; j++) {
            c[0] = (unsigned char) i;
            c[1] = (unsigned char) j;
            c[2] = 0;
            fprintf(stdout, "%s", c);
        }
        fprintf(stdout, "\n");
    }
    return 0;
}
