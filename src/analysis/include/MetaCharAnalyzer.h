/******************************************************************************
*
* File     :MetaCharAnalyzer.h
* Version  :$Id: MetaCharAnalyzer.h
* Desc     :默认的分词方式,是一个字一个字分解,这是给中文分词用的,如果是英文的分词
*			需要重新写一个Analyzer

* Log      :Created by jianhao, 2009-12-15
*
******************************************************************************/
#ifndef _META_CHAR_ANALYZER_H_
#define _META_CHAR_ANALYZER_H_

#include "Analyzer.h"

namespace analysis {

class CAnalyzer;
class CTokenStream;


class CMetaCharAnalyzer : public CAnalyzer
{
public:
	/* 构造函数,构造对象可以指定stop words的列表,列表用字符串的形式表示 */
	CMetaCharAnalyzer();
	/* 析构函数 */
	virtual ~CMetaCharAnalyzer(void);
	/* 对提供的文本,创建一个token的流,可以依次取得分解并切过滤后的token(term) */
	/* 调用者在使用完返回的CTokenStream指针后，必须释放CTokenStream对象指针 */
	CTokenStream *tokenStream(const char* szSentence,basis::n32_t nSLen = 0);
	/* 静态生成该类一个对象的函数 */
	static CAnalyzer* create();
};


}// end analysis namespace

#endif // #ifndef _META_CHAR_ANALYZER_H_
