
#ifndef _KS_BUILD_TOEKNIZER_H_
#define _KS_BUILD_TOEKNIZER_H_

#include "scws/scws.h"
#include "analysis/Analyzer.h"
#include "analysis/MetaCharAnalyzer.h"
#include "analysis/TokenStream.h"
#include "analysis/Token.h"

#define TOKENIZER_TYPE_TAOBAOCN "TAOBAO_CHN"
#define TOKENIZER_TYPE_METACHAR "METACHAR"
#define TOKENIZER_TYPE_SCWS "SCWS"

#define SCWS_DICT_FILENAME "dict.xdb" 
#define SCWS_UTF8_DICT_FILENAME "dict.utf8.xdb" 
#define SCWS_RULE_FILENAME "rules.ini" 
#define SCWS_UTF8_RULE_FILENAME "rules.utf8.ini" 

class ITokenizer
{
public:
    ITokenizer(){}
    virtual ~ITokenizer(){}
    virtual int load(const char *pValue) = 0;
    virtual int next(const char *&pTokenText, int32_t &iTextLen, int32_t &iTokenSerial) = 0;
};

class CScwsTokenizer: public ITokenizer
{
public:
    CScwsTokenizer(const char *pConfPath, scws_t pScwsTokenizer) : m_pConfPath(pConfPath), m_pScwsTokenizer(pScwsTokenizer)
    {
        init();
    }
    ~CScwsTokenizer()
    {
       scws_free(m_pScwsTokenizer); 
    }
    virtual int load(const char *pValue)
    {
       m_pValue = pValue;
       scws_send_text(m_pScwsTokenizer, pValue, strlen(pValue));
       m_pScwsResult = scws_get_result(m_pScwsTokenizer); 
       return 0;
    }
    virtual int next(const char *&pTokenText, int32_t &iTextLen, int32_t &iTokenSerial)
    {
        if (m_pScwsResult != NULL){
           pTokenText = m_pValue + m_pScwsResult->off;
           iTextLen = m_pScwsResult->len;
           iTokenSerial = m_pScwsResult->off; 
           m_pScwsResult = m_pScwsResult->next;
           if (NULL == m_pScwsResult){
              scws_free_result(m_pScwsResult);
              m_pScwsResult = scws_get_result(m_pScwsTokenizer);
           }
           return 0;
        }
        return -1;
    }
private:
    int init()
    {
       scws_set_charset(m_pScwsTokenizer, "utf-8");
       char confFile[PATH_MAX];
       snprintf(confFile, PATH_MAX, "%s/%s", m_pConfPath, SCWS_DICT_FILENAME);
       scws_set_dict(m_pScwsTokenizer, confFile, SCWS_XDICT_XDB);
       snprintf(confFile, PATH_MAX, "%s/%s", m_pConfPath, SCWS_RULE_FILENAME);
       scws_set_rule(m_pScwsTokenizer, confFile);
       return 0;
    }
private:
    const char *m_pConfPath; 
    scws_t m_pScwsTokenizer;
    scws_res_t m_pScwsResult;
    const char *m_pszEncodeing;
    const char *m_pValue; 
};

class CKsBuildTokenizer: public ITokenizer
{
public:
    CKsBuildTokenizer(analysis::CAnalyzer *pAnalyzer)
    {
        m_pAnalyzer = pAnalyzer;
        m_pTokenStream = NULL;
    }
    virtual ~CKsBuildTokenizer()
    {
        if (m_pAnalyzer){
            delete m_pAnalyzer;
            m_pAnalyzer = NULL;
        }
    }
    virtual int load(const char *pValue)
    {
        m_pTokenStream = m_pAnalyzer->tokenStream(pValue);
        if (m_pTokenStream != NULL)
        {
            return 0;
        }
        return -1;
    }
    virtual int next(const char *&pTokenText, int32_t &iTextLen, int32_t &iTokenSerial)
    {
        analysis::CToken *pToken = NULL;
        if ((pToken = m_pTokenStream->next()) != NULL){
            pTokenText = pToken->m_szText;
            iTextLen = pToken->m_nTextLen;
            iTokenSerial = pToken->m_nSerial;
            return 0;
        }
        return -1;
    }
 private:
    analysis::CAnalyzer *m_pAnalyzer;
    analysis::CTokenStream *m_pTokenStream;
};
/*
class CAliwsTokenizer: public ITokenizer
{
public:
    CAliwsTokenizer(ws::AliTokenizer *pTokenizer, ws::SegResult *pTokenResult, int32_t iState):m_pTokenizer(pTokenizer),
        m_RetrieveIter(pTokenResult, ws::RETRIEVE_ITERATOR),m_SemanticIter(pTokenResult, ws::RETRIEVE_ITERATOR)
    {
        m_pTokenResult = pTokenResult;
        m_pSegToken = NULL;
        m_iState = iState;
    }
    virtual ~CAliwsTokenizer()
    {
        if (m_pTokenizer != NULL){
            m_pTokenizer->ReleaseSegResult(m_pTokenResult);
        }
    }
    virtual int load(const char *pValue)
    {
        int ret = -1;
        if (0 == m_iState){
            ret = m_pTokenizer->Segment(pValue, strlen(pValue)+1, ws::UTF8, ws::SEG_TOKEN_RETRIEVE | ws::SEG_TOKEN_SEMANTIC, m_pTokenResult);
        }
        else if (1 == m_iState){
            ret = m_pTokenizer->Segment(pValue, strlen(pValue)+1, ws::UTF8, ws::SEG_TOKEN_RETRIEVE_BASIC, m_pTokenResult);
        }
        if (ret != 0) {
            return -1;
        }
        m_RetrieveIter = m_pTokenResult->Begin(ws::RETRIEVE_ITERATOR);
        m_SemanticIter = m_pTokenResult->Begin(ws::SEMANTIC_ITERATOR);
        m_pSegToken = m_pTokenResult->GetFirstToken(ws::ACCESSORY_LIST);
        return 0;
    }
    virtual int next(const char *&pTokenText, int32_t &iTextLen, int32_t &iTokenSerial)
    {
        while (m_RetrieveIter != m_pTokenResult->End(ws::RETRIEVE_ITERATOR)) {
            if(0 == m_SemanticIter->pWord[0]){
                ++m_RetrieveIter;
                continue;
            }
            pTokenText = m_RetrieveIter->pWord;
            iTextLen = m_RetrieveIter->length;
            iTokenSerial = m_RetrieveIter->charOffset;
            ++m_RetrieveIter;
            return 0;
        }
        if (0 == m_iState){
            while (m_SemanticIter != m_pTokenResult->End(ws::SEMANTIC_ITERATOR)) {
                if ((m_SemanticIter->tokenType & ws::SEG_TOKEN_RETRIEVE) != 0) {
                    ++m_SemanticIter;
                    continue;
                }
                if (0 == m_SemanticIter->pWord[0]){
                    ++m_SemanticIter;
                    continue;
                }
                pTokenText = m_SemanticIter->pWord;
                iTextLen = m_SemanticIter->length;
                iTokenSerial = m_SemanticIter->charOffset;
                ++m_SemanticIter;
                return 0;
            }
            while (m_pSegToken) {
                if(m_pSegToken->pWord[0]==0){
                    continue;
                    m_pSegToken = m_pSegToken->pRightSibling;
                }
                pTokenText = m_pSegToken->pWord;
                iTextLen = m_pSegToken->length;
                iTokenSerial = m_pSegToken->charOffset;
                m_pSegToken = m_pSegToken->pRightSibling;
                return 0;
            }
        }
        return -1;
    }
 private:
    ws::AliTokenizer * m_pTokenizer;
    ws::SegResult * m_pTokenResult;
    ws::SegResult::Iterator m_RetrieveIter;
    ws::SegResult::Iterator m_SemanticIter;
    ws::SegToken * m_pSegToken;
    int32_t m_iState;
};
*/
class CTokenizerFactory
{
public:
    CTokenizerFactory()
    {
        // m_pAliTokenizerFactory = NULL;
        m_iState = 0;
        m_pszConfPath = NULL;
    }
    ~CTokenizerFactory()
    {
        if (m_pszConfPath != NULL)
        {
            free(m_pszConfPath);
            m_pszConfPath = NULL;
        }
    }
    int init(const char* cfgFile, const char *wsType, int32_t iState = 0)
    {
        if(wsType != NULL && 0 == strcasecmp(wsType, TOKENIZER_TYPE_SCWS)){
           m_pszConfPath = strdup(cfgFile);
        }
        return 0;
    }
    ITokenizer* getTokenizer(const char* tokenizerId)
    {
        if (tokenizerId != NULL && 0 == strcasecmp(tokenizerId, TOKENIZER_TYPE_METACHAR)){
            analysis::CAnalyzer *pAnalyzer = analysis::CMetaCharAnalyzer::create();
            if (pAnalyzer != NULL){
                ITokenizer *pTokenizer = new (std::nothrow) CKsBuildTokenizer(pAnalyzer);
                if (pTokenizer != NULL){
                    return pTokenizer;
                }
                delete pTokenizer;
                pTokenizer = NULL;
            }
            return NULL;
        }
        /* else if(tokenizerId != NULL && 0 == strcasecmp(tokenizerId, TOKENIZER_TYPE_TAOBAOCN)){
            ws::AliTokenizer *pAliTokenizer = m_pAliTokenizerFactory->GetAliTokenizer(tokenizerId);
            if (NULL == pAliTokenizer) {
                return NULL;
            }
            ws::SegResult  *pSegResult = pAliTokenizer->CreateSegResult();
            if (NULL == pSegResult) {
                return NULL;
            }
            ITokenizer *pTokenizer = new (std::nothrow) CAliwsTokenizer (pAliTokenizer, pSegResult, m_iState);
            if (pTokenizer != NULL)
            {
                return pTokenizer;
            }
            pAliTokenizer->ReleaseSegResult(pSegResult);
            return NULL;
        }
        */
        else if(tokenizerId != NULL && 0 == strcasecmp(tokenizerId, TOKENIZER_TYPE_SCWS)){
            scws_t pScwsTokenizer = scws_new(); 
            if (NULL == pScwsTokenizer){
                return NULL;
            }
            ITokenizer *pTokenizer = new (std::nothrow) CScwsTokenizer(m_pszConfPath, pScwsTokenizer);
            if (pTokenizer != NULL)
            {
                return pTokenizer;
            }
            return NULL;
        }
        return NULL;
    }
private:
    //ws::AliTokenizerFactory *m_pAliTokenizerFactory;
    // 0表示给建索引应，1表示query statement用 
    int32_t m_iState;
    char *m_pszConfPath; 
};

#endif //_KS_BUILD_TOEKNIZER_H_

