//#include "util/configure.h"
#include "framework/Context.h"
#include "framework/namespace.h"
#include "mxml.h"

class Statistic{
    public:
        int32_t init(const mxml_node_t *_pMxmlNode) {return SUCCESS;}
        int32_t statistic(const FRAMEWORK::Context &context){return SUCCESS;}
        uint32_t static getSerialSize() {return 0;}
        uint32_t static serialize(char *ptr, uint32_t size){return 0;}

};
