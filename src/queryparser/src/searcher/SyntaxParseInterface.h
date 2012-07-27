/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: SyntaxInterface.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_SYNTAXINTERFACE_H_
#define SEARCHER_SYNTAXINTERFACE_H_

#include "util/HashMap.hpp"
#include "commdef/commdef.h"
#include "util/MemPool.h"
#include "queryparser/qp_syntax_tree.h"
#include "qp_header.h"
#include "QueryParser.h"

#define SYNTAX_KEYWORD_PARSER    "syntax_keyword_parser"
#define SYNTAX_KEYVALUE_PARSER    "syntax_keyvalue_parser"
#define SYNTAX_MULTIVALUE_PARSER "syntax_multivalue_parser" 
#define SYNTAX_OTHER_PARSER      "syntax_other_parser" 
#define FILTER_SENTENCE_PARSER   "filter_sentence_parser"
#define FILTER_MULTIVALUE_PARSER "filter_multivalue_parser"
#define FILTER_KEYVALUE_PARSER   "filter_keyvalue_parser"


namespace queryparser {
class SyntaxParseInterface 
{ 
public: 
    typedef util::HashMap<const char*, qp_field_info_t*> FieldDict; 
    typedef util::HashMap<const char*, qp_field_info_t*>::iterator FieldDictIter; 

public:
    SyntaxParseInterface(){};
    ~SyntaxParseInterface(){};

    virtual int doSyntaxParse(MemPool* mempool, FieldDict *field_dict, 
                              qp_syntax_tree_t* tree, qp_syntax_tree_t* not_tree, 
                              char* name, int nlen, char* string, int slen) = 0;

};


}
#endif //SEARCHER_SYNTAXINTERFACE_H_
