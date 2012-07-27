#include "DocumentRewrite.h"
#include "util/ScopedLock.h"
#include <new>

namespace blender_interface {

DocumentRewrite DocumentRewrite::m_instance;
DocumentRewrite::~DocumentRewrite() 
{
	clear();
}

void DocumentRewrite::clear()
{
	WR_LOCK(m_lock);
	for (int i=0; i<m_qf.size(); i++)
	{
		if (m_qf[i] != NULL)
		{
			delete m_qf[i];
		}
	}
	m_qf.clear();
	WR_UNLOCK(m_lock);
}

DocumentRewrite &DocumentRewrite::getInstance()
{
	return DocumentRewrite::m_instance;
}

int32_t DocumentRewrite::parse(const char *szConfName)
{
	FILE *fp;
	mxml_node_t *pXMLTree = NULL;
	mxml_node_t *pXMLNode = NULL;

	static const char* QUERYFILTER_CONF = "documentrewrite";
	static const char* QFITEM_CONF = "drwitem";

	/* 打开配置文件,并使用mxml装载配置信息 */
	if ((fp = fopen(szConfName, "r")) == NULL) {
			TERR( "配置文件 %s 打开出错, 文件可能不存在.", szConfName);
			return false;
	}
	pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
	fclose(fp);
	if (pXMLTree == NULL) {
			TERR( "配置文件 %s 格式有错, 请根据错误提示修正您的配置文件.", szConfName);
			return false;
	}

	pXMLNode = mxmlFindElement(pXMLTree, pXMLTree, QUERYFILTER_CONF, NULL, NULL, MXML_DESCEND);
	pXMLNode = mxmlFindElement(pXMLNode, pXMLTree, QFITEM_CONF, NULL, NULL, MXML_DESCEND);
	while( pXMLNode != NULL )
	{
        static const char* ATTRIBUTE_MODULE_NAME = "name";
        static const char* ATTRIBUTE_MODULE_CONF = "conf";
        const char* szName;              
        const char* szConf;            
        szName = mxmlElementGetAttr(pXMLNode , ATTRIBUTE_MODULE_NAME);
        szConf = mxmlElementGetAttr(pXMLNode , ATTRIBUTE_MODULE_CONF);

        if ( szName == NULL ) {
                TERR("name属性没指定");
        }
        else if(strlen(szName) == 0) {
                TERR("name属性为空");
        }
        else 
	{
		WR_LOCK(m_lock);
		if (m_name.find(string(szName)) == m_name.end())
		{
			m_name.insert(string(szName));
		
			InterfaceType *documentrewrite = new(std::nothrow) InterfaceType(szName);
			if (documentrewrite == NULL)
				return KS_ENOMEM;
			if((*documentrewrite).fail())
			{
					TERR("so load failed, name is %s\n", szName);
					delete documentrewrite;
			}
			else
			{
				if ((*documentrewrite)->init(szConf) != 0)
				{
					TERR("init Module[%s] by `%s' error", szName, SAFE_STRING(szConf));
					delete documentrewrite;
					continue;
				}
                                TLOG("init Module[%s] by `%s' success", szName, SAFE_STRING(szConf));
				m_qf.push_back(documentrewrite);
			}
		}
		WR_UNLOCK(m_lock);
        }       
	pXMLNode = mxmlFindElement(pXMLNode, pXMLTree, QFITEM_CONF, NULL, NULL, MXML_DESCEND);
	}//while
	if(pXMLTree)
		mxmlDelete(pXMLTree);
	return true;
}

int32_t DocumentRewrite::rewrite(Document **&docs, uint32_t &size, const DRWParam & param)
{
	RD_LOCK(m_lock);
	if (m_qf.size() <= 0)
	{
		return KS_SUCCESS;
	}
	for (int i=0; i<m_qf.size(); i++)
	{
		if ((*m_qf[i])->rewrite(docs, size, param) < 0)
		{
			return KS_EFAILED;
		}
	}
	RD_UNLOCK(m_lock);
	return KS_SUCCESS;
}
}

