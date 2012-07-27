#include "commdef/SourceFilter.h"
#include "commdef/commdef.h"
#include <algorithm>
#include "util/StringUtility.h"

static bool str_cmp(char *a,char *b)//safe for NULL
{
    if(a==NULL)
        return true;
    else if(b==NULL)
        return false;
    else if (strcmp(a,b)<0) return true;
    else
        return false;
}



SourceFilter::SourceFilter()
	{}

SourceFilter::SourceFilter(const char * src)
	{
		parse(src);
	}

SourceFilter::~SourceFilter(){
	for ( int i = 0; i < _source_control.size(); ++i){
		delete[] _source_control[i];
	}
}

int32_t SourceFilter::parse(const char *str){
	if(str == NULL || strlen(str) == 0) return KS_EFAILED;
	char * src = util::replicate(str);
	strtok(_source_control,src,static_cast<int>(strlen(str)),STRSPLIT);		
	std::sort(_source_control.begin(),_source_control.end(),str_cmp);
	DELSTR(src);
	return KS_SUCCESS;	
}


int32_t SourceFilter::process(const char *src,bool &drop){
	drop = false;
        char * Svc=util::replicate(src);

           if(Svc) //check if in blacklist
             {
               if(Svc[0]!='\0')//getSvc()=='0\'
               {   
                  if(std::binary_search(_source_control.begin(),_source_control.end(),Svc,str_cmp))
                  drop=true;
               }
               else
               {
                  char *tmp="-";  
                  if(std::binary_search(_source_control.begin(),_source_control.end(),tmp,str_cmp))
                  drop=true;        
               }     
                       
             } 
           
           else //server not exists,must config "-" in merge_server
             {
               char *tmp="-";  
               if(std::binary_search(_source_control.begin(),_source_control.end(),tmp,str_cmp))
               drop=true;        
             }  
	DELSTR(Svc);
	return KS_SUCCESS;

}
void SourceFilter::strtok(std::vector<char *>& res, char *szInput, int len, char *szChars) {
	res.clear();
	char *p1 = szInput;
	while (*p1 != '\0') {
		char *p2 = szChars;
		while(*p2 != '\0') {
			if (*p1 == *p2) {
				*p1 = '\0';
				break;
			}
			p2++;
		}
		p1++;
	}

	p1 = szInput;
	while(p1 < szInput + len) {
		if(*p1 == '\0') { p1++; continue; }
		char *e = util::replicate(p1);
		res.push_back(e);
		while (*p1!='\0' && p1 < szInput + len) { p1++; }
	}
}

