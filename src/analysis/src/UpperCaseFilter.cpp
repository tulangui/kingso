#include <stdlib.h>
#include "UpperCaseFilter.h"
#include "Token.h"
#include "StringUtility.h"

using namespace utility;

namespace analysis {

/* 过滤下一个token */
CToken *CUpperCaseFilter::next()
{	
	if(!m_pInput)
		return NULL;

	CToken *pToken = m_pInput->next();

	if(NULL == pToken)
		return NULL;
	
	string_utility::toUpper(pToken->m_szText);

	return pToken;
}

}
