#include <dlfcn.h>
#include <stdio.h>
#include <mxml.h>
#include <stdint.h>
#include "dlmodule/DlModuleManager.h"
#include "util/crc.h"

namespace dlmodule_manager {




CDlModule::CDlModule(const char * p_Value):
	pHandle(NULL),CreateInterfaceFunc(NULL),DestroyInterfaceFunc(NULL)
{

    m_szSoInnerName[0] = 0;
    m_szSoInnerVersion[0] = 0;

	int ret = pthread_rwlock_init(&m_module_rwlock , NULL);
	if(0 != ret)
	{
		TERR("CDlModule::CDlModule r/w lock init failed (%d)." , ret);
	}
	

//    m_szFilename = utility::string_utility::replicate( p_Value );
    snprintf(m_szFilename,sizeof(m_szFilename),"%s",p_Value);



}

void CDlModule::init()
{
	if(!pHandle)
	{ // not opened
		int ret = 0;
		ret = pthread_rwlock_wrlock(&m_module_rwlock);
		if(0 == ret)
		{ // exclusive locked
			if(!pHandle)
			{ // still not opened, open it
				pHandle = dlopen(m_szFilename,RTLD_NOW);
				if(!pHandle )
				{
					TERR("dlopen return %s", dlerror());
					pthread_rwlock_unlock(&m_module_rwlock);
					return;
				}
	
				CreateInterfaceFunc = (CreateInterfaceFuncType)dlsym(pHandle , CREATEINTERFACENAME);
				if( !CreateInterfaceFunc ) {
					TERR("find interface failed, return %s", dlerror());
					pthread_rwlock_unlock(&m_module_rwlock);
					return;
				}
	
				DestroyInterfaceFunc = (DestroyInterfaceFuncType)dlsym(pHandle , DESTROYINTERFACENAME);
				if( !DestroyInterfaceFunc ) {
					TERR("find interface failed, return %s", dlerror());
					pthread_rwlock_unlock(&m_module_rwlock);
					return;
				}
				
				typedef const char * (*GetStringInfoFuncType)(void);
				
				GetStringInfoFuncType GetSoNameFunc = (GetStringInfoFuncType)dlsym(pHandle , GETSONAME);
				if( GetSoNameFunc )	{
					const char * name = GetSoNameFunc();
					if(name){
					//	m_szSoInnerName = utility::string_utility::replicate( name );
                        snprintf(m_szSoInnerName,sizeof(m_szSoInnerName),"%s",name);
                    }
				}
				
				GetStringInfoFuncType GetSoVersionFunc = (GetStringInfoFuncType)dlsym(pHandle , GETSOVERSION);
				if( GetSoVersionFunc )	{
					const char * version = GetSoVersionFunc();
					if(version){
//						m_szSoInnerVersion = utility::string_utility::replicate( version );
                        snprintf(m_szSoInnerVersion,sizeof(m_szSoInnerVersion),"%s",version);
                    }
				}

				TLOG("so module %s is loaded, inner name is %s, inner version is %s" , 
						m_szFilename , (m_szSoInnerName[0]?m_szSoInnerName:"undefined") , 
						(m_szSoInnerVersion[0]?m_szSoInnerVersion:"undefined") );
			}
			pthread_rwlock_unlock(&m_module_rwlock);
		}
		else
		{
			TERR("CDlModule::init can't get wrlock (%d)." , ret);
		}
	}
}
		
CDlModule::~CDlModule()
{
	int ret = 0;
	ret = pthread_rwlock_wrlock(&m_module_rwlock);
	if(0 != ret)
	{
		TERR("CDlModule::~CDlModule can't get wrlock (%d)." , ret);
	}
	CreateInterfaceFunc = NULL;
	DestroyInterfaceFunc = NULL;
	if(pHandle)
		dlclose(pHandle);
	pHandle = NULL;
	
	ret = pthread_rwlock_unlock(&m_module_rwlock);
	if(0 != ret)
	{
		TERR("CDlModule::~CDlModule unlock wrlock failed (%d)." , ret);
	}
	ret = pthread_rwlock_destroy(&m_module_rwlock);
	if(0 != ret)
	{
		TERR("CDlModule::~CDlModule destroy wrlock failed (%d)." , ret);
	}

    m_szFilename[0]=0;
    m_szSoInnerName[0]=0;
    m_szSoInnerVersion[0]=0;
}

static int32_t hashmap_cmp(const void* key1,const void *key2){
    return strcmp((const char*)key1,(const char*)key2);
}

static uint64_t hashmap_hash(const void *key){
    uint64_t hash = get_crc64((const char*)key,strlen((const char*)key));
    return hash; 
}

CDlModuleManager::CDlModuleManager(){
    m_dlSingleModules = hashmap_create(128,0,hashmap_hash,hashmap_cmp);
    m_dlModules = hashmap_create(128,0,hashmap_hash,hashmap_cmp);
    if(!m_dlSingleModules){
        TERR("m_dlSingleModules init failed.");
        return;
    }
    
    if(!m_dlModules){
        TERR("m_dlSingleModules init failed.");
        return;
    }

}

static void dlmodule_hash_destroy_key(void *key,void *value, void *user_data){
    delete []((char*)key);
}

static void dlmodule_hash_destroy_all(void *key,void *value, void *user_data){
    delete []((char*)key);
    delete ((CDlModule*)value);
}

CDlModuleManager::~CDlModuleManager()
{
    hashmap_destroy(m_dlSingleModules,dlmodule_hash_destroy_all,NULL);
    hashmap_destroy(m_dlModules,dlmodule_hash_destroy_key,NULL);
    
    /*
	module_single_list_type::HashEntry *pEntry;
	for (pEntry = m_dlSingleModules.m_pFirst; pEntry != NULL; pEntry = pEntry->_seq_next) {
		delete pEntry->_value;
	}
	m_dlSingleModules.clear();
	module_list_type::HashEntry *pEntry2;
	for (pEntry2 = m_dlModules.m_pFirst; pEntry2 != NULL; pEntry2 = pEntry2->_seq_next) {
		delete[] pEntry2->_key;
	}
	m_dlModules.clear();
    */
}

/* 解析xml文件 */
bool CDlModuleManager::parse(const char* szConfName)
{
	FILE *fp;
	mxml_node_t *pXMLTree = NULL;
	mxml_node_t *pXMLNode = NULL;
	
	static const char* MODULES_CONF = "modules";
	static const char* MODULE_CONF = "module";

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

	pXMLNode = mxmlFindElement(pXMLTree, pXMLTree, MODULES_CONF, NULL, NULL, MXML_DESCEND);
	
	pXMLNode = mxmlFindElement(pXMLNode, pXMLTree, MODULE_CONF, NULL, NULL, MXML_DESCEND);
	while( pXMLNode != NULL ) {
		static const char* ATTRIBUTE_MODULE_KEY = "name";
		static const char* ATTRIBUTE_MODULE_VALUE = "file";
		const char* szKey;		
		const char* szValue;		
		szKey = mxmlElementGetAttr(pXMLNode , ATTRIBUTE_MODULE_KEY);
		szValue = mxmlElementGetAttr(pXMLNode , ATTRIBUTE_MODULE_VALUE);
	
		if ( szKey == NULL ) {
			TERR("module字段name属性没指定");
		}
		else if ( szValue == NULL ) {
			TERR("module字段value属性没指定");
		}
		else if(strlen(szKey) == 0) {
			TERR("module字段name属性为空");
		}
		else if(strlen(szValue) == 0) {
			TERR("module字段value属性为空");
		}
		else {
            //szKey 名称
            //szValue Path
			//module_list_type::HashEntry *pEntry = m_dlModules.lookupEntry( szKey );
            void *vValue = (void*)hashmap_find(m_dlModules,(void*)szKey);

			if (vValue == NULL) {

                //fix it: 取真实路径
			//	uint32_t pathhash = BKDRHash(szValue);
                void *vValue2 = (void*)hashmap_find(m_dlSingleModules,(void*)szValue);
			//	module_single_list_type::HashEntry *pEntry2 = m_dlSingleModules.lookupEntry( pathhash );
				if (vValue2 == NULL) {
					CDlModule * pModule = new CDlModule(szValue);
					if(pModule) {
						//char * szName = utility::string_utility::replicate( szKey );
                        int tmpLen = strlen(szKey)+1;
                        char *szName = new char[tmpLen];

                        tmpLen = strlen(szValue)+1;
                        char *szPath = new char[tmpLen];

                        
						if(szName&&szPath) {
                            snprintf(szName,tmpLen,"%s",szKey);
                            snprintf(szPath,tmpLen,"%s",szValue);
							//m_dlSingleModules.insert( pathhash , pModule );
                            hashmap_insert(m_dlSingleModules,szPath,pModule);
							//m_dlModules.insert( szName , pModule );
                            hashmap_insert(m_dlModules,szName,pModule);
						}
						else TERR("CDlModuleManager::parse szName create failed");
					}
					else TERR("CDlModule create failed");
				}
				else {
					TWARN("多个别名指向了 %s 模块，仅加载一次",szValue);
					//char * szName = utility::string_utility::replicate( szKey );
                    int tmpLen = strlen(szKey)+1;
                    char *szName = new char[tmpLen];
					if(szName) {
                        snprintf(szName,tmpLen,"%s",szKey);
                        hashmap_insert(m_dlModules,szName , vValue2 );
					}
					else TERR("CDlModuleManager::parse szName create failed");
				}
			}
			else TWARN("定义了多个别名为 %s 的模块, 仅加载第一项",szKey);
		}
		
		pXMLNode = mxmlFindElement(pXMLNode, pXMLTree, MODULE_CONF, NULL, NULL, MXML_DESCEND);
	}

	if(pXMLTree)
		mxmlDelete(pXMLTree);

	return true;
}

/* 通过名称得到例外词表 */
CDlModule* CDlModuleManager::getModule(const char* pszName) const
{
	if (pszName == NULL)
		return NULL;
	CDlModule * default_v = NULL;
    default_v = (CDlModule*)hashmap_find(m_dlModules,(void*)pszName);
//	return m_dlModules.find(pszName,default_v);
    return default_v;
}

uint32_t CDlModuleManager::BKDRHash(const char *str)
{
	uint32_t seed = 131; // 31 131 1313 13131 131313 etc..
	uint32_t hash = 0;
 
	while (*str)
	{
		hash = hash * seed + (*str++);
	}
 
	return (hash & 0x7FFFFFFF);
}

}
