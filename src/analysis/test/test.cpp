#include "Analyzer.h"
#include "MetaCharAnalyzer.h"
#include "TokenStream.h"
#include "Token.h"

using namespace analysis;

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "less params: input_string\n");
        return 0;
    }

    char *str = argv[1];
    fprintf(stdout, "%s\n", str);

    CAnalyzer *pAnalyzer = CMetaCharAnalyzer::create();
    CTokenStream *pTokenStream = pAnalyzer->tokenStream(str);
    CToken *pToken = NULL;
    while ((pToken = pTokenStream->next()) != NULL) 
    {
        fprintf(stdout, "%s(%u %u %u), ", pToken->m_szText, pToken->m_nStartOffset, pToken->m_nEndOffset, pToken->m_nSerial);
    }
    fprintf(stdout, "\n");

    delete pAnalyzer;

    return 0;
}
