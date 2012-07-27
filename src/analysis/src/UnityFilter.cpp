#include "Token.h"
#include "UnityFilter.h"
#include "SpecialWordItem.h"
//#include "dictionary/DictionaryHelper.h"

namespace analysis {
/*构造函数*/
CUnityFilter::CUnityFilter(CTokenStream *pInput) : CTokenFilter(pInput)
{
	//m_DictHelper = NULL;
}
/*析构函数*/
CUnityFilter::~CUnityFilter()
{
}
/*设置字典句柄*/
/*需要保证pDictHelper已经init() */
/*void CUnityFilter::setDictHelper(CDictionaryHelper * pDictHelper)
{
	m_DictHelper = pDictHelper;
}*/
/*过滤下一个token*/
CToken *CUnityFilter::next()
{
	int count = 0;
	char * sz_alias = NULL;
	CSpecialWordItem *pWordItem = NULL;

	if(!m_pInput)
		return NULL;

	CToken *pToken = m_pInput->next();
	if(NULL == pToken)
		return NULL;

/*	if(m_DictHelper != NULL ){
		count = m_DictHelper->getUnityWord(pToken->m_szText, -1, pWordItem);
	}*/
	if( pWordItem!= NULL && !pWordItem->alias.empty())
	{
		sz_alias = (pWordItem->alias)[0];	
	}
	if(sz_alias != NULL)
	{
		pToken->setText(sz_alias,strlen(sz_alias));
	}

	if(pWordItem != NULL)	
	{
		delete pWordItem;
		pWordItem = NULL;
	}

	return pToken;
}
}

