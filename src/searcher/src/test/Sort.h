//#include "util/configure.h"
#include "framework/Context.h"
#include "framework/namespace.h"
#include "mxml.h"

class Sort{
public:
    int32_t init(const mxml_node_t *_pMxmlNode){return SUCCESS;}
	int32_t sort(const FRAMEWORK::Context &context){return SUCCESS;}
};
