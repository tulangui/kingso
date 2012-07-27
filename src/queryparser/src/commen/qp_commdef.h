/***********************************************************************************
 * Describe : Query Parser 公共define
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-22
 **********************************************************************************/

#ifndef _QP_COMMDEF_H_
#define _QP_COMMDEF_H_


#define AND_OPT "AND"              // 与操作符
#define NOT_OPT "NOT"              // 或操作符
#define OR_OPT  "OR"               // 非操作符
#define AND_LEN 3                  // 与操作符长度
#define NOT_LEN 3                  // 或操作符长度
#define OR_LEN  2                  // 非操作符长度

#define BRACKET_HEAD_OPT "("       // 前半括号
#define BRACKET_TAIL_OPT ")"       // 后半括号
#define BRACKET_HEAD_LEN 1         // 前半括号长度
#define BRACKET_TAIL_LEN 1         // 后半括号长度

#define KSQ_TAG "ksq"              // kingso rewirite q名字
#define KSQ_LEN 3                  // kingso rewirite q名字长度

#define SUB_QUERY_EQUAL  '='       // 子query名与值的分隔符
#define SUB_QUERY_END    '#'       // 子query结束符
#define SUB_STRING_EQUAL '='       // 子句名与值的分隔符
#define SUB_STRING_END   '&'       // 子句结束符

#define QKV_OPT "::"               // q中名与只的分割符
#define QKV_LEN 2                  // q中名与只的分割符长度

#define PHRASE_TAG "phrase"        // phrase标签名字
#define PHRASE_LEN 6               // phrase标签名字长度


#endif
