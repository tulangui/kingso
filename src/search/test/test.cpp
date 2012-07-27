#include "queryparser/QueryParser.h"
#include "online_reader/OnlineReader.h"
#include "commdef/commdef.h"
#include "queryparser/SyntaxInfo.h"
#include "search/Search.h"
#include "commdef/SearchResult.h"
#include "framework/Context.h"
#include "commdef/commdef.h"
#include "index_lib/IndexReader.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/IndexLib.h"
#include "mxml.h"
#include "util/MemPool.h"
#include "pool/MemMonitor.h"
#include <string.h>
#include <stdio.h>

using namespace queryparser;
using namespace search;
using namespace framework;
using namespace index_lib;
using namespace online_reader;

int decode_url(const char* src, int srclen, char* dest, int dsize);

int main(int argc, char* argv[])
{
    if(argc < 4)
    {
        fprintf(stderr, "less params: query_path output_path conf_path\n");
        return 0;
    }
	char* in_path = argv[1];
	char* out_path = argv[2];
    char* conf_path = argv[3];
	FILE* in_file = 0;
	FILE* out_file = 0;
    FILE* conf_file = 0;
	char line[MAX_QYERY_LEN+1];
//	char temp[MAX_QYERY_LEN+1];
	QueryParser qp;
    Search se;
    Context context;
    mxml_node_t *pXMLTree;
    IndexReader *pIndexReader = IndexReader::getInstance();
    ProfileManager *pProfileMgr = ProfileManager::getInstance();
    ProfileDocAccessor *pProfileDoc = ProfileManager::getDocAccessor();

    OnlineReader ol;

    if(pIndexReader == NULL)
    {
        fprintf(stderr, "IndexReader get failed!\n");
        return -1;
    }
	if(pProfileMgr == NULL)
    {
        fprintf(stderr, "ProfileManager get failed!\n");
        return -1;
    }
	if(pProfileDoc == NULL)
    {
        fprintf(stderr, "ProfileDocAccessor get failed!\n");
        return -1;
    }

	in_file = fopen(in_path, "rb");
	if(!in_file)
    {
		fprintf(stderr, "open query file error, exit!\n");
		return -1;
	}	
    out_file = fopen(out_path, "w");
	if(!out_file)
    {
		fprintf(stderr, "open output file error, exit!\n");
        fclose(in_file);
		return -1;
	} 
    conf_file = fopen(conf_path, "rb");
	if(!conf_file)
    {
		fprintf(stderr, "open conf file error, exit!\n"); 
        fclose(in_file);
        fclose(out_file);
		return -1;
	}
    pXMLTree = mxmlLoadFile(NULL, conf_file, MXML_NO_CALLBACK);
    if (pXMLTree == NULL)
    {
        fprintf(stderr, "parse conf file failed!\n");
        fclose(conf_file);
        fclose(in_file);
        fclose(out_file);
        return -1;
    }
    fclose(conf_file);
		
    MemPool memPool;
    MemMonitor memMonitor(&memPool, 1024*1024*200);
    memPool.setMonitor(&memMonitor);
    memMonitor.enableException();
    context.setMemPool(&memPool);
    if(init(pXMLTree))
    {
		printf("indexlib init error, exit!\n");
		fclose(in_file);
		fclose(out_file);
		return -1;
	}	
	if(qp.init(pXMLTree))
    {
		printf("qp init error, exit!\n");
		fclose(in_file);
		fclose(out_file);
		return -1;
	}	
    if(ol.init(pXMLTree))
    {
		printf("online_reader init error, exit!\n");
		fclose(in_file);
		fclose(out_file);
		return -1;
	}
    if(se.init(pXMLTree))
    {
		printf("search init error, exit!\n");
		fclose(in_file);
		fclose(out_file);
		return -1;
	}
	const ProfileField *pField = pProfileDoc->getProfileField("nid");
    if(!pField)
    {
        printf("no nid field in profile, exit!\n");
        fclose(in_file);
        fclose(out_file);
        return -1;
    }
	while(fgets(line, MAX_QYERY_LEN+1, in_file))
    {
		//decode_url(line, strlen(line), temp, MAX_QYERY_LEN+1);
		QPResult qpresult(&memPool);
        int ret = 0;
        line[strlen(line)-1] = 0;
        ret = qp.doParse(&context, &qpresult, line);
		if(ret != 0)
        {
            fprintf(stderr, "parse failed! %d : %s\n", ret, line);
			continue;
		}
        SearchResult seRet;
        ret = se.doQuery(&context, &qpresult, &seRet);
        if(ret != 0)
        {
            fprintf(stderr, "doQuery failed! %d : %s\n", ret, line);
            continue;
        }
        fprintf(out_file, "%s\n", line);
        fprintf(out_file, "docsearch: %u    docfound: %u\n", seRet.nDocSearch, seRet.nDocFound);
        if(seRet.nDocFound > 20)
            seRet.nDocFound = 20;
        for(uint32_t i = 0; i < seRet.nDocFound; i++)
        {
            fprintf(out_file,"%u ", seRet.nDocIds[i]);
        }
        fprintf(out_file, "\n");

        fprintf(stdout,"http://10.232.41.135:5511/bin/search?_sid_=");
        for(uint32_t i = 0; i < seRet.nDocFound; i++)
        {
            fprintf(stdout,"%lu", pProfileDoc->getInt(seRet.nDocIds[i], pField));
            if (i != seRet.nDocFound-1)
                fprintf(stdout,",");
            //fprintf(out_file,"%lu ", pProfileDoc->getInt(seRet.nDocIds[i], pField));
        }
        fprintf(out_file, "\n\n");

        memPool.reset();
		//qpresult.print();
	}
	
	fclose(in_file);
	fclose(out_file);
	
	return 0;
}

int decode_url(const char* src, int srclen, char* dest, int dsize)
{
	int val = 0;
	int dcnt = 0;
	char tmp[3];

	tmp[2] = '\0';

	for(int i=0;i<srclen;){
		if(dcnt+1>=dsize){
			return -1;
		}
		unsigned char ch = src[i];
		if(ch=='%' && i+2<srclen){
			tmp[0] = src[i+1];
			tmp[1] = src[i+2];
			sscanf(tmp, "%x", &val);
			dest[dcnt++] = val;
			i+=3;
		}
		else{
			if(ch=='+'){
				dest[dcnt++] = ' ';
			}
			else{
				dest[dcnt++] = ch;
			}
			i+=1;
		}
	}

	dest[dcnt] = 0;

	return dcnt;
}
