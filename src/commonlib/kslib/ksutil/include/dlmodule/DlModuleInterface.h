#ifndef _DLMODULE_INTERFACE_
#define _DLMODULE_INTERFACE_

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <pthread.h>

namespace dlmodule_interface {

// to create uuid , execute the follow script:
// uuidgen | awk -F "-" '{print "DECLARE_UUID(0x" $1 ,  ", 0x" $2 ", 0x" $3 ", 0x" $4 ", 0x" $5 ")"}'
#if defined (__LP64__) || defined (__64BIT__) || defined (_LP64) || (__WORDSIZE == 64)
	#define DECLARE_UUID(u1 , u2 , u3 , u4 , u5) \
		public: \
			static const uint64_t uuid_high = (uint64_t(u1) << 32) | (uint64_t(u2) << 16) | (uint64_t(u3)); \
			static const uint64_t uuid_low = (uint64_t(u4) << 48) | (uint64_t(u5));
#else
	#define DECLARE_UUID(u1 , u2 , u3 , u4 , u5) \
		public: \
			static const uint64_t uuid_high = (uint64_t(u1) << 32) | (uint64_t(u2) << 16) | (uint64_t(u3)); \
			static const uint64_t uuid_low = (uint64_t(u4) << 48) | (uint64_t(u5##ULL));
#endif

	class IBaseInterface
	{
        public: 
	  typedef int RETURN_VALUE;
	  static const int SUCCEED = 0;
	public:
		IBaseInterface() {}
		virtual ~IBaseInterface() {}
	};	
}

#define DECLARE_SO_INTERFACE(var,interface_type,moname) dlmodule_interface::dlmodule_proxy<interface_type> var(moname);
#define DECLARE_SO_TYPE(interface_type, proxy_type) typedef dlmodule_interface::dlmodule_proxy<interface_type> proxy_type;





#define DEFINE_SO_VERSION(soname , vstr)	extern "C" const char * GetSoName(void) \
{ \
	static const char * soname_string = soname; \
	return soname_string; \
} \
extern "C" const char * GetVersion(void) \
{ \
	static const char * version_string = vstr; \
	return version_string; \
}


#define BEGIN_SO_INTERFACE()	extern "C" dlmodule_interface::IBaseInterface * CreateInterface(uint64_t uuid_high , uint64_t uuid_low , char * extend) {

#define IMPLEMENT_SO_INTERFACE_ENTRY(interface_type , concrete_type) \
	if( (interface_type::uuid_high == uuid_high) && (interface_type::uuid_low == uuid_low) ) \
		return reinterpret_cast<dlmodule_interface::IBaseInterface *>(dynamic_cast<interface_type *>(new concrete_type));

#define IMPLEMENT_SO_INTERFACE_EXTEND_ENTRY(extend_type , interface_type , concrete_type) \
	if( !strcmp(extend_type , extend) && (interface_type::uuid_high == uuid_high) && (interface_type::uuid_low == uuid_low) ) \
		return reinterpret_cast<dlmodule_interface::IBaseInterface *>(dynamic_cast<interface_type *>(new concrete_type));

#define END_SO_INTERFACE() \
	else \
		return NULL; \
} \
extern "C" void DestroyInterface(dlmodule_interface::IBaseInterface * p) \
{ \
	delete p; \
}


#define BEGIN_SO_SINGLE_INTERFACE()	extern "C" dlmodule_interface::IBaseInterface * CreateInterface(uint64_t uuid_high , uint64_t uuid_low , char * extend) {

#define IMPLEMENT_SO_SINGLE_INTERFACE_ENTRY(interface_type , concrete_type) \
	if( (interface_type::uuid_high == uuid_high) && (interface_type::uuid_low == uuid_low) ) {\
		static concrete_type me;\
		return reinterpret_cast<dlmodule_interface::IBaseInterface *>(dynamic_cast<interface_type *>(&me));}

#define IMPLEMENT_SO_SINGLE_INTERFACE_EXTEND_ENTRY(extend_type , interface_type , concrete_type) \
	if( !strcmp(extend_type , extend) && (interface_type::uuid_high == uuid_high) && (interface_type::uuid_low == uuid_low) ) {\
		static concrete_type me;\
		return reinterpret_cast<dlmodule_interface::IBaseInterface *>(dynamic_cast<interface_type *>(&me));}

#define END_SO_SINGLE_INTERFACE() \
	else \
		return NULL; \
} \
extern "C" void DestroyInterface(dlmodule_interface::IBaseInterface * p) \
{ \
	; \
}




#endif // _DLMODULE_INTERFACE_



