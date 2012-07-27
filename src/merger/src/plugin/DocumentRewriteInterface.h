#ifndef _DOCUMENT_REWRITE_INTERFACE_H_
#define _DOCUMENT_REWRITE_INTERFACE_H_

#include "dlmodule/DlModuleInterface.h"
//#include "blender.h"
#include "Document.h"
#include "util/Log.h"

namespace blender_interface {

struct DRWParam {
	DRWParam() {
		query_string = NULL;
		query_size = 0;
		match_keywords = NULL;
		matchkeyword_size = 0;
	}
	const char * query_string;
	uint32_t query_size;
	const char * match_keywords;
	uint32_t matchkeyword_size;
	int m_iSubStart;
	int m_iSubNum;
};

class DocumentRewriteInterface: public dlmodule_interface::IBaseInterface
{
DECLARE_UUID(0x19574883 , 0x8880, 0x4132, 0xba9b, 0x40447af62d1e)
        public:
                DocumentRewriteInterface() : IBaseInterface(), LOG(G_LOG) { }
                virtual ~DocumentRewriteInterface(){}
//return: 0,success
                virtual int32_t init(const char *conffile) = 0;
                virtual int32_t rewrite(Document **&docs, uint32_t &size, const DRWParam & param) = 0;

        protected:
    
        public:
                alog_fun LOG;
                
};

}
#endif

