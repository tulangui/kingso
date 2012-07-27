#ifndef TOKENFILTER_H
#define TOKENFILTER_H

#include "TokenStream.h"

namespace analysis {

class CToken;

class CTokenFilter : public CTokenStream
{
public:
	/* 构造函数 */
	CTokenFilter(CTokenStream *pInput)
	{
		pNext = pInput;
		m_pInput = pInput;
	}
	/* 稀构函数 */
	virtual ~CTokenFilter(void) {}

protected:
	CTokenStream *m_pInput;	//保留

};

}  //end namespace

#endif
