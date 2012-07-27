#include "OtherSyntaxParser.h"
namespace queryparser {
OtherSyntaxParser::OtherSyntaxParser()
{
}
OtherSyntaxParser::~OtherSyntaxParser()
{
}

int OtherSyntaxParser::doSyntaxParse(MemPool* mempool, FieldDict *field_dict, qp_syntax_tree_t* tree, 
        qp_syntax_tree_t* not_tree,
        char* name, int nlen, char* string, int slen)
{

    if(!mempool || !field_dict || !tree || !not_tree || !name || nlen<=0 || !string || slen<=0){
		TNOTE("qp_syntax_other_parse error, return!");
		return -1;
	}
	
	// 处理参数"o"
	if(nlen==1 && name[0]=='o'){
		if(slen==2 && ((string[0]=='o' && string[1]=='r') || (string[0]=='O' && string[1]=='R'))){
		}
		else{
		}
	}
	// 处理参数"qprohibite"
	else if(nlen==10 && strncmp("qprohibite", name, 10)==0){
		if((slen==1 && (string[0]=='Y' || string[0]=='y')) ||
		   (slen==3 && strncasecmp("yes", string, 3)==0)){
		}
	}
	// 处理参数"nk"
	else if(nlen==2 && strncmp("nk", name, 2)==0){
		if((slen==1 && (string[0]=='Y' || string[0]=='y')) ||
		   (slen==3 && strncasecmp("yes", string, 3)==0)){
		}
	}
	
	return 0;
}
}
