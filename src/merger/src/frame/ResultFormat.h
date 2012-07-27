#ifndef _FRAMEWORK_APP_SEARCHER_RESULT_FORMAT_H_
#define _FRAMEWORK_APP_SEARCHER_RESULT_FORMAT_H_
#include "commdef/ClusterResult.h"
#include "statistic/StatisticResult.h"
#include "sort/SortResult.h"
#include "util/MemPool.h"
#include "util/common.h"
#include "util/namespace.h"
#include <map>
#include <string>

#define VERSION_5_0 "Ven413"
#define MOVE_CURSOR(p, left, all, n) { if(p != (char*)SG_TMP) p += n; left -= n; all += n; }
static char SG_TMP[1024L*10240L];

static uint32_t showVariantType(char * ptr, uint32_t size,
        int type, statistic::ValueType * v) {
    if(type == DT_STRING) {
        uint32_t cost;
        cost = strlen(v->svalue);
        if(cost > size) return size + 1;
        memcpy(ptr, v->svalue, cost);
        return cost;
    } else if(type == DT_INT8 || type == DT_INT16 || type == DT_INT32 || type == DT_INT64 || type == DT_KV32) {
        return snprintf(ptr, size, "%lld", v->lvalue);
    } else if(type == DT_UINT8 || type == DT_UINT16 || type == DT_UINT32 || type == DT_UINT64 ) {
        return snprintf(ptr, size, "%llu", v->ulvalue);
    } else if(type == DT_DOUBLE) {
        return snprintf(ptr, size, "%f", v->dvalue);
    } else if(type == DT_FLOAT) {
        return snprintf(ptr, size, "%f", v->fvalue);
    }

    return 0;
}

struct result_container_t {
    commdef::ClusterResult *pClusterResult;
    //QueryParameter *param;
    const char *query;
    uint32_t cost;
};

namespace merger
{
enum ESupportCoding
{
    sc_UTF8,
    sc_Unicode,
    sc_Big5,
    sc_GBK,
    sc_Ansi
};
};

class ResultFormat {
    public:
        ResultFormat(MemPool *heap = NULL) : _heap(heap) { }
        virtual ~ResultFormat() { }
    public:
        int32_t format(const result_container_t &container, 
                char * &out_data, uint32_t &out_size);
    protected:
        virtual uint32_t doFormat(const result_container_t &container,
                char *out_data, uint32_t out_size) = 0;
    protected:
        void * alloc_memory(uint32_t size);
        void free_memory(void *ptr);
    private:
        MemPool *_heap;
};

class ResultFormatV3 : public ResultFormat {
    public:
        ResultFormatV3(MemPool * heap = NULL) : ResultFormat(heap) { }
        ~ResultFormatV3() { }
    protected:
        virtual uint32_t doFormat(const result_container_t & container,
                char * out_data, uint32_t out_size);
    public:
        static bool accept(const char * name);
};

class ResultFormatXMLConfig;

class ResultFormatXMLRef {
    public:
        ResultFormatXMLRef();
        ~ResultFormatXMLRef();
    public:
        int32_t initialize(const char *path);
        uint32_t doFormat(const result_container_t &container,
                char *out_data, uint32_t out_size);
        char *convertRestrictedChar(const char *pchStr, merger::ESupportCoding eEncodingType);
        const char *getName();
    private:
		ResultFormatXMLConfig *_conf;
};

class ResultFormatXML : public ResultFormat {
	public:
		ResultFormatXML(ResultFormatXMLRef &ref, MemPool *heap) : 
			ResultFormat(heap), _ref(ref) { 
			}
		~ResultFormatXML() { }
	protected:
		virtual uint32_t doFormat(const result_container_t &container,
				char *out_data, uint32_t out_size);
	private:
		ResultFormatXMLRef &_ref;
	public:
		static bool accept(const char *name);
};

class ResultFormatPB : public ResultFormat {
	public:
		ResultFormatPB(MemPool *heap) : ResultFormat(heap) { }
		~ResultFormatPB() { }
	protected:
		virtual uint32_t doFormat(const result_container_t &container,
				char *out_data, uint32_t out_size);
	public:
		static bool accept(const char *name);
};

class ResultFormatFactory {
	public:
		ResultFormatFactory() { }
		~ResultFormatFactory();
	public:
		ResultFormat * make(const char *name, MemPool *heap = NULL);
		void recycle(ResultFormat *p);
		int32_t registerXML(const char *path);
	private:
		static ResultFormatV3 s_v3;
		static ResultFormatXMLRef s_xml_ref;
		static ResultFormatXML s_xml;
		std::map<std::string, ResultFormatXMLRef*> _mp_xml;
		static ResultFormatPB s_pb;
};

#define STOP '\1'
#define STOP_STR "\1"
#define DOC_END '\2'
#define DOC_END_STR "\2"
#define VERSION_4_0 "V4.0"
#define VERSION_3_0 "V3.0"
#define VERSION_2_0 "V2.0"
#define SECTION_CONSTANT "const"
#define SECTION_SPECIALKEY "special"
#define SECTION_ECHOKEY "echo"
#define SECTION_STATINFO "stat"
#define SECTION_DATA "data"

#define QUERY_FAILURE "ERROR"
#define QUERY_SUCCESS "OK"

#define DOECHO_UNKNOWN 0
#define DOECHO_SYNONMYS 1
#define DOECHO_CATEGORY 2
#define DOECHO_SPELLCHECK_LESS 3
#define DOECHO_WORDTOKENIZER_LESS 4
#define DOECHO_SPELLCHECK 5
#define DOECHO_WORDTOKENIZER 6
#define DOECHO_DODELETE 7

#endif 
