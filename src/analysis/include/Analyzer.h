/******************************************************************************
 *
 * File     :CAnalyzer.h
 * Version  :$Id: Analyzer.h,v 1.9 2005/03/29 09:19:24 jamesyu Exp $
 * Desc     :分析器,负责对文本进行分词处理,提供选定的分词方式和过滤方式,对不同的
			 文本,用户需要派生自己的分析器,在分析器中指定分词方式(通过tokenizer
			 的子类)和过滤器(filter的子类)来完成

 * Log      :Create by yusiheng, 2004-12-24
 *
 ******************************************************************************/
#ifndef ANALYZER_H
#define ANALYZER_H

//#include "ObjectFactory.h"
//#include "Singleton.h"
#include "StringUtility.h"
#include "StopFilter.h"
#include "UnityFilter.h"
//#include "DictionaryHelper.h"

namespace analysis 
{

class CTokenStream;
class CTokenizer;
class CStopFilter;

class CAnalyzer
{
public:
	/* 构造函数 */
	CAnalyzer(void)
	{
		m_bIncreasePos = true;
		m_szStopWords  = NULL;
		m_pTokenizer   = NULL;
		m_pTokenStream = NULL;
		m_pStopFilter  = NULL;
//        m_pUnityFilter = NULL;
	}
	/* 在此声明析构函数,是为了由对象工厂创建的analyzer脂粉能被释放 */
	virtual ~CAnalyzer(void)
	{
		if (m_szStopWords != NULL)
			delete[] m_szStopWords;
	    if (m_pTokenStream != NULL)
        delete m_pTokenStream;
	}
	/* 对提供的文本,创建一个token的流,可以依次取得分解,过滤后的token(term) */
	virtual CTokenStream *tokenStream(const char* szSentence,n32_t nSLen = 0) = 0;
	/* 为分词器设定stop word列表 */
	virtual void setStopWordList(const char *szStopWordList)
	{
		if (m_szStopWords == NULL&&szStopWordList != NULL)
			m_szStopWords = utility::string_utility::replicate(szStopWordList);
	    if (m_pStopFilter != NULL)
	        m_pStopFilter->setStopWordList(szStopWordList);
	}
	/*设置 unityFilter的dictHelper*/
/*	virtual void setDictHelper(dictionary::CDictionaryHelper * pDictHelper)
	{
	    if (m_pUnityFilter != NULL)
	        m_pUnityFilter->setDictHelper(pDictHelper);
	}*/
	/* 各继承此基类的子类都必须将其对象回到这里注册 */
//	static void registerAnalyzer(const char* szDictionaryPath, const char *szBwsConf = NULL);
    /* 根据分词器工厂指针注册新的分词器,各继承子类负责添加各分词器 */
//    static void registerAnalyzer(utility::CObjectFactory<CAnalyzer> *pAnalyzerFactory, 
//								const char* szDictionaryPath,
//								const char *szBwsConf = NULL);
    /* 判读是否为stopword */
	bool IsStopWord(const char* szWord) {
	    if (m_pStopFilter != NULL) {
		    return m_pStopFilter->IsStopWord(szWord);
	    } else {
		    return false;
	    }
	}
	/* 判断是否需要增长位置 */
	bool isIncreasePos() {
		return m_bIncreasePos;
	}

protected:
	bool m_bIncreasePos;			//判断是否在索引时累加每个field分词出来的token位置
	char* m_szStopWords;			//本分词器要求被过滤的函数
	CTokenStream* m_pTokenStream;   //记录最后一个tokenStream，用来释放整个tokenStream链
	CTokenizer*   m_pTokenizer;     //记录最开始的tokenizer,用来设置需要分词的字符串
    CStopFilter*  m_pStopFilter;    //记录stopFilter，用来设置stopWords
//	CUnityFilter* m_pUnityFilter;   //记录UnityFilter，用来设置dictHelper
};

//typedef utility::CObjectFactory<CAnalyzer> CAnalyzerFactory;
//typedef utility::Singleton<CAnalyzerFactory> AnalyzerFactory;		//单件分词器工厂

}	//namespace analysis

#endif // ANALYZER_H
