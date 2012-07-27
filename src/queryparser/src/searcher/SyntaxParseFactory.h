/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: SyntaxParseFactory.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_SYNTAXPARSEFACTORY_H_
#define SEARCHER_SYNTAXPARSEFACTORY_H_

#include "MultivalueSyntaxParser.h"
#include "util/MemPool.h"
#include "SyntaxParseInterface.h"
#include "KeyWordSyntaxParser.h"
#include "KeyvalueSyntaxParser.h"
#include "OtherSyntaxParser.h"

namespace queryparser {
class SyntaxParseFactory
{ 
public:
    SyntaxParseFactory() {}
    ~SyntaxParseFactory() {}

    SyntaxParseInterface *createSyntaxParser(MemPool *mempool, char *parser)
    {
        if(strncmp(parser, SYNTAX_KEYWORD_PARSER, 20) == 0) {
            return NEW( mempool, KeyWordSyntaxParser)();
        } else if(strncmp(parser, SYNTAX_MULTIVALUE_PARSER, 20) == 0) {
            return NEW(mempool, MultivalueSyntaxParser)();  
        } else if(strncmp(parser, SYNTAX_KEYVALUE_PARSER, 20) == 0) {
            return NEW(mempool, KeyvalueSyntaxParser)(); 
        } else if(strncmp(parser, SYNTAX_OTHER_PARSER, 20) == 0 ) {
            //return NEW(mempool, OtherSyntaxParser)();
        }

        return NULL;
    }


};


}
#endif //SEARCHER_SYNTAXPARSEFACTORY_H_
