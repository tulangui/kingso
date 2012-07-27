//#include "util/configure.h"
#include "framework/Context.h"
#include "framework/namespace.h"
#include "mxml.h"

class IndexSearcher{

public:
	int32_t init(const mxml_node_t *_pMxmlNode){return SUCCESS;}
	int32_t doSearch(const FRAMEWORK::Context &context){return SUCCESS;}
};
