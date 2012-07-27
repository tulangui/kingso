#ifndef __UNITYFILTER_H__
#define __UNITYFILTER_H__

#include "TokenFilter.h"
//#include "dictionary/DictionaryHelper.h"

namespace analysis 
{
	class CTokenStream;
	
	class CUnityFilter :public CTokenFilter 
        {
	public:
		/*构造函数*/
		CUnityFilter(CTokenStream *pInput);
		/*析构函数*/
		~CUnityFilter();
		/* 过滤下一个token */
		CToken *next();
		/*设置字典句柄*/
//		void setDictHelper(dictionary::CDictionaryHelper * pDictHelper);

//	private:
//		dictionary::CDictionaryHelper *m_DictHelper;
	};
}

#endif

