#include <string.h>
#include "Token.h"
#include "StopFilter.h"
#include "StringUtility.h"

using namespace utility;

namespace analysis {

/* 构造函数,szStopWords字符串,各stop word之间用逗号格开 */
CStopFilter::CStopFilter(CTokenStream *pInput, const char* szStopWords) : CTokenFilter(pInput)
{
	m_reserve = 0;
	m_pStopWordsTable = NULL;
    setStopWordList(szStopWords);
}
/* 设置stop word list */
void CStopFilter::setStopWordList(const char* szStopWords)
{
    clearStopWordList();
    m_pStopWordsTable = NULL;
	if (szStopWords != NULL) {
		unsigned char cHashSizeStep = 0;
		m_pStopWordsTable = new CHashMap<char, char*>(cHashSizeStep);
		m_pStopWordsTable->setMultiKey(true);
		char *szPtr = string_utility::replicate(szStopWords);
		char *szStopWord = szPtr;
		char* pToken = strchr(szPtr, STOP_WORD_SPLIT_CHAR);
		do {
			if (pToken != NULL) {
				*pToken ++ = '\0';
			}
			if (szStopWord[0] != '\0') {	//空字符串忽略
				char *szWord = string_utility::trim(szStopWord);
				m_pStopWordsTable->insert(szWord[0], szWord);
			}
			if (pToken == NULL)
				break;
			szStopWord = pToken;
			pToken = strchr(pToken, STOP_WORD_SPLIT_CHAR);
		} while(true);
		delete[] szPtr;
	}
}
/* 清除stopwords list */
void CStopFilter::clearStopWordList()
{
	if (m_pStopWordsTable != NULL) {
		CHashMap<char, char*>::HashEntry *pEntry = NULL;
		for (pEntry = m_pStopWordsTable->m_pFirst; pEntry != NULL; pEntry = pEntry->_seq_next) {
			delete[] pEntry->_value;
		}
		delete m_pStopWordsTable;
		m_pStopWordsTable = NULL;
	}    
}
/* 稀构函数 */
CStopFilter::~CStopFilter(void)
{
	clearStopWordList();
}
/* 过滤下一个token */
CToken *CStopFilter::next()
{
	if (!m_pInput)
		return NULL;

	if (m_reserve < m_reserveTokens.size())
    {
        m_reserveTokens[m_reserve ++]->toUTF8();
		return m_reserveTokens[m_reserve ++];
    }

	m_reserve = 0;
	m_reserveTokens.clear();
	char *ptr = NULL;
	bool bEqual = false;
	CHashMap<char, char*>::HashEntry *pEntry = NULL;
	for (CToken *pToken = m_pInput->next(); pToken != NULL; pToken = m_pInput->next()) {
		if (m_pStopWordsTable != NULL) {
			pEntry = m_pStopWordsTable->lookupEntry(pToken->m_szText[0]);
			while (pEntry != NULL) {
				if (string_utility::equal(pToken->m_szText, pEntry->_value)) {	//这个token直接是一个stop word,考虑到后续还可能存在stop word,所以这里不能直接return
					bEqual = true;
					break;
				}
				if (strstr(pToken->m_pText, pEntry->_value) == pToken->m_pText && 
				    strlen(pEntry->_value) > strlen(pToken->m_szText)) {	//判断后续的字符有没有可能属于stop word
					do {
						CToken *pStopToken = m_pInput->next();
						if (pStopToken == NULL)	//等于NULL的可能性很小
                        {
                            pToken->toUTF8();
							return pToken;
                        }
						ptr = strstr(pEntry->_value,pStopToken->m_szText);
						/* 不属于stop word */
						if (ptr == NULL) {
							m_reserveTokens.push_back(pStopToken);
							break;
						}
						//判断是否为最后一个token
						size_t nLen = ptr - pEntry->_value + pStopToken->m_nTextLen;
						if (nLen == strlen(pEntry->_value)) {
							m_reserveTokens.clear();	//清空预留队列
							m_reserve = 0;
							return next();
						}
						//预留token的目的,是为了防止 自然语言分词时的正确性stop word
						//比如"她 说 的 确实 在理","的确"是stop word,这段程序就会起作用了
						else {
							m_reserveTokens.push_back(pStopToken);
						}
					} while(true);
				}
				pEntry = pEntry->_next;	//具有相同key值的entry
			}
			if (!bEqual)
            {
                pToken->toUTF8();
				return pToken;
            }
			bEqual = false;
		}
		else {
            pToken->toUTF8();
			return pToken;
		}
	}

	return NULL;
}

bool CStopFilter::IsStopWord(const char* szWord)
{
	bool b = false;
	CHashMap<char, char*>::HashEntry *pEntry = NULL;
	if (m_pStopWordsTable != NULL && szWord != NULL) {
			pEntry = m_pStopWordsTable->lookupEntry(szWord[0]);
			while (pEntry != NULL) {
				if (string_utility::equal(szWord, pEntry->_value)) {
					b = true;
					break;
				}
				pEntry = pEntry->_next;	//具有相同key值的entry
			}
	} 
	return b;
}


}
