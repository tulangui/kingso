#include <stdlib.h>
#include <string.h>
#include "Token.h"
#include "NormalizeFilter.h"
#include "StringUtility.h"
#include "BasisDef.h"

using namespace utility;

namespace analysis {

/* 过滤下一个token */
CToken *CNormalizeFilter::next()
{	
	if(!m_pInput)
		return NULL;

	CToken *pToken = m_pInput->next();

	if (NULL == pToken)
		return NULL;
	//是否存在连续的空格,单双字节的空格都算
	char *ptr, *szToken;
	if ((ptr=strstr(pToken->m_szText,"  ")) != NULL || (ptr=strstr(pToken->m_szText, "　　")) != NULL) {
		szToken = string_utility::replicate(pToken->m_szText);
		/*size_t nPos = (ptr - pToken->m_szText);	
        //计算下一个空格的位置
        if (szToken[nPos] == ' ')
            nPos++;
        else 
            nPos += 2;*/
        size_t nPos = 0;

        //为了防止出现一个汉字的第二字节和后一个汉字的头字节都为A1的情况，这里从头开始扫描
		bool bSpace = false;
		for (size_t k = nPos; k < pToken->m_nTextLen; k++) {
			if (szToken[k] == ' ' || ((unsigned char)szToken[k]==0xA1 && (unsigned char)szToken[k+1]==0xA1)) {
				if ((unsigned char)szToken[k] == 0xA1)	//双字节
					k ++;
				if (!bSpace) {
					pToken->m_szText[nPos++] = ' ';	//不能存在连续的空格
					bSpace = true;
				}
				continue;
			}
			pToken->m_szText[nPos++] = szToken[k];
			if (string_utility::isChinese(szToken[k]))
				pToken->m_szText[nPos++] = szToken[++k];
			bSpace = false;
		}
		delete[] szToken;
		pToken->m_szText[nPos] = '\0';
		pToken->m_nTextLen = nPos;
	}

	return pToken;
}

}
