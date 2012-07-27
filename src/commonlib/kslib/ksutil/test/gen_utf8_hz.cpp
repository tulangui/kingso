#include <stdio.h>
#include <string.h>
#include "util/py_code.h"

int main(int argc, char* argv[])
{
	char word[256];

	for(int i=1;i<=65535;i++){
		unsigned short val = i;
		unicode_to_utf8(&val, 1, word, sizeof(word));
		fprintf(stdout, "%s\n", word);
	}

	return 0;
}

		

