/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: KeyWordSyntaxParser.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_KEYWORDSYNTAXPARSER_H_
#define SEARCHER_KEYWORDSYNTAXPARSER_H_

#include "SyntaxParseInterface.h"
#include "commdef/commdef.h"
#include "queryparser/qp_syntax_tree.h"

namespace queryparser {
typedef enum
{
	KFT_NORMAL,     // 普通类型
	KFT_OPERAT,     // 逻辑类型
	KFT_SUBTREE,    // 子句类型
}
_qp_keyword_field_type_t;

typedef struct {
    char *begin;
    char *end;
}qp_keyword_field_t;

class KeyWordSyntaxParser : public SyntaxParseInterface
{ 
public:
    KeyWordSyntaxParser();
    ~KeyWordSyntaxParser();

    virtual int doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
                      qp_syntax_tree_t* not_tree,
                      char* name, int nlen, char* string, int slen);

private:


    NodeLogicType qp_match_relation(char* string, int slen);

    qp_syntax_node_t *_qp_parse_normal_field(MemPool* mempool, FieldDict* field_dict, char* name, int nlen, char *string, int slen);

    int splitQueryString(MemPool *mempool, char *string, int slen, char ** sub_string);

    qp_syntax_node_t *_qp_keyword_parse(MemPool *mempool, FieldDict *field_dict, 
            char *name, int nlen, char *string, int slen);

    bool doPreProcess(char *string, int slen, char *temp);

    void swap(qp_keyword_field_t *field1, qp_keyword_field_t *field2);
};


}
#endif //SEARCHER_KEYWORDSYNTAXPARSER_H_
