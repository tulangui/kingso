/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: FilterParseFactory.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_FILTERPARSEFACTORY_H_
#define SEARCHER_FILTERPARSEFACTORY_H_

#include "KeyvalueFilterParser.h"
#include "MoreFilterParser.h"
#include "LessFilterParser.h" 
#include "MultivalueFilterParser.h" 
#include "LessMoreFilterParser.h"
#include "SyntaxParseInterface.h"
#include "FilterParseInterface.h"
#include "util/MemPool.h"
namespace queryparser {
class FilterParseFactory
{ 
public:
    FilterParseFactory(){}
    ~FilterParseFactory(){}
    
    FilterParseInterface *createFilterParser(MemPool *mempool, char *&string, char *parser, bool is_to_filter)
    {
        if(!is_to_filter) { 
            char *equal = NULL;
            char *flag = NULL;

            if(equal=strchr(string, '<')){
                string = equal+1;
                return NEW(mempool, LessFilterParser)();
            } else if(equal=strchr(string, '>')){
                string = equal+1;
                return NEW(mempool, MoreFilterParser)();
            } else if(equal=strchr(string, '=')){
                if(flag=strchr(equal+1, '[')){
                    string = flag+1;
                    return NEW (mempool, LessMoreFilterParser)();
                } else if(flag=strchr(equal+1, ':')){
                    string = flag+1;
                    return NEW(mempool, KeyvalueFilterParser)();
                } else {
                    string = equal+1;
                    return NEW(mempool, MultivalueFilterParser)();	
                }
            }
        } else {
            if(!parser) {
                return NULL;
            }
            if(strncmp(parser,  SYNTAX_KEYVALUE_PARSER, 20) == 0) {
                return NEW(mempool, KeyvalueFilterParser)();
            } else if(strncmp(parser, SYNTAX_MULTIVALUE_PARSER, 20) == 0) {
                return NEW(mempool, MultivalueFilterParser)();	
            } 
            return NULL;
        }
        return NULL;
    }
};


}
#endif //SEARCHER_FILTERPARSEFACTORY_H_
