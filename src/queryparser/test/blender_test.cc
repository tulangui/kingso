#include "queryparser/QRWResult.h"
#include "queryparser/QueryParserRewriter.h"
#include "commdef/commdef.h"
#include "util/MemPool.h"
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


using namespace queryparser;


typedef struct
{
	struct timeval begin;
	struct timeval end;
}
qp_timer_t;

void timer_start(qp_timer_t* timer)
{
	gettimeofday(&timer->begin, 0);
}

unsigned long long timer_stop(qp_timer_t* timer)
{
	gettimeofday(&timer->end, 0);
	
	return ((timer->end.tv_sec-timer->begin.tv_sec)*1000000+(timer->end.tv_usec-timer->begin.tv_usec));
}

int main(int argc, char* argv[])
{
	char* query_path = (char*)"./input.txt";
	char* conf_path = (char*)"./queryparser.xml";
	char* log_conf_path = (char*)"./queryparser.xml";
	mxml_node_t* root = 0;	
	FILE* query_file = 0;
	FILE* conf_file = 0;
	char line[MAX_QYERY_LEN+1];
	QueryParserRewriter qpr;
	qp_timer_t timer;
	unsigned long long time = 0;
	unsigned long long cnt = 0;
	int gogo = 0;
	MemPool mempool;
	
	if(argc>3){
		log_conf_path = argv[3];
		alog::Configurator::configureLogger(log_conf_path);
	}
	else{
		alog::Configurator::configureRootLogger();
	}
	
	if(argc>1){
		conf_path = argv[1];
	}
	
	conf_file = fopen(conf_path, "rb");
	if(!conf_file){
		TERR("fopen error, exit!\n");
		return -1;
	}
	
	root = mxmlLoadFile(0, conf_file, MXML_NO_CALLBACK);
	if(!root){
		TERR("mxmlLoadFile error, exit!\n");
		fclose(conf_file);
		return -1;
	}
	
	fclose(conf_file);
	
	if(argc>2){
		query_path = argv[2];
	}
	
	query_file = fopen(query_path, "rb");
	if(!query_file){
		TERR("fopen error, exit!\n");
		return -1;
	}
	
	if(qpr.init(root)){
		TERR("qpr init error, exit!\n");
		fclose(query_file);
		return -1;
	}
	
	mxmlRelease(root);
	
	if(argc>4){
		gogo = atoi(argv[4]);
	}
	
	do{
		fseek(query_file, 0, SEEK_SET);
		while(fgets(line, MAX_QYERY_LEN+1, query_file)){
			int len = strlen(line);
			if(line[len-1]=='\n'){
				line[len-1] = 0;
			}
			if(len<=0){
				continue;
			}
			TDEBUG("query : %s", line);
			timer_start(&timer);
			QRWResult qrwresult(&mempool);
			if(qpr.doRewrite(0, &qrwresult, line)){
				TERR("[%llu]qpr doRewrite error : %s!", cnt, line);
				continue;
			}
			time += timer_stop(&timer);
			qrwresult.print();
			TDEBUG("=============================\n");
			cnt ++;
			if(cnt%1000000==0){
				TNOTE("@@@ processed %llu query, average time %llu usec", cnt, time/cnt);
			}
			mempool.reset();
		}
	} while(gogo);
	
	fclose(query_file);
	
	TNOTE("@@@ processed %llu query, average time %llu usec\n", cnt, (unsigned long long)(time/cnt));
	
	return 0;
}

