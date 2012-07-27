#include <stdio.h>
#include <string.h>
#include <util/py_code.h>

int main(int argc, char* argv[])
{
	char buff[1024];
	int len = 0;
	int word_num = 0;
	unsigned short unicode_array[1024];

	while(fgets(buff, sizeof(buff), stdin)){
		len = strlen(buff);
		while(buff[len-1]=='\n'){
			buff[--len] = '\0';
		}
		word_num = utf8_to_unicode(buff, len, unicode_array, sizeof(unicode_array));

		for(int i=0;i<word_num;i++){
			fprintf(stdout, "%d\t", unicode_array[i]);
		}
		fprintf(stdout, "\n");
	}

	return 0;
}
