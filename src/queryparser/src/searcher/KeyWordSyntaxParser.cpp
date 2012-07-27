#include "KeyWordSyntaxParser.h"
#include "StringUtil.h"
#include "SyntaxParseFactory.h"
#include "FilterParseFactory.h"
#include "index_lib/IndexReader.h"
#include "util/Log.h"


using namespace index_lib;

namespace queryparser {
KeyWordSyntaxParser::KeyWordSyntaxParser()
{
}
KeyWordSyntaxParser::~KeyWordSyntaxParser()
{
}

NodeLogicType KeyWordSyntaxParser::qp_match_relation(char* string, int slen)
{
	if(slen==2 && string[0]=='O' && string[1]=='R'){
		return LT_OR;
	}
	if(slen==3 && string[0]=='A' && string[1]=='N' && string[2]=='D'){
		return LT_AND;
	}
	if(slen==3 && string[0]=='N' && string[1]=='O' && string[2]=='T'){
		return LT_NOT;
	}
	return LT_NONE;
}

qp_syntax_node_t *KeyWordSyntaxParser::_qp_parse_normal_field(MemPool* mempool, FieldDict* field_dict, char* name, int nlen, char *string, int slen)
{
    SyntaxParseFactory syntax_factory;
    FilterParseFactory filter_factory;
    SyntaxParseInterface *p_syntax = NULL;
    FilterParseInterface *p_filter = NULL;
    qp_syntax_tree_t* tree = NULL;
    qp_syntax_tree_t* not_tree = NULL;
    char *field_name = NULL;
    char *field_value = NULL;
    int real_nlen = 0;
    int real_vlen = 0;
    char temp_str[1024];
    qp_syntax_node_t *node = NULL;
    if(name == NULL || nlen == 0) {
        return NULL; 
    }
    char* p = strstr(string, "::");
	
	IndexReader* reader = IndexReader::getInstance();
    FieldDictIter iter;
	if(p){
        field_name = string;
        *p = '\0';
        iter = field_dict->find(field_name);
        if(iter == field_dict->end()) {
            TNOTE("field_dict did not find field:%s", field_name);
            return NULL;
        }
        if(iter->value->arg_dest==QP_SYNTAX_DEST ) {
            field_name = iter->value->field;
            if( strcmp(iter->value->parser, SYNTAX_KEYWORD_PARSER) == 0){
                *p = ':';
                char *q = strchr(p+2, ':');
                if(q) {
                    field_name = string;
                    *q = '\0';
                    field_value = q+1;
                } else {
                    field_value = p+2;
                }
                p_syntax = syntax_factory.createSyntaxParser(mempool, iter->value->parser);
                if(!p_syntax){
                    TNOTE("create syntax parser error, return!");
                    return NULL;
                }
            } else {
                field_value = p+2;
                p_syntax = syntax_factory.createSyntaxParser(mempool, iter->value->parser);
                if(!p_syntax){
                    TNOTE("create syntax parser error, return!");
                    return NULL;
                }
            }
        } else {
            TNOTE("qp_parser_manager_find error, return!");
            return NULL;
        }
        tree = qp_syntax_tree_create(mempool);
        not_tree = qp_syntax_tree_create(mempool);
        if(!tree || !not_tree){
            TNOTE("qp_syntax_tree_create error, return!");
            return NULL;
        }
        if(p_syntax && p_syntax->doSyntaxParse(mempool, field_dict, tree, not_tree, field_name, strlen(field_name), field_value, strlen(field_value))){
            TNOTE("syntax_parse_func error, return!");
            return NULL;
        }
        return tree->root;
    } else {
        node = qp_syntax_node_create(mempool, name, nlen, string, slen, LT_NONE, 0, false);
        return node;
    }
    return NULL;
}

void str_trim(char *&begin, char *&end) 
{
    char *cur = begin;
    int braket_level = 0;

    while(begin < end-1) {
        if(*begin == '('  && *end == ')') {
            for(cur = begin; cur <= end; cur++) {
                if(*cur == '(') {
                    braket_level ++;
                }
                if(*cur == ')') {
                    braket_level --; 
                    if(braket_level <= 0) {
                        break; 
                    }
                }
            }
            if(cur == end) {
                *begin = 0;
                *end = 0;
                begin ++;
                end--;
            } else {
                break;
            }
        } else {
            if(*begin == ' ' || *begin == '\t') {
                *begin = 0;
                begin ++; 
            } else {
                if(*end == ' ' || *end == '\t') {
                    *end = 0; 
                    end --;
                } else {
                    break;
                }
            }
        }
    }
    return;
}

int KeyWordSyntaxParser::splitQueryString(MemPool *mempool, char *string, int slen, char ** sub_string) 
{
    qp_keyword_field_t fields[1024];
    char *cur = NULL;
    int index = 0;
    char *begin = string;
    char *end = string+slen-1;
    char *p_bak = NULL;
    char *space = NULL; 
    char *p = NULL;
    int braket_level = 0;
    NodeLogicType relation;
    if(slen == 1) {
        sub_string[0] = string;
        return 1;
    }
    str_trim(begin, end);
    bool is_last_or = false;
    if(strncmp(begin, "NOT", 3) == 0) {
        if(*end == ')') {
            for(cur = end; cur>=begin; cur--) {
                if(*cur == ')') {
                    braket_level ++;
                }
                if(*cur == '(') {
                    if(--braket_level <= 0 ) {
                        break;
                    }
                }
            }
            p = cur-1;
            while(p > begin && *p == ' ' || *p == '\t') {
                p--;
            }
            while(p > begin && *p != ' ') {
                p--;
            }
            relation = qp_match_relation(p+1, 2);
            if(relation == LT_OR) { 
                is_last_or = true;
            }
        } else {
            cur = end; 
            while(cur > begin && *cur != ' ') {
                cur --;
            }
            while(cur > begin && *cur == ' ') {
                cur--;
            }
            while(cur > begin && *cur != ' ') {
                cur--; 
            }
            relation = qp_match_relation(cur+1, 2);
            if(relation == LT_OR) { 
                is_last_or = true;
            }
        }
    }
    if(strncmp(begin, "NOT", 3) == 0 && !is_last_or == true) {
        cur = begin + 3;
        while(cur < end && *cur == ' ') {
            cur++;
        }
        if(cur == end) {
            TWARN("q is err");
            return -1;
        }
        if(*cur == '(') {
            cur++;
            char *tmp = cur;
            sub_string[2] = cur; 
            int braket_level = 1;
            for(; cur<end; cur++) {
                if(*cur == '(') {
                    braket_level++;
                } else if(*cur == ')') {
                    braket_level--;
                    if(braket_level <= 0) {
                        break;
                    }
                }
            }
            if(cur == end || cur++ == end || cur == tmp+1 ) {
                TWARN("q is err");
                return -1;
            }
            *(cur-1) = '\0';
            while(cur < end && *cur == ' ') {
                cur++;
            }
            if(cur == end) {
                TWARN("q is err");
                return -1;
            }
            p = cur;
            while(cur < end && *cur != ' ') {
                cur++;
            }
            relation = qp_match_relation(p, cur-p);
            if(relation == LT_OR) {
                TWARN("q is err : not a or b");
                return -1;
            } else if(relation == LT_AND) {
                sub_string[0] = cur;
                if(cur==end) {
                    TWARN("q is err");
                    return -1;
                }
            } else { 
                sub_string[0] = p;
            }
            sub_string[1] = (char *) NEW_VEC(mempool, char, 4);
            snprintf(sub_string[1], 4, "%s", "NOT");
            return 3;
        } else {
            TWARN("q is error");
            return -1;
        }
    } else {
        sub_string[0] = begin;
        if(*end == ')') {
            for(cur = end; cur>=begin; cur--) {
                if(*cur == ')') {
                    braket_level ++;
                }
                if(*cur == '(') {
                    if(--braket_level <= 0 ) {
                        break;
                    }
                }
            }
            if(cur == begin && *cur != '(') {
                TERR("braket not match");
                return -1; 
            }
            *end = '\0';
            p = end-1;
            while(p > begin && *p == ' ' || *p == '\t') {
                *p = 0;
                p--;
            }
            p = cur+1;
            str_trim(p, end); 
            sub_string[2] = p;
            if(cur == begin) {
                sub_string[0] = p;
                return 1; 
            }
            cur--; 
            while(cur > begin && *cur == ' ') {
                cur--;
            }
            *(cur+1) = '\0';
            while(cur > begin && *cur != ' ') {
                cur--; 
            }
            p = cur+1;
            relation = qp_match_relation(p, strlen(p));
            if(relation != LT_NONE) { 
                sub_string[1] = p;
            } else {
                sub_string[1] = (char *) NEW_VEC(mempool, char, 4);
                snprintf(sub_string[1], 4, "%s", "AND");
                return 3; 
            }
            while(cur > begin && *cur == ' ') {
                cur--; 
            }
            if(cur == begin && *cur == ' ') {
                TERR("q is error!");
                return -1;
            }
            *(cur + 1 ) = '\0';
            return 3;
        } else {
            cur = end; 
            while(cur > begin && *cur != ' ') {
                cur --;
            }
            if(cur == begin && *cur != ' '){
                return 1; 
            }
            sub_string[2] = cur+1;

            while(cur > begin && *cur == ' ') {
                cur--;
            }
            *(cur+1) = '\0';
            while(cur > begin && *cur != ' ') {
                cur--; 
            }
            p = cur+1;
            relation = qp_match_relation(p, strlen(p));
            if(relation != LT_NONE) { 
                sub_string[1] = p;
            } else {
                sub_string[1] = (char *) NEW_VEC(mempool, char, 4);
                snprintf(sub_string[1], 4, "%s", "AND");
                return 3; 
            }
            while(cur > begin && *cur == ' ') {
                cur--; 
            }
            if(cur == begin && *cur == ' ') {
                TERR("q is error!");
                return -1;
            }
            *(cur + 1 ) = '\0';
            return 3;
            sub_string[1] = (char *) NEW_VEC(mempool, char, 4);
            snprintf(sub_string[1], 4, "%s", "AND");

            while(cur > begin && *cur == ' ') {
                cur--; 
            }
            *(cur+1) = '\0';
            return 3;
        }
    }
}
qp_syntax_node_t *KeyWordSyntaxParser::_qp_keyword_parse(MemPool *mempool, FieldDict *field_dict, 
        char *name, int nlen, char *string, int slen)
{
    char *sub_string[3];
    qp_syntax_node_t *node = NULL;
    qp_syntax_node_t *left_child = NULL;
    qp_syntax_node_t *right_child = NULL;
    int ret = 0;
    if(slen == 0) {
        return NULL;
    }
    ret = splitQueryString(mempool, string, slen, sub_string);
    if(ret  == 1) {
        node =  _qp_parse_normal_field(mempool, field_dict, name, nlen, sub_string[0], strlen(sub_string[0]));
        return node;
    } else if( ret == 3) {
        left_child = _qp_keyword_parse(mempool, field_dict, name, nlen, sub_string[0], strlen(sub_string[0]));
        right_child = _qp_keyword_parse(mempool, field_dict, name, nlen, sub_string[2], strlen(sub_string[2]));
        if(left_child == NULL || right_child == NULL) {
            return NULL;
        }
        NodeLogicType relation = qp_match_relation(sub_string[1],  strlen(sub_string[1])); 
        if(relation == LT_NONE) {
            TERR("relation == LT_NONE");
            return NULL;
        }
        node = qp_syntax_node_create(mempool, name, nlen, sub_string[1], strlen(sub_string[1]), relation, 0, false);
        if(node) {
            node->left_child = left_child;
            node->right_child = right_child;
        }
        return node;
    } else {
        return NULL;
    }
    return NULL;
}

bool KeyWordSyntaxParser::doPreProcess(char *string, int slen, char *temp)
{
    int braket_level = 0;
    char *cur = NULL;
    char *end = string + slen;
    int index = 0;
    qp_keyword_field_t fields[1024];
    bool is_in_package = false; 
    int package_braket_level = 0;
    if(*string == '(') {
        *string = '\0';
        string++;
        for(cur=string; cur<end; cur++) {
            if(*cur == '(') {
                package_braket_level++;
            }
            if(*cur == ')') {
                if(package_braket_level-- == 0) {
                    *cur = ' ';
                    break;
                }
            }
        }
    }
    char *begin = string;
    package_braket_level = 0;
    for(cur=string; cur<end; cur++) {
        if(*cur == '(') {
            if(cur > string && *(cur-1) == ' ') {
                if(is_in_package) {
                    package_braket_level ++;
                } else {
                    begin = cur; 
                }
            } else {
                package_braket_level = 0;
                is_in_package = true;
            }
        } else if(*cur == ')') {
            if(!is_in_package) {
                if(cur > begin && *(cur-1) == ' ') {
                    fields[index].begin = begin; 
                    fields[index].end = cur; 
                    index++;
                    begin = cur+1;
                }
            } else {
                if(package_braket_level-- <= 0) {
                    is_in_package = false;
                    fields[index].begin = begin; 
                    fields[index].end = cur; 
                    index++;
                    begin = cur+1;

                }
            }
        } else if(*cur == ' ') {
            if(!is_in_package && cur>string && *(cur-1) != ' ' && *(cur-1) != ')') {
                fields[index].begin = begin; 
                fields[index].end = cur; 
                index++;
                begin = cur;
            }
        }
    }
    if(index == 0) {
        fields[0].begin = string;
        fields[0].end   = end;
        index++; 
    } else if(*(end-1) != ')' && *(end-1) != ' ') {
        fields[index].begin = begin;
        fields[index].end = cur;
        index++;
    }
    char *new_str = temp;
    char *p = NULL;
    char *q = NULL;
    char *package = NULL;
    for(int i=0; i<index; i++) {
        bool is_package = false;
        for(cur = fields[i].begin+1; cur < fields[i].end; cur++) {
            if(*cur == '(' && *(cur-1) != ' ') {
                is_package = true; 
                package = cur;
                break; 
            }
        }
        if(!is_package) {
            for(cur = fields[i].begin; cur<=fields[i].end; cur++) {
                *new_str++ = *cur;
            }
        } else {
            bool is_should_add = false;
            braket_level = 0;
            begin = package;
            for(cur=package; cur <=fields[i].end; cur++) {
                if(*cur == '(') {
                    if(cur == package) {
                        char *p_add_bracket = cur+1;
                        while(p_add_bracket < fields[i].end) {
                            if (*p_add_bracket++ == '(') {
                                is_should_add = true;
                                *new_str++ = ' ';
                                *new_str++ = '(';
                                break;
                            }
                        }
                    }
                    braket_level++;
                    if(braket_level > 1) {
                        *new_str++ = *cur;
                    }
                    begin = cur+1;
                } else if(*cur == ')' ) {
                    braket_level--;
                    *cur = '\0';
                    NodeLogicType relation = qp_match_relation(begin, strlen(begin));
                    if(relation == LT_NONE && strlen(begin) != 0) {
                        for(p=fields[i].begin; p<package; p++) {
                            *new_str++ = *p;
                        }
                    } 
                    for(p=begin; p<cur; p++) {
                        *new_str++ = *p;
                    }
                    if(braket_level != 0) {
                        *new_str++ = ')';
                    } else {
                        if(is_should_add == true) {
                            *new_str++ = ' ';
                            *new_str++ = ')';
                        }
                    }
                    begin = cur+1;
                } else if( *cur == ' ') {
                    *cur = '\0';
                    NodeLogicType relation = qp_match_relation(begin, strlen(begin));
                    if(relation == LT_NONE && strlen(begin) != 0) {
                        for(p=fields[i].begin; p<package; p++) {
                            *new_str++ = *p;
                        }
                    } 
                    for(p=begin; p<cur; p++) {
                        *new_str++ = *p;
                    }
                    *new_str++ = ' ';
                    begin = cur+1;
                }
            }
        }
    }
    return true;
}
void KeyWordSyntaxParser::swap(qp_keyword_field_t *field1, qp_keyword_field_t *field2) 
{
}

int KeyWordSyntaxParser::doSyntaxParse(MemPool* mempool,
	FieldDict *field_dict, qp_syntax_tree_t* tree, qp_syntax_tree_t* not_tree,
	char* name, int nlen, char* string, int slen)
{
	qp_syntax_tree_t* sub_tree = 0;
	int retval = 0;
	
	if(!mempool || !tree || !not_tree || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_syntax_keyword_parse argument error, return!");
		return -1;
	}
    
    sub_tree = qp_syntax_tree_create(mempool);
    if(!sub_tree) {
        TNOTE("qp_syntax_tree_create failed!");
        return -1;
    }

    bool is_not_tree = false;	
    char *end = string+slen-1;
    str_trim(string, end);
    str_trim(string, end);
    char *temp = NULL;
    char *p = NULL;
    bool is_package = false;
    for(p=string+2; p<end; p++) {
        if(*p == '(' && *(p-1) != ' ') {
            is_package = true;
            break;
        }
    }
    if(is_package){
        temp = (char *) NEW_VEC(mempool, char, 20*slen+1);
        if(!temp) {
            return -1; 
        }
        memset(temp, 0, 20*slen+1);
        doPreProcess(string, strlen(string), temp);
    } else {
        temp = (char *) NEW_VEC(mempool, char, 2*slen+1);
        if(!temp) {
            return -1; 
        }
        memset(temp, 0, 2*slen+1);
        snprintf(temp, slen+1, "%s", string);
    }
    sub_tree->root = _qp_keyword_parse(mempool, field_dict, name, nlen, temp, strlen(temp)); //现阶段不考虑o=or
    if(!sub_tree->root) {
        TNOTE("qp tree is null");
        return -1;
    }
    if(qp_syntax_tree_append_tree(mempool, tree, sub_tree, LT_AND)){
        TNOTE("qp_syntax_tree_append_tree error, return!");
        return -1;
    }

	return 0;
}
}
