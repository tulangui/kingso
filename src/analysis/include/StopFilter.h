#ifndef STOP_FILTER_H
#define STOP_FILTER_H

#include "TokenFilter.h"
#include "HashMap.h"
//#include "Svector.h"
#include <vector>

using namespace utility;

namespace analysis {

class CTokenStream;

class CStopFilter : public CTokenFilter
{
public:
	/* 构造函数,szStopWords字符串,各stop word之间用逗号格开 */
	CStopFilter(CTokenStream *pInput, const char* szStopWords);
	/* 设置stop words */
	void setStopWordList(const char *szStopWords);
	/* 稀构函数 */
	~CStopFilter(void);
	/* 过滤下一个token */
	CToken *next();
	/* 一个词是否是一个stopword */
	bool IsStopWord(const char* szWord);

protected:
    /* 清除stopwords list */
    void clearStopWordList();
        
private:
	basis::n32_t m_reserve;									//预留值取出大小
    std::vector<CToken *> m_reserveTokens;		//在判断过程中,留存下的token
	CHashMap<char, char*> *m_pStopWordsTable;	//用stop word的第一个字节建立hashmap对应表

};

}	//end namespace analysis

#endif
