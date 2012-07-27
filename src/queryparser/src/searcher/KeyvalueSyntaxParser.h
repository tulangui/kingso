/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: KeyvalueSyntaxParser.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_KEYVALUESYNTAXPARSER_H_
#define SEARCHER_KEYVALUESYNTAXPARSER_H_

#include "SyntaxParseInterface.h"
#include "commdef/commdef.h"
#include "queryparser/qp_syntax_tree.h"
namespace queryparser {
class KeyvalueSyntaxParser : public SyntaxParseInterface
{ 
    public:
        KeyvalueSyntaxParser();
        ~KeyvalueSyntaxParser();
        
        virtual int doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
                      qp_syntax_tree_t* not_tree,
                      char* name, int nlen, char* string, int slen);
};
}
#endif //SEARCHER_KEYVALUESYNTAXPARSER_H_
