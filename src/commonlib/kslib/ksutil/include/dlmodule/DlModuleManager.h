#ifndef _DLMODULE_MANAGER_
#define _DLMODULE_MANAGER_

#include <inttypes.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include "DlModuleInterface.h"
#include "util/hashmap.h"
#include "util/Log.h"

#define CREATEINTERFACENAME "CreateInterface"
#define DESTROYINTERFACENAME "DestroyInterface"
#define GETSONAME "GetSoName"
#define GETSOVERSION "GetVersion"

namespace dlmodule_manager {

	class CDlModule
	{
	public:
		typedef dlmodule_interface::IBaseInterface * p_interface_type;
		typedef p_interface_type (*CreateInterfaceFuncType)(uint64_t , uint64_t , const char * );
		typedef void (*DestroyInterfaceFuncType)(p_interface_type);
	public:		
		CDlModule(const char * p_Value);
		~CDlModule();
		void init();
		bool fail()
		{ 
			return ( (pHandle == NULL) || (CreateInterfaceFunc == NULL) || (DestroyInterfaceFunc == NULL) ); 
		}
	public:
		p_interface_type create(uint64_t uuid_high , uint64_t uuid_low , const char * extend = NULL)
		{
			int ret = 0;
			p_interface_type p = NULL;
			ret = pthread_rwlock_rdlock(&m_module_rwlock);
			if(0 != ret)
				TERR("CDlModule::create加读写锁失败.");

			if(CreateInterfaceFunc)
				p = CreateInterfaceFunc(uuid_high , uuid_low , (extend?extend:"") );
			ret = pthread_rwlock_unlock(&m_module_rwlock);
			if(0 != ret)
				TERR("CDlModule::create解读写锁失败.");
			return p;
		}
		void destroy(p_interface_type p)
		{
			int ret = 0;
			ret = pthread_rwlock_rdlock(&m_module_rwlock);
			if(0 != ret)
				TERR("CDlModule::destroy加读写锁失败.");

			if(DestroyInterfaceFunc)
				DestroyInterfaceFunc(dynamic_cast<dlmodule_interface::IBaseInterface *>(p));
			ret = pthread_rwlock_unlock(&m_module_rwlock);
			if(0 != ret)
				TERR("CDlModule::destroy解读写锁失败.");
		}
	private:
		char m_szFilename[PATH_MAX];	//so文件名称
		char m_szSoInnerName[PATH_MAX];
		char m_szSoInnerVersion[PATH_MAX];
	private:
		void * pHandle;
		CreateInterfaceFuncType CreateInterfaceFunc;
		DestroyInterfaceFuncType DestroyInterfaceFunc;
		pthread_rwlock_t m_module_rwlock;
	};

	class CDlModuleManager
	{
	private:
//		typedef utility::CHashMap<uint32_t, CDlModule *> module_single_list_type;
//		module_single_list_type m_dlSingleModules;
        hashmap_t m_dlSingleModules;

//		typedef utility::CHashMap<const char*, CDlModule *> module_list_type;
//		module_list_type m_dlModules;
        hashmap_t m_dlModules;

		CDlModuleManager();
	public:
		~CDlModuleManager();
	public:
		static CDlModuleManager * getModuleManager(void)
		{
			static CDlModuleManager manager;
			return &manager;
		}
	public:
		/* 解析xml文件 */
		bool parse(const char* szConfName);
		/* 通过名称得到例外词表 */
		CDlModule* getModule(const char* pszName) const;
	private:
		// BKDR Hash Function
		static uint32_t BKDRHash(const char *str);
        static uint32_t INTHash(uint32_t key){return key;}
	};

}

namespace dlmodule_interface {

	template<typename interface_type>
	class dlmodule_proxy
	{
	private:
		typedef dlmodule_manager::CDlModule dll_loader_type;
		typedef interface_type * p_interface_type;
		p_interface_type interface;
		dll_loader_type * pLoader;
	public:		
		dlmodule_proxy():interface(NULL),pLoader(NULL){};
		dlmodule_proxy(const char * moname , const char * extend = NULL):interface(NULL),pLoader(NULL)
		{
			load(moname , extend);
		}
		~dlmodule_proxy()
		{
			if(pLoader && interface)
				pLoader->destroy(interface);
			interface=NULL;
			pLoader = NULL;
		}
		void load(const char * moname , const char * extend = NULL)
		{
			pLoader = dlmodule_manager::CDlModuleManager::getModuleManager()->getModule(moname);
			if(!pLoader)
			{
				TERR("load module %s failed\n" , moname);
				return;
			}
			pLoader->init();
			if(pLoader->fail())
				return ;
			dll_loader_type::p_interface_type pi = pLoader->create(interface_type::uuid_high , interface_type::uuid_low , extend);
			if(pi)
				interface = reinterpret_cast<p_interface_type>(pi);

		}
		p_interface_type operator ->()
		{
			return interface;	
		}
		p_interface_type get_interface()
		{
			return interface;	
		}
		operator p_interface_type()
		{
			return interface;	
		}
		operator bool()const
		{
			return !fail();
		}
		bool fail() const
		{
			return (interface == NULL);
		}
	};
	
}
#endif // _DLMODULE_MANAGER_
