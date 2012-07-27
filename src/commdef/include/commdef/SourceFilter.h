#ifndef __SOURCE_FILTER__H
#define __SOURCE_FILTER__H
#include "util/common.h"
#include <vector>

#define DELSTR(str) if(str) { \
	           	delete [] str; \
			str = NULL; \
		     }

#define 	STRSPLIT 	":/ ,|"

class SourceFilter {
	public:
		SourceFilter();
		SourceFilter(const char *src);
		~SourceFilter();
	public:
		int32_t process(const char *src,bool & drop);
		int32_t parse(const char *str);
//		char *getSource() {return _psSource;}
//		char *getApp() {return _psAppName;}
//		char *getSvc() {return _psSvcName;}
//		char *getMachine() {return _psMachineName;}
	public:


	private:
//		void extractSource(const char *src);
		void strtok(std::vector<char *>& res, char *szInput, int len, char *szChars);

	private:
//		char * _psSvcName;
//		char * _psAppName;
//		char * _psMachineName;
//		char * _psSource;
		std::vector<char *>  _source_control;
			
};

#endif
