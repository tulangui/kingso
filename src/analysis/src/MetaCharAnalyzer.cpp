#include "MetaCharAnalyzer.h"

#include "Analyzer.h"
#include "MetaCharTokenizer.h"
#include "StopFilter.h"
#include "LowerCaseFilter.h"
#include "UpperCaseFilter.h"
#include "GBK_T2GBK_SFilter.h"
#include "NormalizeFilter.h"

namespace analysis { 

/* 构造函数,由外部指定stop words的列表 */
CMetaCharAnalyzer::CMetaCharAnalyzer()
{
	CTokenStream* pTokenStream = new CMetaCharTokenizer();
	m_pTokenizer   = static_cast<CTokenizer *>(pTokenStream);
	pTokenStream = new CGBK_T2GBK_SFilter(pTokenStream);
	pTokenStream = new CLowerCaseFilter(pTokenStream);
	pTokenStream = new CNormalizeFilter(pTokenStream);
	pTokenStream = new CStopFilter(pTokenStream, m_szStopWords);
	m_pStopFilter = static_cast<CStopFilter *>(pTokenStream);
	m_pTokenStream = pTokenStream;   
}
/* 析构函数 */
CMetaCharAnalyzer::~CMetaCharAnalyzer()
{
}
/* 调用者负责是否释放返回值 */
CTokenStream* CMetaCharAnalyzer::tokenStream(const char *szSentence,basis::n32_t nSLen)
{
	if (NULL == szSentence)
		return NULL;
	if (nSLen <= 0)
		nSLen = strlen(szSentence);

	m_pTokenizer->setText(szSentence,nSLen);
	return m_pTokenStream;
}
/* 静态创建对象函数 */
CAnalyzer* CMetaCharAnalyzer::create()
{
	return new CMetaCharAnalyzer();
}

}
