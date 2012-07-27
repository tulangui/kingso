/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: OtherSyntaxParser.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_OTHERSYNTAXPARSER_H_
#define SEARCHER_OTHERSYNTAXPARSER_H_
#include "SyntaxParseInterface.h"

namespace queryparser {
class OtherSyntaxParser : public SyntaxParseInterface
{ 
    public:
        OtherSyntaxParser();
        ~OtherSyntaxParser();
        
        virtual int doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
                      qp_syntax_tree_t* not_tree,
                      char* name, int nlen, char* string, int slen);
};

}
#endif
