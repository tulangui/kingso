#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <util/py_code.h>

enum code_op_t{
	TO_LOWER,
	TO_UPPER,
	TO_BJ,
	GBK_TO_UTF8,
	UTF8_TO_GBK,
	UTF8_NORMALIZE
};


static void show_help(const char* progname);

int main(int argc, char* argv[])
{
	char arg = '\0';
	char  inbuf[1*1024*1024];
	char  dest[1*1024*1024];
	code_op_t op;

	if(argc==1){
		show_help(argv[0]);
		exit(-1);
	}

	while((arg=getopt(argc, argv, "lubgjn"))!=-1){
		switch(arg){
			case 'l':
				op = TO_LOWER;
				break;
			case 'u':
				op = TO_UPPER;
				break;
			case 'b':
				op = TO_BJ;
				break;
			case 'g':
				op = GBK_TO_UTF8;
				break;
			case 'j':
				op = UTF8_TO_GBK;
				break;
			case 'n':
				op = UTF8_NORMALIZE;
				break;
			default:
				show_help(argv[0]);
				exit(-1);
		}
	}


	while(fgets(inbuf, sizeof(inbuf), stdin)){
		if(op==TO_LOWER){
			py_2lower(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else if(op==TO_UPPER){
			py_2upper(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else if(op==TO_BJ){
			py_2bj(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else if(op==GBK_TO_UTF8){
			py_gbk_to_utf8(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else if(op==UTF8_TO_GBK){
			py_utf8_to_gbk(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else if(op==UTF8_NORMALIZE){
			py_utf8_normalize(inbuf, strlen(inbuf), dest, sizeof(dest));
		}
		else{
			assert(0);
		}
		fprintf(stdout, "%s", dest);

	}

	return 0;
}

void show_help(const char* progname)
{
	fprintf(stderr, "usage : %s -lubgj <infile >outfile\n", progname);
	fprintf(stderr, "      : -l to lower\n");
	fprintf(stderr, "      : -u to upper\n");
	fprintf(stderr, "      : -b to banjiao\n");
	fprintf(stderr, "      : -g gbk to utf8\n");
	fprintf(stderr, "      : -j utf8 to gbk\n");
	fprintf(stderr, "      : -n utf8 normalize\n");
}
