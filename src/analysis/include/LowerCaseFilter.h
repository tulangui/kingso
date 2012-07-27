#ifndef LOWER_CASE_FILTER_H
#define LOWER_CASE_FILTER_H

#include "TokenFilter.h"

namespace analysis {

class CToken;
class CTokenStream;

class CLowerCaseFilter : public CTokenFilter
{
public:
	/* 构造函数 */
	CLowerCaseFilter(CTokenStream *pInput) : CTokenFilter(pInput){}
	/* 空稀构 */
	~CLowerCaseFilter(void){}
	/* 过滤下一个token */
	CToken *next();
};

}

#endif
