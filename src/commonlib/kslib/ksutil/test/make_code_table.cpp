#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_UNICODE  65535

static void show_help(const char* progname);

int main(int argc, char* argv[])
{
	char arg = '\0';
	int key = 0;
	int value = 0;
	unsigned short array[MAX_UNICODE+1];
	char buff[1024];
	const char* table_name = NULL;
	bool value_is_key = false;
	bool unfound_as_mark = false;

	while((arg=getopt(argc, argv, "urt:"))!=-1){
		switch(arg){
			case 'u':
				unfound_as_mark = true;
			case 'r':
				value_is_key = true;
				break;
			case 't':
				table_name = optarg;
				break;
			default:
				show_help(argv[0]);
				break;
		}
	}
	if(table_name==NULL){
		show_help(argv[0]);
		exit(-1);
	}

	for(int i=0;i<=MAX_UNICODE;i++){
		if(unfound_as_mark == true){
			array[i] = '?';
		}
		else{
			array[i] = i;
		}
	}

	while(fgets(buff, sizeof(buff), stdin)){
		// skip comments
		char* p = strchr(buff, '#');
		if(p!=NULL){
			*p = 0;
		}

		int num = 0;
		if(value_is_key==true){
			num = sscanf(buff, "%x%x", &value, &key);
		}
		else{
			num = sscanf(buff, "%x%x", &key, &value);
		}
		if(num!=2){
			continue;
		}

		array[key] = value;
	}

	fprintf(stdout, "unsigned short %s[65536] = {\n", table_name);

	for(int i=0;i<=MAX_UNICODE;i++){
		fprintf(stdout, "\t%d,\n", array[i]);
	}
	fprintf(stdout, "};\n");

	return 0;
}


void show_help(const char* progname)
{
	fprintf(stderr, "usage: %s [r] -t [table_name] <stdin >stdout\n", progname);
	fprintf(stderr, "     : if with para -r, first column is value, second column is key\n");
}
