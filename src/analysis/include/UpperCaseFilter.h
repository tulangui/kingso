/******************************************************************************
*
* File     :UpperCaseFilter.h
* Version  :$Id: UpperCaseFilter.h v 0.1  2009/09/07  09:10  
* Desc     :将Token转化成大写
*			
* Log      :
*
******************************************************************************/

#ifndef UPPER_CASE_FILTER_H
#define UPPER_CASE_FILTER_H

#include "TokenFilter.h"

namespace analysis {

class CToken;
class CTokenStream;

class CUpperCaseFilter : public CTokenFilter
{
public:
	/* 构造函数 */
	CUpperCaseFilter(CTokenStream *pInput) : CTokenFilter(pInput){}
	/* 稀构函数 */
	~CUpperCaseFilter(void){}
	/* 过滤下一个token */
	CToken *next();
};

}

#endif
