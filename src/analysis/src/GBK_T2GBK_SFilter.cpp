#include <stdlib.h>
#include "GBK_T2GBK_SFilter.h"
#include "Token.h"

namespace analysis
{

static unsigned char tbGBK_T2GBK_S[] = {
	#include "GBK_T2GBK_S.h"
};

bool CGBK_T2GBK_SFilter::isGBK1(unsigned char c)
{
	return (c>0x81 - 1)&&(c<0xFD + 1);
}

bool CGBK_T2GBK_SFilter::isGBK2(unsigned char c)
{
	return (c>0x40 - 1) && (c<0xFE + 1) && (c!=0x7F);
}

int CGBK_T2GBK_SFilter::GBKTableOffset(unsigned char c1, unsigned char c2)
{
	int nRet = (((int)c1 - 0x81)*(0xFE - 0x40 + 1) + ((int)c2 - 0x40)) << 1;
	return nRet;
}

bool CGBK_T2GBK_SFilter::isChineseCharacter(unsigned char c1, unsigned char c2)
{
	return (c1==0xA3&&(c2>0xA0&&c2<0xFF));
}

long CGBK_T2GBK_SFilter::ConvertTo(const char *pchFrom, char *pchTo)
{
	if(NULL == pchFrom || NULL == pchTo)
		return -1;

	int nOffset, n = 0;
	const char *szSrc = pchFrom;

	if (tbGBK_T2GBK_S==NULL || szSrc == NULL || strlen(szSrc) < 2)
		return -1;

	long lLen = strlen(pchFrom);
	for (int i = 0; szSrc[i] && i < lLen; i++) {
		if (isChineseCharacter(szSrc[i], szSrc[i+1])) {
			pchTo[n++] = (unsigned char)szSrc[++i] - 128;
		}
		else if (isGBK1(szSrc[i]) && isGBK2(szSrc[i+1])) {
			nOffset = GBKTableOffset(szSrc[i], szSrc[i+1]);
			pchTo[n++] = tbGBK_T2GBK_S[nOffset];
			pchTo[n++] = tbGBK_T2GBK_S[nOffset+1];
			i++;
		}
		else {
			pchTo[n++] = szSrc[i];
		}
	}
	pchTo[n] = '\0';

	return n;
}

CToken* CGBK_T2GBK_SFilter::next()
{
	if(!m_pInput)
		return NULL;

	CToken *pToken = m_pInput->next();

	if(NULL == pToken)
		return NULL;

	int nOffset, n = 0;
	const char *szSrc = pToken->m_szText;

	if (tbGBK_T2GBK_S==NULL || szSrc == NULL || strlen(szSrc) < 2)
		return pToken;

	for (int i = 0; szSrc[i]; i++) {
		if (isChineseCharacter(szSrc[i], szSrc[i+1])) {
			pToken->m_szText[n++] = (unsigned char)szSrc[++i] - 128;
            //双字节转为单字节，长度需要减一
            pToken->m_nTextLen--;
		}
		else if (isGBK1(szSrc[i]) && isGBK2(szSrc[i+1])) {
			nOffset = GBKTableOffset(szSrc[i], szSrc[i+1]);
			pToken->m_szText[n++] = tbGBK_T2GBK_S[nOffset];
			pToken->m_szText[n++] = tbGBK_T2GBK_S[nOffset+1];
			i ++;
		}
		else {
			pToken->m_szText[n++] = szSrc[i];
		}
	}
	pToken->m_szText[n] = '\0';

	return pToken;
}

}

