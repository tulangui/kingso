#ifndef _DOCUMENT_REWRITE_
#define _DOCUMENT_REWRITE_

#include "DocumentRewriteInterface.h"
#include "dlmodule/DlModuleInterface.h"
#include "dlmodule/DlModuleManager.h"
#include "Document.h"
#include "util/ThreadLock.h"
#include "mxml.h"
#include "commdef/commdef.h"
#include <vector>
#include <set>
#include <pthread.h>

using namespace std;

namespace blender_interface {

class DocumentRewrite
{
DECLARE_SO_TYPE(blender_interface::DocumentRewriteInterface, InterfaceType)
	public:
		static DocumentRewrite &getInstance();
		uint32_t size(){return m_qf.size();}
		int32_t parse(const char *conffile);
		int32_t rewrite(Document **&docs, uint32_t &size, const DRWParam & param);
		virtual ~DocumentRewrite();
		void clear();
	private:
		DocumentRewrite(){};
		DocumentRewrite &operator=(const DocumentRewrite&);
		static DocumentRewrite m_instance;
		vector <InterfaceType *> m_qf;
		set <string> m_name;
		UTIL::RWLock m_lock;

};

}

#endif
