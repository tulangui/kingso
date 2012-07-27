#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <map>
#include <cassert>

using namespace std;

void show_help(const char* progname);
int split(const char* s,vector<string>& list,const char *delim);

int main(int argc, char* argv[])
{
	int key = 0;
	int val = 0;
	char arg = '\0';
	char buff[1024];
	char* dictfile = NULL;
	FILE* fp = NULL;
	vector<string> vec;
	map<int, int> mymap;
	map<int, int>::iterator it;


	while((arg=getopt(argc, argv, "d:"))!=-1){
		switch(arg){
			case 'd':
				dictfile = optarg;
				break;
			default:
				show_help(argv[0]);
				exit(-1);
		}
	}

	fp = fopen(dictfile, "rb");
	assert(fp);
	while(fgets(buff, sizeof(buff), fp)){
		split(buff, vec, "\t \n");
		key = strtol(vec[0].c_str(), NULL, 0);
		val = strtol(vec[1].c_str(), NULL, 0);
		mymap[key] = val;
	}
	fclose(fp);
	
	while(fgets(buff, sizeof(buff), stdin)){
		// skip comments
		char* p = strchr(buff, '#');
		if(p!=NULL){
			*p = 0;
		}

		int num = 0;
		num = sscanf(buff, "%x%x", &key, &val);
		if(num!=2){
			continue;
		}
		it = mymap.find(val);
		if(it!=mymap.end()){
			val = it->second;
			fprintf(stderr, "haha\n");
		}

		fprintf(stdout, "0x%X\t0x%X\n", key, val);

	}

	return 0;
}

void show_help(const char* progname)
{
	fprintf(stderr, "%s -d dictfile <stdin >stdout\n", progname);
}


int split(const char* s,vector<string>& list,const char *delim)
{
	const char* begin = s;
	const char* end;

	list.clear();
	while (*begin) {
		while (*begin && strchr(delim,*begin))
			++begin;

		end = begin;
		while (*end && strchr(delim,*end) == NULL)
			++end;

		if (*begin)
			list.push_back(string(begin,end - begin));

		begin = end;
	}

	return list.size();
}
