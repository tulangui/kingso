#include <stdio.h>
#include <string.h>
#include <util/py_code.h>

int main(int argc, char* argv[])
{

	char buff[1024];

	while(fgets(buff, sizeof(buff), stdin)){

		char* p = strchr(buff, '\t');
		if(!p){
			continue;
		}

		int pos = 0;
		unsigned short key = get_utf8_word(buff, p-buff, &pos);
		pos = 0;
		unsigned short val = get_utf8_word(p+1, 3, &pos);

		fprintf(stdout, "%d\t%d  // %s", key, val, buff);

	}

	return 0;
}
